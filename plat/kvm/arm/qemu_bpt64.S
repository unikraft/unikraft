/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <arm/arm64/pagetable.S>

#include <uk/config.h>
#include <uk/reloc.h>

/* ------------------------- Memory Map of QEMU virt -----------------------
 *
 * 0x0000000000000000 - 0x0000000007ffffff	Hole:          0    - 128MiB
 * 0x0000000008000000 - 0x000000003fffffff	Devices:     128MiB -   1GiB
 * 0x0000000040000000 - 0x0000007fffffffff	Kernel:        1GiB - 512GiB
 * 0x0000008000000000 - 0x000000ffffffffff	PCIe hi mem: 512GiB -   1TiB
 *
 * Notice: The page tables below use the Unikraft indexing convention (x86).
 */
.section .data
.align 4
.global bpt_unmap_mrd
bpt_unmap_mrd:
	.quad	0x0000000040000000		/* 1 GiB */
	.quad	0x0000000040000000		/* 1 GiB */
	.quad	0x0000000000000000		/* Page-aligned */
	/* QEMU-VIRT's legacy RAM max is 255 GiB, but it can also depend on the
	 * settings, see QEMU upstream commit:
	 * 50a17297e2f0c ("hw/arm/virt: Bump the 255GB initial RAM limit").
	 * In our case, here we assume that the RAM can only go up to the PCIe
	 * high memory.
	 */
	.quad	(255 - 1) * 0x0000000040000000
	.quad	(255 - 1) * 0x0000000000040000  /* Page count */
	.short	0x0000000000000000
	.short	0x0000000000000010		/* UKPLAT_MEMRF_UNMAP */
	.space	36

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
#if CONFIG_PAGING
	ur_pte  arm64_bpt_l2_pt511, PTE_TYPE_TABLE
#else /* !CONFIG_PAGING */
	pte_zero	, 1
#endif /* !CONFIG_PAGING */

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

#if CONFIG_PAGING
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
.globl arm64_bpt_l0_pt0
arm64_bpt_l0_pt0:
	pte_fill	0x0000000040000000, 512, 0, PTE_PAGE_NORMAL_RWX
