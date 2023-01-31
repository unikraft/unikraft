/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Wei Chen <wei.chen@arm.com>
 *          Eduard Vintila <eduard.vintila47@gmail.com>
 *
 * Copyright (c) 2018, Arm Ltd. All rights reserved.
 * Copyright (c) 2022, University of Bucharest. All rights reserved.
 *
 * Some parts from _init_paging() are taken from the x86 setup.c source code:
 *
 * Authors: Dan Williams
 *          Martin Lucina
 *          Ricardo Koller
 *          Felipe Huici <felipe.huici@neclab.eu>
 *          Florian Schmidt <florian.schmidt@neclab.eu>
 *          Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2015-2017 IBM
 * Copyright (c) 2016-2017 Docker, Inc.
 * Copyright (c) 2017 NEC Europe Ltd., NEC Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <uk/config.h>
#include <uk/arch/limits.h>
#include <uk/arch/types.h>
#include <uk/plat/common/sections.h>
#include <uk/essentials.h>
#include <uk/plat/bootstrap.h>
#include <uk/print.h>
#include <uk/assert.h>
#include <kvm/config.h>
#include <libfdt.h>
#include <uart/ns16550.h>
#include <string.h>
#include <riscv/sbi.h>
#include <riscv/traps.h>
#include <uk/plat/time.h>
#include <kvm/intctrl.h>

#ifdef CONFIG_PAGING
#include <uk/plat/paging.h>
#include <uk/falloc.h>
#include <uk/plat/common/sections.h>
#endif

struct kvmplat_config _libkvmplat_cfg = {0};

#define MAX_CMDLINE_SIZE 1024
static char cmdline[MAX_CMDLINE_SIZE];
static const char *appname = CONFIG_UK_NAME;

extern void _libkvmplat_newstack(uint64_t stack_start, void (*tramp)(void *),
				 void *arg);

static void _init_dtb(void *dtb_pointer)
{
	int ret, dtb_size;

	if ((ret = fdt_check_header(dtb_pointer)))
		UK_CRASH("Invalid DTB: %s\n", fdt_strerror(ret));

	uk_pr_info("Found device tree at: %p\n", dtb_pointer);

	/* Move the DTB at the end of the kernel image */
	dtb_size = fdt_totalsize(dtb_pointer);
	_libkvmplat_cfg.dtb = memmove((void *)__END, dtb_pointer, dtb_size);
}

static void _dtb_get_cmdline(char *cmdline, size_t maxlen)
{
	int fdtchosen, len;
	const char *fdtcmdline;

	fdtchosen = fdt_path_offset(_libkvmplat_cfg.dtb, "/chosen");
	if (fdtchosen < 0)
		goto enocmdl;
	fdtcmdline =
	    fdt_getprop(_libkvmplat_cfg.dtb, fdtchosen, "bootargs", &len);
	if (!fdtcmdline || (len <= 0))
		goto enocmdl;

	if (likely(maxlen >= (unsigned int)len))
		maxlen = len;
	else
		uk_pr_err("Command line too long, truncated\n");

	strncpy(cmdline, fdtcmdline, maxlen);
	/* ensure null termination */
	cmdline[maxlen - 1] = '\0';

	uk_pr_info("Command line: %s\n", cmdline);
	return;

enocmdl:
	uk_pr_info("No command line found\n");
}

