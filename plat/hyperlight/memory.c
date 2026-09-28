/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/*
 * Hyperlight memory initialisation.
 *
 * _ukplat_mem_mappings_init() is a no-op: the host sets up all
 * page-table mappings before guest entry.
 *
 * When CONFIG_LIBUKPAGING is enabled, ukplat_mem_init() allocates a
 * block of scratch memory for the frame allocator and adopts the
 * host's existing page tables.  Scratch memory is EPT-writable, so
 * page table frames allocated from it can be modified by both the CPU
 * page-table walker and guest code — unlike the PEB heap which sits
 * in the read-only snapshot EPT slot.
 */

#include <uk/config.h>

/*
 * Called by plat/common/memory.c during ukplat_memallocator_set().
 * Hyperlight sets up page tables on the host side before guest entry,
 * so there are no guest-side mappings to initialise.
 */
int _ukplat_mem_mappings_init(void)
{
	return 0;
}

/*
 * Overrides the weak ukplat_mem_init() from plat/common/memory.c.
 *
 * When paging is disabled, this is a simple no-op — the host already
 * set up all mappings, and the boot code will use the identity-mapped
 * FREE regions directly for the buddy allocator.
 *
 * When paging IS enabled, the version below allocates scratch memory
 * for the frame allocator and adopts the host's existing page tables.
 */
#if !CONFIG_LIBUKPAGING

int ukplat_mem_init(void)
{
	return 0;
}

void hyperlight_paging_reinit(void)
{
	/* No frame allocator to reinit without paging. */
}

#else /* CONFIG_LIBUKPAGING */

#include <uk/arch/limits.h>
#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/fallocbuddy.h>
#include <uk/paging.h>
#include <uk/paging/arch.h>
#include <uk/plat/memory.h>
#include <uk/plat/common/bootinfo.h>
#include <uk/print.h>

#include <hyperlight/hcall.h>
#include <hyperlight/mem.h>

static struct uk_pagetable hyperlight_pt;

static inline __paddr_t read_cr3(void)
{
	__paddr_t val;

	__asm__ volatile("mov %%cr3, %0" : "=r"(val));
	return val & ~0xFFFULL;
}

/*
 * Determine how much scratch memory to give the paging frame allocator.
 *
 * 1. Ask the host via GetPagingBudget (explicit control).
 * 2. If the host doesn't register it, compute dynamically:
 *    use 75% of remaining scratch, leaving 25% for CoW faults.
 */
static __sz hl_paging_fa_size(void)
{
	__u64 budget;
	__u64 bump_pos, max_avail, available;

	/* Try host function first */
	budget = hl_call_get_paging_budget();
	if (budget > 0)
		return (__sz)(budget & ~((__u64)__PAGE_SIZE - 1));

	/* Dynamic fallback: read bump pointer, compute remaining */
	bump_pos = *(volatile __u64 *)HL_SCRATCH_ALLOC_GVA;
	max_avail = HL_MAX_GPA + 1 - 2 * __PAGE_SIZE; /* 2 reserved pages */
	if (bump_pos >= max_avail)
		return 0;

	available = max_avail - bump_pos;
	return (__sz)((available * 3 / 4) & ~((__u64)__PAGE_SIZE - 1));
}

/*
 * Overrides the weak ukplat_mem_init() from plat/common/memory.c.
 *
 * Hyperlight's memory has two EPT-backed regions:
 *  - Snapshot (low GPAs): code + PEB heap — EPT read-only
 *  - Scratch  (high GPAs): I/O + CoW pages — EPT writable
 *
 * Standard Unikraft feeds the PEB heap (FREE regions) to the frame
 * allocator, but those GPAs are EPT read-only.  Writing page table
 * entries backed by heap GPAs crashes with MmioWriteUnmapped.
 *
 * Instead, we allocate a block of scratch memory via the bump
 * allocator (hl_scratch_alloc_pages) and feed THAT to the frame
 * allocator.  Since scratch is EPT-writable, page table frames and
 * mapped-page frames are all writable — no CoW aliasing issues.
 *
 * The FREE heap regions are NOT consumed here.  With paging enabled,
 * boot.c uses HEAP_BASE to map the heap via uk_paging_page_map(),
 * backed entirely by scratch-allocated frames.
 */
