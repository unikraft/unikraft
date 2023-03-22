/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Michalis Pappas <michalis.pappas@opensynergy.com>
 *
 * Based on plat/kvm/x86/pagetable64.S by Marc Rittinghaus.
 *
 * Copyright (c) 2022, OpenSynergy GmbH. All rights reserved.
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
#include <uk/arch/lcpu.h>
#include <uk/arch/paging.h>
#include <uk/asm.h>
#include <arch/reloc.h>

#define PTE_BLOCK_DEVICE_nGnRnE	(PTE_ATTR_DEVICE_nGnRnE + PTE_TYPE_BLOCK)
#define PTE_BLOCK_DEVICE_nGnRE	(PTE_ATTR_DEVICE_nGnRE + PTE_TYPE_BLOCK)
#define PTE_BLOCK_NORMAL_RW	(PTE_ATTR_NORMAL_RW + PTE_TYPE_BLOCK)
#define PTE_BLOCK_NORMAL_RWX	(PTE_ATTR_NORMAL_RWX + PTE_TYPE_BLOCK)
#define PTE_PAGE_NORMAL_RO	(PTE_ATTR_NORMAL_RO  + PTE_TYPE_PAGE)
#define PTE_PAGE_NORMAL_RWX	(PTE_ATTR_NORMAL_RWX  + PTE_TYPE_PAGE)

/**
 * Outputs a single table descriptor
 *
 * @param paddr physical address of the linked PT
 */
.macro pte_tbl_desc paddr
	.quad \paddr + PTE_TYPE_TABLE
.endm

/**
 * Outputs a single block / page descriptor
 *
 * @param paddr physical address
 * @param attr  PT attributes
 */
.macro pte_map_desc paddr, attr
	.quad \paddr + \attr
.endm

/* Outputs a number of block / table descriptors for a contiguous
 * mapping, starting at the provided physical address.
 *
 * @param paddr physical address of the beginning of the area to map
 * @param pages number of pages to map
 * @param level level of the page table the PTEs are intended for
 * @param pte additional flags for PTE
 */
.macro pte_fill paddr, pages, lvl, attr
.ifle \pages
	.exitm
.endif
	pte_map_desc (\paddr + PAGE_Lx_SIZE(\lvl) * 0), \attr
.ifgt (\pages - 8)
	/* Have to do some unrolling to not exceed max nested macros */
	pte_map_desc (\paddr + PAGE_Lx_SIZE(\lvl) * 1), \attr
	pte_map_desc (\paddr + PAGE_Lx_SIZE(\lvl) * 2), \attr
	pte_map_desc (\paddr + PAGE_Lx_SIZE(\lvl) * 3), \attr
	pte_map_desc (\paddr + PAGE_Lx_SIZE(\lvl) * 4), \attr
	pte_map_desc (\paddr + PAGE_Lx_SIZE(\lvl) * 5), \attr
	pte_map_desc (\paddr + PAGE_Lx_SIZE(\lvl) * 6), \attr
	pte_map_desc (\paddr + PAGE_Lx_SIZE(\lvl) * 7), \attr
	pte_fill    (\paddr + PAGE_Lx_SIZE(\lvl) * 8), (\pages - 8), \lvl, \attr
.else
	pte_fill (\paddr + PAGE_Lx_SIZE(\lvl) * 1), (\pages - 1), \lvl, \attr
.endif
.endm

/* Outputs a number of invalid PTEs
 *
 * @param paddr physical address (not used)
 * @param pages number of PTEs
 */
.macro pte_zero paddr=0, pages
	.fill \pages, 0x8, 0
.endm

/* ----------------------- KVM MEMORY MAP (QEMU virt) ---------------------
 *
 * 0x0000000000000000 - 0x0000000007ffffff	Hole:          0    - 128MiB
 * 0x0000000008000000 - 0x000000003fffffff	Devices:     128MiB -   1GiB
 * 0x0000000040000000 - 0x0000007fffffffff	Kernel:        1GiB - 512GiB
 * 0x0000008000000000 - 0x000000ffffffffff	PCIe hi mem: 512GiB -   1TiB
 *
 * Notice: The page tables below use the Unikraft indexing convention (x86).
 */
