/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/*
 * Hyperlight memory layout constants.
 *
 * Values must match hyperlight-common layout.rs and
 * arch/{amd64,aarch64}/{layout,vmem}.rs.
 */

#ifndef __HYPERLIGHT_MEM_H__
#define __HYPERLIGHT_MEM_H__

/*
 * Scratch region — always-writable memory at the top of the virtual
 * address space.  Used for I/O buffers, the guest stack, exception
 * stack, and CoW page allocation (bump allocator).
 *
 * The host stores metadata at fixed offsets below the scratch top.
 */
#if defined(__x86_64__)
#define HL_SCRATCH_TOP_GVA	0xFFFFFFFFFFFFEFFF
#elif defined(__aarch64__)
/* Lower half: Hyperlight enables only TTBR0 on arm64 */
#define HL_SCRATCH_TOP_GVA	0x0000FFFFFFFFDFFF
#endif

/* Offsets downward from (HL_SCRATCH_TOP_GVA + 1) */
#define HL_SCRATCH_SIZE_OFF	0x08 /* u64: total scratch size */
#define HL_SCRATCH_ALLOC_OFF	0x10 /* u64: bump allocator pointer */

/* Computed addresses */
#define HL_SCRATCH_SIZE_GVA	(HL_SCRATCH_TOP_GVA + 1 - HL_SCRATCH_SIZE_OFF)
#define HL_SCRATCH_ALLOC_GVA	(HL_SCRATCH_TOP_GVA + 1 - HL_SCRATCH_ALLOC_OFF)

#if defined(__x86_64__)
/*
 * x86-64 page table entry flags.
 *
 * Hyperlight pre-sets the Accessed and Dirty bits to prevent the CPU
 * from writing to the page tables (which would fault on CoW PTEs).
 */
#define PTE_PRESENT	(1ULL << 0)
#define PTE_RW		(1ULL << 1)
#define PTE_ACCESSED	(1ULL << 5)
#define PTE_DIRTY	(1ULL << 6)
#define PTE_NX		(1ULL << 63)

#define PTE_ADDR_MASK	0x000FFFFFFFFFF000ULL /* bits 51:12 */
#define PTE_AVL_COW	(1ULL << 9)           /* software CoW marker */

/*
 * Maximum guest physical / virtual addresses.  The scratch region
 * sits at the top of the 40-bit GPA space; the GVA alias is just
 * below the canonical hole.
 */
#define HL_MAX_GPA	0x0000000FFFFFFFFFULL
#define HL_MAX_GVA	HL_SCRATCH_TOP_GVA

#elif defined(__aarch64__)
/*
 * arm64 (VMSAv8-64, 4 KiB granule) level-3 descriptor bits.
 *
 * Hyperlight maps every page with AF set, so the CPU never updates a
 * descriptor, and marks a copy-on-write page read-only (AP[2]) with the
 * first software-reserved bit set.
 */
#define PTE_PRESENT	(1ULL << 0)
#define PTE_AP_RO	(1ULL << 7)           /* AP[2]: read-only */

#define PTE_ADDR_MASK	0x0000FFFFFFFFF000ULL /* bits 47:12 */
#define PTE_AVL_COW	(1ULL << 55)          /* software CoW marker */

/*
 * The scratch region sits just below the host-call I/O page at the top
 * of the 36-bit IPA space; its GVA alias just below the I/O page's.
 */
#define HL_MAX_GPA	0x0000000FFFFFBFFFULL
#define HL_MAX_GVA	HL_SCRATCH_TOP_GVA

/* Host calls are stores to this page: one 8-byte slot per port. */
#define HL_IO_PAGE_GVA	0x0000FFFFFFFFE000ULL
#endif

#if !__ASSEMBLY__
#include <uk/arch/types.h>

/*
 * Scratch region base addresses (GPA and GVA).  Set by
 * hyperlight_cow_init() at boot; used by the paging arch layer
 * to translate between physical and virtual addresses.
 *
 * For GPAs in the scratch region:
 *   GVA = hl_scratch_base_gva + (GPA - hl_scratch_base_gpa)
 * For GPAs in the identity-mapped low region:
 *   GVA = GPA
 */
extern __u64 hl_scratch_base_gpa;
extern __u64 hl_scratch_base_gva;

/*
 * Bump allocator: allocate n contiguous pages from scratch memory.
 * Returns the GPA of the first page.  Thread-safe (uses atomic xadd).
 * Defined in cow.c.
 */
__u64 hl_scratch_alloc_pages(__u64 n);

/*
 * Give the copy-on-write page at @va a fresh scratch page, without
 * copying its contents.  Defined in cow.c.
 */
void hyperlight_cow_fresh(__u64 va);

/*
 * Boot-time CoW page fault handler (entry64.S).
 *
 * Self-contained assembly that resolves CoW faults by walking page
 * tables in scratch, bump-allocating a fresh page, copying content,
 * and remapping as writable.  Uses IST=0 (current stack) and has
 * no BSS dependencies — safe to use while BSS pages are still CoW.
 */
void _cow_asm_pf_handler(void);

/*
 * Re-initialise the paging frame allocator after snapshot restore.
 *
 * Called from hyperlight_dispatch_function's snapshot fixup path.
 * On non-restore dispatches (FA still valid) this returns immediately.
 * Defined in memory.c (CONFIG_LIBUKPAGING); no-op stub otherwise.
 */
void hyperlight_paging_reinit(void);
#endif /* !__ASSEMBLY__ */

#endif /* __HYPERLIGHT_MEM_H__ */
