/* SPDX-License-Identifier: ISC */
/*
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
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose with or without fee is hereby granted, provided
 * that the above copyright notice and this permission notice appear
 * in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <string.h>
#include <uk/plat/common/sections.h>
#include <x86/cpu.h>
#include <x86/traps.h>
#include <kvm/config.h>
#include <kvm/console.h>
#include <kvm/intctrl.h>
#include <kvm-x86/bootinfo.h>
#include <kvm-x86/multiboot.h>
#include <kvm-x86/multiboot_defs.h>
#include <uk/arch/limits.h>
#include <uk/arch/types.h>
#include <uk/plat/console.h>
#include <uk/assert.h>
#include <uk/essentials.h>
#include <x86/acpi/acpi.h>

#include <uk/plat/lcpu.h>
#include <uk/plat/common/lcpu.h>

#ifdef CONFIG_PAGING
#include <uk/plat/paging.h>
#include <uk/falloc.h>
#endif /* CONFIG_PAGING */

#define PLATFORM_MEM_START 0x100000
#define PLATFORM_MAX_MEM_ADDR 0x100000000 /* 4 GiB */

#define MAX_CMDLINE_SIZE 8192
static char cmdline[MAX_CMDLINE_SIZE];

struct kvmplat_config _libkvmplat_cfg = { 0 };
struct uk_bootinfo bootinfo = { 0 };

static void _convert_mbinfo(struct multiboot_info *mi)
{
	multiboot_memory_map_t *m;
	multiboot_module_t *mod1;
	size_t offset;

	if (mi->flags & MULTIBOOT_INFO_CMDLINE)
		bootinfo.u64_cmdline = (__u64)mi->cmdline;
	else
		bootinfo.u64_cmdline = 0;

	/*
	 * Look for the first chunk of memory at PLATFORM_MEM_START.
	 */
	for (offset = 0; offset < mi->mmap_length;
	     offset += m->size + sizeof(m->size)) {
		m = (void *)(__uptr)(mi->mmap_addr + offset);
		if (m->addr == PLATFORM_MEM_START
		    && m->type == MULTIBOOT_MEMORY_AVAILABLE) {
			break;
		}
	}
	UK_ASSERT(offset < mi->mmap_length);

	/*
	 * Cap our memory size to PLATFORM_MAX_MEM_SIZE for which the initial
	 * static page table defines mappings for. Don't apply the limit when
	 * paging is enabled as we take the information about the heap regions
	 * to initialize the frame allocator.
	 */
	bootinfo.max_addr = m->addr + m->len;
	if (bootinfo.max_addr > PLATFORM_MAX_MEM_ADDR)
		bootinfo.max_addr = PLATFORM_MAX_MEM_ADDR;
	UK_ASSERT(bootinfo.max_addr >= (size_t) __END);

	/*
	 * Reserve space for boot stack at the end of found memory
	 */
	if ((bootinfo.max_addr - m->addr) < __STACK_SIZE)
		UK_CRASH("Not enough memory to allocate boot stack\n");

	/*
	 * Search for initrd (called boot module according to multiboot)
	 */
	if (mi->mods_count == 0) {
		uk_pr_debug("No initrd present\n");
		bootinfo.has_initrd = 0;
		return;
	}

	/*
	 * NOTE: We are only taking the first boot module as initrd.
	 *       Initrd arguments and further modules are ignored.
	 */
	UK_ASSERT(mi->mods_addr);

	mod1 = (multiboot_module_t *)((uintptr_t)mi->mods_addr);
	UK_ASSERT(mod1->mod_end >= mod1->mod_start);

	if (mod1->mod_end == mod1->mod_start) {
		uk_pr_debug("Ignoring empty initrd\n");
		bootinfo.initrd_start = 0;
		bootinfo.initrd_end = 0;
		bootinfo.initrd_length = 0;
		bootinfo.has_initrd = 0;
	} else {
		bootinfo.initrd_start = mod1->mod_start;
		bootinfo.initrd_end = mod1->mod_end;
		bootinfo.initrd_length = mod1->mod_end - mod1->mod_start;
		bootinfo.has_initrd = 1;
	}
}

static inline void _get_cmdline(struct uk_bootinfo *bi)
{
	char *mi_cmdline;

	if (bi->u64_cmdline) {
		mi_cmdline = (char *)bi->u64_cmdline;

		if (strlen(mi_cmdline) > sizeof(cmdline) - 1)
			uk_pr_err("Command line too long, truncated\n");
		strncpy(cmdline, mi_cmdline, sizeof(cmdline));
	} else {
		/* Use image name as cmdline to provide argv[0] */
		uk_pr_debug("No command line present\n");
		strncpy(cmdline, CONFIG_UK_NAME, sizeof(cmdline));
	}

	/* ensure null termination */
	cmdline[(sizeof(cmdline) - 1)] = '\0';
}

