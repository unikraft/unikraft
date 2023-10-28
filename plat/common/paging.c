/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *
 * Copyright (c) 2021, Karlsruhe Institute of Technology (KIT).
 *                     All rights reserved.
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

/* API based on prototype for virtual memory by:
 * Stefan Teodorescu <stefanl.teodorescu@gmail.com>
 * See https://github.com/unikraft/unikraft/pull/247
 */

#include <string.h>
#include <errno.h>

#include <uk/config.h>
#include <uk/arch/limits.h>
#include <uk/arch/lcpu.h>
#include <uk/essentials.h>
#include <uk/assert.h>
#include <uk/print.h>
#include <uk/plat/paging.h>
#include <uk/plat/common/sections.h>
#include <uk/plat/common/bootinfo.h>
#include <uk/falloc.h>

#define __PLAT_CMN_ARCH_PAGING_H__
#if defined CONFIG_ARCH_ARM_64
#include <arm/arm64/paging.h>
#elif defined CONFIG_ARCH_X86_64
#include <x86/paging.h>
#else
#error "Architecture not supported by paging API"
#endif

/* Forward declarations */
static inline int pg_pt_alloc(struct uk_pagetable *pt, __vaddr_t *pt_vaddr,
			      __paddr_t *pt_paddr, unsigned int level);

static inline void pg_pt_free(struct uk_pagetable *pt, __vaddr_t pt_vaddr,
			      unsigned int level);

static int pg_page_split(struct uk_pagetable *pt, __vaddr_t pt_vaddr,
			 __vaddr_t vaddr, unsigned int level);

static int pg_page_unmap(struct uk_pagetable *pt, __vaddr_t pt_vaddr,
			 unsigned int level, __vaddr_t vaddr, __sz len,
			 unsigned long flags);

#ifdef CONFIG_PAGING_STATS
#define PAGE_FLAG_INTERN_STATS_KEEP	0x80000000 /* Don't update stats */
#endif /* CONFIG_PAGING_STATS */

/* The largest level for which PAGE_Lx_HAS() returns a non-zero value, that is
 * the largest level which can map a page.
 */
static unsigned int pg_page_largest_level = PAGE_LEVEL;

/*
 * Pointer to currently active page table.
 * TODO: With SMP support, this should move to a CPU-local variable or we
 * need a way to derive the struct pagetable pointer from the configured
 * HW page table base pointer.
 */
static struct uk_pagetable kernel_pt;
static struct uk_pagetable *pg_active_pt;

struct uk_pagetable *ukplat_pt_get_active(void)
{
	return pg_active_pt;
}

int ukplat_pt_set_active(struct uk_pagetable *pt)
{
	int rc;

	rc = ukarch_pt_write_base(pt->pt_pbase);
	if (rc)
		return rc;

	pg_active_pt = pt;

	return 0;
}

static int pg_pt_clone(struct uk_pagetable *pt_dst, struct uk_pagetable *pt_src,
		       unsigned long flags)
{
	unsigned int lvl = PT_LEVELS - 1;
	__vaddr_t pt_vaddr_scache[PT_LEVELS];
	__vaddr_t pt_vaddr_dcache[PT_LEVELS];
	__vaddr_t pt_svaddr, pt_dvaddr;
	__paddr_t pt_dpaddr, pt_dpaddr_root;
	__pte_t pte;
	unsigned int pte_idx_cache[PT_LEVELS];
	unsigned int pte_idx = 0;
	int rc;

	UK_ASSERT(pt_src->pt_vbase != __VADDR_INV);
	UK_ASSERT(pt_src->pt_pbase != __PADDR_INV);

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
	if (flags & PAGE_FLAG_CLONE_NEW)
		goto EXIT;

	do {
		rc = ukarch_pte_read(pt_svaddr, lvl, pte_idx, &pte);
		if (unlikely(rc))
			goto EXIT_FREE;

		/* This is a page table. Copy it and descent. */
		if (PT_Lx_PTE_PRESENT(pte, lvl) && !PAGE_Lx_IS(pte, lvl)) {
			rc = pg_pt_alloc(pt_dst, &pt_dvaddr, &pt_dpaddr,
					 lvl - 1);
			if (unlikely(rc))
				goto EXIT_FREE;

			pt_svaddr = pgarch_pt_pte_to_vaddr(pt_src, pte, lvl);

			/* Set the PTE in the destination page table to point
			 * to the copy of the lower-level page table
			 */
			pte = pgarch_pt_pte_create(pt_dst, pt_dpaddr, lvl, pte,
						   lvl);

			rc = ukarch_pte_write(pt_vaddr_dcache[lvl], lvl,
					      pte_idx, pte);
			if (unlikely(rc)) {
				pg_pt_free(pt_dst, pt_dvaddr, lvl - 1);
				goto EXIT_FREE;
			}

			pte_idx_cache[lvl] = pte_idx;

			UK_ASSERT(lvl > PAGE_LEVEL);
			lvl--;

			pt_vaddr_scache[lvl] = pt_svaddr;
			pt_vaddr_dcache[lvl] = pt_dvaddr;

			pte_idx = 0;
			continue;
		}

#ifdef CONFIG_PAGING_STATS
		if (PT_Lx_PTE_PRESENT(pte, lvl)) {
			UK_ASSERT(PAGE_Lx_IS(pte, lvl));
			pt_dst->nr_lx_pages[lvl]++;
		}
#endif /* CONFIG_PAGING_STATS */

		/* Copy whatever PTE we have here */
		rc = ukarch_pte_write(pt_dvaddr, lvl, pte_idx, pte);
		if (unlikely(rc))
			goto EXIT_FREE;

		/* At this point we reached the last PTE in this page table and
		 * we have to walk up one level again.
		 */
		if (pte_idx == PT_Lx_PTES(lvl) - 1) {
			do {
				/* If we reached the top level, stop. */
				if (lvl == PT_LEVELS - 1)
					break;

				/* Go up one level */
				pte_idx = pte_idx_cache[++lvl];
				UK_ASSERT(pte_idx < PT_Lx_PTES(lvl));
			} while (pte_idx == PT_Lx_PTES(lvl) - 1);

			pt_svaddr = pt_vaddr_scache[lvl];
			pt_dvaddr = pt_vaddr_dcache[lvl];
		}

		pte_idx++;
	} while (pte_idx < PT_Lx_PTES(lvl));

EXIT:
	UK_ASSERT(lvl == PT_LEVELS - 1);

	/* We successfully created a new page table hierarchy. Now assign it to
	 * the destination page table. We do not free any existing page tables,
	 * but assume that the caller provided us an uninitialized page table or
	 * dst and src are the same.
	 */
	pt_dst->pt_vbase = pt_vaddr_dcache[PT_LEVELS - 1];
	pt_dst->pt_pbase = pt_dpaddr_root;

	return 0;

EXIT_FREE:
	pg_page_unmap(pt_dst, pt_vaddr_dcache[PT_LEVELS - 1], PT_LEVELS - 1,
		      __VADDR_ANY, __SZ_MAX, PAGE_FLAG_KEEP_FRAMES);

	pg_pt_free(pt_dst, pt_vaddr_dcache[PT_LEVELS - 1], PT_LEVELS - 1);

	return rc;
}

