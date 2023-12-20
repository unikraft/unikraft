/* SPDX-License-Identifier: MIT */
/******************************************************************************
 * common.c
 *
 * Common stuff special to x86 goes here.
 *
 * Copyright (c) 2002-2003, K A Fraser & R Neugebauer
 * Copyright (c) 2005, Grzegorz Milos, Intel Research Cambridge
 * Copyright (c) 2017, Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */
/* mm.c
 ****************************************************************************
 * (C) 2003 - Rolf Neugebauer - Intel Research Cambridge
 * (C) 2005 - Grzegorz Milos - Intel Research Cambridge
 ****************************************************************************
 *
 *        File: mm.c
 *      Author: Rolf Neugebauer (neugebar@dcs.gla.ac.uk)
 *     Changes: Grzegorz Milos
 *
 *        Date: Aug 2003, chages Aug 2005
 *
 * Environment: Xen Minimal OS
 * Description: memory management related functions
 *              contains buddy page allocator from Xen.
 *
 ****************************************************************************
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <uk/arch/types.h>
#include <uk/arch/limits.h>
#include <uk/config.h>
#include <uk/print.h>
#include <uk/assert.h>
#include <uk/plat/config.h>
#include <uk/plat/console.h>
#include <uk/plat/bootstrap.h>
#include <uk/plat/common/bootinfo.h>
#include <x86/cpu.h>
#include <x86/traps.h>

#include <xen/xen.h>
#include <common/console.h>
#include <common/events.h>
#ifdef __X86_64__
#include <xen-x86/hypercall64.h>
#else
#include <xen-x86/hypercall32.h>
#endif
#include <xen-x86/irq.h>
#include <xen-x86/mm.h>
#include <xen-x86/setup.h>
#include <xen/arch-x86/cpuid.h>
#include <xen/arch-x86/hvm/start_info.h>

start_info_t *HYPERVISOR_start_info;
shared_info_t *HYPERVISOR_shared_info;

/*
 * Just allocate the kernel stack here. SS:ESP is set up to point here
 * in head.S.
 */
char _libxenplat_bootstack[2*__STACK_SIZE];

static inline void _init_traps(void)
{
	traps_lcpu_init(NULL);
}

static inline void _init_shared_info(void)
{
	int ret;
	unsigned long pa = HYPERVISOR_start_info->shared_info;
	extern char _libxenplat_shared_info[__PAGE_SIZE];

	if ((ret = HYPERVISOR_update_va_mapping(
		 (unsigned long)_libxenplat_shared_info, __pte(pa | 7),
		 UVMF_INVLPG)))
		UK_CRASH("Failed to map shared_info: %d\n", ret);
	HYPERVISOR_shared_info = (shared_info_t *)_libxenplat_shared_info;
}

static int _init_mem(struct ukplat_bootinfo *const bi)
{
	unsigned long start_pfn, max_pfn;
	struct ukplat_memregion_desc mrd;
	int rc;

	_init_mem_prepare(&start_pfn, &max_pfn);

	if (max_pfn >= MAX_MEM_SIZE / __PAGE_SIZE)
		max_pfn = MAX_MEM_SIZE / __PAGE_SIZE - 1;

	uk_pr_info("     start_pfn: %lx\n", start_pfn);
	uk_pr_info("       max_pfn: %lx\n", max_pfn);

	_init_mem_build_pagetable(&start_pfn, &max_pfn);
	_init_mem_clear_bootstrap();
	_init_mem_set_readonly((void *)__TEXT, (void *)__ERODATA);

	/* Fill out mrd array */
	/* heap */
	mrd = (struct ukplat_memregion_desc) {
		.vbase = (__vaddr_t)to_virt(start_pfn << PAGE_SHIFT),
		.pbase = start_pfn << PAGE_SHIFT,
		.len   = (max_pfn - start_pfn) << PAGE_SHIFT,
		.type  = UKPLAT_MEMRT_FREE,
		.flags = UKPLAT_MEMRF_READ | UKPLAT_MEMRF_WRITE |
			 UKPLAT_MEMRF_MAP,
	};
#if CONFIG_UKPLAT_MEMRNAME
	strncpy(mrd.name, "heap", sizeof(mrd.name) - 1);
#endif
	rc = ukplat_memregion_list_insert(&bi->mrds, &mrd);
	if (unlikely(rc < 0))
		return rc;

	mrd = (struct ukplat_memregion_desc) {
		.vbase = VIRT_DEMAND_AREA,
		.pbase = __PADDR_MAX,
		.len   = DEMAND_MAP_PAGES * PAGE_SIZE,
		.type  = UKPLAT_MEMRT_RESERVED,
		.flags = UKPLAT_MEMRF_READ | UKPLAT_MEMRF_MAP,
	};
#if CONFIG_UKPLAT_MEMRNAME
	strncpy(mrd.name, "demand", sizeof(mrd.name) - 1);
#endif
	rc = ukplat_memregion_list_insert(&bi->mrds, &mrd);
	if (unlikely(rc < 0))
		return rc;

	_init_mem_demand_area((unsigned long)mrd.vbase, DEMAND_MAP_PAGES);

	/* initrd */
	mrd = (struct ukplat_memregion_desc){0};
	if (HYPERVISOR_start_info->mod_len) {
		if (HYPERVISOR_start_info->flags & SIF_MOD_START_PFN) {
			mrd.pbase = HYPERVISOR_start_info->mod_start;
			mrd.vbase = (__vaddr_t)to_virt(mrd.pbase);
		} else {
			mrd.pbase = HYPERVISOR_start_info->mod_start;
			mrd.vbase = mrd.pbase;
		}
		mrd.len = (__sz)HYPERVISOR_start_info->mod_len;
		mrd.type = UKPLAT_MEMRT_INITRD;
		mrd.flags = UKPLAT_MEMRF_READ | UKPLAT_MEMRF_WRITE |
			    UKPLAT_MEMRF_MAP;
#if CONFIG_UKPLAT_MEMRNAME
		strncpy(mrd.name, "initrd", sizeof(mrd.name) - 1);
#endif
		rc = ukplat_memregion_list_insert(&bi->mrds, &mrd);
		if (unlikely(rc < 0))
			return rc;
	}

	ukplat_memregion_list_coalesce(&bi->mrds);

	return 0;
}