int ukplat_mem_init(void)
{
	struct ukplat_memregion_desc *mrd;
	__paddr_t scratch_block;
	__paddr_t cr3;
	__sz fa_size;
	__u64 fa_pages;
	int rc;

	/*
	 * Determine how much scratch to give the frame allocator.
	 * The host can control this via GetPagingBudget; otherwise
	 * we use 75% of remaining scratch.
	 */
	fa_size = hl_paging_fa_size();
	if (unlikely(fa_size < 16 * __PAGE_SIZE)) {
		uk_pr_err("Not enough scratch for paging (%lu bytes)\n",
			  (unsigned long)fa_size);
		return -ENOMEM;
	}

	fa_pages = fa_size / __PAGE_SIZE;
	scratch_block = hl_scratch_alloc_pages(fa_pages);

	uk_pr_info("Paging frame allocator: %lu pages (%lu KiB) from scratch GPA %lx\n",
		   (unsigned long)fa_pages,
		   (unsigned long)(fa_size >> 10),
		   (unsigned long)scratch_block);

	/*
	 * Initialise the frame allocator with the scratch block.
	 * uk_paging_pt_init() also does one-time arch init and allocates
	 * a dummy PML4 (which we discard below in favour of the host's).
	 */
	rc = uk_paging_pt_init(&hyperlight_pt, scratch_block, fa_size);
	if (unlikely(rc)) {
		uk_pr_err("uk_paging_pt_init failed: %d\n", rc);
		return rc;
	}

	/*
	 * Mark the PEB heap FREE regions as consumed so boot code doesn't
	 * try to use them for the buddy allocator — with HEAP_BASE
	 * enabled, the heap is mapped via paging, not identity-mapped.
	 */
	ukplat_memregion_foreach(&mrd, UKPLAT_MEMRT_FREE, 0, 0) {
		mrd->flags &= ~UKPLAT_MEMRF_PERMS;
	}

	/*
	 * Adopt the host's existing page tables.  The PML4 is in the
	 * scratch region; pgarch_directmap_paddr_to_vaddr converts
	 * the scratch GPA from CR3 to its GVA.
	 */
	cr3 = read_cr3();
	hyperlight_pt.pt_pbase = cr3;
	hyperlight_pt.pt_vbase = pgarch_directmap_paddr_to_vaddr(cr3);

	uk_pr_info("Adopting host page tables: CR3=%lx vbase=%lx\n",
		   (unsigned long)cr3,
		   (unsigned long)hyperlight_pt.pt_vbase);

	/*
	 * Set as active.  Writing the same CR3 just flushes the TLB —
	 * no page table switch.
	 */
	rc = uk_paging_pt_set_active(&hyperlight_pt);
	if (unlikely(rc))
		return rc;

	/*
	 * If the host mapped an initrd via map_file_cow, create
	 * first-stage page table entries for it.  The EPT already
	 * covers these GPAs (the host set that up), but the guest's
	 * CR3 page tables don't — they only cover the snapshot and
	 * scratch regions.  We identity-map the initrd so VA = GPA.
	 *
	 * With 4 KiB pages only: a snapshot takes the pages the guest maps,
	 * and Hyperlight's page table walk does not handle large pages (it
	 * reads a 2 MiB page as a page table).  Files extracted from the
	 * initrd reference it (CONFIG_LIBVFSCORE_AUTOMOUNT_EXTRACT_BORROW),
	 * so it has to survive a snapshot whole.
	 */
	ukplat_memregion_foreach(&mrd, UKPLAT_MEMRT_INITRD, 0, 0) {
		unsigned long pages = mrd->pg_count;

		rc = uk_paging_page_map(&hyperlight_pt,
					mrd->vbase, mrd->pbase, pages,
					UK_PAGING_PAGE_ATTR_PROT_READ,
					UK_PAGING_PAGE_FLAG_FORCE_SIZE |
					UK_PAGING_PAGE_FLAG_SIZE(UK_PAGING_PAGE_LEVEL));
		if (unlikely(rc)) {
			uk_pr_err("Failed to map initrd at %lx: %d\n",
				  (unsigned long)mrd->vbase, rc);
			return rc;
		}

		uk_pr_info("Mapped initrd: %lu pages at VA %lx → GPA %lx\n",
			   pages, (unsigned long)mrd->vbase,
			   (unsigned long)mrd->pbase);
	}

	return 0;
}