static inline void _init_mem(struct uk_bootinfo *bi)
{
	_libkvmplat_cfg.heap.start = ALIGN_UP((uintptr_t)__END, __PAGE_SIZE);
	_libkvmplat_cfg.heap.end = (uintptr_t)bi->max_addr - __STACK_SIZE;
	_libkvmplat_cfg.heap.len =
	    _libkvmplat_cfg.heap.end - _libkvmplat_cfg.heap.start;
	_libkvmplat_cfg.bstack.start = _libkvmplat_cfg.heap.end;
	_libkvmplat_cfg.bstack.end = bi->max_addr;
	_libkvmplat_cfg.bstack.len = __STACK_SIZE;
}

static inline void _init_initrd(struct uk_bootinfo *bi)
{
	uintptr_t heap0_start, heap0_end;
	uintptr_t heap1_start, heap1_end;
	size_t heap0_len, heap1_len;

	if (!bi->has_initrd)
		goto no_initrd;

	_libkvmplat_cfg.initrd.start = bi->initrd_start;
	_libkvmplat_cfg.initrd.end   = bi->initrd_end;
	_libkvmplat_cfg.initrd.len   = bi->initrd_length;

	/*
	 * Check if initrd is part of heap
	 * In such a case, we figure out the remaining pieces as heap
	 */
	if (_libkvmplat_cfg.heap.len == 0) {
		/* We do not have a heap */
		goto out;
	}
	heap0_start = 0;
	heap0_end   = 0;
	heap1_start = 0;
	heap1_end   = 0;
	if (RANGE_OVERLAP(_libkvmplat_cfg.heap.start,
			  _libkvmplat_cfg.heap.len,
			  _libkvmplat_cfg.initrd.start,
			  _libkvmplat_cfg.initrd.len)) {
		if (IN_RANGE(_libkvmplat_cfg.initrd.start,
			     _libkvmplat_cfg.heap.start,
			     _libkvmplat_cfg.heap.len)) {
			/* Start of initrd within heap range;
			 * Use the prepending left piece as heap */
			heap0_start = _libkvmplat_cfg.heap.start;
			heap0_end   = ALIGN_DOWN(_libkvmplat_cfg.initrd.start,
						 __PAGE_SIZE);
		}
		if (IN_RANGE(_libkvmplat_cfg.initrd.start,

			     _libkvmplat_cfg.heap.start,
			     _libkvmplat_cfg.heap.len)) {
			/* End of initrd within heap range;
			 * Use the remaining left piece as heap */
			heap1_start = ALIGN_UP(_libkvmplat_cfg.initrd.end,
					       __PAGE_SIZE);
			heap1_end   = _libkvmplat_cfg.heap.end;
		}
	} else {
		/* Initrd is not overlapping with heap */
		heap0_start = _libkvmplat_cfg.heap.start;
		heap0_end   = _libkvmplat_cfg.heap.end;
	}
	heap0_len = heap0_end - heap0_start;
	heap1_len = heap1_end - heap1_start;

	/*
	 * Update heap regions
	 * We make sure that in we start filling left heap pieces at
	 * `_libkvmplat_cfg.heap`. Any additional piece will then be
	 * placed to `_libkvmplat_cfg.heap2`.
	 */
	if (heap0_len == 0) {
		/* Heap piece 0 is empty, use piece 1 as only */
		if (heap1_len != 0) {
			_libkvmplat_cfg.heap.start = heap1_start;
			_libkvmplat_cfg.heap.end   = heap1_end;
			_libkvmplat_cfg.heap.len   = heap1_len;
		} else {
			_libkvmplat_cfg.heap.start = 0;
			_libkvmplat_cfg.heap.end   = 0;
			_libkvmplat_cfg.heap.len   = 0;
		}
		_libkvmplat_cfg.heap2.start = 0;
		_libkvmplat_cfg.heap2.end   = 0;
		_libkvmplat_cfg.heap2.len   = 0;
	} else {
		/* Heap piece 0 has memory */
		_libkvmplat_cfg.heap.start = heap0_start;
		_libkvmplat_cfg.heap.end   = heap0_end;
		_libkvmplat_cfg.heap.len   = heap0_len;
		if (heap1_len != 0) {
			_libkvmplat_cfg.heap2.start = heap1_start;
			_libkvmplat_cfg.heap2.end   = heap1_end;
			_libkvmplat_cfg.heap2.len   = heap1_len;
		} else {
			_libkvmplat_cfg.heap2.start = 0;
			_libkvmplat_cfg.heap2.end   = 0;
			_libkvmplat_cfg.heap2.len   = 0;
		}
	}

	/*
	 * Double-check that initrd is not overlapping with previously allocated
	 * boot stack. We crash in such a case because we assume that multiboot
	 * places the initrd close to the beginning of the heap region. One need
	 * to assign just more memory in order to avoid this crash.
	 */
	if (RANGE_OVERLAP(_libkvmplat_cfg.heap.start, _libkvmplat_cfg.heap.len,
			  _libkvmplat_cfg.initrd.start,
			  _libkvmplat_cfg.initrd.len))
		UK_CRASH("Not enough space at end of memory for boot stack\n");
out:
	return;

no_initrd:
	_libkvmplat_cfg.initrd.start = 0;
	_libkvmplat_cfg.initrd.end   = 0;
	_libkvmplat_cfg.initrd.len   = 0;
	_libkvmplat_cfg.heap2.start  = 0;
	_libkvmplat_cfg.heap2.end    = 0;
	_libkvmplat_cfg.heap2.len    = 0;
	return;
}

