/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *
 * Copyright (c) 2021, Karlsruhe Institute of Technology (KIT).
 *                     All rights reserved.
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
#include <uk/arch/paging.h>
#include <uk/reloc.h>

#define PTE_RO		X86_PTE_PRESENT

#ifdef CONFIG_LIBUKSEV
#define PTE_RW		(X86_PTE_PRESENT | X86_PTE_RW | X86_PTE_ENCRYPT)
#define PTE_RW_SHARED		(X86_PTE_PRESENT | X86_PTE_RW)
#else
#define PTE_RW		(X86_PTE_PRESENT | X86_PTE_RW)
#endif  /* CONFIG_LIBUKSEV */

#define PT_LVL		0
#define PD_LVL		1
#define PDPT_LVL	2
#define PML4_LVL	3

/**
 * Outputs a single page table entry
 *
 * @param paddr physical address for PTE
 * @param pte additional flags for PTE
 */
.macro pte paddr, pte
	.quad \paddr + \pte
.endm

/**
 * Outputs a single page table entry that automatically contains the large/huge
 * page flag depending on the page table level
 *
 * @param paddr physical address for PTE
 * @param level level of the page table the PTE is intended for
 * @param pte additional flags for PTE
 */
.macro pte_page paddr, level, pte
.ifeq (\level - PAGE_HUGE_LEVEL)
	pte \paddr, (\pte + X86_PTE_PSE)
	.exitm
.endif
.ifeq (\level - PAGE_LARGE_LEVEL)
	pte \paddr, (\pte + X86_PTE_PSE)
.else
	pte \paddr, \pte
.endif
.endm

/**
 * Outputs a number of page table entries for a continuous mapping starting at
 * the provided physical address. Page size is chosen according to level.
 *
 * @param paddr physical address of the beginning of the area to map
 * @param pages number of pages to map
 * @param level level of the page table the PTEs are intended for
 * @param pte additional flags for PTE
 */
.macro pte_fill paddr, pages, lvl, pte
.ifle \pages
	.exitm
.endif
	pte_page (\paddr + PAGE_Lx_SIZE(\lvl) * 0), \lvl, \pte
.ifgt \pages - 8
	/* Have to do some unrolling to not exceed max nested macros */
	pte_page (\paddr + PAGE_Lx_SIZE(\lvl) * 1), \lvl, \pte
	pte_page (\paddr + PAGE_Lx_SIZE(\lvl) * 2), \lvl, \pte
	pte_page (\paddr + PAGE_Lx_SIZE(\lvl) * 3), \lvl, \pte
	pte_page (\paddr + PAGE_Lx_SIZE(\lvl) * 4), \lvl, \pte
	pte_page (\paddr + PAGE_Lx_SIZE(\lvl) * 5), \lvl, \pte
	pte_page (\paddr + PAGE_Lx_SIZE(\lvl) * 6), \lvl, \pte
	pte_page (\paddr + PAGE_Lx_SIZE(\lvl) * 7), \lvl, \pte
	pte_fill (\paddr + PAGE_Lx_SIZE(\lvl) * 8), (\pages - 8), \lvl, \pte
.else
	pte_fill (\paddr + PAGE_Lx_SIZE(\lvl) * 1), (\pages - 1), \lvl, \pte
.endif
.endm

/**
 * Outputs a number of non-present page table entries
 *
 * @param paddr physical address (not used)
 * @param pages number of PTEs
 */
.macro pte_zero paddr=0, pages
	.fill \pages, 0x8, 0
.endm

.align 4
.global bpt_unmap_mrd
bpt_unmap_mrd:
	.quad	0x0000000000000000		/* 0 GiB */
	.quad	0x0000000000000000		/* 0 GiB */
	.quad	0x0000000000000000		/* Page-aligned */
	.quad	0x0000000100000000		/* 4 GiB */
	.quad	0x0000000000100000		/* Page count */
	.short	0x0000000000000000
	.short	0x0000000000000010		/* UKPLAT_MEMRF_UNMAP */
	.fill	36, 1, 0

/* x86-64 Boot Page Table
 *
 * We map the first 4GB using max 2MB pages to keep compatibility for systems
 * without 1GB page support. If the paging API is enabled, we also do a 1:1
 * mapping of the first 512GB of physical memory at the high end of the address
 * space. We use 1GB pages for this. The paging API can thus only be used on
 * systems supporting 1GB pages.
 *
 * 0x0000000000000000 - 0x00000000ffffffff Mapping of first 4GB
 * However, the first page is inaccessible.
 *
 * If paging is enabled:
 * 0xffffff8000000000 - 0xffffffffffffffff Mapping of first 512GB (for PTs)
 */
.align 0x1000
x86_bpt_pt0_0_0: /* 4K pages */
	pte_zero 0x0000000000000000, 0x001
	pte_fill 0x0000000000001000, 0x1ff, PT_LVL, PTE_RW

.align 0x1000
x86_bpt_pd0_0: /* 2M pages */
	ur_pte x86_bpt_pt0_0_0, PTE_RW
	pte_fill 0x0000000000200000, 0x1ff, PD_LVL, PTE_RW

x86_bpt_pd0_1: /* 2M pages */
	pte_fill 0x0000000040000000, 0x200, PD_LVL, PTE_RW

x86_bpt_pd0_2: /* 2M pages */
	pte_fill 0x0000000080000000, 0x200, PD_LVL, PTE_RW

x86_bpt_pd0_3: /* 2M pages */
	pte_fill 0x00000000c0000000, 0x200, PD_LVL, PTE_RW

.align 0x1000
x86_bpt_pdpt0: /* 1G pages */
	ur_pte	x86_bpt_pd0_0, PTE_RW
	ur_pte	x86_bpt_pd0_1, PTE_RW
	ur_pte	x86_bpt_pd0_2, PTE_RW
	ur_pte	x86_bpt_pd0_3, PTE_RW
	pte_zero , 0x1fc

/* Page table for 512 GiB direct-mapped physical memory */
#ifdef CONFIG_PAGING
.align 0x1000
x86_bpt_pdpt511: /* 1G pages */
	pte_fill 0x0000000000000000, 0x200, PDPT_LVL, PTE_RW
#endif /* CONFIG_PAGING */

.align 0x1000
.global x86_bpt_pml4
x86_bpt_pml4:
	ur_pte	x86_bpt_pdpt0, PTE_RW
	pte_zero , 0x1fe
#ifdef CONFIG_PAGING
	ur_pte	x86_bpt_pdpt511, PTE_RW
#else
	pte_zero , 0x001
#endif /* CONFIG_PAGING */
