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

#include <uk/config.h>
#include <uk/arch/limits.h>
#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/falloc.h>
#include <uk/paging/arch.h>
#include <uk/paging.h>
#include <uk/plat/memory.h>
#include <uk/print.h>

/* Forward declarations */
static inline int pg_pt_alloc(struct uk_pagetable *pt, __vaddr_t *pt_vaddr,
			      __paddr_t *pt_paddr, unsigned int level);

static inline void pg_pt_free(struct uk_pagetable *pt, __vaddr_t pt_vaddr,
			      unsigned int level);

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

#ifdef CONFIG_LIBUKPAGING_STATS
/* Don't update stats */
#define UK_PAGING_PAGE_FLAG_INTERN_STATS_KEEP	0x80000000
#endif /* CONFIG_LIBUKPAGING_STATS */

/* The largest level for which UK_PAL_PAGE_Lx_HAS() returns a non-zero value,
 * that is the largest level which can map a page.
 */
static unsigned int pg_page_largest_level = UK_PAL_PAGE_LEVEL;

/*
 * Pointer to currently active page table.
 * TODO: With SMP support, this should move to a CPU-local variable or we
 * need a way to derive the struct pagetable pointer from the configured
 * HW page table base pointer.
 */
static struct uk_pagetable kernel_pt;
static struct uk_pagetable *pg_active_pt;

struct uk_pagetable *uk_paging_pt_get_active(void)
{
	return pg_active_pt;
}

int uk_paging_pt_set_active(struct uk_pagetable *pt)
{
	int rc;

	rc = uk_pal_pt_write_base(pt->pt_pbase);
	if (rc)
		return rc;

	pg_active_pt = pt;

	return 0;
}