#ifdef CONFIG_PAGING
/* TODO: Find an appropriate solution to manage the address space layout
 * without the presence of any more advanced virtual memory management.
 * For now, we simply map the heap statically at 0x400000000 (16 GiB).
 */
#define PG_HEAP_MAP_START			(1UL << 34) /* 16 GiB */

/* Initial page table struct used for paging API to absorb statically defined
 * startup page table.
 */
static struct uk_pagetable kernel_pt;

static void _init_paging(struct multiboot_info *mi)
{
	struct kvmplat_config_memregion *mr[2];
	multiboot_memory_map_t *m;
	__sz offset, len;
	__paddr_t start, paddr;
	__vaddr_t vaddr;
	__sz free_memory, res_memory;
	unsigned long frames;
	int rc;

	/* Initialize the frame allocator by taking away the memory from the
	 * larger heap area. We setup a new heap area later.
	 */
	if (_libkvmplat_cfg.heap2.len > _libkvmplat_cfg.heap.len) {
		mr[0] = &_libkvmplat_cfg.heap2;
		mr[1] = &_libkvmplat_cfg.heap;
	} else {
		mr[0] = &_libkvmplat_cfg.heap;
		mr[1] = &_libkvmplat_cfg.heap2;
	}

	offset = mr[0]->start - PAGE_ALIGN_UP(mr[0]->start);
	start  = PAGE_ALIGN_UP(mr[0]->start);
	len    = PAGE_ALIGN_DOWN(mr[0]->len - offset);

	rc = ukplat_pt_init(&kernel_pt, start, len);
	if (unlikely(rc))
		goto EXIT_FATAL;

	/* Also add memory of the smaller heap region. Since the region might
	 * be as small as a single page or less, we do not treat errors as
	 * fatal here
	 */
	offset = mr[1]->start - PAGE_ALIGN_UP(mr[1]->start);
	start  = PAGE_ALIGN_UP(mr[1]->start);
	len    = PAGE_ALIGN_DOWN(mr[1]->len - offset);

	ukplat_pt_add_mem(&kernel_pt, start, len);

	/* Switch to new page table */
	rc = ukplat_pt_set_active(&kernel_pt);
	if (unlikely(rc))
		goto EXIT_FATAL;

	/* Unmap all 1:1 mappings extending over the first megabyte, the kernel
	 * image and initrd. The boot page table maps the first 4 GiB with
	 * everything starting from 2 MiB mapped as 2 MiB large pages (see
	 * pagetable64.S).
	 */
	offset = mr[0]->start - PAGE_LARGE_ALIGN_UP(mr[0]->start);
	start  = PAGE_LARGE_ALIGN_UP(mr[0]->start);
	len    = PAGE_LARGE_ALIGN_DOWN(mr[0]->len - offset);

	rc = ukplat_page_unmap(&kernel_pt, start,
			       (PLATFORM_MAX_MEM_ADDR - start) >> PAGE_SHIFT,
			       PAGE_FLAG_KEEP_FRAMES);
	if (unlikely(rc))
		goto EXIT_FATAL;

	/* Add remaining physical memory that has not been added to the heaps
	 * previously. Also map regions marked as RESERVED 1:1 because this
	 * will include memory-mapped ACPI and APIC tables and registers. For
	 * now, we map these ranges as read-only.
	 */
	for (offset = 0; offset < mi->mmap_length;
	     offset += m->size + sizeof(m->size)) {
		m = (void *)(__uptr)(mi->mmap_addr + offset);

		if (m->addr <= PLATFORM_MEM_START)
			continue;

		if (m->type == MULTIBOOT_MEMORY_RESERVED) {
			vaddr  = paddr = PAGE_ALIGN_DOWN(m->addr);
			frames = DIV_ROUND_UP(m->len, PAGE_SIZE);

			rc = ukplat_page_map(&kernel_pt, vaddr, paddr, frames,
					     PAGE_ATTR_PROT_READ, 0);
			if (unlikely(rc))
				goto EXIT_FATAL;
		} else if (m->type == MULTIBOOT_MEMORY_AVAILABLE) {
			rc = ukplat_pt_add_mem(&kernel_pt, m->addr, m->len);
			if (unlikely(rc))
				goto EXIT_FATAL;
		}
	}

	/* Setup and map heap */

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

	res_memory = __STACK_SIZE;			/* boot stack */
	res_memory += PT_PAGES(frames) << PAGE_SHIFT;	/* page tables */

	_libkvmplat_cfg.heap.start = PG_HEAP_MAP_START;
	_libkvmplat_cfg.heap.end = PG_HEAP_MAP_START + free_memory - res_memory;
	_libkvmplat_cfg.heap.len = _libkvmplat_cfg.heap.end -
				   _libkvmplat_cfg.heap.start;

	uk_pr_info("HEAP area @ %"__PRIpaddr" - %"__PRIpaddr
		   " (%"__PRIsz" bytes)\n",
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
	_libkvmplat_cfg.bstack.len = _libkvmplat_cfg.bstack.end -
				     _libkvmplat_cfg.bstack.start;

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
#define _init_paging(mi) do { } while (0)
#endif /* CONFIG_PAGING */

static void __noreturn _libkvmplat_entry2(void)
{
	ukplat_entry_argp(NULL, cmdline, sizeof(cmdline));

	ukplat_lcpu_halt();
}

void _libkvmplat_entry(struct lcpu *lcpu, void *arg)
{
	struct multiboot_info *mi = (struct multiboot_info *)arg;
	int rc;

	_libkvmplat_init_console();

	/* Initialize trap vector table */
	traps_table_init();

	/* Initialize LCPU of bootstrap processor */
	rc = lcpu_init(lcpu);
	if (unlikely(rc))
		UK_CRASH("Failed to init bootstrap processor\n");

	intctrl_init();

	uk_pr_info("Entering from KVM (x86)...\n");
	uk_pr_info("     multiboot: %p\n", mi);

	_convert_mbinfo(mi);

	/*
	 * The multiboot structures may be anywhere in memory, so take a copy of
	 * everything necessary before we initialise memory allocation.
	 */
	_get_cmdline(&bootinfo);
	_init_mem(&bootinfo);
	_init_initrd(&bootinfo);
#ifdef CONFIG_PAGING
	_init_paging(mi);
#endif /* CONFIG_PAGING */

	if (_libkvmplat_cfg.initrd.len)
		uk_pr_info("        initrd: %p\n",
			   (void *)_libkvmplat_cfg.initrd.start);
	uk_pr_info("    heap start: %p\n", (void *)_libkvmplat_cfg.heap.start);
	if (_libkvmplat_cfg.heap2.len)
		uk_pr_info(" heap start (2): %p\n",
			   (void *)_libkvmplat_cfg.heap2.start);
	uk_pr_info("     stack top: %p\n",
		   (void *)_libkvmplat_cfg.bstack.start);

#ifdef CONFIG_HAVE_SMP
	rc = acpi_init();
	if (likely(rc == 0)) {
		rc = lcpu_mp_init(CONFIG_UKPLAT_LCPU_RUN_IRQ,
				  CONFIG_UKPLAT_LCPU_WAKEUP_IRQ,
				  NULL);
		if (unlikely(rc))
			uk_pr_err("SMP init failed: %d\n", rc);
	} else {
		uk_pr_err("ACPI init failed: %d\n", rc);
	}
#endif /* CONFIG_HAVE_SMP */

#ifdef CONFIG_HAVE_SYSCALL
	_init_syscall();
#endif /* CONFIG_HAVE_SYSCALL */

#if CONFIG_HAVE_X86PKU
	_check_ospke();
#endif /* CONFIG_HAVE_X86PKU */

	/*
	 * Switch away from the bootstrap stack as early as possible.
	 */
	uk_pr_info("Switch from bootstrap stack to stack @%p\n",
		   (void *)_libkvmplat_cfg.bstack.end);
	lcpu_arch_jump_to((void *)_libkvmplat_cfg.bstack.end,
			  _libkvmplat_entry2);
}