int ukplat_pt_init(struct uk_pagetable *pt, __paddr_t start, __sz len)
{
	static __u8 initialized; /* = 0 on startup */
	unsigned int lvl;
	int rc;

	/* If this is the first time we setup a new page table, perform
	 * architecture-dependent initialization of the paging API
	 */
	if (!initialized) {
		rc = pgarch_init();
		if (unlikely(rc))
			return rc;

		/* Find out largest supported level that can map a page */
		for (lvl = PT_LEVELS - 1; lvl > PAGE_LEVEL; lvl--)
			if (PAGE_Lx_HAS(lvl)) {
				pg_page_largest_level = lvl;
				break;
			}

		initialized = 1;
	}

	UK_ASSERT(start <= __PADDR_MAX - len);
	UK_ASSERT(ukarch_paddr_range_isvalid(start, len));

	/* Initialize the frame allocator and any architecture-dependent parts
	 * of the new page table
	 */
	memset(pt, 0, sizeof(struct uk_pagetable));

	rc = pgarch_pt_init(pt, start, len);
	if (unlikely(rc))
		return rc;

	/* We create the new page table from the page table hierarchy that is
	 * currently configured in hardware. To that end we just set the root
	 * address. Note that it is not a problem that the page tables have not
	 * been allocated by the frame allocator. Unmapping pages that do not
	 * stem from the memory allocator silently fails and is ignored.
	 */
	pt->pt_pbase = ukarch_pt_read_base();
	pt->pt_vbase = pgarch_pt_map(pt, pt->pt_pbase, PT_LEVELS - 1);
	if (unlikely(pt->pt_vbase == __VADDR_INV))
		return -ENOMEM;

#ifdef CONFIG_PAGING_STATS
	/* If we have stats active, we need to discover all mappings etc. We
	 * simplify this by just cloning the page table hierarchy.
	 */
	rc = pg_pt_clone(pt, pt, 0);
	if (unlikely(rc))
		return rc;
#endif

	return 0;
}

int ukplat_pt_add_mem(struct uk_pagetable *pt, __paddr_t start, __sz len)
{
	if (len == 0)
		return 0;

	UK_ASSERT(start <= __PADDR_MAX - len);
	UK_ASSERT(ukarch_paddr_range_isvalid(start, len));

	return pgarch_pt_add_mem(pt, start, len);
}

int ukplat_pt_clone(struct uk_pagetable *pt, struct uk_pagetable *pt_src,
		    unsigned long flags)
{
	UK_ASSERT(pt != pt_src);

	return pg_pt_clone(pt, pt_src, flags);
}

int ukplat_pt_free(struct uk_pagetable *pt, unsigned long flags)
{
	int rc;

	UK_ASSERT(pt->pt_vbase != __VADDR_INV);
	UK_ASSERT(pt->pt_pbase != __PADDR_INV);

	rc = pg_page_unmap(pt, pt->pt_vbase, PT_LEVELS - 1, __VADDR_ANY,
			   __SZ_MAX, flags & PAGE_FLAG_KEEP_FRAMES);
	if (unlikely(rc))
		return rc;

	/* Also free the top-level page table */
	pg_pt_free(pt, pt->pt_vbase, PT_LEVELS - 1);

	pt->pt_vbase = __VADDR_INV;
	pt->pt_pbase = __PADDR_INV;

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
		rc = ukarch_pte_read(*pt_vaddr, lvl, PT_Lx_IDX(vaddr, lvl),
				     &lpte);
		if (unlikely(rc))
			goto EXIT;

		if (!PT_Lx_PTE_PRESENT(lpte, lvl) || PAGE_Lx_IS(lpte, lvl))
			goto EXIT;

		*pt_vaddr = pgarch_pt_pte_to_vaddr(pt, lpte, lvl);
		lvl--;
	}

	UK_ASSERT(lvl == to_level);
	rc = ukarch_pte_read(*pt_vaddr, lvl, PT_Lx_IDX(vaddr, to_level), &lpte);

EXIT:
	*level = lvl;
	*pte = lpte;

	return rc;
}