.section .data

.global arm64_bpt_l3_pt0

/* L3: 0 - 2TiB (512GiB / entry)
 *
 * 0x0000000000000000 - 0x0000007fffffffff	Table descriptor to l2_pt0
 * 0x0000008000000000 - 0x000000ffffffffff	Table descriptor to l2_pt1
 * 0x0000010000000000 - 0x0000ff7fffffffff	Unmapped
 * 0x0000ff8000000000 - 0x0000ffffffffffff	Table descriptor to l2_pt511
 */
.align 12
arm64_bpt_l3_pt0:
	ur_pte  arm64_bpt_l2_pt0, PTE_TYPE_TABLE
	ur_pte  arm64_bpt_l2_pt1, PTE_TYPE_TABLE
	pte_zero	, 509
#ifdef CONFIG_PAGING
	ur_pte  arm64_bpt_l2_pt511, PTE_TYPE_TABLE
#else
	pte_zero	, 1
#endif /* CONFIG_PAGING */

/* L2: 0 - 512GiB (1GiB / entry)
 *
 * 0x0000000000000000 - 0x000000003fffffff	Table descriptor to l1_pt0
 * 0x0000000040000000 - 0x000000007fffffff	Table descriptor to l1_pt1
 * 0x0000000080000000 - 0x000000003fffffff	RAM       @   2GiB
 * 0x0000000400000000 - 0x0000007fffffffff	PCIe ECAM @ 256GiB
 */
.align 12
arm64_bpt_l2_pt0:
	ur_pte  arm64_bpt_l1_pt0, PTE_TYPE_TABLE
	ur_pte  arm64_bpt_l1_pt1, PTE_TYPE_TABLE
	pte_fill	0x0000000080000000, 254, 2, PTE_BLOCK_NORMAL_RW
	pte_fill	0x0000004000000000, 256, 2, PTE_BLOCK_DEVICE_nGnRnE

/* L2: 512GiB - 1TiB (1GiB / entry)
 *
 * 0x0000008000000000 - 0x000000ffffffffff	PCIe hi-mem @ 512GiB
 */
.align 12
arm64_bpt_l2_pt1:
	pte_fill	0x0000008000000000, 512, 2, PTE_BLOCK_DEVICE_nGnRE

#ifdef CONFIG_PAGING
/* L2: 255.5 TiB - 256TiB (1GiB / entry)
 *
 * 0x0000ff8000000000 - 0x0000ffffffffffff	Direct-mapped
 */
.align 12
arm64_bpt_l2_pt511:
	pte_fill	0x0000000000000000, 512, 2, PTE_BLOCK_NORMAL_RW
#endif /* CONFIG_PAGING */

/* L1: 0 - 1GiB (2MiB / entry)
 *
 * 0x0000000000000000 - 0x0000000007ffffff	Hole:   @ 0
 * 0x0000000008000000 - 0x000000003fffffff	Devices @ 128MiB
 */
.align 12
arm64_bpt_l1_pt0:
	pte_zero	0x0000000000000000, 64
	pte_fill	0x0000000008000000, 448, 1, PTE_BLOCK_DEVICE_nGnRE

/* L1: 1GiB - 2GiB (2MiB / entry)
 *
 * 0x0000000000000000 - 0x00000000001fffff	Table descriptor to l0_pt0
 * 0x0000000040200000 - 0x000000007fffffff	RAM	@ 1.2GiB
 */
.align 12
arm64_bpt_l1_pt1:
	ur_pte  arm64_bpt_l0_pt0, PTE_TYPE_TABLE
	pte_fill	0x0000000040200000, 511, 1, PTE_BLOCK_NORMAL_RWX

/* L0: 1GiB - 1.2GiB (4KiB / entry)
 *
 * 0x0000000040000000 - 0x00000000401fffff	Kernel	@ 1GiB
 */
.align 12
arm64_bpt_l0_pt0:
	pte_fill	0x0000000040000000, 256, 0, PTE_PAGE_NORMAL_RO
	pte_fill	0x0000000040100000, 256, 0, PTE_PAGE_NORMAL_RWX