static char *cmdline;
static __sz cmdline_len;

static void _libxenplat_x86bootinfo_setup_cmdl(struct ukplat_bootinfo *bi)
{
	cmdline_len = strlen((char *)HYPERVISOR_start_info->cmd_line);
	if (!cmdline_len)
		cmdline_len = sizeof(CONFIG_UK_NAME) - 1;

	cmdline = ukplat_memregion_alloc(cmdline_len, UKPLAT_MEMRT_CMDLINE,
					 UKPLAT_MEMRF_READ | UKPLAT_MEMRF_MAP);
	if (unlikely(!cmdline))
		UK_CRASH("Could not allocate command-line memory");

	strncpy(cmdline, (const char *)HYPERVISOR_start_info->cmd_line,
		cmdline_len);
	cmdline[cmdline_len] = '\0';

	bi->cmdline = (__u64)cmdline;
	bi->cmdline_len = cmdline_len;

	/* Tag this scratch cmdline as a kernel resource, to distinguish it
	 * from the original cmdline obtained above
	 */
	cmdline = ukplat_memregion_alloc(cmdline_len, UKPLAT_MEMRT_KERNEL,
					 UKPLAT_MEMRF_READ | UKPLAT_MEMRF_MAP);
	if (unlikely(!cmdline))
		UK_CRASH("Could not allocate scratch command-line memory");

	strncpy(cmdline, (const char *)HYPERVISOR_start_info->cmd_line,
		cmdline_len);
	cmdline[cmdline_len] = '\0';
}

static void _libxenplat_x86bootinfo_setup(void)
{
	const char bl[] = "Xen";
	const char bp[] = "PVH";
	struct ukplat_bootinfo *bi;

	bi = ukplat_bootinfo_get();
	if (unlikely(!bi))
		UK_CRASH("Failed to get bootinfo\n");

	memcpy(bi->bootloader, bl, sizeof(bl));

	memcpy(bi->bootprotocol, bp, sizeof(bp));

	if (_init_mem(bi) < 0)
		UK_CRASH("Failed to initialize memory\n");

	_libxenplat_x86bootinfo_setup_cmdl(bi);
}

void _libxenplat_x86entry(void *start_info) __noreturn;
void _libxenplat_x86entry(void *start_info)
{
	_init_traps();
	HYPERVISOR_start_info = (start_info_t *)start_info;
	prepare_console(); /* enables buffering for console */

	uk_pr_info("Entering from Xen (x86, PV)...\n");

	_init_shared_info(); /* remaps shared info */

	/* Set up events. */
	init_events();

	uk_pr_info("    start_info: %p\n", HYPERVISOR_start_info);
	uk_pr_info("   shared_info: %p\n", HYPERVISOR_shared_info);
	uk_pr_info("hypercall_page: %p\n", hypercall_page);

	init_console();

	_libxenplat_x86bootinfo_setup();

#if CONFIG_HAVE_X86PKU
	_check_ospke();
#endif /* CONFIG_HAVE_X86PKU */

	ukplat_entry_argp(CONFIG_UK_NAME, cmdline, cmdline_len);
}