/*
 * After a restore the frame allocator gets its memory a chunk at a time.
 *
 * Its metadata (a bitmap per zone and level, a header in each free block)
 * lives in the scratch it manages, which the restore released: every page
 * of it written costs the host a fresh page.  Writing it for the whole
 * budget took about a millisecond per restore, mostly for memory the next
 * request never maps.  So the allocator takes its memory from the scratch
 * bump allocator HL_FA_CHUNK at a time, up to the same budget as before:
 * when an allocation finds it full, the wrappers below add a chunk and
 * retry.  Taking the chunks as needed, rather than reserving the budget
 * up front, keeps them next to the copy-on-write pages, so a restore
 * touches as few of the host's (huge) pages as it can.  The wrappers run
 * in the page fault handler, hence ukfallocbuddy's addmem has to be
 * ISR-safe.
 */
#define HL_FA_CHUNK (512UL << 10)

/* Pieces smaller than this are not worth a zone of their own. */
#define HL_FA_MIN_ZONE (64UL << 10)

/* What the allocator may still take from scratch. */
static __sz hl_fa_budget;

static int (*hl_fa_falloc)(struct uk_falloc *fa, __paddr_t *paddr,
			   unsigned long frames, unsigned long flags);
static int (*hl_fa_falloc_from_range)(struct uk_falloc *fa, __paddr_t *paddr,
				      unsigned long frames, unsigned long flags,
				      __paddr_t min, __paddr_t max);

/*
 * Add [start, start + len) to the frame allocator as zones that are each
 * a naturally aligned power of two.  The buddy allocator merges a freed
 * block with its buddy when the buddy starts inside the zone, without
 * checking that it ends there: in a zone whose end is not aligned to the
 * block, the merged block would reach past it.  In an aligned power-of-two
 * zone a buddy that starts inside also ends inside.
 */
static int hl_fa_add_range(__paddr_t start, __sz len)
{
	int rc;

	while (len >= HL_FA_MIN_ZONE) {
		/* The largest power of two @start is aligned to and @len holds. */
		__sz size = start ? (__sz)1 << __builtin_ctzl(start) : len;

		while (size > len)
			size >>= 1;
		if (size >= HL_FA_MIN_ZONE) {
			rc = uk_paging_pt_add_mem(&hyperlight_pt, start, size);
			if (unlikely(rc))
				return rc;
		}
		start += size;
		len -= size;
	}
	return 0;
}

/*
 * Give the allocator a chunk of scratch large enough for an aligned block
 * of @frames: a power of two, at least twice that, on a multiple of its
 * size.  add_mem puts a zone's metadata at its start, so the upper half of
 * such a chunk is an aligned free block.  The scratch skipped to reach that
 * boundary is added too, as smaller zones.
 */
static int hl_fa_grow(unsigned long frames)
{
	struct uk_falloc *fa = hyperlight_pt.fa;
	__sz size = HL_FA_CHUNK, len;
	__paddr_t start;
	__u64 bump;

	while (size < (__sz)frames * __PAGE_SIZE * 2)
		size <<= 1;
	bump = *(volatile __u64 *)HL_SCRATCH_ALLOC_GVA;
	len = ALIGN_UP(bump, size) - bump + size;
	if (len > hl_fa_budget)
		return -ENOMEM;
	start = hl_scratch_alloc_pages(len / __PAGE_SIZE);
	hl_fa_budget -= len;

	/* The budget already counts in the totals (see
	 * hyperlight_paging_reinit()); add_mem counts it again.
	 */
	fa->total_memory -= len;
	fa->free_memory -= len;
	return hl_fa_add_range(start, len);
}

static int hl_falloc_grow(struct uk_falloc *fa, __paddr_t *paddr,
			  unsigned long frames, unsigned long flags)
{
	__paddr_t want = *paddr;
	int rc = hl_fa_falloc(fa, paddr, frames, flags);

	if (rc == -ENOMEM && hl_fa_grow(frames) == 0) {
		*paddr = want;
		rc = hl_fa_falloc(fa, paddr, frames, flags);
	}
	return rc;
}

