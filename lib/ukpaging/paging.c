/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, Karlsruhe Institute of Technology (KIT).
 *                     All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* API based on prototype for virtual memory by:
 * Stefan Teodorescu <stefanl.teodorescu@gmail.com>
 * See https://github.com/unikraft/unikraft/pull/247
 */

#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include <uk/config.h>
#include <uk/arch.h>
#include <uk/arch/limits.h>
#include <uk/arch/spinlock.h>
#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/falloc.h>
#include <uk/lcpu.h>
#include <uk/paging/arch.h>
#include <uk/paging.h>
#include <uk/pcpuvar.h>
#include <uk/pt.h>
#include <uk/plat/memory.h>
#include <uk/plat/spinlock.h>
#include <uk/print.h>

/* Forward declarations */
static inline int pg_pt_alloc(struct uk_pagetable *pt, __vaddr_t *pt_vaddr,
			      __paddr_t *pt_paddr, unsigned int level);

static int pg_batch_reserve_frame(struct uk_pagetable *pt);
static inline void pg_pt_free(struct uk_pagetable *pt, __vaddr_t pt_vaddr,
			      unsigned int level, bool defer);

static int pg_page_split(struct uk_pagetable *pt, __vaddr_t pt_vaddr,
			 __vaddr_t vaddr, unsigned int level);

static int pg_page_mapx(struct uk_pagetable *pt, __vaddr_t pt_vaddr,
			unsigned int level, __vaddr_t vaddr, __paddr_t paddr,
			__sz len, unsigned long attr, unsigned long flags,
			__pte_t tmpl, unsigned int tmpl_level,
			struct uk_paging_page_mapx *mapx);

static int pg_page_unmap(struct uk_pagetable *pt, __vaddr_t pt_vaddr,
			 unsigned int level, __vaddr_t vaddr, __sz len,
			 unsigned long flags);

/* Returns the virtual address of the page table that a PTE links to */
static inline __vaddr_t pg_pte_to_vaddr(__pte_t pte,
					unsigned int level __maybe_unused)
{
	return uk_pt_frame_vaddr(UK_PAL_PT_Lx_PTE_PADDR(pte, level));
}

#ifdef CONFIG_LIBUKPAGING_STATS
/* Don't update stats */
#define UK_PAGING_PAGE_FLAG_INTERN_STATS_KEEP	0x80000000
#endif /* CONFIG_LIBUKPAGING_STATS */

/* The largest level for which UK_PAL_PAGE_Lx_HAS() returns a non-zero value,
 * that is the largest level which can map a page.
 */
static unsigned int pg_page_largest_level = UK_PAL_PAGE_LEVEL;

static struct uk_pagetable kernel_pt;
static __uk_pcpuvar struct uk_pagetable *pg_active_pt;
static __uk_pcpuvar struct uk_paging_free_batch *pg_cpu_defer_batch;

struct uk_pagetable *uk_paging_pt_get_active(void)
{
	return uk_pcpuvar_current_get(pg_active_pt);
}

int uk_paging_pt_set_active(struct uk_pagetable *pt)
{
	int rc;

	rc = uk_pt_activate(&pt->pt);
	if (unlikely(rc))
		return rc;

	uk_pcpuvar_current_set(pg_active_pt, pt);
	return 0;
}

#if CONFIG_LIBUKPAGING_SHARED
static bool pg_index_shared(struct uk_pagetable *pt, unsigned int index)
{
	return pt->shared_parent && index >= pt->shared_first &&
	       index - pt->shared_first < pt->shared_count;
}

struct uk_pagetable *uk_paging_pt_get_owner(struct uk_pagetable *pt,
					    __vaddr_t vaddr)
{
	unsigned int index = UK_PAL_PT_Lx_IDX(vaddr, UK_PAL_PT_LEVELS - 1);

	while (pg_index_shared(pt, index))
		pt = pt->shared_parent;
	return pt;
}

static bool pg_range_shared(struct uk_pagetable *pt, __vaddr_t start, __sz len)
{
	unsigned int first, last;

	if (!pt->shared_parent)
		return false;
	if (start == UK_PAL_VADDR_INV || len == __SZ_MAX)
		return true;
	first = UK_PAL_PT_Lx_IDX(start, UK_PAL_PT_LEVELS - 1);
	last = UK_PAL_PT_Lx_IDX(start + len - 1, UK_PAL_PT_LEVELS - 1);
	return first < pt->shared_first + pt->shared_count &&
	       last >= pt->shared_first;
}

static bool pg_uses_shared(struct uk_pagetable *pt, struct uk_pagetable *owner)
{
	while (pt) {
		if (pt == owner)
			return true;
		pt = pt->shared_parent;
	}
	return false;
}

#endif
static int pg_pt_clone(struct uk_pagetable *pt_dst, struct uk_pagetable *pt_src,
#if CONFIG_LIBUKPAGING_SHARED
		       unsigned long flags, unsigned int first,
		       unsigned int count)
#else
		       unsigned long flags)