static void _init_dtb_mem(void)
{
	int fdt_mem, prop_len = 0, prop_min_len;
	int naddr, nsize;
	const __u64 *regs;
	__u64 mem_base, mem_size, max_addr;

	/* search for assigned VM memory in DTB */
	if (fdt_num_mem_rsv(_libkvmplat_cfg.dtb) != 0)
		uk_pr_warn("Reserved memory is not supported\n");

	fdt_mem = fdt_node_offset_by_prop_value(
	    _libkvmplat_cfg.dtb, -1, "device_type", "memory", sizeof("memory"));
	if (fdt_mem < 0) {
		uk_pr_warn("No memory found in DTB\n");
		return;
	}

	naddr = fdt_address_cells(_libkvmplat_cfg.dtb, fdt_mem);
	if (naddr < 0 || naddr >= FDT_MAX_NCELLS)
		UK_CRASH("Could not find proper address cells!\n");

	nsize = fdt_size_cells(_libkvmplat_cfg.dtb, fdt_mem);
	if (nsize < 0 || nsize >= FDT_MAX_NCELLS)
		UK_CRASH("Could not find proper size cells!\n");

	/*
	 * QEMU will always provide us at least one bank of memory.
	 * unikraft will use the first bank for the time-being.
	 */
	regs = fdt_getprop(_libkvmplat_cfg.dtb, fdt_mem, "reg", &prop_len);

	/*
	 * The property must contain at least the start address
	 * and size, each of which is 8-bytes.
	 */
	prop_min_len = (int)sizeof(fdt32_t) * (naddr + nsize);
	if (regs == NULL || prop_len < prop_min_len)
		UK_CRASH("Bad 'reg' property: %p %d\n", regs, prop_len);

	/* If we have more than one memory bank, give a warning messasge */
	if (prop_len > prop_min_len)
		uk_pr_warn("Currently, we support only one memory bank!\n");

	mem_base = fdt64_to_cpu(regs[0]);
	mem_size = fdt64_to_cpu(regs[1]);
	if (mem_base > __TEXT)
		UK_CRASH("Fatal: Image outside of RAM\n");

	max_addr = mem_base + mem_size;
	uk_pr_info("Memory base: 0x%lx, size: 0x%lx, max_addr: 0x%lx\n",
		   mem_base, mem_size, max_addr);

	_libkvmplat_cfg.bstack.end = ALIGN_DOWN(max_addr, __STACK_ALIGN_SIZE);
	_libkvmplat_cfg.bstack.len = ALIGN_UP(__STACK_SIZE, __STACK_ALIGN_SIZE);
	_libkvmplat_cfg.bstack.start =
	    _libkvmplat_cfg.bstack.end - _libkvmplat_cfg.bstack.len;

	_libkvmplat_cfg.heap.start =
	    ALIGN_UP(__END + fdt_totalsize(_libkvmplat_cfg.dtb), __PAGE_SIZE);
	_libkvmplat_cfg.heap.end = _libkvmplat_cfg.bstack.start;
	_libkvmplat_cfg.heap.len =
	    _libkvmplat_cfg.heap.end - _libkvmplat_cfg.heap.start;

	if (_libkvmplat_cfg.heap.start > _libkvmplat_cfg.heap.end)
		UK_CRASH("Not enough memory, giving up...\n");
}

#ifdef CONFIG_PAGING
/* TODO: Find an appropriate solution to manage the address space layout
 * without the presence of any more advanced virtual memory management.
 * For now, we simply map the heap statically at 0x400000000 (16 GiB).
 */
#define PG_HEAP_MAP_START (1UL << 34) /* 16 GiB */

/* The static boot pagetable */
static struct uk_pagetable boot_pt;

/* New pagetable */
static struct uk_pagetable kernel_pt;

/* RAM base address from the linker script */
extern char _start_ram_addr[];

static int _identity_map_region(struct uk_pagetable *pt, __paddr_t start,
				 __paddr_t end, unsigned int prot)
{
	int rc;
	__paddr_t al_start = PAGE_ALIGN_DOWN(start);
	__paddr_t al_end = PAGE_ALIGN_UP(end);
	__sz pages = (al_end - al_start) >> PAGE_SHIFT;

	rc = ukplat_page_map(pt, al_start, al_start, pages, prot, 0);

	return rc;
}