static int pg_pt_clone(struct uk_pagetable *pt_dst, struct uk_pagetable *pt_src,
		       unsigned long flags)
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

	UK_ASSERT(pt_src->pt_vbase != UK_PAL_VADDR_INV);
	UK_ASSERT(pt_src->pt_pbase != UK_PAL_PADDR_INV);

	if (pt_dst != pt_src)
		memset(pt_dst, 0, sizeof(struct uk_pagetable));

	/* Use the same frame allocator for the new page table */
	pt_dst->fa = pt_src->fa;

	pt_vaddr_scache[lvl] = pt_svaddr = pt_src->pt_vbase;

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
		rc = uk_pal_pte_read(pt_svaddr, lvl, pte_idx, &pte);
		if (unlikely(rc))
			goto EXIT_FREE;

		/* This is a page table. Copy it and descent. */
		if (UK_PAL_PT_Lx_PTE_PRESENT(pte, lvl) &&
		    !UK_PAL_PAGE_Lx_IS(pte, lvl)) {
			rc = pg_pt_alloc(pt_dst, &pt_dvaddr, &pt_dpaddr,
					 lvl - 1);
			if (unlikely(rc))
				goto EXIT_FREE;

			pt_svaddr = pgarch_pt_pte_to_vaddr(pt_src, pte, lvl);

			/* Set the PTE in the destination page table to point
			 * to the copy of the lower-level page table
			 */
			pte = uk_pal_pt_pte_create(pt_dpaddr, lvl, pte, lvl);

			rc = uk_pal_pte_write(pt_vaddr_dcache[lvl], lvl,
					      pte_idx, pte);
			if (unlikely(rc)) {
				pg_pt_free(pt_dst, pt_dvaddr, lvl - 1);
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
		if (UK_PAL_PT_Lx_PTE_PRESENT(pte, lvl)) {
			UK_ASSERT(UK_PAL_PAGE_Lx_IS(pte, lvl));
			pt_dst->nr_lx_pages[lvl]++;
		}
#endif /* CONFIG_LIBUKPAGING_STATS */

		/* Copy whatever PTE we have here */
		rc = uk_pal_pte_write(pt_dvaddr, lvl, pte_idx, pte);
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
	pt_dst->pt_vbase = pt_vaddr_dcache[UK_PAL_PT_LEVELS - 1];
	pt_dst->pt_pbase = pt_dpaddr_root;

	return 0;

EXIT_FREE:
	pg_page_unmap(pt_dst, pt_vaddr_dcache[UK_PAL_PT_LEVELS - 1],
		      UK_PAL_PT_LEVELS - 1, UK_PAL_VADDR_INV, __SZ_MAX,
		      UK_PAGING_PAGE_FLAG_KEEP_FRAMES);

	pg_pt_free(pt_dst, pt_vaddr_dcache[UK_PAL_PT_LEVELS - 1],
		   UK_PAL_PT_LEVELS - 1);

	return rc;
}

int uk_paging_pt_init(struct uk_pagetable *pt, __paddr_t start, __sz len)
{
	static __u8 initialized; /* = 0 on startup */
	unsigned int lvl;
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

	rc = pgarch_pt_init(pt, start, len);
	if (unlikely(rc))
		return rc;

	/* Allocate a new top-level page table */
	rc = pg_pt_alloc(pt, &pt->pt_vbase, &pt->pt_pbase,
			 UK_PAL_PT_LEVELS - 1);
	if (unlikely(rc))
		return rc;

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
	rc = pg_page_mapx(pt, pt->pt_vbase, UK_PAL_PT_LEVELS - 1,
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
	rc = pg_pt_clone(pt, pt, 0);
	if (unlikely(rc))
		return rc;
#endif

	return 0;
}

int uk_paging_pt_add_mem(struct uk_pagetable *pt, __paddr_t start, __sz len)
{
	if (len == 0)
		return 0;

	UK_ASSERT(start <= __PADDR_MAX - len);
	UK_ASSERT(uk_pal_paddr_range_isvalid(start, len));

	return pgarch_pt_add_mem(pt, start, len);
}

int uk_paging_pt_clone(struct uk_pagetable *pt, struct uk_pagetable *pt_src,
		       unsigned long flags)
{
	UK_ASSERT(pt != pt_src);

	return pg_pt_clone(pt, pt_src, flags);
}

int uk_paging_pt_free(struct uk_pagetable *pt, unsigned long flags)
{
	int rc;

	UK_ASSERT(pt->pt_vbase != UK_PAL_VADDR_INV);
	UK_ASSERT(pt->pt_pbase != UK_PAL_PADDR_INV);

	rc = pg_page_unmap(pt, pt->pt_vbase, UK_PAL_PT_LEVELS - 1,
			   UK_PAL_VADDR_INV, __SZ_MAX,
			   flags & UK_PAGING_PAGE_FLAG_KEEP_FRAMES);
	if (unlikely(rc))
		return rc;

	/* Also free the top-level page table */
	pg_pt_free(pt, pt->pt_vbase, UK_PAL_PT_LEVELS - 1);

	pt->pt_vbase = UK_PAL_VADDR_INV;
	pt->pt_pbase = UK_PAL_PADDR_INV;

	return 0;
}

static inline int pg_pt_walk(struct uk_pagetable *pt, __vaddr_t *pt_vaddr,
			     __vaddr_t vaddr, unsigned int *level,
			     unsigned int to_level, __pte_t *pte)
{
	unsigned int lvl = *level;
	__pte_t lpte;
	int rc;

	while (lvl > to_level) {
		rc = uk_pal_pte_read(*pt_vaddr, lvl,
				     UK_PAL_PT_Lx_IDX(vaddr, lvl),
				     &lpte);
		if (unlikely(rc))
			goto EXIT;

		if (!UK_PAL_PT_Lx_PTE_PRESENT(lpte, lvl) ||
		    UK_PAL_PAGE_Lx_IS(lpte, lvl))
			goto EXIT;

		*pt_vaddr = pgarch_pt_pte_to_vaddr(pt, lpte, lvl);
		lvl--;
	}

	UK_ASSERT(lvl == to_level);
	rc = uk_pal_pte_read(*pt_vaddr, lvl, UK_PAL_PT_Lx_IDX(vaddr, to_level),
			     &lpte);

EXIT:
	*level = lvl;
	*pte = lpte;

	return rc;
}

int uk_paging_pt_walk(struct uk_pagetable *pt, __vaddr_t vaddr,
		      unsigned int *level, __vaddr_t *pt_vaddr,
		      __pte_t *pte)
{
	unsigned int lvl = UK_PAL_PT_LEVELS - 1;
	unsigned int to_lvl = (level) ? *level : UK_PAL_PAGE_LEVEL;
	__vaddr_t tmp_pt_vaddr = pt->pt_vbase;
	__pte_t tmp_pte;
	int rc;

	UK_ASSERT(pt->pt_vbase != UK_PAL_VADDR_INV);
	UK_ASSERT(pt->pt_pbase != UK_PAL_PADDR_INV);

	UK_ASSERT(uk_pal_vaddr_isvalid(vaddr));
	UK_ASSERT(to_lvl < UK_PAL_PT_LEVELS);

	rc = pg_pt_walk(pt, &tmp_pt_vaddr, vaddr, &lvl, to_lvl, &tmp_pte);

	if (pt_vaddr)
		*pt_vaddr = tmp_pt_vaddr;
	if (level)
		*level = lvl;
	if (pte)
		*pte = tmp_pte;

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

static inline int pg_pt_alloc(struct uk_pagetable *pt, __vaddr_t *pt_vaddr,
			      __paddr_t *pt_paddr, unsigned int level)
{
	__pte_t invalid;
	__paddr_t new_pt_paddr = UK_PAL_PADDR_INV;
	__vaddr_t new_pt_vaddr;
	unsigned int i, rc;

	UK_ASSERT(level < UK_PAL_PT_LEVELS);
	invalid = UK_PAL_PT_Lx_PTE_INVALID(level);

	rc = pg_falloc(pt, &new_pt_paddr, UK_PAL_PAGE_LEVEL);
	if (unlikely(rc))
		return rc;

	new_pt_vaddr = pgarch_pt_map(pt, new_pt_paddr, level);
	if (unlikely(new_pt_vaddr == UK_PAL_VADDR_INV))
		goto EXIT_FREE;

	/* Clear the page table */
	for (i = 0; i < UK_PAL_PT_Lx_PTES(level); ++i) {
		rc = uk_pal_pte_write(new_pt_vaddr, level, i, invalid);
		if (unlikely(rc))
			goto EXIT_FREE;
	}

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
			      unsigned int level)
{
	__paddr_t pt_paddr;

	UK_ASSERT(level < UK_PAL_PT_LEVELS);

	pt_paddr = pgarch_pt_unmap(pt, pt_vaddr, level);
	UK_ASSERT(pt_paddr != UK_PAL_PADDR_INV);

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
			rc = uk_pal_pte_read(pt_vaddr, lvl, pte_idx, &pte);
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

				pt_vaddr = pgarch_pt_pte_to_vaddr(pt, pte, lvl);
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

				pte = uk_pal_pt_pte_create(pt_paddr, lvl, pte, tmpl_level);
				rc = uk_pal_pte_write(pt_vaddr_cache[lvl], lvl,
						      pte_idx, pte);
				if (unlikely(rc)) {
					pg_pt_free(pt, pt_vaddr, lvl - 1);
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
		rc = uk_pal_pte_read(pt_vaddr, lvl, pte_idx, &pte);
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

		rc = uk_pal_pte_write(pt_vaddr, lvl, pte_idx, pte);
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

		if (UK_PAL_PT_Lx_PTE_PRESENT(orig_pte, lvl) &&
		    pt == pg_active_pt)
			uk_pal_tlb_flush_entry(vaddr);

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
	__sz len;

	if (unlikely(pages == 0))
		return 0;

	UK_ASSERT(level < UK_PAL_PT_LEVELS);
	UK_ASSERT(UK_PAL_PAGE_Lx_HAS(level));
	UK_ASSERT(pages <= (__SZ_MAX / UK_PAL_PAGE_Lx_SIZE(level)));

	len = pages * UK_PAL_PAGE_Lx_SIZE(level);

#ifdef CONFIG_LIBUKPAGING_STATS
	UK_ASSERT(!(flags & UK_PAGING_PAGE_FLAG_INTERN_STATS_KEEP));
#endif /* CONFIG_LIBUKPAGING_STATS */

	UK_ASSERT(pt->pt_vbase != UK_PAL_VADDR_INV);
	UK_ASSERT(pt->pt_pbase != UK_PAL_PADDR_INV);

	UK_ASSERT(vaddr <= __VADDR_MAX - len);

	return pg_page_mapx(pt, pt->pt_vbase, UK_PAL_PT_LEVELS - 1, vaddr,
			    paddr, len, attr, flags,
			    UK_PAL_PT_Lx_PTE_INVALID(UK_PAL_PAGE_LEVEL),
			    UK_PAL_PAGE_LEVEL, mapx);
}

static int pg_page_split(struct uk_pagetable *pt, __vaddr_t pt_vaddr,
			 __vaddr_t vaddr, unsigned int level)
{
	unsigned int to_lvl;
	__vaddr_t new_pt_vaddr;
	__paddr_t new_pt_paddr;
	__paddr_t paddr;
	__pte_t pte;
	unsigned long attr;
	unsigned long flags;
	int rc;

	UK_ASSERT(level > UK_PAL_PAGE_LEVEL);
	UK_ASSERT(UK_PAL_PAGE_Lx_HAS(level));
	UK_ASSERT(UK_PAL_PAGE_Lx_ALIGNED(vaddr, level));

	rc = uk_pal_pte_read(pt_vaddr, level, UK_PAL_PT_Lx_IDX(vaddr, level),
			     &pte);
	if (unlikely(rc))
		return rc;

	UK_ASSERT(UK_PAL_PAGE_Lx_IS(pte, level));

	attr = uk_pal_attr_from_pte(pte, level);

	/* Find the next smaller page size */
	to_lvl = pg_largest_level(vaddr, 0, __SZ_MAX, level - 1);
	UK_ASSERT(to_lvl <= level - 1);

	/* Create a page table that will hold all mappings and potential
	 * child tables.
	 */
	rc = pg_pt_alloc(pt, &new_pt_vaddr, &new_pt_paddr, level - 1);
	if (unlikely(rc))
		return rc;

	flags = UK_PAGING_PAGE_FLAG_SIZE(to_lvl) |
		UK_PAGING_PAGE_FLAG_FORCE_SIZE;
#ifdef CONFIG_LIBUKPAGING_STATS
	flags |= UK_PAGING_PAGE_FLAG_INTERN_STATS_KEEP;
#endif /* CONFIG_LIBUKPAGING_STATS */

	/* Create mappings of the next smaller page size that map the same
	 * contiguous range of physical memory than the input page
	 */
	paddr = UK_PAL_PT_Lx_PTE_PADDR(pte, level);

	UK_ASSERT(vaddr <= __VADDR_MAX - UK_PAL_PAGE_Lx_SIZE(level));

	rc = pg_page_mapx(pt, new_pt_vaddr, level - 1, vaddr, paddr,
			  UK_PAL_PAGE_Lx_SIZE(level), attr, flags, pte,
			  level, NULL);
	if (unlikely(rc))
		goto EXIT_FREE;

	/* Update the original PTE to point to the split page */
	pte = uk_pal_pt_pte_create(new_pt_paddr, level - 1, pte, level);

	rc = uk_pal_pte_write(pt_vaddr, level,
			      UK_PAL_PT_Lx_IDX(vaddr, level), pte);
	if (unlikely(rc))
		goto EXIT_FREE;

#ifdef CONFIG_LIBUKPAGING_STATS
	UK_ASSERT(pt->nr_lx_pages[level] > 0);
	pt->nr_lx_pages[level]--;
	pt->nr_lx_pages[to_lvl] += UK_PAL_PAGE_Lx_SIZE(level) /
				   UK_PAL_PAGE_Lx_SIZE(to_lvl);
	pt->nr_lx_splits[level]++;
#endif /* CONFIG_LIBUKPAGING_STATS */

	return 0;

EXIT_FREE:
	flags = UK_PAGING_PAGE_FLAG_KEEP_FRAMES;
#ifdef CONFIG_LIBUKPAGING_STATS
	flags |= UK_PAGING_PAGE_FLAG_INTERN_STATS_KEEP;
#endif /* CONFIG_LIBUKPAGING_STATS */

	pg_page_unmap(pt, new_pt_vaddr, level - 1, UK_PAL_VADDR_INV,
		      __SZ_MAX, flags);

	pg_pt_free(pt, new_pt_vaddr, level - 1);

	return rc;
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
		rc = uk_pal_pte_read(pt_vaddr, lvl, pte_idx, &pte);
		if (unlikely(rc))
			return rc;

		if (UK_PAL_PT_Lx_PTE_PRESENT(pte, lvl)) {

			/* There is a page table at this PTE.
			 * Descent, if allowed.
			 */
			if (!UK_PAL_PAGE_Lx_IS(pte, lvl)) {
				if ((flags & UK_PAGING_PAGE_FLAG_FORCE_SIZE) &&
				    lvl == to_lvl)
					return -EFAULT;

				pt_vaddr = pgarch_pt_pte_to_vaddr(pt, pte, lvl);

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
			new_pte = (flags & UK_PAGING_PAGE_FLAG_KEEP_PTES) ?
					UK_PAL_PT_Lx_PTE_CLEAR_PRESENT(pte, lvl) :
					UK_PAL_PT_Lx_PTE_INVALID(lvl);

			rc = uk_pal_pte_write(pt_vaddr, lvl, pte_idx, new_pte);
			if (unlikely(rc))
				return rc;

			if (vaddr != UK_PAL_VADDR_INV && pt == pg_active_pt)
				uk_pal_tlb_flush_entry(vaddr);

#ifdef CONFIG_LIBUKPAGING_STATS
			if (!(flags & UK_PAGING_PAGE_FLAG_INTERN_STATS_KEEP)) {
				UK_ASSERT(pt->nr_lx_pages[lvl] > 0);
				pt->nr_lx_pages[lvl]--;
			}
#endif /* CONFIG_LIBUKPAGING_STATS */

			if (!(flags & UK_PAGING_PAGE_FLAG_KEEP_FRAMES))
				pg_ffree(pt, UK_PAL_PT_Lx_PTE_PADDR(pte, lvl),
					 lvl);
		}

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
				rc = uk_pal_pte_read(pt_vaddr, plvl, i, &pte);
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
				rc = uk_pal_pte_read(pt_vaddr, plvl, i, &pte);
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
			rc = uk_pal_pte_write(pt_vaddr, lvl, pte_idx,
					      UK_PAL_PT_Lx_PTE_INVALID(lvl));
			if (unlikely(rc))
				return rc;

			if (vaddr != UK_PAL_VADDR_INV && pt == pg_active_pt)
				uk_pal_tlb_flush_entry(vaddr);

			pg_pt_free(pt, pt_vaddr_cache[plvl], plvl);
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

	if (vaddr == UK_PAL_VADDR_INV && pt == pg_active_pt)
		uk_pal_tlb_flush();

	return 0;
}

int uk_paging_page_unmap(struct uk_pagetable *pt, __vaddr_t vaddr,
			 unsigned long pages, unsigned long flags)
{
	unsigned int level = UK_PAGING_PAGE_FLAG_SIZE_TO_LEVEL(flags);
	__sz len = __SZ_MAX;

	if (unlikely(pages == 0))
		return 0;

	if (vaddr != UK_PAL_VADDR_INV) {
		/* Ensure that the length does not overflow */
		UK_ASSERT(level < UK_PAL_PT_LEVELS);
		UK_ASSERT(UK_PAL_PAGE_Lx_HAS(level));
		UK_ASSERT(pages <= (__SZ_MAX / UK_PAL_PAGE_Lx_SIZE(level)));

		len = pages * UK_PAL_PAGE_Lx_SIZE(level);
	}

#ifdef CONFIG_LIBUKPAGING_STATS
	UK_ASSERT(!(flags & UK_PAGING_PAGE_FLAG_INTERN_STATS_KEEP));
#endif /* CONFIG_LIBUKPAGING_STATS */

	UK_ASSERT(pt->pt_vbase != UK_PAL_VADDR_INV);
	UK_ASSERT(pt->pt_pbase != UK_PAL_PADDR_INV);

	return pg_page_unmap(pt, pt->pt_vbase, UK_PAL_PT_LEVELS - 1, vaddr, len,
			     flags);
}

static int pg_page_set_attr(struct uk_pagetable *pt, __vaddr_t pt_vaddr,
			    unsigned int level, __vaddr_t vaddr, __sz len,
			    unsigned long new_attr, unsigned long flags)
{
	unsigned int to_lvl = UK_PAGING_PAGE_FLAG_SIZE_TO_LEVEL(flags);
	unsigned int lvl = level;
	__vaddr_t pt_vaddr_cache[UK_PAL_PT_LEVELS];
	__pte_t pte, new_pte;
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
		rc = uk_pal_pte_read(pt_vaddr, lvl, pte_idx, &pte);
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

				pt_vaddr = pgarch_pt_pte_to_vaddr(pt, pte, lvl);

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
			new_pte = uk_pal_pte_create(UK_PAL_PT_Lx_PTE_PADDR(pte, lvl),
						    new_attr, lvl, pte, lvl);

			rc = uk_pal_pte_write(pt_vaddr, lvl, pte_idx, new_pte);
			if (unlikely(rc))
				return rc;

			if (vaddr != UK_PAL_VADDR_INV && pt == pg_active_pt)
				uk_pal_tlb_flush_entry(vaddr);
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

	if (vaddr == UK_PAL_VADDR_INV && pt == pg_active_pt)
		uk_pal_tlb_flush();

	return 0;
}

int uk_paging_page_set_attr(struct uk_pagetable *pt, __vaddr_t vaddr,
			    unsigned long pages, unsigned long new_attr,
			    unsigned long flags)
{
	unsigned int level = UK_PAGING_PAGE_FLAG_SIZE_TO_LEVEL(flags);
	__sz len = __SZ_MAX;

	if (unlikely(pages == 0))
		return 0;

	if (vaddr != UK_PAL_VADDR_INV) {
		/* Ensure that the length does not overflow */
		UK_ASSERT(level < UK_PAL_PT_LEVELS);
		UK_ASSERT(UK_PAL_PAGE_Lx_HAS(level));
		UK_ASSERT(pages <= (__SZ_MAX / UK_PAL_PAGE_Lx_SIZE(level)));

		len = pages * UK_PAL_PAGE_Lx_SIZE(level);
	}

#ifdef CONFIG_LIBUKPAGING_STATS
	UK_ASSERT(!(flags & UK_PAGING_PAGE_FLAG_INTERN_STATS_KEEP));
#endif /* CONFIG_LIBUKPAGING_STATS */

	UK_ASSERT(pt->pt_vbase != UK_PAL_VADDR_INV);
	UK_ASSERT(pt->pt_pbase != UK_PAL_PADDR_INV);

	return pg_page_set_attr(pt, pt->pt_vbase, UK_PAL_PT_LEVELS - 1, vaddr,
				len, new_attr, flags);
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