#endif
{
	unsigned int lvl = UK_PAL_PT_LEVELS - 1;
	__vaddr_t pt_vaddr_scache[UK_PAL_PT_LEVELS];
	__vaddr_t pt_vaddr_dcache[UK_PAL_PT_LEVELS];
	__vaddr_t pt_svaddr, pt_dvaddr;
	__paddr_t pt_dpaddr, pt_dpaddr_root;
	__pte_t pte;
	unsigned int pte_idx_cache[UK_PAL_PT_LEVELS];
	unsigned int pte_idx = 0;
	int rc;

	UK_ASSERT(uk_pt_vbase(&pt_src->pt) != UK_PAL_VADDR_INV);
	UK_ASSERT(uk_pt_pbase(&pt_src->pt) != UK_PAL_PADDR_INV);

	if (pt_dst != pt_src) {
		memset(pt_dst, 0, sizeof(struct uk_pagetable));
		ukarch_spin_init(&pt_dst->lock);
	}

	/* Use the same frame allocator for the new page table */
	pt_dst->fa = pt_src->fa;
#if CONFIG_LIBUKPAGING_SHARED
	if (count) {
		pt_dst->shared_parent = pt_src;
		pt_dst->shared_first = first;
		pt_dst->shared_count = count;
	}
#endif

	pt_svaddr = uk_pt_vbase(&pt_src->pt);
	pt_vaddr_scache[lvl] = pt_svaddr;

	/* Allocate a new top-level page table */
	rc = pg_pt_alloc(pt_dst, &pt_dvaddr, &pt_dpaddr_root, lvl);
	if (unlikely(rc))
		return rc;

	pt_vaddr_dcache[lvl] = pt_dvaddr;

	/* We should create a new page table hierarchy instead of doing a deep
	 * copy of the entire source page table. Cancel the copy. Note that
	 * this page table will be completely empty and thus do not map any
	 * code or data segments of the kernel.
	 */
	if (flags & UK_PAGING_PAGE_FLAG_CLONE_NEW)
		goto EXIT;

	do {
		rc = uk_pt_pte_read_at(&pt_src->pt, pt_svaddr, lvl, pte_idx,
				       &pte);
		if (unlikely(rc))
			goto EXIT_FREE;

		/* This is a page table. Copy it and descent. */
#if CONFIG_LIBUKPAGING_SHARED
		if (UK_PAL_PT_Lx_PTE_PRESENT(pte, lvl) &&
		    !UK_PAL_PAGE_Lx_IS(pte, lvl) &&
		    !(lvl == UK_PAL_PT_LEVELS - 1 &&
		      pg_index_shared(pt_dst, pte_idx))) {
#else
		if (UK_PAL_PT_Lx_PTE_PRESENT(pte, lvl) &&
		    !UK_PAL_PAGE_Lx_IS(pte, lvl)) {
#endif
			rc = pg_pt_alloc(pt_dst, &pt_dvaddr, &pt_dpaddr,
					 lvl - 1);
			if (unlikely(rc))
				goto EXIT_FREE;

			pt_svaddr = pg_pte_to_vaddr(pte, lvl);

			/* Set the PTE in the destination page table to point
			 * to the copy of the lower-level page table
			 */
			pte = uk_pal_pt_pte_create(pt_dpaddr, lvl, pte, lvl);

			rc = uk_pt_pte_write_at(&pt_dst->pt,
						pt_vaddr_dcache[lvl], lvl,
						pte_idx, UK_PAL_VADDR_INV,
						pte);
			if (unlikely(rc)) {
				pg_pt_free(pt_dst, pt_dvaddr, lvl - 1, false);
				goto EXIT_FREE;
			}

			pte_idx_cache[lvl] = pte_idx;

			UK_ASSERT(lvl > UK_PAL_PAGE_LEVEL);
			lvl--;

			pt_vaddr_scache[lvl] = pt_svaddr;
			pt_vaddr_dcache[lvl] = pt_dvaddr;

			pte_idx = 0;
			continue;
		}

#ifdef CONFIG_LIBUKPAGING_STATS
		if (UK_PAL_PT_Lx_PTE_PRESENT(pte, lvl) &&
		    UK_PAL_PAGE_Lx_IS(pte, lvl)) {
			pt_dst->nr_lx_pages[lvl]++;
		}
#endif /* CONFIG_LIBUKPAGING_STATS */

		/* Copy whatever PTE we have here */
		rc = uk_pt_pte_write_at(&pt_dst->pt, pt_dvaddr, lvl, pte_idx,
					UK_PAL_VADDR_INV, pte);
		if (unlikely(rc))
			goto EXIT_FREE;

		/* At this point we reached the last PTE in this page table and
		 * we have to walk up one level again.
		 */
		if (pte_idx == UK_PAL_PT_Lx_PTES(lvl) - 1) {
			do {
				/* If we reached the top level, stop. */
				if (lvl == UK_PAL_PT_LEVELS - 1)
					break;

				/* Go up one level */
				pte_idx = pte_idx_cache[++lvl];
				UK_ASSERT(pte_idx < UK_PAL_PT_Lx_PTES(lvl));
			} while (pte_idx == UK_PAL_PT_Lx_PTES(lvl) - 1);

			pt_svaddr = pt_vaddr_scache[lvl];
			pt_dvaddr = pt_vaddr_dcache[lvl];
		}

		pte_idx++;
	} while (pte_idx < UK_PAL_PT_Lx_PTES(lvl));

EXIT:
	UK_ASSERT(lvl == UK_PAL_PT_LEVELS - 1);

	/* We successfully created a new page table hierarchy. Now assign it to
	 * the destination page table. We do not free any existing page tables,
	 * but assume that the caller provided us an uninitialized page table or
	 * dst and src are the same.
	 */
	rc = uk_pt_init(&pt_dst->pt, pt_dpaddr_root);
	if (unlikely(rc))
		goto EXIT_FREE;

	return 0;

EXIT_FREE:
	pg_page_unmap(pt_dst, pt_vaddr_dcache[UK_PAL_PT_LEVELS - 1],
		      UK_PAL_PT_LEVELS - 1, UK_PAL_VADDR_INV, __SZ_MAX,
		      UK_PAGING_PAGE_FLAG_KEEP_FRAMES);

	pg_pt_free(pt_dst, pt_vaddr_dcache[UK_PAL_PT_LEVELS - 1],
		   UK_PAL_PT_LEVELS - 1, false);

	return rc;
}

int uk_paging_pt_init(struct uk_pagetable *pt, __paddr_t start, __sz len)
{
	static __u8 initialized; /* = 0 on startup */
	unsigned int lvl;
	__vaddr_t pt_vaddr;
	__paddr_t pt_paddr;
	int rc;

	/* If this is the first time we setup a new page table, perform
	 * architecture-dependent initialization of the paging API
	 */
	if (!initialized) {
		rc = uk_pal_paging_init();
		if (unlikely(rc))
			return rc;

		/* Find out largest supported level that can map a page */
		for (lvl = UK_PAL_PT_LEVELS - 1; lvl > UK_PAL_PAGE_LEVEL; lvl--)
			if (UK_PAL_PAGE_Lx_HAS(lvl)) {
				pg_page_largest_level = lvl;
				break;
			}

		initialized = 1;
	}

	UK_ASSERT(start <= __PADDR_MAX - len);
	UK_ASSERT(uk_pal_paddr_range_isvalid(start, len));

	/* Initialize the frame allocator and any architecture-dependent parts
	 * of the new page table
	 */
	memset(pt, 0, sizeof(struct uk_pagetable));
	ukarch_spin_init(&pt->lock);

	rc = pgarch_pt_init(pt, start, len);
	if (unlikely(rc))
		return rc;

	/* Allocate a new top-level page table */
	rc = pg_pt_alloc(pt, &pt_vaddr, &pt_paddr, UK_PAL_PT_LEVELS - 1);
	if (unlikely(rc))
		return rc;

	rc = uk_pt_init(&pt->pt, pt_paddr);
	if (unlikely(rc)) {
		pg_pt_free(pt, pt_vaddr, UK_PAL_PT_LEVELS - 1, false);
		return rc;
	}

#if CONFIG_LIBUKPAGING_DIRECTMAP
	/* FIXME: The direct-mapped page table is an architecture-specific
	 * abstraction, so it should be mapped by the pgarch_ API. What
	 * currently prevents us from doing so is that upon pgarch_pt_init()
	 * we haven't yet allocated a top-level pagetable.
	 *
	 * Options:
	 * 1. Move both the allocation of the top-level page table to
	 *    pgarch_pt_init() and make it a requriement that it is
	 *    initialized by that function. Move mapping of the direct
	 *    mapped region into pgarch_pt_init(). Requires that the
	 *    pgarch layer has access to pg_pt_alloc() and pg_pt_map().
	 *
	 * 2. Create a pgarch_pt_post_init() and move mapping of the
	 *    direct-mapped region there. It also requires mapping
	 *    capabilities from pgarch.
	 */
	rc = pg_page_mapx(pt, uk_pt_vbase(&pt->pt), UK_PAL_PT_LEVELS - 1,
			  PGARCH_DIRECTMAP_AREA_START, /* vaddr */
			  0x00000000, /* paddr */
			  PGARCH_DIRECTMAP_AREA_SIZE, /* len */
			  UK_PAL_PAGE_ATTR_PROT_READ | UK_PAL_PAGE_ATTR_PROT_WRITE, /* attr */
			  0, /* flags */
			  UK_PAL_PT_Lx_PTE_INVALID(UK_PAL_PAGE_LEVEL),
			  UK_PAL_PAGE_LEVEL, NULL);
	if (unlikely(rc))
		return rc;
#endif /* CONFIG_LIBUKPAGING_DIRECTMAP */

#ifdef CONFIG_LIBUKPAGING_STATS
	/* If we have stats active, we need to discover all mappings etc. We
	 * simplify this by just cloning the page table hierarchy.
	 */
	rc = pg_pt_clone(pt, pt, 0, 0, 0);
	if (unlikely(rc))
		return rc;
#endif

	return 0;
}

int uk_paging_pt_add_mem(struct uk_pagetable *pt, __paddr_t start, __sz len)
{
	unsigned long irqf;
	int rc;

	if (len == 0)
		return 0;

	UK_ASSERT(start <= __PADDR_MAX - len);
	UK_ASSERT(uk_pal_paddr_range_isvalid(start, len));

	ukplat_spin_lock_irqsave(&pt->lock, irqf);
	rc = pgarch_pt_add_mem(pt, start, len);
	ukplat_spin_unlock_irqrestore(&pt->lock, irqf);
	return rc;
}

int uk_paging_pt_clone(struct uk_pagetable *pt, struct uk_pagetable *pt_src,
		       unsigned long flags)
{
	unsigned long irqf;
	int rc;

	UK_ASSERT(pt != pt_src);

	ukplat_spin_lock_irqsave(&pt_src->lock, irqf);
#if CONFIG_LIBUKPAGING_SHARED
	if (pt_src->shared_parent && !(flags & UK_PAGING_PAGE_FLAG_CLONE_NEW))
		rc = -ENOTSUP;
	else
		rc = pg_pt_clone(pt, pt_src, flags, 0, 0);
#else
	rc = pg_pt_clone(pt, pt_src, flags);
#endif
	ukplat_spin_unlock_irqrestore(&pt_src->lock, irqf);
	return rc;
}

#if CONFIG_LIBUKPAGING_SHARED
int uk_paging_pt_clone_shared(struct uk_pagetable *pt,
			      struct uk_pagetable *pt_src,
			      __vaddr_t start, __sz len)
{
	const unsigned int level = UK_PAL_PT_LEVELS - 1;
	const __sz span = UK_PAL_PAGE_Lx_SIZE(level);
	unsigned int first = UK_PAL_PT_Lx_IDX(start, level);
	unsigned int count, i;
	unsigned long irqf;
	__pte_t pte;
	int rc = -EINVAL;

	if (pt == pt_src || !len || start > __VADDR_MAX - len ||
	    !uk_pal_vaddr_range_isvalid(start, len) ||
	    !UK_PAL_PAGE_Lx_ALIGNED(start, level) || len % span ||
	    len / span > UK_PAL_PT_Lx_PTES(level) - first)
		return -EINVAL;
	count = len / span;

	ukplat_spin_lock_irqsave(&pt_src->lock, irqf);
	if (pt_src->shared_users == ~0U) {
		rc = -EOVERFLOW;
		goto out;
	}
	if (pt_src->shared_parent &&
	    (first != pt_src->shared_first || count != pt_src->shared_count))
		goto out;
	for (i = first; i < first + count; i++) {
		rc = uk_pt_pte_read_at(&pt_src->pt, uk_pt_vbase(&pt_src->pt),
				       level, i, &pte);
		if (rc)
			goto out;
		if (!UK_PAL_PT_Lx_PTE_PRESENT(pte, level) ||
		    UK_PAL_PAGE_Lx_IS(pte, level)) {
			rc = -ENOTSUP;
			goto out;
		}
	}
	rc = pg_pt_clone(pt, pt_src, 0, first, count);
	if (!rc)
		pt_src->shared_users++;
out:
	ukplat_spin_unlock_irqrestore(&pt_src->lock, irqf);
	return rc;
}

static void pg_shared_parent_release(void *arg)
{
	struct uk_pagetable *pt = arg;
	struct uk_pagetable *parent = pt->shared_parent;
	unsigned long irqf;

	ukplat_spin_lock_irqsave(&parent->lock, irqf);
	UK_ASSERT(parent->shared_users);
	parent->shared_users--;
	ukplat_spin_unlock_irqrestore(&parent->lock, irqf);
	pt->shared_parent = __NULL;
}

#endif
int uk_paging_pt_free(struct uk_pagetable *pt, unsigned long flags)
{
	struct uk_paging_free_batch local_batch;
	struct uk_paging_free_batch *batch;
	unsigned long irqf;
	int rc;

	UK_ASSERT(uk_pt_vbase(&pt->pt) != UK_PAL_VADDR_INV);
	UK_ASSERT(uk_pt_pbase(&pt->pt) != UK_PAL_PADDR_INV);

	uk_paging_free_batch_init(&local_batch, pt);
	ukplat_spin_lock_irqsave(&pt->lock, irqf);
#if CONFIG_LIBUKPAGING_SHARED
	if (pt->shared_users) {
		ukplat_spin_unlock_irqrestore(&pt->lock, irqf);
		return -EBUSY;
	}
#endif
	batch = uk_pcpuvar_current_get(pg_cpu_defer_batch);
	if (!batch || batch->pt != pt)
		batch = &local_batch;
	UK_ASSERT(pt->defer_batch == __NULL);
	pt->defer_batch = batch;

	rc = pg_page_unmap(pt, uk_pt_vbase(&pt->pt), UK_PAL_PT_LEVELS - 1,
			   UK_PAL_VADDR_INV, __SZ_MAX,
			   flags & UK_PAGING_PAGE_FLAG_KEEP_FRAMES);
	if (!rc)
		rc = pg_batch_reserve_frame(pt);
	if (unlikely(rc)) {
		pt->defer_batch = __NULL;
		ukplat_spin_unlock_irqrestore(&pt->lock, irqf);
		if (batch == &local_batch)
			uk_paging_free_batch_finish(&local_batch);
		return rc;
	}

	/* Also free the top-level page table */
	pg_pt_free(pt, uk_pt_vbase(&pt->pt), UK_PAL_PT_LEVELS - 1, true);

	uk_pt_term(&pt->pt);
#if CONFIG_LIBUKPAGING_SHARED
	if (pt->shared_parent)
		uk_paging_batch_defer(batch, &pt->shared_release,
				      pg_shared_parent_release, pt);
#endif
	pt->defer_batch = __NULL;
	ukplat_spin_unlock_irqrestore(&pt->lock, irqf);

	if (batch == &local_batch)
		uk_paging_free_batch_finish(&local_batch);

	return 0;
}

int uk_paging_pt_walk(struct uk_pagetable *pt, __vaddr_t vaddr,
		      unsigned int *level, __vaddr_t *pt_vaddr,
		      __pte_t *pte)
{
	unsigned long irqf;
	int rc;

#if CONFIG_LIBUKPAGING_SHARED
	pt = uk_paging_pt_get_owner(pt, vaddr);
#endif
	ukplat_spin_lock_irqsave(&pt->lock, irqf);
	rc = uk_pt_walk(&pt->pt, vaddr, level, pt_vaddr, pte);
	ukplat_spin_unlock_irqrestore(&pt->lock, irqf);
	return rc;
}

#define PG_Lx_L0_PAGES(lvl)					\
	(1UL << (UK_PAL_PAGE_Lx_SHIFT(lvl) - UK_PAL_PAGE_Lx_SHIFT(0)))

static inline int pg_falloc(struct uk_pagetable *pt, __paddr_t *paddr,
			    unsigned int level)
{
	unsigned long pages = PG_Lx_L0_PAGES(level);

	UK_ASSERT(level < UK_PAL_PT_LEVELS);
	UK_ASSERT(pt->fa->falloc);

	return pt->fa->falloc(pt->fa, paddr, pages, FALLOC_FLAG_ALIGNED);
}

static inline void pg_ffree(struct uk_pagetable *pt, __paddr_t paddr,
			    unsigned int level)
{
	unsigned long pages = PG_Lx_L0_PAGES(level);
	int rc __maybe_unused;

	UK_ASSERT(level < UK_PAL_PT_LEVELS);

	UK_ASSERT(pt->fa->ffree);
	rc = pt->fa->ffree(pt->fa, paddr, pages);

	/* We expect the free to succeed or to fail with -EFAULT if
	 * the address is not in the range managed by the allocator (e.g.,
	 * mapping of the kernel code segment), or -ENOMEM if the memory has
	 * not been previously allocated or already been freed (e.g., due to
	 * a stale mapping or multiple mappings pointing to the same frame
	 * during unmap). We silently ignore all of these as the frame
	 * allocator must be able to gracefully handle these scenarios. Just
	 * capture unexpected errors with this assert.
	 */
	UK_ASSERT(rc == 0 || rc == -EFAULT || rc == -ENOMEM);
}

#define PG_FREE_CHUNK_CAPACITY						\
	((UK_PAGING_PAGE_SIZE - sizeof(struct uk_paging_free_chunk *) -	\
	  sizeof(__paddr_t) - sizeof(unsigned int)) /			\
	 sizeof(struct uk_paging_free_entry))

struct uk_paging_free_chunk {
	struct uk_paging_free_chunk *next;
	__paddr_t chunk_paddr;
	unsigned int count;
	struct uk_paging_free_entry entries[PG_FREE_CHUNK_CAPACITY];
};

void uk_paging_free_batch_init(struct uk_paging_free_batch *batch,
			       struct uk_pagetable *pt)
{
	UK_ASSERT(batch);

	batch->pt = pt;
	batch->min_vaddr = __VADDR_MAX;
	batch->max_vaddr = 0;
	batch->has_tlb_range = 0;
	batch->full_flush = 0;
	batch->inline_count = 0;
	batch->chunks = __NULL;
#if CONFIG_LIBUKPAGING_DEFERRED_RELEASE
	batch->calls = __NULL;
#endif
}

#if CONFIG_LIBUKPAGING_DEFERRED_RELEASE
void uk_paging_batch_defer(struct uk_paging_free_batch *batch,
			   struct uk_paging_deferred_call *call,
			   void (*release)(void *arg), void *arg)
{
	UK_ASSERT(batch && batch->pt);
	UK_ASSERT(call);
	UK_ASSERT(release);

	call->release = release;
	call->arg = arg;
	call->next = batch->calls;
	batch->calls = call;
}

#endif
static inline void pg_batch_record_tlb(struct uk_paging_free_batch *batch,
				       __vaddr_t vaddr, __sz page_size)
{
	__vaddr_t end;

	if (!batch)
		return;
	if (vaddr == UK_PAL_VADDR_INV || page_size == 0) {
		batch->full_flush = 1;
		return;
	}
	end = (vaddr <= __VADDR_MAX - page_size) ? (vaddr + page_size) :
						   __VADDR_MAX;
	if (!batch->has_tlb_range) {
		batch->min_vaddr = vaddr;
		batch->max_vaddr = end;
		batch->has_tlb_range = 1;
		return;
	}
	if (vaddr < batch->min_vaddr)
		batch->min_vaddr = vaddr;
	if (end > batch->max_vaddr)
		batch->max_vaddr = end;
}

/* Ensure we can defer a frame before invalidating its mapping. */
static int pg_batch_reserve_frame(struct uk_pagetable *pt)
{
	struct uk_paging_free_batch *batch = pt->defer_batch;
	struct uk_paging_free_chunk *chunk;
	__paddr_t chunk_paddr = UK_PAL_PADDR_INV;
	int rc;

	if (!batch || batch->inline_count < UK_PAGING_FREE_BATCH_INLINE)
		return 0;
	chunk = batch->chunks;
	if (chunk && chunk->count < PG_FREE_CHUNK_CAPACITY)
		return 0;

	rc = pg_falloc(pt, &chunk_paddr, UK_PAL_PAGE_LEVEL);
	if (unlikely(rc))
		return rc;
	chunk = (struct uk_paging_free_chunk *)uk_pt_frame_vaddr(chunk_paddr);
	chunk->next = batch->chunks;
	chunk->chunk_paddr = chunk_paddr;
	chunk->count = 0;
	batch->chunks = chunk;
	return 0;
}

static void pg_batch_defer_frame(struct uk_pagetable *pt, __paddr_t paddr,
				 unsigned int level)
{
	struct uk_paging_free_batch *batch = pt->defer_batch;
	struct uk_paging_free_entry *entry;

	if (!batch) {
		pg_ffree(pt, paddr, level);
		return;
	}

	if (batch->inline_count < UK_PAGING_FREE_BATCH_INLINE) {
		entry = &batch->inline_entries[batch->inline_count++];
	} else {
		struct uk_paging_free_chunk *chunk = batch->chunks;

		UK_ASSERT(chunk && chunk->count < PG_FREE_CHUNK_CAPACITY);
		entry = &chunk->entries[chunk->count++];
	}
	entry->paddr = paddr;
	entry->level = level;
}

void uk_paging_batch_begin(struct uk_paging_free_batch *batch,
			   struct uk_pagetable *pt)
{
	uk_paging_free_batch_init(batch, pt);
	if (pt) {
		UK_ASSERT(uk_pcpuvar_current_get(pg_cpu_defer_batch) == __NULL);
		uk_pcpuvar_current_set(pg_cpu_defer_batch, batch);
	}
}

void uk_paging_batch_end(struct uk_paging_free_batch *batch)
{
	UK_ASSERT(batch);
	if (batch->pt) {
		UK_ASSERT(uk_pcpuvar_current_get(pg_cpu_defer_batch) == batch);
		uk_pcpuvar_current_set(pg_cpu_defer_batch, __NULL);
	}
}

#if CONFIG_HAVE_SMP
#define PG_TLB_SHOOTDOWN_FULL_THRESHOLD (32UL * UK_PAGING_PAGE_SIZE)

struct pg_tlb_shootdown_req {
	struct uk_pagetable *pt;
	__vaddr_t start;
	__vaddr_t end;
	__u8 full_flush;
};

static void pg_tlb_shootdown_ipi(struct uk_lcpu_regs *regs __unused, void *arg)
{
	const struct pg_tlb_shootdown_req *req = arg;
	__vaddr_t va;

#if CONFIG_LIBUKPAGING_SHARED
	if (!uk_pt_isactive(&req->pt->pt) &&
	    !pg_uses_shared(uk_paging_pt_get_active(), req->pt))
#else
	if (!uk_pt_isactive(&req->pt->pt))
#endif
		return;

	if (req->full_flush || req->end <= req->start ||
	    (req->end - req->start) > PG_TLB_SHOOTDOWN_FULL_THRESHOLD) {
		uk_pal_tlb_flush();
		return;
	}

	for (va = req->start; va < req->end; va += UK_PAGING_PAGE_SIZE) {
		uk_pal_tlb_flush_entry(va);
		if (unlikely(va > __VADDR_MAX - UK_PAGING_PAGE_SIZE))
			break;
	}
}

static void pg_batch_shootdown_remote_tlbs(struct uk_paging_free_batch *batch)
{
	__u64 targets[CONFIG_UKPLAT_CPU_MAXCOUNT];
	__u64 self_idx = uk_pcpuvar_current_get(uk_pcpuvar_cpu_idx);
	unsigned int cpu_count = uk_lcpu_discovered_count();
	unsigned int count = 0, queued, i;
	struct pg_tlb_shootdown_req req;
	struct uk_lcpu_func fn;
	int rc;

	if (!batch->full_flush && !batch->has_tlb_range)
		return;
	if (cpu_count <= 1)
		return;
	if (cpu_count > CONFIG_UKPLAT_CPU_MAXCOUNT)
		cpu_count = CONFIG_UKPLAT_CPU_MAXCOUNT;

	uk_arch_mb();
	for (i = 0; i < cpu_count; i++) {
		struct uk_lcpu *lcpu;
		int state;

		if (i == self_idx)
			continue;
		lcpu = &uk_pcpuvar_lval(i, uk_lcpus);
		state = UK_READ_ONCE(lcpu->state);
		if (!uk_lcpu_state_is_online(state))
			continue;
		targets[count++] = i;
	}
	if (!count)
		return;
	if (unlikely(uk_lcpu_irqs_disabled()))
		UK_CRASH("Remote TLB invalidation with local IRQs disabled\n");

	req.pt = batch->pt;
	req.start = UK_PAGING_PAGE_ALIGN_DOWN(batch->min_vaddr);
	req.end = batch->max_vaddr;
	req.full_flush = batch->full_flush;
	fn.fn = pg_tlb_shootdown_ipi;
	fn.user = &req;

	/* Run lock-free with local IRQs enabled so concurrent cross-CPU
	 * shootdown IPIs are serviced immediately without deadlock.
	 */
	queued = count;
	rc = uk_lcpu_run(targets, &queued, &fn, 0);
	if (unlikely(rc))
		UK_CRASH("Cannot queue remote TLB invalidation: %d\n", rc);
	rc = uk_lcpu_wait(targets, &queued, 0);
	if (unlikely(rc))
		UK_CRASH("Cannot complete remote TLB invalidation: %d\n", rc);
	uk_arch_mb();
}
#else /* !CONFIG_HAVE_SMP */
static inline void
pg_batch_shootdown_remote_tlbs(struct uk_paging_free_batch *batch __unused)
{
}
#endif /* !CONFIG_HAVE_SMP */

void uk_paging_free_batch_finish(struct uk_paging_free_batch *batch)
{
	struct uk_paging_free_chunk *chunk, *next;
#if CONFIG_LIBUKPAGING_DEFERRED_RELEASE
	struct uk_paging_deferred_call *call, *calls;
#endif
	struct uk_pagetable *pt;
	unsigned int i;

	if (!batch || !batch->pt)
		return;

	if (batch->full_flush)
		uk_pt_flush(&batch->pt->pt);
#if CONFIG_LIBUKPAGING_SHARED
	if ((batch->full_flush || batch->has_tlb_range) &&
	    uk_paging_pt_get_active() != batch->pt &&
	    pg_uses_shared(uk_paging_pt_get_active(), batch->pt))
		uk_pal_tlb_flush();
#endif
	pg_batch_shootdown_remote_tlbs(batch);

	pt = batch->pt;
	for (i = 0; i < batch->inline_count; i++)
		pg_ffree(pt, batch->inline_entries[i].paddr,
			 batch->inline_entries[i].level);

	chunk = batch->chunks;
	while (chunk) {
		__paddr_t chunk_paddr = chunk->chunk_paddr;

		next = chunk->next;
		for (i = 0; i < chunk->count; i++)
			pg_ffree(pt, chunk->entries[i].paddr,
				 chunk->entries[i].level);
		pg_ffree(pt, chunk_paddr, UK_PAL_PAGE_LEVEL);
		chunk = next;
	}

	batch->inline_count = 0;
	batch->chunks = __NULL;
	batch->has_tlb_range = 0;
	batch->full_flush = 0;

#if CONFIG_LIBUKPAGING_DEFERRED_RELEASE
	calls = batch->calls;
	batch->calls = __NULL;
	while (calls) {
		call = calls;
		calls = call->next;
		call->release(call->arg);
	}
#endif
}

static inline int pg_pt_alloc(struct uk_pagetable *pt, __vaddr_t *pt_vaddr,
			      __paddr_t *pt_paddr, unsigned int level)
{
	__paddr_t new_pt_paddr = UK_PAL_PADDR_INV;
	__vaddr_t new_pt_vaddr;
	unsigned int rc;

	UK_ASSERT(level < UK_PAL_PT_LEVELS);

	rc = pg_falloc(pt, &new_pt_paddr, UK_PAL_PAGE_LEVEL);
	if (unlikely(rc))
		return rc;

	new_pt_vaddr = uk_pt_frame_vaddr(new_pt_paddr);

	rc = uk_pt_table_create(new_pt_vaddr, level);
	if (unlikely(rc))
		goto EXIT_FREE;

#ifdef CONFIG_LIBUKPAGING_STATS
	pt->nr_pt_pages[level]++;
#endif /* CONFIG_LIBUKPAGING_STATS */

	*pt_vaddr = new_pt_vaddr;
	*pt_paddr = new_pt_paddr;

	return 0;

EXIT_FREE:
	pg_ffree(pt, new_pt_paddr, level);
	return -ENOMEM;
}

static inline void pg_pt_free(struct uk_pagetable *pt, __vaddr_t pt_vaddr,
			      unsigned int level, bool defer)
{
	__paddr_t pt_paddr;

	UK_ASSERT(level < UK_PAL_PT_LEVELS);

	pt_paddr = uk_pt_frame_paddr(pt_vaddr);

	if (defer && pt->defer_batch)
		pt->defer_batch->full_flush = 1;
	if (defer)
		pg_batch_defer_frame(pt, pt_paddr, UK_PAL_PAGE_LEVEL);
	else
		pg_ffree(pt, pt_paddr, UK_PAL_PAGE_LEVEL);

#ifdef CONFIG_LIBUKPAGING_STATS
	UK_ASSERT(pt->nr_pt_pages[level] > 0);
	pt->nr_pt_pages[level]--;
#endif /* CONFIG_LIBUKPAGING_STATS */
}

static inline int pg_largest_level(__vaddr_t vaddr, __paddr_t paddr, __sz len,
				   unsigned int max_lvl)
{
	unsigned int lvl = max_lvl;

	while (lvl > UK_PAL_PAGE_LEVEL) {
		if (UK_PAL_PAGE_Lx_HAS(lvl) &&
		    UK_PAL_PAGE_Lx_ALIGNED(vaddr, lvl) &&
		    UK_PAL_PAGE_Lx_ALIGNED(paddr, lvl) &&
		    UK_PAL_PAGE_Lx_SIZE(lvl) <= len)
			return lvl;

		lvl--;
	}

	return lvl;
}

static int pg_page_mapx(struct uk_pagetable *pt, __vaddr_t pt_vaddr,
			unsigned int level, __vaddr_t vaddr, __paddr_t paddr,
			__sz len, unsigned long attr, unsigned long flags,
			__pte_t tmpl, unsigned int tmpl_level,
			struct uk_paging_page_mapx *mapx)
{
	unsigned int to_lvl = UK_PAGING_PAGE_FLAG_SIZE_TO_LEVEL(flags);
	unsigned int max_lvl = pg_page_largest_level;
	unsigned int lvl = level;
	unsigned int tmp_lvl;
	__vaddr_t pt_vaddr_cache[UK_PAL_PT_LEVELS];
	__paddr_t pt_paddr;
	__pte_t pte, orig_pte;
	__sz page_size;
	unsigned int pte_idx;
	int rc, alloc_pmem;

	UK_ASSERT(len > 0);
	UK_ASSERT(UK_PAL_PAGE_Lx_ALIGNED(len, to_lvl));
	UK_ASSERT(UK_PAL_PAGE_Lx_ALIGNED(vaddr, to_lvl));
	UK_ASSERT(uk_pal_vaddr_range_isvalid(vaddr, len));

	if (paddr != UK_PAL_PADDR_INV) {
		UK_ASSERT(UK_PAL_PAGE_Lx_ALIGNED(paddr, to_lvl));
		UK_ASSERT(paddr <= __PADDR_MAX - len);
		UK_ASSERT(uk_pal_paddr_range_isvalid(paddr, len));

		alloc_pmem = 0;
	} else
		alloc_pmem = 1;

	if (!(flags & UK_PAGING_PAGE_FLAG_FORCE_SIZE)) {
		if (level < max_lvl)
			max_lvl = level;

		to_lvl = pg_largest_level(vaddr, paddr, len, max_lvl);
	}

	UK_ASSERT(lvl >= to_lvl);
	pt_vaddr_cache[lvl] = pt_vaddr;

	pte_idx = UK_PAL_PT_Lx_IDX(vaddr, lvl);
	page_size = UK_PAL_PAGE_Lx_SIZE(lvl);

	do {
		/* This loop is responsible for walking the page table down
		 * until we reach the desired level. If there is a page table
		 * missing on the way it is allocated and linked.
		 */
		while (lvl > to_lvl) {
			/* We are too high and need to walk further down */
			rc = uk_pt_pte_read_at(&pt->pt, pt_vaddr, lvl, pte_idx,
					       &pte);
			if (unlikely(rc))
				return rc;
			if (UK_PAL_PT_Lx_PTE_PRESENT(pte, lvl)) {
				/* If there is already a larger page mapped
				 * at this address and we have a mapx, we
				 * split the page until we reach the target
				 * level and let the mapx decide what to do.
				 * Otherwise, we bail out.
				 */
				if (UK_PAL_PAGE_Lx_IS(pte, lvl)) {
					if (!mapx)
						return -EEXIST;

					rc = pg_page_split(pt, pt_vaddr, vaddr,
							   lvl);
					if (unlikely(rc))
						return rc;

					continue;
				}

				pt_vaddr = pg_pte_to_vaddr(pte, lvl);
			} else {
				/* There is nothing here, not even a page table.
				 * So allocate a new one and link it.
				 */
				rc = pg_pt_alloc(pt, &pt_vaddr, &pt_paddr,
						 lvl - 1);
				if (unlikely(rc))
					return rc;

				if (!(flags & UK_PAGING_PAGE_FLAG_KEEP_PTES))
					pte = tmpl;
				else
					tmpl_level = lvl;

				rc = uk_pt_table_link_at(&pt->pt,
							 pt_vaddr_cache[lvl],
							 lvl, vaddr, pt_paddr,
							 pte, tmpl_level);
				if (unlikely(rc)) {
					pg_pt_free(pt, pt_vaddr, lvl - 1, false);
					return rc;
				}
			}

			UK_ASSERT(lvl > UK_PAL_PAGE_LEVEL);
			lvl--;

			pt_vaddr_cache[lvl] = pt_vaddr;

			pte_idx = UK_PAL_PT_Lx_IDX(vaddr, lvl);
			page_size = UK_PAL_PAGE_Lx_SIZE(lvl);
		}

		UK_ASSERT(lvl == to_lvl);
		UK_ASSERT(UK_PAL_PAGE_Lx_HAS(lvl));

		/* At this point, we are at the target level and know that
		 * pages can be mapped at this level.
		 */
		rc = uk_pt_pte_read_at(&pt->pt, pt_vaddr, lvl, pte_idx, &pte);
		if (unlikely(rc))
			return rc;

		orig_pte = pte;

		if (UK_PAL_PT_Lx_PTE_PRESENT(pte, lvl)) {
			/* It could be that there is a page table linked at
			 * this PTE. In this case, we descent further down to
			 * the next level that allows to map pages.
			 */
			if (!UK_PAL_PAGE_Lx_IS(pte, lvl) &&
			    (!(flags & UK_PAGING_PAGE_FLAG_FORCE_SIZE))) {
				UK_ASSERT(lvl > UK_PAL_PAGE_LEVEL);
				to_lvl = pg_largest_level(0, 0, __SZ_MAX,
							  lvl - 1);
				UK_ASSERT(to_lvl < lvl);

				continue;
			}

			/* This is not a page table. However, we are on the
			 * correct level. If we have a mapping function we let
			 * the mapping function decide what we should do with
			 * the existing mapping. Otherwise, bail out.
			 */
			if (!mapx)
				return -EEXIST;

			paddr = UK_PAL_PT_Lx_PTE_PADDR(pte, lvl);
		} else if (alloc_pmem) {
			UK_ASSERT(!UK_PAL_PT_Lx_PTE_PRESENT(pte, lvl));

			paddr = UK_PAL_PADDR_INV;
			rc = pg_falloc(pt, &paddr, lvl);
			if (unlikely(rc)) {
TOO_BIG:
				/* We could not allocate a contiguous,
				 * self-aligned block of physical memory with
				 * the requested size. If we should map largest
				 * possible size, we reduce page size.
				 */
				if ((flags & UK_PAGING_PAGE_FLAG_FORCE_SIZE) ||
				    lvl == UK_PAL_PAGE_LEVEL)
					return rc;

				/* Find lower level that allows to map pages */
				to_lvl = pg_largest_level(0, 0, __SZ_MAX,
							  lvl - 1);
				UK_ASSERT(to_lvl < lvl);

				/* Restrict following mappings */
				max_lvl = to_lvl;

				continue;
			}

			/* If something goes wrong in the following, we must
			 * free the physical memory!
			 */
		}

		UK_ASSERT(UK_PAL_PAGE_Lx_ALIGNED(vaddr, lvl));
		UK_ASSERT(UK_PAL_PAGE_Lx_ALIGNED(paddr, lvl));

		if (!(flags & UK_PAGING_PAGE_FLAG_KEEP_PTES))
			pte = tmpl;
		else
			tmpl_level = lvl;

		pte = uk_pal_pte_create(paddr, attr, lvl, pte, tmpl_level);

		if (mapx) {
			/* pte will always be prepared with the PTE that we
			 * intend to write if the mapx returns success.
			 * Accessing the current PTE can be done by reading it
			 * from the page table.
			 */
			UK_ASSERT(mapx->map);
			rc = mapx->map(pt, vaddr, pt_vaddr, lvl, &pte,
				       mapx->ctx);
			if (unlikely(rc)) {
				if (alloc_pmem &&
				    !UK_PAL_PT_Lx_PTE_PRESENT(orig_pte, lvl))
					pg_ffree(pt, paddr, lvl);

				if (rc == UK_PAGING_PAGE_MAPX_ESKIP)
					goto NEXT_PTE;

				if (rc == UK_PAGING_PAGE_MAPX_ETOOBIG) {
					rc = -ENOMEM;
					goto TOO_BIG;
				}

				UK_ASSERT(rc < 0);
				return rc;
			}
		}

		UK_ASSERT(UK_PAL_PAGE_Lx_ALIGNED(UK_PAL_PT_Lx_PTE_PADDR(pte, lvl), lvl));

		rc = uk_pt_pte_write_at(&pt->pt, pt_vaddr, lvl, pte_idx,
					UK_PAL_VADDR_INV, pte);
		if (unlikely(rc)) {
			if (alloc_pmem &&
			    !UK_PAL_PT_Lx_PTE_PRESENT(orig_pte, lvl))
				pg_ffree(pt, paddr, lvl);

			return rc;
		}

#ifdef CONFIG_LIBUKPAGING_STATS
		if (!(flags & UK_PAGING_PAGE_FLAG_INTERN_STATS_KEEP))
			pt->nr_lx_pages[lvl]++;
#endif /* CONFIG_LIBUKPAGING_STATS */

		if (UK_PAL_PT_Lx_PTE_PRESENT(orig_pte, lvl)) {
			uk_pt_flush_entry(&pt->pt, vaddr);
			pg_batch_record_tlb(pt->defer_batch, vaddr, page_size);
		}

NEXT_PTE:
		UK_ASSERT(len >= page_size);
		len -= page_size;

		if (len == 0)
			break;

		UK_ASSERT(vaddr <= __VADDR_MAX - page_size);
		UK_ASSERT(paddr <= __PADDR_MAX - page_size);

		/* We need to map more pages. If we have reached the last PTE
		 * in this page table, we have to walk up again until we reach
		 * a page table where this is not the last PTE. We then walk
		 * down to the target level again.
		 */
		if (pte_idx == UK_PAL_PT_Lx_PTES(lvl) - 1) {
			do {
				UK_ASSERT(lvl <= level);

				/* Go up one level */
				pte_idx = UK_PAL_PT_Lx_IDX(vaddr, ++lvl);
				UK_ASSERT(pte_idx < UK_PAL_PT_Lx_PTES(lvl));
			} while (pte_idx == UK_PAL_PT_Lx_PTES(lvl) - 1);

			pt_vaddr = pt_vaddr_cache[lvl];

			vaddr += page_size;
			paddr += page_size;

			/* When we reach the last PTE in a page table, this is
			 * always a point where the vaddr will be aligned to a
			 * larger page size, if existing. So re-evaluate the
			 * target level.
			 */
			if (!(flags & UK_PAGING_PAGE_FLAG_FORCE_SIZE)) {
				/* The new target level must not be larger than
				 * the updated current level as we know there
				 * are page tables down to that level anyways.
				 */
				tmp_lvl = (max_lvl > lvl) ? lvl : max_lvl;
				if (alloc_pmem)
					paddr = UK_PAL_PADDR_INV;

				to_lvl = pg_largest_level(vaddr, paddr, len,
							  tmp_lvl);
				UK_ASSERT(to_lvl <= lvl);
			}

			page_size = UK_PAL_PAGE_Lx_SIZE(lvl);
		} else {
			vaddr += page_size;
			paddr += page_size;

			if (len < page_size) {
				UK_ASSERT(!(flags & UK_PAGING_PAGE_FLAG_FORCE_SIZE));

				/* We should not map another whole page at the
				 * current level. We therefore search for the
				 * next smaller page size, that fits the
				 * remaining len. Since we know addresses are
				 * aligned at this level, we know that they
				 * will implicitly be aligned for all lower
				 * levels.
				 */
				to_lvl = pg_largest_level(0, 0, len, lvl - 1);
				UK_ASSERT(to_lvl < lvl);
			}
		}

		pte_idx++;
		UK_ASSERT(pte_idx < UK_PAL_PT_Lx_PTES(lvl));

	} while (1);

	return 0;
}

int uk_paging_page_mapx(struct uk_pagetable *pt, __vaddr_t vaddr,
			__paddr_t paddr, unsigned long pages,
			unsigned long attr, unsigned long flags,
			struct uk_paging_page_mapx *mapx)
{
	unsigned int level = UK_PAGING_PAGE_FLAG_SIZE_TO_LEVEL(flags);
	struct uk_paging_free_batch local_batch;
	struct uk_paging_free_batch *batch;
	unsigned long irqf;
	__sz len;
	int rc;

	if (unlikely(pages == 0))
		return 0;

	UK_ASSERT(level < UK_PAL_PT_LEVELS);
	UK_ASSERT(UK_PAL_PAGE_Lx_HAS(level));
	UK_ASSERT(pages <= (__SZ_MAX / UK_PAL_PAGE_Lx_SIZE(level)));

	len = pages * UK_PAL_PAGE_Lx_SIZE(level);
#if CONFIG_LIBUKPAGING_SHARED
	if (pg_range_shared(pt, vaddr, len))
		return -EACCES;
#endif

#ifdef CONFIG_LIBUKPAGING_STATS
	UK_ASSERT(!(flags & UK_PAGING_PAGE_FLAG_INTERN_STATS_KEEP));
#endif /* CONFIG_LIBUKPAGING_STATS */

	UK_ASSERT(vaddr <= __VADDR_MAX - len);

	uk_paging_free_batch_init(&local_batch, pt);
	ukplat_spin_lock_irqsave(&pt->lock, irqf);
	UK_ASSERT(uk_pt_vbase(&pt->pt) != UK_PAL_VADDR_INV);
	UK_ASSERT(uk_pt_pbase(&pt->pt) != UK_PAL_PADDR_INV);
	batch = uk_pcpuvar_current_get(pg_cpu_defer_batch);
	if (!batch || batch->pt != pt)
		batch = &local_batch;
	UK_ASSERT(pt->defer_batch == __NULL);
	pt->defer_batch = batch;

	rc = pg_page_mapx(pt, uk_pt_vbase(&pt->pt), UK_PAL_PT_LEVELS - 1, vaddr,
			  paddr, len, attr, flags,
			  UK_PAL_PT_Lx_PTE_INVALID(UK_PAL_PAGE_LEVEL),
			  UK_PAL_PAGE_LEVEL, mapx);

	pt->defer_batch = __NULL;
	ukplat_spin_unlock_irqrestore(&pt->lock, irqf);

	if (batch == &local_batch)
		uk_paging_free_batch_finish(&local_batch);

	return rc;
}

static int pg_page_split(struct uk_pagetable *pt, __vaddr_t pt_vaddr,
			 __vaddr_t vaddr, unsigned int level)
{
	unsigned int to_lvl __maybe_unused = level - 1;
	__vaddr_t new_pt_vaddr;
	__paddr_t new_pt_paddr;
	int rc;

	UK_ASSERT(level > UK_PAL_PAGE_LEVEL);
	UK_ASSERT(UK_PAL_PAGE_Lx_HAS(level));
	UK_ASSERT(UK_PAL_PAGE_Lx_ALIGNED(vaddr, level));

	/* Create a page table that will hold all mappings and potential
	 * child tables.
	 */
	rc = pg_pt_alloc(pt, &new_pt_vaddr, &new_pt_paddr, level - 1);
	if (unlikely(rc))
		return rc;

	/* Replace the page with mappings of the next smaller page size that
	 * map the same contiguous range of physical memory
	 */
	rc = uk_pt_table_split_at(&pt->pt, pt_vaddr, level, vaddr,
				  new_pt_vaddr, new_pt_paddr);
	if (unlikely(rc)) {
		pg_pt_free(pt, new_pt_vaddr, level - 1, false);
		return rc;
	}

	if (pt == uk_paging_pt_get_active())
		uk_pal_tlb_flush_entry(vaddr);
	pg_batch_record_tlb(pt->defer_batch, vaddr, UK_PAL_PAGE_Lx_SIZE(level));

#ifdef CONFIG_LIBUKPAGING_STATS
	UK_ASSERT(pt->nr_lx_pages[level] > 0);
	pt->nr_lx_pages[level]--;
	pt->nr_lx_pages[to_lvl] += UK_PAL_PAGE_Lx_SIZE(level) /
				   UK_PAL_PAGE_Lx_SIZE(to_lvl);
	pt->nr_lx_splits[level]++;
#endif /* CONFIG_LIBUKPAGING_STATS */

	return 0;
}

static int pg_page_unmap(struct uk_pagetable *pt, __vaddr_t pt_vaddr,
			 unsigned int level, __vaddr_t vaddr, __sz len,
			 unsigned long flags)
{
	unsigned int to_lvl = UK_PAGING_PAGE_FLAG_SIZE_TO_LEVEL(flags);
	unsigned int plvl, lvl = level;
	__vaddr_t pt_vaddr_cache[UK_PAL_PT_LEVELS];
	__pte_t pte, new_pte;
	__sz page_size;
	unsigned int pte_idx_cache[UK_PAL_PT_LEVELS];
	unsigned int first_pte_idx[UK_PAL_PT_LEVELS];
	unsigned int pte_idx, i;
	int rc, skip_pt_free;

	UK_ASSERT(lvl >= to_lvl);
	pt_vaddr_cache[lvl] = pt_vaddr;

	if (vaddr == UK_PAL_VADDR_INV) {
		UK_ASSERT(len == __SZ_MAX);

		pte_idx = 0;
		page_size = 0;
	} else {
		UK_ASSERT(len > 0);
		UK_ASSERT(UK_PAL_PAGE_Lx_ALIGNED(len, to_lvl));
		UK_ASSERT(UK_PAL_PAGE_Lx_ALIGNED(vaddr, to_lvl));
		UK_ASSERT(vaddr <= __VADDR_MAX - len);
		UK_ASSERT(uk_pal_vaddr_range_isvalid(vaddr, len));

		pte_idx = UK_PAL_PT_Lx_IDX(vaddr, lvl);
		page_size = UK_PAL_PAGE_Lx_SIZE(lvl);
	}

	first_pte_idx[lvl] = pte_idx;
	skip_pt_free = (flags & UK_PAGING_PAGE_FLAG_KEEP_PTES);

	do {
		rc = uk_pt_pte_read_at(&pt->pt, pt_vaddr, lvl, pte_idx, &pte);
		if (unlikely(rc))
			return rc;

#if CONFIG_LIBUKPAGING_SHARED
		if (lvl == UK_PAL_PT_LEVELS - 1 && pg_index_shared(pt, pte_idx))
			goto next_entry;
#endif

		if (UK_PAL_PT_Lx_PTE_PRESENT(pte, lvl)) {

			/* There is a page table at this PTE.
			 * Descent, if allowed.
			 */
			if (!UK_PAL_PAGE_Lx_IS(pte, lvl)) {
				if ((flags & UK_PAGING_PAGE_FLAG_FORCE_SIZE) &&
				    lvl == to_lvl)
					return -EFAULT;

				pt_vaddr = pg_pte_to_vaddr(pte, lvl);

				pte_idx_cache[lvl] = pte_idx;

				UK_ASSERT(lvl > UK_PAL_PAGE_LEVEL);
				lvl--;

				pt_vaddr_cache[lvl] = pt_vaddr;

				if (vaddr == UK_PAL_VADDR_INV) {
					pte_idx = 0;
					UK_ASSERT(page_size == 0);
				} else {
					pte_idx = UK_PAL_PT_Lx_IDX(vaddr, lvl);
					page_size = UK_PAL_PAGE_Lx_SIZE(lvl);
				}

				first_pte_idx[lvl] = pte_idx;
				skip_pt_free = (flags & UK_PAGING_PAGE_FLAG_KEEP_PTES);

				continue;
			}

			UK_ASSERT(UK_PAL_PAGE_Lx_IS(pte, lvl));

			/* At this point, we know that there is a page mapped
			 * here. If we do not enforce a certain page size we
			 * might have to split the page (i.e., it is larger
			 * than the remaining len to unmap, or it is not
			 * aligned to the current vaddr).
			 */
			if ((flags & UK_PAGING_PAGE_FLAG_FORCE_SIZE) &&
			    lvl != to_lvl)
				return -EFAULT;

			if ((page_size > len) ||
			    (!UK_PAL_PAGE_Lx_ALIGNED(vaddr, lvl))) {
				UK_ASSERT(lvl > UK_PAL_PAGE_LEVEL);

				rc = pg_page_split(pt, pt_vaddr,
					UK_PAL_PAGE_Lx_ALIGN_DOWN(vaddr, lvl),
					lvl);
				if (unlikely(rc))
					return rc;

				continue;
			}

			UK_ASSERT(len >= page_size);
			UK_ASSERT(UK_PAL_PAGE_Lx_ALIGNED(vaddr, lvl));

			/* At this point, we know that we can safely invalidate
			 * the current PTE as it is a page with a size that
			 * is below the remaining len to unmap and the address
			 * we want to unmap is aligned to the page size.
			 */
			if (!(flags & UK_PAGING_PAGE_FLAG_KEEP_FRAMES)) {
				rc = pg_batch_reserve_frame(pt);
				if (unlikely(rc))
					return rc;
			}
			new_pte = (flags & UK_PAGING_PAGE_FLAG_KEEP_PTES) ?
					UK_PAL_PT_Lx_PTE_CLEAR_PRESENT(pte, lvl) :
					UK_PAL_PT_Lx_PTE_INVALID(lvl);

			rc = uk_pt_pte_write_at(&pt->pt, pt_vaddr, lvl, pte_idx,
						vaddr, new_pte);
			if (unlikely(rc))
				return rc;

			pg_batch_record_tlb(pt->defer_batch, vaddr, page_size);

#ifdef CONFIG_LIBUKPAGING_STATS
			if (!(flags & UK_PAGING_PAGE_FLAG_INTERN_STATS_KEEP)) {
				UK_ASSERT(pt->nr_lx_pages[lvl] > 0);
				pt->nr_lx_pages[lvl]--;
			}
#endif /* CONFIG_LIBUKPAGING_STATS */

			if (!(flags & UK_PAGING_PAGE_FLAG_KEEP_FRAMES))
				pg_batch_defer_frame(pt,
					UK_PAL_PT_Lx_PTE_PADDR(pte, lvl), lvl);
		}

#if CONFIG_LIBUKPAGING_SHARED
next_entry:
#endif
		/* If this is not the last PTE and there are still pages to
		 * unmap, we continue to the next PTE in this level
		 */
		if ((pte_idx < UK_PAL_PT_Lx_PTES(lvl) - 1) && len > page_size) {
			UK_ASSERT(vaddr <= __VADDR_MAX - page_size);
			vaddr += page_size;
			len -= page_size;
			pte_idx++;

			continue;
		}

		UK_ASSERT((pte_idx == UK_PAL_PT_Lx_PTES(lvl) - 1) ||
			  (len <= page_size));

		/* At this point, we either invalidated the last PTE in this
		 * page table and we have to walk up again, or we tried to
		 * invalidate the last page in the mapping (i.e.,
		 * len <= page size). Note that if len < page size there was no
		 * mapping for the remaining page and the mapping should have
		 * been on a lower level. In all cases, we can free page
		 * tables on the way up if the caller did not request to keep
		 * the PTEs and all other PTEs are INVALID.
		 */
		while ((pte_idx == UK_PAL_PT_Lx_PTES(lvl) - 1) ||
		       ((len <= page_size) && !skip_pt_free)) {

			/* If we reached the top level, stop. */
			if (lvl == level)
				break;

			/* Save PTE index and level in case we try to free
			 * the PT
			 */
			i = pte_idx;
			plvl = lvl;

			/* Go up one level */
			pte_idx = pte_idx_cache[++lvl];
			UK_ASSERT(pte_idx < UK_PAL_PT_Lx_PTES(lvl));

			if (skip_pt_free)
				continue;

#if CONFIG_LIBUKPAGING_SHARED
			/* Keep shared top-level links valid while their branch
			 * temporarily contains no mappings.
			 */
			if (lvl == UK_PAL_PT_LEVELS - 1 && pt->shared_users) {
				skip_pt_free = 1;
				continue;
			}
#endif

			/* Check if the PTEs we did not touch are all invalid.
			 * In that case we can free the page table. We start
			 * with the entries following the unmapped range and
			 * continue with the entries preceding the range. If
			 * we find a non-zero PTE, we know the page table is
			 * still in use and we cannot free it. This means we
			 * cannot remove any of the upper page table levels.
			 * Note that we always have to include the PTE that
			 * we started with in the level, because when we run
			 * this while loop multiple times and go up each level
			 * we might not freed the lower-level page table. So it
			 * is not guaranteed that the "current" PTE is always
			 * invalid.
			 */
			while (i++ < UK_PAL_PT_Lx_PTES(plvl) - 1) {
				rc = uk_pt_pte_read_at(&pt->pt, pt_vaddr, plvl,
						       i, &pte);
				if (unlikely(rc))
					return rc;

				if (pte != UK_PAL_PT_Lx_PTE_INVALID(plvl)) {
					skip_pt_free = 1;
					break;
				}
			}

			if (skip_pt_free)
				continue;

			i = first_pte_idx[plvl];
			do {
				rc = uk_pt_pte_read_at(&pt->pt, pt_vaddr, plvl,
						       i, &pte);
				if (unlikely(rc))
					return rc;

				if (pte != UK_PAL_PT_Lx_PTE_INVALID(plvl)) {
					skip_pt_free = 1;
					break;
				}
			} while (i-- > 0);

			if (skip_pt_free)
				continue;

			pt_vaddr = pt_vaddr_cache[lvl];
			/* At this point, we know that the page table does not
			 * contain any valid entries and we can safely free it
			 */
			rc = pg_batch_reserve_frame(pt);
			if (unlikely(rc))
				return rc;
			rc = uk_pt_pte_write_at(&pt->pt, pt_vaddr, lvl, pte_idx,
						vaddr,
						UK_PAL_PT_Lx_PTE_INVALID(lvl));
			if (unlikely(rc))
				return rc;

			pg_batch_record_tlb(pt->defer_batch, vaddr, page_size);
			pg_pt_free(pt, pt_vaddr_cache[plvl], plvl, true);
		}

		if (len <= page_size)
			break;

		pt_vaddr = pt_vaddr_cache[lvl];

		UK_ASSERT(vaddr <= __VADDR_MAX - page_size);
		vaddr += page_size;
		len -= page_size;

		if (vaddr != UK_PAL_VADDR_INV)
			page_size = UK_PAL_PAGE_Lx_SIZE(lvl);

		/* Do not move this into the while condition. We use continue */
		if (++pte_idx == UK_PAL_PT_Lx_PTES(lvl))
			break;

	} while (1);

	if (vaddr == UK_PAL_VADDR_INV) {
		uk_pt_flush(&pt->pt);
		pg_batch_record_tlb(pt->defer_batch, UK_PAL_VADDR_INV, 0);
	}

	return 0;
}

int uk_paging_page_unmap(struct uk_pagetable *pt, __vaddr_t vaddr,
			 unsigned long pages, unsigned long flags)
{
	unsigned int level = UK_PAGING_PAGE_FLAG_SIZE_TO_LEVEL(flags);
	struct uk_paging_free_batch local_batch;
	struct uk_paging_free_batch *batch;
	unsigned long irqf;
	__sz len = __SZ_MAX;
	int rc;

	if (unlikely(pages == 0))
		return 0;

	if (vaddr != UK_PAL_VADDR_INV) {
		/* Ensure that the length does not overflow */
		UK_ASSERT(level < UK_PAL_PT_LEVELS);
		UK_ASSERT(UK_PAL_PAGE_Lx_HAS(level));
		UK_ASSERT(pages <= (__SZ_MAX / UK_PAL_PAGE_Lx_SIZE(level)));

		len = pages * UK_PAL_PAGE_Lx_SIZE(level);
	}
#if CONFIG_LIBUKPAGING_SHARED
	if (pg_range_shared(pt, vaddr, len))
		return -EACCES;
#endif

#ifdef CONFIG_LIBUKPAGING_STATS
	UK_ASSERT(!(flags & UK_PAGING_PAGE_FLAG_INTERN_STATS_KEEP));
#endif /* CONFIG_LIBUKPAGING_STATS */

	uk_paging_free_batch_init(&local_batch, pt);
	ukplat_spin_lock_irqsave(&pt->lock, irqf);
	UK_ASSERT(uk_pt_vbase(&pt->pt) != UK_PAL_VADDR_INV);
	UK_ASSERT(uk_pt_pbase(&pt->pt) != UK_PAL_PADDR_INV);
	batch = uk_pcpuvar_current_get(pg_cpu_defer_batch);
	if (!batch || batch->pt != pt)
		batch = &local_batch;
	UK_ASSERT(pt->defer_batch == __NULL);
	pt->defer_batch = batch;

	rc = pg_page_unmap(pt, uk_pt_vbase(&pt->pt), UK_PAL_PT_LEVELS - 1, vaddr, len,
			   flags);

	pt->defer_batch = __NULL;
	ukplat_spin_unlock_irqrestore(&pt->lock, irqf);

	if (batch == &local_batch)
		uk_paging_free_batch_finish(&local_batch);

	return rc;
}

static int pg_page_set_attr(struct uk_pagetable *pt, __vaddr_t pt_vaddr,
			    unsigned int level, __vaddr_t vaddr, __sz len,
			    unsigned long new_attr, unsigned long flags)
{
	unsigned int to_lvl = UK_PAGING_PAGE_FLAG_SIZE_TO_LEVEL(flags);
	unsigned int lvl = level;
	__vaddr_t pt_vaddr_cache[UK_PAL_PT_LEVELS];
	__pte_t pte;
	__sz page_size;
	unsigned int pte_idx_cache[UK_PAL_PT_LEVELS];
	unsigned int pte_idx;
	int rc;

	UK_ASSERT(lvl >= to_lvl);
	pt_vaddr_cache[lvl] = pt_vaddr;

	if (vaddr == UK_PAL_VADDR_INV) {
		UK_ASSERT(len == __SZ_MAX);

		pte_idx = 0;
		page_size = 0;
	} else {
		UK_ASSERT(len > 0);
		UK_ASSERT(UK_PAL_PAGE_Lx_ALIGNED(len, to_lvl));
		UK_ASSERT(UK_PAL_PAGE_Lx_ALIGNED(vaddr, to_lvl));
		UK_ASSERT(vaddr <= __VADDR_MAX - len);
		UK_ASSERT(uk_pal_vaddr_range_isvalid(vaddr, len));

		pte_idx = UK_PAL_PT_Lx_IDX(vaddr, lvl);
		page_size = UK_PAL_PAGE_Lx_SIZE(lvl);
	}

	do {
		rc = uk_pt_pte_read_at(&pt->pt, pt_vaddr, lvl, pte_idx, &pte);
		if (unlikely(rc))
			return rc;

		if (UK_PAL_PT_Lx_PTE_PRESENT(pte, lvl)) {
			/* Check if there is a page table at this PTE. In that
			 * case descent, if allowed.
			 */
			if (!UK_PAL_PAGE_Lx_IS(pte, lvl)) {
				if ((flags & UK_PAGING_PAGE_FLAG_FORCE_SIZE) &&
				    lvl == to_lvl)
					return -EFAULT;

				pt_vaddr = pg_pte_to_vaddr(pte, lvl);

				pte_idx_cache[lvl] = pte_idx;

				UK_ASSERT(lvl > UK_PAL_PAGE_LEVEL);
				lvl--;

				pt_vaddr_cache[lvl] = pt_vaddr;

				if (vaddr == UK_PAL_VADDR_INV) {
					pte_idx = 0;
					UK_ASSERT(page_size == 0);
				} else {
					pte_idx = UK_PAL_PT_Lx_IDX(vaddr, lvl);
					page_size = UK_PAL_PAGE_Lx_SIZE(lvl);
				}

				continue;
			}

			UK_ASSERT(UK_PAL_PAGE_Lx_IS(pte, lvl));

			/* At this point, we know that there is a page mapped
			 * here. If we do not enforce a certain page size we
			 * might have to split the page (i.e., it is larger
			 * than the remaining len to change, or it is not
			 * aligned to the current vaddr).
			 */
			if ((flags & UK_PAGING_PAGE_FLAG_FORCE_SIZE) &&
			    lvl != to_lvl)
				return -EFAULT;

			if ((page_size > len) ||
			    (!UK_PAL_PAGE_Lx_ALIGNED(vaddr, lvl))) {
				UK_ASSERT(lvl > UK_PAL_PAGE_LEVEL);

				rc = pg_page_split(pt, pt_vaddr,
					UK_PAL_PAGE_Lx_ALIGN_DOWN(vaddr, lvl),
					lvl);
				if (unlikely(rc))
					return rc;

				continue;
			}

			UK_ASSERT(UK_PAL_PAGE_Lx_ALIGNED(vaddr, lvl));

			/* At this point, we know that we can safely change the
			 * current PTE as it is a page with a size that is
			 * below the remaining len to change and the address we
			 * want to change is aligned to the page size.
			 */
			rc = uk_pt_set_attr_at(&pt->pt, pt_vaddr, lvl, vaddr,
					       new_attr);
			if (unlikely(rc))
				return rc;

			pg_batch_record_tlb(pt->defer_batch, vaddr, page_size);
		}

		/* Bail out if there is nothing more to do */
		if (page_size >= len)
			break;

		len -= page_size;

		UK_ASSERT(vaddr <= __VADDR_MAX - page_size);
		vaddr += page_size;

		/* We need to change more pages. If we have reached the last PTE
		 * in this page table, we have to walk up again until we reach
		 * a page table where this is not the last PTE.
		 */
		if (pte_idx == UK_PAL_PT_Lx_PTES(lvl) - 1) {
			do {
				/* If we reached the top level, stop. */
				if (lvl == level)
					break;

				/* Go up one level */
				pte_idx = pte_idx_cache[++lvl];
				UK_ASSERT(pte_idx < UK_PAL_PT_Lx_PTES(lvl));
			} while (pte_idx == UK_PAL_PT_Lx_PTES(lvl) - 1);

			pt_vaddr = pt_vaddr_cache[lvl];

			if (vaddr != UK_PAL_VADDR_INV)
				page_size = UK_PAL_PAGE_Lx_SIZE(lvl);
		}

		/* Do not move this into the while condition. We use continue */
		if (++pte_idx == UK_PAL_PT_Lx_PTES(lvl))
			break;

	} while (1);

	if (vaddr == UK_PAL_VADDR_INV) {
		uk_pt_flush(&pt->pt);
		pg_batch_record_tlb(pt->defer_batch, UK_PAL_VADDR_INV, 0);
	}

	return 0;
}

int uk_paging_page_set_attr(struct uk_pagetable *pt, __vaddr_t vaddr,
			    unsigned long pages, unsigned long new_attr,
			    unsigned long flags)
{
	unsigned int level = UK_PAGING_PAGE_FLAG_SIZE_TO_LEVEL(flags);
	struct uk_paging_free_batch local_batch;
	struct uk_paging_free_batch *batch;
	unsigned long irqf;
	__sz len = __SZ_MAX;
	int rc;

	if (unlikely(pages == 0))
		return 0;

	if (vaddr != UK_PAL_VADDR_INV) {
		/* Ensure that the length does not overflow */
		UK_ASSERT(level < UK_PAL_PT_LEVELS);
		UK_ASSERT(UK_PAL_PAGE_Lx_HAS(level));
		UK_ASSERT(pages <= (__SZ_MAX / UK_PAL_PAGE_Lx_SIZE(level)));

		len = pages * UK_PAL_PAGE_Lx_SIZE(level);
	}
#if CONFIG_LIBUKPAGING_SHARED
	if (pg_range_shared(pt, vaddr, len))
		return -EACCES;
#endif

#ifdef CONFIG_LIBUKPAGING_STATS
	UK_ASSERT(!(flags & UK_PAGING_PAGE_FLAG_INTERN_STATS_KEEP));
#endif /* CONFIG_LIBUKPAGING_STATS */

	uk_paging_free_batch_init(&local_batch, pt);
	ukplat_spin_lock_irqsave(&pt->lock, irqf);
	UK_ASSERT(uk_pt_vbase(&pt->pt) != UK_PAL_VADDR_INV);
	UK_ASSERT(uk_pt_pbase(&pt->pt) != UK_PAL_PADDR_INV);
	batch = uk_pcpuvar_current_get(pg_cpu_defer_batch);
	if (!batch || batch->pt != pt)
		batch = &local_batch;
	UK_ASSERT(pt->defer_batch == __NULL);
	pt->defer_batch = batch;

	rc = pg_page_set_attr(pt, uk_pt_vbase(&pt->pt), UK_PAL_PT_LEVELS - 1, vaddr,
			      len, new_attr, flags);

	pt->defer_batch = __NULL;
	ukplat_spin_unlock_irqrestore(&pt->lock, irqf);

	if (batch == &local_batch)
		uk_paging_free_batch_finish(&local_batch);

	return rc;
}

__paddr_t uk_paging_virt_to_phys(__vaddr_t address)
{
	struct uk_pagetable *pt = uk_paging_pt_get_active();
	__vaddr_t vaddr = (__vaddr_t)address;
	__pte_t pte;
	unsigned int level = UK_PAGING_PAGE_LEVEL;
	unsigned long offset;
	int rc __maybe_unused;

	rc = uk_paging_pt_walk(pt, UK_PAGING_PAGE_ALIGN_DOWN(vaddr),
			       &level, __NULL, &pte);
	UK_ASSERT(rc == 0);

	UK_ASSERT(UK_PAGING_PT_Lx_PTE_PRESENT(pte, level));
	UK_ASSERT(UK_PAGING_PAGE_Lx_IS(pte, level));

	offset = vaddr - UK_PAGING_PAGE_Lx_ALIGN_DOWN(vaddr, level);

	return UK_PAGING_PT_Lx_PTE_PADDR(pte, level) + offset;
}

__vaddr_t uk_paging_page_kmap(struct uk_pagetable *pt, __paddr_t paddr,
			      unsigned long pages, unsigned long flags)
{
	unsigned int level = UK_PAGING_PAGE_FLAG_SIZE_TO_LEVEL(flags);
	__sz len = __SZ_MAX;

	UK_ASSERT(pages > 0);
	UK_ASSERT(level < UK_PAL_PT_LEVELS);
	UK_ASSERT(UK_PAL_PAGE_Lx_HAS(level));
	UK_ASSERT(pages <= (__SZ_MAX / UK_PAL_PAGE_Lx_SIZE(level)));

	len = pages * UK_PAL_PAGE_Lx_SIZE(level);

	return pgarch_kmap(pt, paddr, len);
}

void uk_paging_page_kunmap(struct uk_pagetable *pt, __vaddr_t vaddr,
			   unsigned long pages, unsigned long flags)
{
	unsigned int level = UK_PAGING_PAGE_FLAG_SIZE_TO_LEVEL(flags);
	__sz len = __SZ_MAX;

	UK_ASSERT(pages > 0);
	UK_ASSERT(level < UK_PAL_PT_LEVELS);
	UK_ASSERT(UK_PAL_PAGE_Lx_HAS(level));
	UK_ASSERT(pages <= (__SZ_MAX / UK_PAL_PAGE_Lx_SIZE(level)));

	len = pages * UK_PAL_PAGE_Lx_SIZE(level);

	pgarch_kunmap(pt, vaddr, len);
}

static inline unsigned long bootinfo_to_page_attr(__u16 flags)
{
	unsigned long prot = 0;

	if (flags & UKPLAT_MEMRF_READ)
		prot |= UK_PAL_PAGE_ATTR_PROT_READ;
	if (flags & UKPLAT_MEMRF_WRITE)
		prot |= UK_PAL_PAGE_ATTR_PROT_WRITE;
	if (flags & UKPLAT_MEMRF_EXECUTE)
		prot |= UK_PAL_PAGE_ATTR_PROT_EXEC;

	return prot;
}

int uk_paging_init(void)
{
	struct ukplat_memregion_desc *mrd;
	unsigned long prot;
	int rc;

	/* Initialize the frame allocator with the free physical memory
	 * regions supplied via the boot info. The new page table uses the
	 * one currently active.
	 */
	rc = -ENOMEM; /* In case there is no region */
	ukplat_memregion_foreach(&mrd, UKPLAT_MEMRT_FREE, 0, 0) {
		UK_ASSERT_VALID_FREE_MRD(mrd);

		/* Not mapped */
		mrd->vbase = __U64_MAX;
		mrd->flags &= ~UKPLAT_MEMRF_PERMS;

		if (!kernel_pt.fa) {
			rc = uk_paging_pt_init(&kernel_pt, mrd->pbase,
					       mrd->pg_count * UK_PAL_PAGE_SIZE);
			if (unlikely(rc))
				kernel_pt.fa = NULL;
		} else {
			rc = uk_paging_pt_add_mem(&kernel_pt, mrd->pbase,
						  mrd->pg_count * UK_PAL_PAGE_SIZE);
		}

		/* We do not fail if we cannot add this memory region to the
		 * frame allocator. If the range is too small to hold the
		 * metadata, this is expected. Just ignore this error.
		 */
		if (unlikely(rc && rc != -ENOMEM))
			uk_pr_err("Cannot add %12lx-%12lx to paging: %d\n",
				  mrd->pbase, mrd->pbase + mrd->len, rc);
	}

	/* The frame allocator should've only had page-aligned memory regions
	 * added to it. Make sure nothing happened in the meantime.
	 */
	UK_ASSERT(!(kernel_pt.fa->free_memory & ~UK_PAL_PAGE_MASK));

	if (unlikely(!kernel_pt.fa))
		return rc;

	/* Perform mappings */
	ukplat_memregion_foreach(&mrd, 0, 0, 0) {
		/* Free mem is managed by falloc */
		if (mrd->type == UKPLAT_MEMRT_FREE)
			continue;

		UK_ASSERT_VALID_MRD(mrd);

		prot  = bootinfo_to_page_attr(mrd->flags);

		rc = uk_paging_page_map(&kernel_pt, mrd->vbase, mrd->pbase,
					mrd->pg_count, prot, 0);
		if (unlikely(rc))
			return rc;
	}

	/* Activate page table */
	rc = uk_paging_pt_set_active(&kernel_pt);
	if (unlikely(rc))
		return rc;

	return 0;
}