static void _init_paging(void)
{
	struct kvmplat_config_memregion *mr;
	__sz offset, len;
	__paddr_t start;
	__sz free_memory, res_memory;
	unsigned long frames;
	int rc;

	/* Initialize the frame allocator by taking away the memory from the
	 * heap area.
	 */
	mr = &_libkvmplat_cfg.heap;

	offset = mr->start - PAGE_ALIGN_UP(mr->start);
	start = PAGE_ALIGN_UP(mr->start);
	len = PAGE_ALIGN_DOWN(mr->len - offset);

	rc = ukplat_pt_init(&boot_pt, start, len);
	if (unlikely(rc))
		goto EXIT_FATAL;

	/* Clone the boot page table so we can unmap stuff */
	rc = ukplat_pt_clone(&kernel_pt, &boot_pt, 0);
	if (unlikely(rc))
		goto EXIT_FATAL;

	/*
	 * The static boot pagetable identity maps the first 4GiB of RAM as RWX.
	 * Unmap this region, so we can later map each section
	 * in the kernel image with its appropriate attributes.
	 */
	start = (__paddr_t)_start_ram_addr;
	len = (PAGE_HUGE_SIZE * 4) >> PAGE_SHIFT;
	rc = ukplat_page_unmap(&kernel_pt, start, len, PAGE_FLAG_KEEP_FRAMES);
	if (unlikely(rc))
		goto EXIT_FATAL;

	/*
	 * TODO: The static boot pagetable also identity maps the MMIO region
	 * 0x0 - 0x80000000. Maybe we should unmap this region as well and
	 * somehow use virtual addresses from the direct mapping to do MMIO,
	 * instead.
	 */

	/* Kernel image sections */
	_identity_map_region(&kernel_pt, __TEXT, __ETEXT,
			    PAGE_ATTR_PROT_READ | PAGE_ATTR_PROT_EXEC);
	_identity_map_region(&kernel_pt, __ETEXT, __RODATA,
				PAGE_ATTR_PROT_RW);
	_identity_map_region(&kernel_pt, __RODATA, __ECTORS,
			    PAGE_ATTR_PROT_READ);
	_identity_map_region(&kernel_pt, __ECTORS, __END,
				PAGE_ATTR_PROT_RW);

	/* Map the DTB, which resides immediately after the kernel image */
	_identity_map_region(&kernel_pt, __END,
			     __END + fdt_totalsize(_libkvmplat_cfg.dtb),
			     PAGE_ATTR_PROT_RW);

	/* Switch to the new page table */
	rc = ukplat_pt_set_active(&kernel_pt);
	if (unlikely(rc))
		goto EXIT_FATAL;

	/* TODO: We don't have any virtual address space management yet. We are
	 * also missing demand paging and the means to dynamically assign frames
	 * to the heap or other areas (e.g., mmap). We thus simply statically
	 * pre-map the RAM as heap.
	 * To map all this memory we also need page tables. This memory won't be
	 * available for use by the heap, so we reduce the heap size by this
	 * amount. We compute the number of page tables for the worst case
	 * (i.e., 4K pages). Also reserve some space for the boot stack.
	 */
	free_memory = kernel_pt.fa->free_memory;
	frames = free_memory >> PAGE_SHIFT;

	res_memory = __STACK_SIZE;		      /* boot stack */
	res_memory += PT_PAGES(frames) << PAGE_SHIFT; /* page tables */

	_libkvmplat_cfg.heap.start = PG_HEAP_MAP_START;
	_libkvmplat_cfg.heap.end = PG_HEAP_MAP_START + free_memory - res_memory;
	_libkvmplat_cfg.heap.len =
	    _libkvmplat_cfg.heap.end - _libkvmplat_cfg.heap.start;

	uk_pr_info("HEAP area @ %"__PRIpaddr
		   " - %"__PRIpaddr
		   " (%"__PRIsz
		   " bytes)\n",
		   (__paddr_t)_libkvmplat_cfg.heap.start,
		   (__paddr_t)_libkvmplat_cfg.heap.end,
		   _libkvmplat_cfg.heap.len);

	frames = _libkvmplat_cfg.heap.len >> PAGE_SHIFT;

	rc = ukplat_page_map(&kernel_pt, _libkvmplat_cfg.heap.start,
			     __PADDR_ANY, frames, PAGE_ATTR_PROT_RW, 0);
	if (unlikely(rc))
		goto EXIT_FATAL;

	/* Forget about heap2 */
	_libkvmplat_cfg.heap2.start = 0;
	_libkvmplat_cfg.heap2.end = 0;
	_libkvmplat_cfg.heap2.len = 0;

	/* Setup and map boot stack */
	_libkvmplat_cfg.bstack.start = _libkvmplat_cfg.heap.end;
	_libkvmplat_cfg.bstack.end = _libkvmplat_cfg.heap.end + __STACK_SIZE;
	_libkvmplat_cfg.bstack.len =
	    _libkvmplat_cfg.bstack.end - _libkvmplat_cfg.bstack.start;

	frames = _libkvmplat_cfg.bstack.len >> PAGE_SHIFT;

	rc = ukplat_page_map(&kernel_pt, _libkvmplat_cfg.bstack.start,
			     __PADDR_ANY, frames, PAGE_ATTR_PROT_RW, 0);
	if (unlikely(rc))
		goto EXIT_FATAL;

	return;
EXIT_FATAL:
	UK_CRASH("Failed to initialize paging (code: %d)\n", -rc);
}
#else /* CONFIG_PAGING */
#define _init_paging()                                                       \
	do {                                                                   \
	} while (0)
#endif /* CONFIG_PAGING */

static void _libkvmplat_entry2(void *arg __attribute__((unused)))
{
	ukplat_entry_argp(DECONST(char *, appname), (char *)cmdline,
			  strlen(cmdline));
}

void _libkvmplat_start(void *opaque __unused, void *dtb_pointer)
{
	_init_dtb(dtb_pointer);

	/* Setup the NS16550 serial UART */
	ns16550_console_init(_libkvmplat_cfg.dtb);

	uk_pr_info("Entering from KVM (riscv64)...\n");

	/* Get command line arguments from DTB */
	_dtb_get_cmdline(cmdline, sizeof(cmdline));

	_init_dtb_mem();

	_init_traps();

	intctrl_init();

	_init_paging();

	uk_pr_info("     heap start: %p\n", (void *)_libkvmplat_cfg.heap.start);
	uk_pr_info("      stack top: %p\n",
		   (void *)_libkvmplat_cfg.bstack.start);

	_libkvmplat_newstack((uint64_t)_libkvmplat_cfg.bstack.end,
			     _libkvmplat_entry2, NULL);
}
