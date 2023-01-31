/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *          Eduard Vintila <eduard.vintila47@gmail.com>
 *
 * Copyright (c) 2021, Karlsruhe Institute of Technology (KIT).
 *                     All rights reserved.
 * Copyright (c) 2022, University of Bucharest. All rights reserved.
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

#include <uk/arch/paging.h>

#define PTE_RW 		(RISCV64_PTE_VALID | RISCV64_PTE_GLOBAL | \
			 RISCV64_PTE_RW)
#define PTE_RWX		(RISCV64_PTE_VALID | RISCV64_PTE_GLOBAL | \
			RISCV64_PTE_RW | RISCV64_PTE_EXEC)
#define PTE_LINK 	(RISCV64_PTE_VALID | RISCV64_PTE_GLOBAL | \
			 RISCV64_PTE_LINK)

#define L0_LVL		0
#define L1_LVL		1
#define L2_LVL		2

#define PADDR_TO_PPN(paddr) ((paddr) >> PAGE_SHIFT)
#define PTE_PPN(ppn) ((ppn) << 10)

/**
 * Outputs a single page table entry
 *
 * @param paddr physical address for PTE
 * @param pte additional flags for PTE
 */
.macro pte paddr, pte
	.quad PTE_PPN(PADDR_TO_PPN(\paddr)) + \pte
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
	pte (\paddr + PAGE_Lx_SIZE(\lvl) * 0), \pte
.ifgt \pages - 8
	/* Have to do some unrolling to not exceed max nested macros */
	pte (\paddr + PAGE_Lx_SIZE(\lvl) * 1), \pte
	pte (\paddr + PAGE_Lx_SIZE(\lvl) * 2), \pte
	pte (\paddr + PAGE_Lx_SIZE(\lvl) * 3), \pte
	pte (\paddr + PAGE_Lx_SIZE(\lvl) * 4), \pte
	pte (\paddr + PAGE_Lx_SIZE(\lvl) * 5), \pte
	pte (\paddr + PAGE_Lx_SIZE(\lvl) * 6), \pte
	pte (\paddr + PAGE_Lx_SIZE(\lvl) * 7), \pte
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

/*
 * This is a bare-bones static page table loaded at boot, which uses only
 * a root PT to map huge (1GiB) pages. If CONFIG_PAGING is selected,
 * finer granularity is later achieved by using the Virtual Memory API to
 * setup a more detailed page table structure.
 */

/* Root page table */
.balign PAGE_SIZE
riscv64_pt_l2_0:
	/* Map MMIO/PCIE space 0x0 - 0x7FFFFFFF as RW */
	pte_fill 0x0000000000000000, 0x002, PAGE_HUGE_LEVEL, PTE_RW
	/* Map 4GiB of RAM as RWX */
	pte_fill 0x0000000080000000, 0x004, PAGE_HUGE_LEVEL, PTE_RWX


	pte_zero 0x0000000180000000, 0x0FA
#ifdef CONFIG_PAGING
	/* Direct-map 0x4000000000 - 0x7fffffffff as RW */
	pte_fill 0x0000000000000000, 0x100, PAGE_HUGE_LEVEL, PTE_RW
#else
	pte_zero 0x0000000000000000, 0x100
#endif
