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

#include <hyperlight/scratch_top.h>

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
/* u64: a guest log level the host asks for, bit 63 set while pending;
 * the guest clears it once taken
 */
#define HL_SCRATCH_GUEST_LOG_LEVEL_OFF	0x30

/* Virtqueue transport geometry (u64 each), published by the host */
/* HL_SCRATCH_ARENA_GPA_OFF (GPA of the transport arena): scratch_top.h */
#define HL_SCRATCH_G2H_QUEUE_SIZE_OFF	0x40 /* G2H descriptors */
#define HL_SCRATCH_G2H_POOL_PAGES_OFF	0x48 /* G2H pool pages */
#define HL_SCRATCH_H2G_QUEUE_SIZE_OFF	0x58 /* H2G descriptors */
#define HL_SCRATCH_H2G_POOL_PAGES_OFF	0x60 /* H2G pool pages */
#define HL_SCRATCH_H2G_BUFFER_SIZE_OFF	0x68 /* H2G receive buffer size */

/* Computed addresses */
#define HL_SCRATCH_SIZE_GVA	(HL_SCRATCH_TOP_GVA + 1 - HL_SCRATCH_SIZE_OFF)
#define HL_SCRATCH_ALLOC_GVA	(HL_SCRATCH_TOP_GVA + 1 - HL_SCRATCH_ALLOC_OFF)
#define HL_SCRATCH_META_GVA(off)	(HL_SCRATCH_TOP_GVA + 1 - (off))

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
 * Scratch sits at the top of both guest-physical and guest-virtual
 * memory, so GVA - GPA is the same for every scratch size, and the base
 * follows from the size the host publishes.  The translation caches
 * nothing, so it stays right when a restore changes the scratch size (the
 * frame allocator and page-table root are rebuilt by
 * hyperlight_paging_reinit).
 *
 * For GPAs in the scratch region:
 *   GVA = GPA + HL_SCRATCH_GVA_MINUS_GPA
 * For GPAs in the identity-mapped low region:
 *   GVA = GPA
 */
#define HL_SCRATCH_GVA_MINUS_GPA	((__u64)HL_MAX_GVA - (__u64)HL_MAX_GPA)

/* Hyperlight's page, which scratch is handed out in */
#define HL_PAGE_SIZE	4096ULL

/* Start of the reserved pages at the top of scratch. */
#define HL_SCRATCH_RESERVED_TOP_GPA \
	(HL_MAX_GPA + 1 - HL_SCRATCH_RESERVED_PAGES * HL_PAGE_SIZE)

/* End of the transport arena: the free pages below the reserved ones. */
#define HL_SCRATCH_TRANSPORT_END_GPA \
	(HL_SCRATCH_RESERVED_TOP_GPA - HL_SCRATCH_TRANSPORT_GAP_PAGES * HL_PAGE_SIZE)

static inline __u64 hl_scratch_base_gpa(void)
{
	return HL_MAX_GPA - *(volatile __u64 *)HL_SCRATCH_SIZE_GVA + 1;
}

/*
 * Where the transport arena may end, given where the host put it: an arena
 * at the scratch base (Hyperlight 0.18.0) may reach the reserved top pages;
 * one above it sits at a fixed address that ends
 * HL_SCRATCH_TRANSPORT_GAP_PAGES below them, so scratch can change size
 * across a snapshot.
 */
static inline __u64 hl_scratch_transport_end_for(__u64 arena)
{
	return arena == hl_scratch_base_gpa() ? HL_SCRATCH_RESERVED_TOP_GPA
					      : HL_SCRATCH_TRANSPORT_END_GPA;
}

/*
 * Bump allocator: allocate n contiguous pages from scratch memory.
 * Returns the GPA of the first page.  Thread-safe (uses atomic xadd).
 * Defined in cow.c.
 */
__u64 hl_scratch_alloc_pages(__u64 n);
/* Exclusive GPA limit of the bump allocator: the transport arena, or the
 * reserved top pages when the host keeps the arena at the scratch base
 * (Hyperlight 0.18.0).  Crashes if the host published any other arena.
 * Defined in cow.c.
 */
__u64 hl_scratch_alloc_limit_gpa(void);

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
 * Called from hyperlight_dispatch_function's snapshot fixup path, so on
 * the first dispatch after each restore.  Crashes the guest if the
 * restored scratch leaves too little room for the frame allocator (a
 * snapshot resized too small).  Defined in memory.c (CONFIG_LIBUKPAGING);
 * no-op stub otherwise.
 */
void hyperlight_paging_reinit(void);
#endif /* !__ASSEMBLY__ */

#endif /* __HYPERLIGHT_MEM_H__ */
