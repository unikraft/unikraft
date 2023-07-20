/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <arm/arm64/pagetable.S>

#include <uk/config.h>
#include <uk/reloc.h>

/* ------------------- Memory Map of Firecracker on Arm64 -----------------
 *
 * 0x0000000000000000 - 0x000000007fffffff	Devices:     0 -    2GiB
 * 0x0000000080000000 - 0x000000ffffffffff	DRAM:     2GiB - 1024GiB
 *
 * Notice: The page tables below use the Unikraft indexing convention (x86).
 */
.section .data
.align 4
.global bpt_unmap_mrd
bpt_unmap_mrd:
	.quad	0x0000000080000000		/* 1 GiB */
	.quad	0x0000000080000000		/* 1 GiB */
	/* FIXME: Unmap to 1TiB */
	.quad	(255 - 1) * 0x0000000080000000
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
 * 0x0000000000000000 - 0x000000007fffffff	Devices
 * 0x0000000080000000 - 0x00000000bfffffff	Table descriptor to l1_pt1
 * 0x00000000c0000000 - 0x0000007fffffffff	RAM @ 3GiB
 */
.align 12
arm64_bpt_l2_pt0:
	pte_fill	0x0000000000000000, 2, 2, PTE_BLOCK_DEVICE_nGnRnE
	ur_pte  arm64_bpt_l1_pt0, PTE_TYPE_TABLE
	pte_fill	0x00000000c0000000, 509, 2, PTE_BLOCK_NORMAL_RW

/* L2: 512GiB - 1TiB (1GiB / entry)
 *
 * 0x0000008000000000 - 0x000000ffffffffff	RAM @ 512GiB
 */
.align 12
arm64_bpt_l2_pt1:
	pte_fill	0x0000008000000000, 512, 2, PTE_BLOCK_NORMAL_RW

#if CONFIG_PAGING
/* L2: 255.5 TiB - 256TiB (1GiB / entry)
 *
 * 0x0000ff8000000000 - 0x0000ffffffffffff	Direct-mapped
 */
.align 12
arm64_bpt_l2_pt511:
	pte_fill	0x0000000000000000, 512, 2, PTE_BLOCK_NORMAL_RW
#endif /* CONFIG_PAGING */

/* L1: 2GiB - 3GiB (2MiB / entry)
 *
 * 0x0000000080000000 - 0x00000000801fffff	Table descriptor to l0_pt0
 * 0x0000000080200000 - 0x00000000bfffffff	RAM @ 2.2GiB
 */
.align 12
arm64_bpt_l1_pt0:
	ur_pte  arm64_bpt_l0_pt0, PTE_TYPE_TABLE
	pte_fill	0x0000000080200000, 511, 1, PTE_BLOCK_NORMAL_RWX

/* L0: 2GiB - 2.2GiB (4KiB / entry)
 *
 * 0x0000000080000000 - 0x00000000801fffff	RAM @ 2GiB
 */
.align 12
.globl arm64_bpt_l0_pt0
arm64_bpt_l0_pt0:
	pte_fill	0x0000000080000000, 512, 0, PTE_PAGE_NORMAL_RWX