static int hl_falloc_from_range_grow(struct uk_falloc *fa, __paddr_t *paddr,
				     unsigned long frames, unsigned long flags,
				     __paddr_t min, __paddr_t max)
{
	int rc = hl_fa_falloc_from_range(fa, paddr, frames, flags, min, max);

	if (rc == -ENOMEM && hl_fa_grow(frames) == 0)
		rc = hl_fa_falloc_from_range(fa, paddr, frames, flags, min, max);
	return rc;
}

/*
 * Re-initialise the paging frame allocator after snapshot restore.
 *
 * The frame allocator (FA) struct, zone metadata, bitmaps, and free-list
 * entries all live in scratch memory — allocated there by ukplat_mem_init
 * during the initial boot (evolve).  Snapshot/restore zeroes scratch
 * (only the rebuilt page tables are copied back), so every byte of FA
 * state is lost.
 *
 * Without re-initialisation, the first demand fault after restore
 * (e.g. a new mmap page, stack growth) calls through the FA's NULL
 * function pointers and crashes.
 *
 * This function also fixes hyperlight_pt.pt_pbase / pt_vbase: after
 * restore the host sets CR3 to the relocated page tables in scratch,
 * but the snapshot copy of hyperlight_pt still holds the evolve-time
 * values.
 *
 * Called ONLY from hyperlight_dispatch_function's snapshot fixup path
 * (ZF=1 dispatch), after pre-faulting and restoring the kernel IDT.
 * This means every call follows a snapshot restore, so we always
 * perform the full reinit unconditionally.
 */
void hyperlight_paging_reinit(void)
{
	struct uk_falloc *fa;
	__paddr_t cr3;
	__sz fa_size, fa_struct_size;
	__u64 bump_pos, max_avail, available;
	__paddr_t scratch_block;
	int rc;

	/* Fix pt_pbase / pt_vbase to match the current CR3. */
	cr3 = read_cr3();
	hyperlight_pt.pt_pbase = cr3;
	hyperlight_pt.pt_vbase = pgarch_directmap_paddr_to_vaddr(cr3);

	/* Compute remaining scratch dynamically.  The host-provided
	 * GetPagingBudget was set at evolve time and does not account
	 * for scratch consumed by the PT copy and CoW pre-fault, so
	 * we always use the bump-pointer calculation here.
	 */
	bump_pos = *(volatile __u64 *)HL_SCRATCH_ALLOC_GVA;
	max_avail = HL_MAX_GPA + 1 - 2 * __PAGE_SIZE;
	if (bump_pos >= max_avail)
		return;

	available = max_avail - bump_pos;
	fa_size = (__sz)((available * 3 / 4) & ~((__u64)__PAGE_SIZE - 1));
	fa_struct_size = ALIGN_UP(uk_fallocbuddy_size(), __PAGE_SIZE);

	if (fa_size < fa_struct_size + HL_FA_CHUNK * 2) {
		uk_pr_warn("paging reinit: not enough scratch (%lu bytes)\n",
			   (unsigned long)fa_size);
		return;
	}

	/* The FA struct gets page(s) of its own; hl_fa_grow() takes the rest
	 * of the budget as it is needed.
	 */
	scratch_block = hl_scratch_alloc_pages(fa_struct_size / __PAGE_SIZE);
	fa = (struct uk_falloc *)pgarch_directmap_paddr_to_vaddr(scratch_block);
	rc = uk_fallocbuddy_init(fa);
	if (unlikely(rc)) {
		uk_pr_warn("paging reinit: fallocbuddy_init failed: %d\n", rc);
		return;
	}
	hyperlight_pt.fa = fa;
	hl_fa_budget = fa_size - fa_struct_size;

	/* sysinfo() and sysconf() report the totals: count the budget from
	 * the start, so they show the memory the allocator can reach.
	 */
	fa->total_memory = hl_fa_budget;
	fa->free_memory = hl_fa_budget;

	hl_fa_falloc = fa->falloc;
	hl_fa_falloc_from_range = fa->falloc_from_range;
	fa->falloc = hl_falloc_grow;
	fa->falloc_from_range = hl_falloc_from_range_grow;

	rc = hl_fa_grow(1);
	if (unlikely(rc)) {
		uk_pr_warn("paging reinit: add_mem failed: %d\n", rc);
		return;
	}

	uk_pr_info("Paging reinit: %lu KiB from scratch GPA %lx\n",
		   (unsigned long)(fa_size >> 10),
		   (unsigned long)scratch_block);
}

#endif /* CONFIG_LIBUKPAGING */