int ukplat_pt_walk(struct uk_pagetable *pt, __vaddr_t vaddr,
		   unsigned int *level, __vaddr_t *pt_vaddr, __pte_t *pte)
{
	unsigned int lvl = PT_LEVELS - 1;
	unsigned int to_lvl = (level) ? *level : PAGE_LEVEL;
	__vaddr_t tmp_pt_vaddr = pt->pt_vbase;
	__pte_t tmp_pte;
	int rc;

	UK_ASSERT(pt->pt_vbase != __VADDR_INV);
	UK_ASSERT(pt->pt_pbase != __PADDR_INV);

	UK_ASSERT(ukarch_vaddr_isvalid(vaddr));
	UK_ASSERT(to_lvl < PT_LEVELS);

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
	(1UL << (PAGE_Lx_SHIFT(lvl) - PAGE_Lx_SHIFT(0)))

static inline int pg_falloc(struct uk_pagetable *pt, __paddr_t *paddr,
			    unsigned int level)
{
	unsigned long pages = PG_Lx_L0_PAGES(level);

	UK_ASSERT(level < PT_LEVELS);
	UK_ASSERT(pt->fa->falloc);

	return pt->fa->falloc(pt->fa, paddr, pages, FALLOC_FLAG_ALIGNED);
}

static inline void pg_ffree(struct uk_pagetable *pt, __paddr_t paddr,
			    unsigned int level)
{
	unsigned long pages = PG_Lx_L0_PAGES(level);
	int rc __maybe_unused;

	UK_ASSERT(level < PT_LEVELS);

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
	__paddr_t new_pt_paddr = __PADDR_ANY;
	__vaddr_t new_pt_vaddr;
	unsigned int i, rc;

	UK_ASSERT(level < PT_LEVELS);
	invalid = PT_Lx_PTE_INVALID(level);

	rc = pg_falloc(pt, &new_pt_paddr, PAGE_LEVEL);
	if (unlikely(rc))
		return rc;

	new_pt_vaddr = pgarch_pt_map(pt, new_pt_paddr, level);
	if (unlikely(new_pt_vaddr == __VADDR_INV))
		goto EXIT_FREE;

	/* Clear the page table */
	for (i = 0; i < PT_Lx_PTES(level); ++i) {
		rc = ukarch_pte_write(new_pt_vaddr, level, i, invalid);
		if (unlikely(rc))
			goto EXIT_FREE;
	}

#ifdef CONFIG_PAGING_STATS
	pt->nr_pt_pages[level]++;
#endif /* CONFIG_PAGING_STATS */

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

	UK_ASSERT(level < PT_LEVELS);

	pt_paddr = pgarch_pt_unmap(pt, pt_vaddr, level);
	UK_ASSERT(pt_paddr != __PADDR_INV);

	pg_ffree(pt, pt_paddr, PAGE_LEVEL);

#ifdef CONFIG_PAGING_STATS
	UK_ASSERT(pt->nr_pt_pages[level] > 0);
	pt->nr_pt_pages[level]--;
#endif /* CONFIG_PAGING_STATS */
}

static inline int pg_largest_level(__vaddr_t vaddr, __paddr_t paddr, __sz len,
				   unsigned int max_lvl)
{
	unsigned int lvl = max_lvl;

	while (lvl > PAGE_LEVEL) {
		if (PAGE_Lx_HAS(lvl) &&
		    PAGE_Lx_ALIGNED(vaddr, lvl) &&
		    PAGE_Lx_ALIGNED(paddr, lvl) &&
		    PAGE_Lx_SIZE(lvl) <= len)
			return lvl;

		lvl--;
	}

	return lvl;
}

static int pg_page_mapx(struct uk_pagetable *pt, __vaddr_t pt_vaddr,
			unsigned int level, __vaddr_t vaddr, __paddr_t paddr,
			__sz len, unsigned long attr, unsigned long flags,
			__pte_t template, unsigned int template_level,
			struct ukplat_page_mapx *mapx)
{
	unsigned int to_lvl = PAGE_FLAG_SIZE_TO_LEVEL(flags);
	unsigned int max_lvl = pg_page_largest_level;
	unsigned int lvl = level;
	unsigned int tmp_lvl;
	__vaddr_t pt_vaddr_cache[PT_LEVELS];
	__paddr_t pt_paddr;
	__pte_t pte, orig_pte;
	__sz page_size;
	unsigned int pte_idx;
	int rc, alloc_pmem;

	UK_ASSERT(len > 0);
	UK_ASSERT(PAGE_Lx_ALIGNED(len, to_lvl));
	UK_ASSERT(PAGE_Lx_ALIGNED(vaddr, to_lvl));
	UK_ASSERT(vaddr <= __VADDR_MAX - len);
	UK_ASSERT(ukarch_vaddr_range_isvalid(vaddr, len));

	if (paddr != __PADDR_ANY) {
		UK_ASSERT(PAGE_Lx_ALIGNED(paddr, to_lvl));
		UK_ASSERT(paddr <= __PADDR_MAX - len);
		UK_ASSERT(ukarch_paddr_range_isvalid(paddr, len));

		alloc_pmem = 0;
	} else
		alloc_pmem = 1;

	if (!(flags & PAGE_FLAG_FORCE_SIZE)) {
		if (level < max_lvl)
			max_lvl = level;

		to_lvl = pg_largest_level(vaddr, paddr, len, max_lvl);
	}

	UK_ASSERT(lvl >= to_lvl);
	pt_vaddr_cache[lvl] = pt_vaddr;

	pte_idx = PT_Lx_IDX(vaddr, lvl);
	page_size = PAGE_Lx_SIZE(lvl);
	do {
		/* This loop is responsible for walking the page table down
		 * until we reach the desired level. If there is a page table
		 * missing on the way it is allocated and linked.
		 */
		while (lvl > to_lvl) {
			/* We are too high and need to walk further down */
			rc = ukarch_pte_read(pt_vaddr, lvl, pte_idx, &pte);
			if (unlikely(rc))
				return rc;

			if (PT_Lx_PTE_PRESENT(pte, lvl)) {
				/* If there is already a larger page mapped
				 * at this address and we have a mapx, we
				 * split the page until we reach the target
				 * level and let the mapx decide what to do.
				 * Otherwise, we bail out.
				 */
				if (PAGE_Lx_IS(pte, lvl)) {
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

				if (!(flags & PAGE_FLAG_KEEP_PTES))
					pte = template;
				else
					template_level = lvl;

				pte = pgarch_pt_pte_create(pt, pt_paddr, lvl,
							   pte, template_level);
				rc = ukarch_pte_write(pt_vaddr_cache[lvl], lvl,
						      pte_idx, pte);
				if (unlikely(rc)) {
					pg_pt_free(pt, pt_vaddr, lvl - 1);
					return rc;
				}
			}

			UK_ASSERT(lvl > PAGE_LEVEL);
			lvl--;

			pt_vaddr_cache[lvl] = pt_vaddr;

			pte_idx = PT_Lx_IDX(vaddr, lvl);
			page_size = PAGE_Lx_SIZE(lvl);
		}

		UK_ASSERT(lvl == to_lvl);
		UK_ASSERT(PAGE_Lx_HAS(lvl));

		/* At this point, we are at the target level and know that
		 * pages can be mapped at this level.
		 */
		rc = ukarch_pte_read(pt_vaddr, lvl, pte_idx, &pte);
		if (unlikely(rc))
			return rc;

		orig_pte = pte;

		if (PT_Lx_PTE_PRESENT(pte, lvl)) {
			/* It could be that there is a page table linked at
			 * this PTE. In this case, we descent further down to
			 * the next level that allows to map pages.
			 */
			if (!PAGE_Lx_IS(pte, lvl) &&
			   (!(flags & PAGE_FLAG_FORCE_SIZE))) {
				UK_ASSERT(lvl > PAGE_LEVEL);
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

			paddr = PT_Lx_PTE_PADDR(pte, lvl);
		} else if (alloc_pmem) {
			UK_ASSERT(!PT_Lx_PTE_PRESENT(pte, lvl));

			paddr = __PADDR_ANY;
			rc = pg_falloc(pt, &paddr, lvl);
			if (unlikely(rc)) {
TOO_BIG:
				/* We could not allocate a contiguous,
				 * self-aligned block of physical memory with
				 * the requested size. If we should map largest
				 * possible size, we reduce page size.
				 */
				if ((flags & PAGE_FLAG_FORCE_SIZE) ||
				    (lvl == PAGE_LEVEL))
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

		UK_ASSERT(PAGE_Lx_ALIGNED(vaddr, lvl));
		UK_ASSERT(PAGE_Lx_ALIGNED(paddr, lvl));

		if (!(flags & PAGE_FLAG_KEEP_PTES))
			pte = template;
		else
			template_level = lvl;

		pte = pgarch_pte_create(paddr, attr, lvl, pte, template_level);

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
				    !PT_Lx_PTE_PRESENT(orig_pte, lvl))
					pg_ffree(pt, paddr, lvl);

				if (rc == UKPLAT_PAGE_MAPX_ESKIP)
					goto NEXT_PTE;

				if (rc == UKPLAT_PAGE_MAPX_ETOOBIG) {
					rc = -ENOMEM;
					goto TOO_BIG;
				}

				UK_ASSERT(rc < 0);
				return rc;
			}
		}

		UK_ASSERT(PAGE_Lx_ALIGNED(PT_Lx_PTE_PADDR(pte, lvl), lvl));

		rc = ukarch_pte_write(pt_vaddr, lvl, pte_idx, pte);
		if (unlikely(rc)) {
			if (alloc_pmem &&
			    !PT_Lx_PTE_PRESENT(orig_pte, lvl))
				pg_ffree(pt, paddr, lvl);

			return rc;
		}

#ifdef CONFIG_PAGING_STATS
		if (!(flags & PAGE_FLAG_INTERN_STATS_KEEP))
			pt->nr_lx_pages[lvl]++;
#endif /* CONFIG_PAGING_STATS */

		if (PT_Lx_PTE_PRESENT(orig_pte, lvl) && pt == pg_active_pt)
			ukarch_tlb_flush_entry(vaddr);

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
		if (pte_idx == PT_Lx_PTES(lvl) - 1) {
			do {
				UK_ASSERT(lvl <= level);

				/* Go up one level */
				pte_idx = PT_Lx_IDX(vaddr, ++lvl);
				UK_ASSERT(pte_idx < PT_Lx_PTES(lvl));
			} while (pte_idx == PT_Lx_PTES(lvl) - 1);

			pt_vaddr = pt_vaddr_cache[lvl];

			vaddr += page_size;
			paddr += page_size;

			/* When we reach the last PTE in a page table, this is
			 * always a point where the vaddr will be aligned to a
			 * larger page size, if existing. So re-evaluate the
			 * target level.
			 */
			if (!(flags & PAGE_FLAG_FORCE_SIZE)) {
				/* The new target level must not be larger than
				 * the updated current level as we know there
				 * are page tables down to that level anyways.
				 */
				tmp_lvl = (max_lvl > lvl) ? lvl : max_lvl;
				if (alloc_pmem)
					paddr = __PADDR_ANY;

				to_lvl = pg_largest_level(vaddr, paddr, len,
							  tmp_lvl);
				UK_ASSERT(to_lvl <= lvl);
			}

			page_size = PAGE_Lx_SIZE(lvl);
		} else {
			vaddr += page_size;
			paddr += page_size;

			if (len < page_size) {
				UK_ASSERT(!(flags & PAGE_FLAG_FORCE_SIZE));

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
		UK_ASSERT(pte_idx < PT_Lx_PTES(lvl));

	} while (1);

	return 0;
}

int ukplat_page_mapx(struct uk_pagetable *pt, __vaddr_t vaddr,
		     __paddr_t paddr, unsigned long pages,
		     unsigned long attr, unsigned long flags,
		     struct ukplat_page_mapx *mapx)
{
	unsigned int level = PAGE_FLAG_SIZE_TO_LEVEL(flags);
	__sz len;

	if (unlikely(pages == 0))
		return 0;

	UK_ASSERT(level < PT_LEVELS);
	UK_ASSERT(PAGE_Lx_HAS(level));
	UK_ASSERT(pages <= (__SZ_MAX / PAGE_Lx_SIZE(level)));

	len = pages * PAGE_Lx_SIZE(level);

#ifdef CONFIG_PAGING_STATS
	UK_ASSERT(!(flags & PAGE_FLAG_INTERN_STATS_KEEP));
#endif /* CONFIG_PAGING_STATS */

	UK_ASSERT(pt->pt_vbase != __VADDR_INV);
	UK_ASSERT(pt->pt_pbase != __PADDR_INV);

	return pg_page_mapx(pt, pt->pt_vbase, PT_LEVELS - 1, vaddr, paddr, len,
			    attr, flags, PT_Lx_PTE_INVALID(PAGE_LEVEL),
			    PAGE_LEVEL, mapx);
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

	UK_ASSERT(level > PAGE_LEVEL);
	UK_ASSERT(PAGE_Lx_HAS(level));
	UK_ASSERT(PAGE_Lx_ALIGNED(vaddr, level));

	rc = ukarch_pte_read(pt_vaddr, level, PT_Lx_IDX(vaddr, level), &pte);
	if (unlikely(rc))
		return rc;

	UK_ASSERT(PAGE_Lx_IS(pte, level));

	attr = pgarch_attr_from_pte(pte, level);

	/* Find the next smaller page size */
	to_lvl = pg_largest_level(vaddr, 0, __SZ_MAX, level - 1);
	UK_ASSERT(to_lvl <= level - 1);

	/* Create a page table that will hold all mappings and potential
	 * child tables.
	 */
	rc = pg_pt_alloc(pt, &new_pt_vaddr, &new_pt_paddr, level - 1);
	if (unlikely(rc))
		return rc;

	flags = PAGE_FLAG_SIZE(to_lvl) | PAGE_FLAG_FORCE_SIZE;
#ifdef CONFIG_PAGING_STATS
	flags |= PAGE_FLAG_INTERN_STATS_KEEP;
#endif /* CONFIG_PAGING_STATS */

	/* Create mappings of the next smaller page size that map the same
	 * contiguous range of physical memory than the input page
	 */
	paddr = PT_Lx_PTE_PADDR(pte, level);
	rc = pg_page_mapx(pt, new_pt_vaddr, level - 1, vaddr, paddr,
			  PAGE_Lx_SIZE(level), attr, flags, pte, level, NULL);
	if (unlikely(rc))
		goto EXIT_FREE;

	/* Update the original PTE to point to the split page */
	pte = pgarch_pt_pte_create(pt, new_pt_paddr, level - 1, pte, level);

	rc = ukarch_pte_write(pt_vaddr, level, PT_Lx_IDX(vaddr, level), pte);
	if (unlikely(rc))
		goto EXIT_FREE;

#ifdef CONFIG_PAGING_STATS
	UK_ASSERT(pt->nr_lx_pages[level] > 0);
	pt->nr_lx_pages[level]--;
	pt->nr_lx_pages[to_lvl] += PAGE_Lx_SIZE(level) / PAGE_Lx_SIZE(to_lvl);
	pt->nr_lx_splits[level]++;
#endif /* CONFIG_PAGING_STATS */

	return 0;

EXIT_FREE:
	flags = PAGE_FLAG_KEEP_FRAMES;
#ifdef CONFIG_PAGING_STATS
	flags |= PAGE_FLAG_INTERN_STATS_KEEP;
#endif /* CONFIG_PAGING_STATS */

	pg_page_unmap(pt, new_pt_vaddr, level - 1, __VADDR_ANY,
		      __SZ_MAX, flags);

	pg_pt_free(pt, new_pt_vaddr, level - 1);

	return rc;
}

static int pg_page_unmap(struct uk_pagetable *pt, __vaddr_t pt_vaddr,
			 unsigned int level, __vaddr_t vaddr, __sz len,
			 unsigned long flags)
{
	unsigned int to_lvl = PAGE_FLAG_SIZE_TO_LEVEL(flags);
	unsigned int plvl, lvl = level;
	__vaddr_t pt_vaddr_cache[PT_LEVELS];
	__pte_t pte, new_pte;
	__sz page_size;
	unsigned int pte_idx_cache[PT_LEVELS];
	unsigned int first_pte_idx[PT_LEVELS];
	unsigned int pte_idx, i;
	int rc, skip_pt_free;

	UK_ASSERT(lvl >= to_lvl);
	pt_vaddr_cache[lvl] = pt_vaddr;

	if (vaddr == __VADDR_ANY) {
		UK_ASSERT(len == __SZ_MAX);

		pte_idx = 0;
		page_size = 0;
	} else {
		UK_ASSERT(len > 0);
		UK_ASSERT(PAGE_Lx_ALIGNED(len, to_lvl));
		UK_ASSERT(PAGE_Lx_ALIGNED(vaddr, to_lvl));
		UK_ASSERT(vaddr <= __VADDR_MAX - len);
		UK_ASSERT(ukarch_vaddr_range_isvalid(vaddr, len));

		pte_idx = PT_Lx_IDX(vaddr, lvl);
		page_size = PAGE_Lx_SIZE(lvl);
	}

	first_pte_idx[lvl] = pte_idx;
	skip_pt_free = (flags & PAGE_FLAG_KEEP_PTES);

	do {
		rc = ukarch_pte_read(pt_vaddr, lvl, pte_idx, &pte);
		if (unlikely(rc))
			return rc;

		if (PT_Lx_PTE_PRESENT(pte, lvl)) {

			/* There is a page table at this PTE.
			 * Descent, if allowed.
			 */
			if (!PAGE_Lx_IS(pte, lvl)) {
				if ((flags & PAGE_FLAG_FORCE_SIZE) &&
				    (lvl == to_lvl))
					return -EFAULT;

				pt_vaddr = pgarch_pt_pte_to_vaddr(pt, pte, lvl);

				pte_idx_cache[lvl] = pte_idx;

				UK_ASSERT(lvl > PAGE_LEVEL);
				lvl--;

				pt_vaddr_cache[lvl] = pt_vaddr;

				if (vaddr == __VADDR_ANY) {
					pte_idx = 0;
					UK_ASSERT(page_size == 0);
				} else {
					pte_idx = PT_Lx_IDX(vaddr, lvl);
					page_size = PAGE_Lx_SIZE(lvl);
				}

				first_pte_idx[lvl] = pte_idx;
				skip_pt_free = (flags & PAGE_FLAG_KEEP_PTES);

				continue;
			}

			UK_ASSERT(PAGE_Lx_IS(pte, lvl));

			/* At this point, we know that there is a page mapped
			 * here. If we do not enforce a certain page size we
			 * might have to split the page (i.e., it is larger
			 * than the remaining len to unmap, or it is not
			 * aligned to the current vaddr).
			 */
			if ((flags & PAGE_FLAG_FORCE_SIZE) && (lvl != to_lvl))
				return -EFAULT;

			if ((page_size > len) ||
			    (!PAGE_Lx_ALIGNED(vaddr, lvl))) {
				UK_ASSERT(lvl > PAGE_LEVEL);

				rc = pg_page_split(pt, pt_vaddr,
					PAGE_Lx_ALIGN_DOWN(vaddr, lvl), lvl);
				if (unlikely(rc))
					return rc;

				continue;
			}

			UK_ASSERT(len >= page_size);
			UK_ASSERT(PAGE_Lx_ALIGNED(vaddr, lvl));

			/* At this point, we know that we can safely invalidate
			 * the current PTE as it is a page with a size that
			 * is below the remaining len to unmap and the address
			 * we want to unmap is aligned to the page size.
			 */
			new_pte = (flags & PAGE_FLAG_KEEP_PTES) ?
					PT_Lx_PTE_CLEAR_PRESENT(pte, lvl) :
					PT_Lx_PTE_INVALID(lvl);

			rc = ukarch_pte_write(pt_vaddr, lvl, pte_idx, new_pte);
			if (unlikely(rc))
				return rc;

			if (vaddr != __VADDR_ANY && pt == pg_active_pt)
				ukarch_tlb_flush_entry(vaddr);

#ifdef CONFIG_PAGING_STATS
			if (!(flags & PAGE_FLAG_INTERN_STATS_KEEP)) {
				UK_ASSERT(pt->nr_lx_pages[lvl] > 0);
				pt->nr_lx_pages[lvl]--;
			}
#endif /* CONFIG_PAGING_STATS */

			if (!(flags & PAGE_FLAG_KEEP_FRAMES))
				pg_ffree(pt, PT_Lx_PTE_PADDR(pte, lvl), lvl);
		}

		/* If this is not the last PTE and there are still pages to
		 * unmap, we continue to the next PTE in this level
		 */
		if ((pte_idx < PT_Lx_PTES(lvl) - 1) && (len > page_size)) {
			UK_ASSERT(vaddr <= __VADDR_MAX - page_size);
			vaddr += page_size;
			len -= page_size;
			pte_idx++;

			continue;
		}

		UK_ASSERT((pte_idx == PT_Lx_PTES(lvl) - 1) ||
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
		while ((pte_idx == PT_Lx_PTES(lvl) - 1) ||
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
			UK_ASSERT(pte_idx < PT_Lx_PTES(lvl));

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
			while (i++ < PT_Lx_PTES(plvl) - 1) {
				rc = ukarch_pte_read(pt_vaddr, plvl, i, &pte);
				if (unlikely(rc))
					return rc;

				if (pte != PT_Lx_PTE_INVALID(plvl)) {
					skip_pt_free = 1;
					break;
				}
			}

			if (skip_pt_free)
				continue;

			i = first_pte_idx[plvl];
			do {
				rc = ukarch_pte_read(pt_vaddr, plvl, i, &pte);
				if (unlikely(rc))
					return rc;

				if (pte != PT_Lx_PTE_INVALID(plvl)) {
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
			rc = ukarch_pte_write(pt_vaddr, lvl, pte_idx,
					      PT_Lx_PTE_INVALID(lvl));
			if (unlikely(rc))
				return rc;

			if (vaddr != __VADDR_ANY && pt == pg_active_pt)
				ukarch_tlb_flush_entry(vaddr);

			pg_pt_free(pt, pt_vaddr_cache[plvl], plvl);
		}

		if (len <= page_size)
			break;

		pt_vaddr = pt_vaddr_cache[lvl];

		UK_ASSERT(vaddr <= __VADDR_MAX - page_size);
		vaddr += page_size;
		len -= page_size;

		if (vaddr != __VADDR_ANY)
			page_size = PAGE_Lx_SIZE(lvl);

		/* Do not move this into the while condition. We use continue */
		if (++pte_idx == PT_Lx_PTES(lvl))
			break;

	} while (1);

	if (vaddr == __VADDR_ANY && pt == pg_active_pt)
		ukarch_tlb_flush();

	return 0;
}

int ukplat_page_unmap(struct uk_pagetable *pt, __vaddr_t vaddr,
		      unsigned long pages, unsigned long flags)
{
	unsigned int level = PAGE_FLAG_SIZE_TO_LEVEL(flags);
	__sz len = __SZ_MAX;

	if (unlikely(pages == 0))
		return 0;

	if (vaddr != __VADDR_ANY) {
		/* Ensure that the length does not overflow */
		UK_ASSERT(level < PT_LEVELS);
		UK_ASSERT(PAGE_Lx_HAS(level));
		UK_ASSERT(pages <= (__SZ_MAX / PAGE_Lx_SIZE(level)));

		len = pages * PAGE_Lx_SIZE(level);
	}

#ifdef CONFIG_PAGING_STATS
	UK_ASSERT(!(flags & PAGE_FLAG_INTERN_STATS_KEEP));
#endif /* CONFIG_PAGING_STATS */

	UK_ASSERT(pt->pt_vbase != __VADDR_INV);
	UK_ASSERT(pt->pt_pbase != __PADDR_INV);

	return pg_page_unmap(pt, pt->pt_vbase, PT_LEVELS - 1, vaddr, len,
			     flags);
}

static int pg_page_set_attr(struct uk_pagetable *pt, __vaddr_t pt_vaddr,
			    unsigned int level, __vaddr_t vaddr, __sz len,
			    unsigned long new_attr, unsigned long flags)
{
	unsigned int to_lvl = PAGE_FLAG_SIZE_TO_LEVEL(flags);
	unsigned int lvl = level;
	__vaddr_t pt_vaddr_cache[PT_LEVELS];
	__pte_t pte, new_pte;
	__sz page_size;
	unsigned int pte_idx_cache[PT_LEVELS];
	unsigned int pte_idx;
	int rc;

	UK_ASSERT(lvl >= to_lvl);
	pt_vaddr_cache[lvl] = pt_vaddr;

	if (vaddr == __VADDR_ANY) {
		UK_ASSERT(len == __SZ_MAX);

		pte_idx = 0;
		page_size = 0;
	} else {
		UK_ASSERT(len > 0);
		UK_ASSERT(PAGE_Lx_ALIGNED(len, to_lvl));
		UK_ASSERT(PAGE_Lx_ALIGNED(vaddr, to_lvl));
		UK_ASSERT(vaddr <= __VADDR_MAX - len);
		UK_ASSERT(ukarch_vaddr_range_isvalid(vaddr, len));

		pte_idx = PT_Lx_IDX(vaddr, lvl);
		page_size = PAGE_Lx_SIZE(lvl);
	}

	do {
		rc = ukarch_pte_read(pt_vaddr, lvl, pte_idx, &pte);
		if (unlikely(rc))
			return rc;

		if (PT_Lx_PTE_PRESENT(pte, lvl)) {
			/* Check if there is a page table at this PTE. In that
			 * case descent, if allowed.
			 */
			if (!PAGE_Lx_IS(pte, lvl)) {
				if ((flags & PAGE_FLAG_FORCE_SIZE) &&
				    (lvl == to_lvl))
					return -EFAULT;

				pt_vaddr = pgarch_pt_pte_to_vaddr(pt, pte, lvl);

				pte_idx_cache[lvl] = pte_idx;

				UK_ASSERT(lvl > PAGE_LEVEL);
				lvl--;

				pt_vaddr_cache[lvl] = pt_vaddr;

				if (vaddr == __VADDR_ANY) {
					pte_idx = 0;
					UK_ASSERT(page_size == 0);
				} else {
					pte_idx = PT_Lx_IDX(vaddr, lvl);
					page_size = PAGE_Lx_SIZE(lvl);
				}

				continue;
			}

			UK_ASSERT(PAGE_Lx_IS(pte, lvl));

			/* At this point, we know that there is a page mapped
			 * here. If we do not enforce a certain page size we
			 * might have to split the page (i.e., it is larger
			 * than the remaining len to change, or it is not
			 * aligned to the current vaddr).
			 */
			if ((flags & PAGE_FLAG_FORCE_SIZE) && (lvl != to_lvl))
				return -EFAULT;

			if ((page_size > len) ||
			    (!PAGE_Lx_ALIGNED(vaddr, lvl))) {
				UK_ASSERT(lvl > PAGE_LEVEL);

				rc = pg_page_split(pt, pt_vaddr,
					PAGE_Lx_ALIGN_DOWN(vaddr, lvl), lvl);
				if (unlikely(rc))
					return rc;

				continue;
			}

			UK_ASSERT(PAGE_Lx_ALIGNED(vaddr, lvl));

			/* At this point, we know that we can safely change the
			 * current PTE as it is a page with a size that is
			 * below the remaining len to change and the address we
			 * want to change is aligned to the page size.
			 */
			new_pte = pgarch_pte_create(PT_Lx_PTE_PADDR(pte, lvl),
						    new_attr, lvl, pte, lvl);

			rc = ukarch_pte_write(pt_vaddr, lvl, pte_idx, new_pte);
			if (unlikely(rc))
				return rc;

			if (vaddr != __VADDR_ANY && pt == pg_active_pt)
				ukarch_tlb_flush_entry(vaddr);
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
		if (pte_idx == PT_Lx_PTES(lvl) - 1) {
			do {
				/* If we reached the top level, stop. */
				if (lvl == level)
					break;

				/* Go up one level */
				pte_idx = pte_idx_cache[++lvl];
				UK_ASSERT(pte_idx < PT_Lx_PTES(lvl));
			} while (pte_idx == PT_Lx_PTES(lvl) - 1);

			pt_vaddr = pt_vaddr_cache[lvl];

			if (vaddr != __VADDR_ANY)
				page_size = PAGE_Lx_SIZE(lvl);
		}

		/* Do not move this into the while condition. We use continue */
		if (++pte_idx == PT_Lx_PTES(lvl))
			break;

	} while (1);

	if (vaddr == __VADDR_ANY && pt == pg_active_pt)
		ukarch_tlb_flush();

	return 0;
}

int ukplat_page_set_attr(struct uk_pagetable *pt, __vaddr_t vaddr,
			 unsigned long pages, unsigned long new_attr,
			 unsigned long flags)
{
	unsigned int level = PAGE_FLAG_SIZE_TO_LEVEL(flags);
	__sz len = __SZ_MAX;

	if (unlikely(pages == 0))
		return 0;

	if (vaddr != __VADDR_ANY) {
		/* Ensure that the length does not overflow */
		UK_ASSERT(level < PT_LEVELS);
		UK_ASSERT(PAGE_Lx_HAS(level));
		UK_ASSERT(pages <= (__SZ_MAX / PAGE_Lx_SIZE(level)));

		len = pages * PAGE_Lx_SIZE(level);
	}

#ifdef CONFIG_PAGING_STATS
	UK_ASSERT(!(flags & PAGE_FLAG_INTERN_STATS_KEEP));
#endif /* CONFIG_PAGING_STATS */

	UK_ASSERT(pt->pt_vbase != __VADDR_INV);
	UK_ASSERT(pt->pt_pbase != __PADDR_INV);

	return pg_page_set_attr(pt, pt->pt_vbase, PT_LEVELS - 1, vaddr, len,
				new_attr, flags);
}

__vaddr_t ukplat_page_kmap(struct uk_pagetable *pt, __paddr_t paddr,
			   unsigned long pages, unsigned long flags)
{
	unsigned int level = PAGE_FLAG_SIZE_TO_LEVEL(flags);
	__sz len = __SZ_MAX;

	UK_ASSERT(pages > 0);
	UK_ASSERT(level < PT_LEVELS);
	UK_ASSERT(PAGE_Lx_HAS(level));
	UK_ASSERT(pages <= (__SZ_MAX / PAGE_Lx_SIZE(level)));

	len = pages * PAGE_Lx_SIZE(level);

	return pgarch_kmap(pt, paddr, len);
}

void ukplat_page_kunmap(struct uk_pagetable *pt, __vaddr_t vaddr,
			unsigned long pages, unsigned long flags)
{
	unsigned int level = PAGE_FLAG_SIZE_TO_LEVEL(flags);
	__sz len = __SZ_MAX;

	UK_ASSERT(pages > 0);
	UK_ASSERT(level < PT_LEVELS);
	UK_ASSERT(PAGE_Lx_HAS(level));
	UK_ASSERT(pages <= (__SZ_MAX / PAGE_Lx_SIZE(level)));

	len = pages * PAGE_Lx_SIZE(level);

	pgarch_kunmap(pt, vaddr, len);
}

static inline unsigned long bootinfo_to_page_attr(__u16 flags)
{
	unsigned long prot = 0;

	if (flags & UKPLAT_MEMRF_READ)
		prot |= PAGE_ATTR_PROT_READ;
	if (flags & UKPLAT_MEMRF_WRITE)
		prot |= PAGE_ATTR_PROT_WRITE;
	if (flags & UKPLAT_MEMRF_EXECUTE)
		prot |= PAGE_ATTR_PROT_EXEC;

	return prot;
}

extern struct ukplat_memregion_desc bpt_unmap_mrd;

int ukplat_paging_init(void)
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
		UK_ASSERT(mrd->vbase == mrd->pbase);
		UK_ASSERT(!(mrd->pbase & ~PAGE_MASK));
		UK_ASSERT(mrd->len);

		/* Not mapped */
		mrd->vbase = __U64_MAX;
		mrd->flags &= ~UKPLAT_MEMRF_PERMS;

		if (!kernel_pt.fa) {
			rc = ukplat_pt_init(&kernel_pt, mrd->pbase,
					    mrd->pg_count * PAGE_SIZE);
			if (unlikely(rc))
				kernel_pt.fa = NULL;
		} else {
			rc = ukplat_pt_add_mem(&kernel_pt, mrd->pbase,
					       mrd->pg_count * PAGE_SIZE);
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
	UK_ASSERT(!(kernel_pt.fa->free_memory & ~PAGE_MASK));

	if (unlikely(!kernel_pt.fa))
		return rc;

	/* Perform unmappings */
	ukplat_memregion_foreach(&mrd, 0, UKPLAT_MEMRF_UNMAP,
				 UKPLAT_MEMRF_UNMAP) {
		/* Ensure unmap memory region descriptors' correctness */
		/* Must be non-empty and aligned end-to-end */
		UK_ASSERT(mrd->len);
		UK_ASSERT(mrd->pg_count * PAGE_SIZE == mrd->len);
		UK_ASSERT(PAGE_ALIGNED(mrd->vbase));
		UK_ASSERT(!mrd->pg_off);
		/* Physical base address must be 0 */
		UK_ASSERT(!mrd->pbase);
		/* Virtual base address must be a valid value */
		UK_ASSERT(mrd->vbase != __U64_MAX);

		rc = ukplat_page_unmap(&kernel_pt, mrd->vbase,
				       mrd->pg_count,
				       PAGE_FLAG_KEEP_FRAMES);
		if (unlikely(rc))
			return rc;
	}

	/* Perform mappings */
	ukplat_memregion_foreach(&mrd, 0, UKPLAT_MEMRF_MAP,
				 UKPLAT_MEMRF_MAP) {
		UK_ASSERT(!(mrd->vbase & ~PAGE_MASK));
		UK_ASSERT(mrd->vbase != __U64_MAX);

#if defined(CONFIG_ARCH_ARM_64)
		if (!RANGE_CONTAIN(bpt_unmap_mrd.pbase, bpt_unmap_mrd.len,
				   mrd->pbase, mrd->pg_count * PAGE_SIZE))
			continue;
#endif

		prot  = bootinfo_to_page_attr(mrd->flags);

		rc = ukplat_page_map(&kernel_pt, mrd->vbase, mrd->pbase,
				     mrd->pg_count, prot, 0);
		if (unlikely(rc))
			return rc;
	}

	/* Activate page table */
	rc = ukplat_pt_set_active(&kernel_pt);
	if (unlikely(rc))
		return rc;

	return 0;
}
