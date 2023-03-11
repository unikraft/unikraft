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

static char cmdline[MAX_GUEST_CMDLINE];

start_info_t *HYPERVISOR_start_info;
shared_info_t *HYPERVISOR_shared_info;

/*
 * Just allocate the kernel stack here. SS:ESP is set up to point here
 * in head.S.
 */
char _libxenplat_bootstack[2*__STACK_SIZE];

/*
 * Memory region description
 */
#define UKPLAT_MEMRD_MAX_ENTRIES 3
unsigned int _libxenplat_mrd_num;
struct ukplat_memregion_desc _libxenplat_mrd[UKPLAT_MEMRD_MAX_ENTRIES];

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

static inline void _init_mem(void)
{
	unsigned long start_pfn, max_pfn;

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
	_libxenplat_mrd[0].pbase = start_pfn << PAGE_SHIFT;
	_libxenplat_mrd[0].vbase = (__vaddr_t)to_virt(_libxenplat_mrd[0].pbase);
	_libxenplat_mrd[0].len   = (max_pfn - start_pfn) << PAGE_SHIFT;
	_libxenplat_mrd[0].type  = UKPLAT_MEMRT_FREE;
	_libxenplat_mrd[0].flags = 0;
#if CONFIG_UKPLAT_MEMRNAME
	strncpy(_libxenplat_mrd[0].name, "heap",
		sizeof(_libxenplat_mrd[0].name) - 1);
#endif

	/* demand area */
	_libxenplat_mrd[1].pbase = __PADDR_MAX;
	_libxenplat_mrd[1].vbase = VIRT_DEMAND_AREA;
	_libxenplat_mrd[1].len   = DEMAND_MAP_PAGES * PAGE_SIZE;
	_libxenplat_mrd[1].type  = UKPLAT_MEMRT_RESERVED;
	_libxenplat_mrd[1].flags = 0;
#if CONFIG_UKPLAT_MEMRNAME
	strncpy(_libxenplat_mrd[1].name, "demand",
		sizeof(_libxenplat_mrd[1].name) - 1);
#endif
	_init_mem_demand_area((unsigned long) _libxenplat_mrd[1].vbase,
			DEMAND_MAP_PAGES);

	_libxenplat_mrd_num = 2;

	/* initrd */
	if (HYPERVISOR_start_info->mod_len) {
		if (HYPERVISOR_start_info->flags & SIF_MOD_START_PFN) {
			_libxenplat_mrd[2].pbase =
				HYPERVISOR_start_info->mod_start;
			_libxenplat_mrd[2].vbase =
				(__vaddr_t)to_virt(_libxenplat_mrd[2].pbase);
		} else {
			_libxenplat_mrd[2].pbase =
				HYPERVISOR_start_info->mod_start;
			_libxenplat_mrd[2].vbase =
				_libxenplat_mrd[2].pbase;
		}
		_libxenplat_mrd[2].len   =
			(size_t) HYPERVISOR_start_info->mod_len;
		_libxenplat_mrd[2].type  = UKPLAT_MEMRT_INITRD;
		_libxenplat_mrd[2].flags = (UKPLAT_MEMRF_WRITE
					    | UKPLAT_MEMRF_MAP);
#if CONFIG_UKPLAT_MEMRNAME
		strncpy(_libxenplat_mrd[2].name, "initrd",
			sizeof(_libxenplat_mrd[2].name) - 1);
#endif
		_libxenplat_mrd_num++;
	}
}

void _libxenplat_x86entry(void *start_info) __noreturn;

void _libxenplat_x86entry(void *start_info)
{
	_init_traps();
	HYPERVISOR_start_info = (start_info_t *)start_info;
	prepare_console(); /* enables buffering for console */

	uk_pr_info("Entering from Xen (x86, PV)...\n");

	_init_shared_info(); /* remaps shared info */

	strncpy(cmdline, (char *)HYPERVISOR_start_info->cmd_line,
		MAX_GUEST_CMDLINE);

	/* Set up events. */
	init_events();

	uk_pr_info("    start_info: %p\n", HYPERVISOR_start_info);
	uk_pr_info("   shared_info: %p\n", HYPERVISOR_shared_info);
	uk_pr_info("hypercall_page: %p\n", hypercall_page);

	_init_mem();

	init_console();

#if CONFIG_HAVE_X86PKU
	_check_ospke();
#endif /* CONFIG_HAVE_X86PKU */

	ukplat_entry_argp(CONFIG_UK_NAME, cmdline, MAX_GUEST_CMDLINE);
}
