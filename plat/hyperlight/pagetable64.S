/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/*
 * Boot page table metadata for Hyperlight.
 *
 * Unlike Unikraft's KVM platform (plat/kvm), we do NOT define guest-
 * side page tables here.  Hyperlight creates page tables on the host
 * side and loads them into CR3 before guest entry — the guest never
 * switches CR3.  The CoW page fault handler walks and modifies those
 * host-provided tables in place.
 *
 * We only define bpt_unmap_mrd, which plat/common/memory.c uses to
 * decide which memory regions are usable.  It computes the upper
 * bound as (vbase + len) and filters out free regions above that.
 * We set a conservative 4 GiB upper bound — the actual mapped range
 * is much smaller (only the guest's code, PEB, heap, and scratch
 * regions).
 */

#include <uk/config.h>

/*
 * bpt_unmap_mrd: a memregion_desc that tells the common memory code
 * how much address space the boot page tables cover.
 *
 * struct ukplat_memregion_desc {
 *   __paddr_t pbase;     +0x00
 *   __vaddr_t vbase;     +0x08
 *   __off     pg_off;    +0x10
 *   __sz      len;       +0x18
 *   __sz      pg_count;  +0x20
 *   __u16     type;      +0x28
 *   __u16     flags;     +0x2a  (UKPLAT_MEMRF_UNMAP = 0x0010)
 *   // padding/reserved  +0x2c..+0x4f
 * };
 */
.align 4
.global bpt_unmap_mrd
bpt_unmap_mrd:
	.quad	0x0000000000000000	/* pbase */
	.quad	0x0000000000000000	/* vbase */
	.quad	0x0000000000000000	/* pg_off */
	.quad	0x0000000100000000	/* len: 4 GiB */
	.quad	0x0000000000100000	/* pg_count: 1M pages */
	.short	0x0000			/* type: UKPLAT_MEMRT_RESERVED */
	.short	0x0010			/* flags: UKPLAT_MEMRF_UNMAP */
	.fill	36, 1, 0		/* padding to full struct size */
