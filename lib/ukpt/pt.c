/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>

#include <uk/assert.h>
#include <uk/config.h>
#include <uk/essentials.h>
#include <uk/pt.h>

/* Physical address of the page table hierarchy that is configured in
 * hardware, as far as this library knows. We only perform TLB maintenance
 * for the active hierarchy. If the platform has not told us which hierarchy
 * is active (see uk_pt_init_active(), uk_pt_activate()), we do not touch the
 * TLB at all.
 */
static __paddr_t pt_active_pbase = UK_PAL_PADDR_INV;

int uk_pt_init(struct uk_pt *pt, __paddr_t pt_pbase, __vaddr_t frame_off)
{
	UK_ASSERT(pt);
	UK_ASSERT(pt_pbase != UK_PAL_PADDR_INV);
	UK_ASSERT(UK_PAL_PAGE_ALIGNED(pt_pbase));

	pt->pt_pbase = pt_pbase;
	pt->pt_frame_off = frame_off;
	pt->pt_vbase = uk_pt_frame_vaddr(pt, pt_pbase);

	return 0;
}

int uk_pt_init_active(struct uk_pt *pt, __vaddr_t frame_off)
{
	__paddr_t pt_pbase;
	int rc;

	pt_pbase = uk_pal_pt_read_base();
	if (unlikely(pt_pbase == UK_PAL_PADDR_INV))
		return -EFAULT;

	rc = uk_pt_init(pt, pt_pbase, frame_off);
	if (unlikely(rc))
		return rc;

	pt_active_pbase = pt_pbase;

	return 0;
}

int uk_pt_activate(struct uk_pt *pt)
{
	int rc;

	UK_ASSERT(pt);
	UK_ASSERT(pt->pt_pbase != UK_PAL_PADDR_INV);

	rc = uk_pal_pt_write_base(pt->pt_pbase);
	if (unlikely(rc))
		return rc;

	pt_active_pbase = pt->pt_pbase;

	return 0;
}

int uk_pt_isactive(struct uk_pt *pt)
{
	UK_ASSERT(pt);

	return (pt->pt_pbase == pt_active_pbase);
}

void uk_pt_flush_entry(struct uk_pt *pt, __vaddr_t vaddr)
{
	UK_ASSERT(pt);

	if (vaddr == UK_PAL_VADDR_INV)
		return;

	if (uk_pt_isactive(pt))
		uk_pal_tlb_flush_entry(vaddr);
}

int uk_pt_walk(struct uk_pt *pt, __vaddr_t vaddr, unsigned int *level,
	       __vaddr_t *tbl_vaddr, __pte_t *pte)
{
	unsigned int lvl = UK_PAL_PT_LEVELS - 1;
	unsigned int to_lvl = (level) ? *level : UK_PAL_PAGE_LEVEL;
	__vaddr_t vbase;
	__pte_t lpte = UK_PAL_PT_Lx_PTE_INVALID(UK_PAL_PAGE_LEVEL);
	int rc = 0;

	UK_ASSERT(pt);
	UK_ASSERT(pt->pt_vbase != UK_PAL_VADDR_INV);
	UK_ASSERT(to_lvl < UK_PAL_PT_LEVELS);

	vbase = pt->pt_vbase;

	while (lvl > to_lvl) {
		rc = uk_pal_pte_read(vbase, lvl, UK_PAL_PT_Lx_IDX(vaddr, lvl),
				     &lpte);
		if (unlikely(rc))
			goto EXIT;

		/* Stop if there is nothing here, or if this is a mapping and
		 * not a page table.
		 */
		if (!UK_PAL_PT_Lx_PTE_PRESENT(lpte, lvl) ||
		    UK_PAL_PAGE_Lx_IS(lpte, lvl))
			goto EXIT;

		vbase = uk_pt_frame_vaddr(pt,
					  UK_PAL_PT_Lx_PTE_PADDR(lpte, lvl));
		lvl--;
	}

	UK_ASSERT(lvl == to_lvl);
	rc = uk_pal_pte_read(vbase, lvl, UK_PAL_PT_Lx_IDX(vaddr, lvl), &lpte);

EXIT:
	if (level)
		*level = lvl;
	if (tbl_vaddr)
		*tbl_vaddr = vbase;
	if (pte)
		*pte = lpte;

	return rc;
}

/* Walks to the page table that holds the entry for vaddr at the given level */
static int pt_walk_to(struct uk_pt *pt, __vaddr_t vaddr, unsigned int level,
		      __vaddr_t *tbl_vaddr)
{
	unsigned int lvl = level;
	int rc;

	rc = uk_pt_walk(pt, vaddr, &lvl, tbl_vaddr, __NULL);
	if (unlikely(rc))
		return rc;

	/* A page table on the way to the requested level is missing */
	if (unlikely(lvl != level))
		return -ENOENT;

	return 0;
}

int uk_pt_table_create(__vaddr_t tbl_vaddr, unsigned int level)
{
	__pte_t invalid;
	unsigned int i;
	int rc;

	UK_ASSERT(level < UK_PAL_PT_LEVELS - 1);
	UK_ASSERT(UK_PAL_PAGE_ALIGNED(tbl_vaddr));

	invalid = UK_PAL_PT_Lx_PTE_INVALID(level);

	for (i = 0; i < UK_PAL_PT_Lx_PTES(level); i++) {
		rc = uk_pal_pte_write(tbl_vaddr, level, i, invalid);
		if (unlikely(rc))
			return rc;
	}

	return 0;
}

int uk_pt_table_isempty(__vaddr_t tbl_vaddr, unsigned int level)
{
	unsigned int i;
	__pte_t pte;
	int rc;

	UK_ASSERT(level < UK_PAL_PT_LEVELS - 1);

	for (i = 0; i < UK_PAL_PT_Lx_PTES(level); i++) {
		rc = uk_pal_pte_read(tbl_vaddr, level, i, &pte);
		if (unlikely(rc))
			return 0;

		if (UK_PAL_PT_Lx_PTE_PRESENT(pte, level))
			return 0;
	}

	return 1;
}

int uk_pt_table_link_at(struct uk_pt *pt, __vaddr_t tbl_vaddr,
			unsigned int level, __vaddr_t vaddr,
			__paddr_t link_paddr, __pte_t tmpl,
			unsigned int tmpl_level)
{
	unsigned int idx = UK_PAL_PT_Lx_IDX(vaddr, level);
	__pte_t pte;
	int rc;

	UK_ASSERT(level > UK_PAL_PAGE_LEVEL);
	UK_ASSERT(level < UK_PAL_PT_LEVELS);
	UK_ASSERT(UK_PAL_PAGE_ALIGNED(link_paddr));

	rc = uk_pal_pte_read(tbl_vaddr, level, idx, &pte);
	if (unlikely(rc))
		return rc;

	if (unlikely(UK_PAL_PT_Lx_PTE_PRESENT(pte, level)))
		return -EEXIST;

	pte = uk_pal_pt_pte_create(link_paddr, level, tmpl, tmpl_level);

	rc = uk_pal_pte_write(tbl_vaddr, level, idx, pte);
	if (unlikely(rc))
		return rc;

	uk_pt_flush_entry(pt, vaddr);

	return 0;
}

int uk_pt_table_link(struct uk_pt *pt, __vaddr_t vaddr, unsigned int level,
		     __paddr_t tbl_paddr, __pte_t tmpl,
		     unsigned int tmpl_level)
{
	__vaddr_t tbl_vaddr;
	int rc;

	rc = pt_walk_to(pt, vaddr, level, &tbl_vaddr);
	if (unlikely(rc))
		return rc;

	return uk_pt_table_link_at(pt, tbl_vaddr, level, vaddr, tbl_paddr,
				   tmpl, tmpl_level);
}

int uk_pt_table_unlink_at(struct uk_pt *pt, __vaddr_t tbl_vaddr,
			  unsigned int level, __vaddr_t vaddr,
			  __paddr_t *unlinked_paddr)
{
	unsigned int idx = UK_PAL_PT_Lx_IDX(vaddr, level);
	__pte_t pte;
	int rc;

	UK_ASSERT(level > UK_PAL_PAGE_LEVEL);
	UK_ASSERT(level < UK_PAL_PT_LEVELS);

	rc = uk_pal_pte_read(tbl_vaddr, level, idx, &pte);
	if (unlikely(rc))
		return rc;

	/* There must be a page table here, not a mapping */
	if (unlikely(!UK_PAL_PT_Lx_PTE_PRESENT(pte, level) ||
		     UK_PAL_PAGE_Lx_IS(pte, level)))
		return -ENOENT;

	if (unlinked_paddr)
		*unlinked_paddr = UK_PAL_PT_Lx_PTE_PADDR(pte, level);

	rc = uk_pal_pte_write(tbl_vaddr, level, idx,
			      UK_PAL_PT_Lx_PTE_INVALID(level));
	if (unlikely(rc))
		return rc;

	uk_pt_flush_entry(pt, vaddr);

	return 0;
}

int uk_pt_table_unlink(struct uk_pt *pt, __vaddr_t vaddr, unsigned int level,
		       __paddr_t *tbl_paddr)
{
	__vaddr_t tbl_vaddr;
	int rc;

	rc = pt_walk_to(pt, vaddr, level, &tbl_vaddr);
	if (unlikely(rc))
		return rc;

	return uk_pt_table_unlink_at(pt, tbl_vaddr, level, vaddr, tbl_paddr);
}

int uk_pt_map_at(struct uk_pt *pt, __vaddr_t tbl_vaddr, unsigned int level,
		 __vaddr_t vaddr, __paddr_t paddr, unsigned long attr,
		 __pte_t tmpl, unsigned int tmpl_level)
{
	unsigned int idx = UK_PAL_PT_Lx_IDX(vaddr, level);
	__pte_t pte;
	int rc;

	UK_ASSERT(level < UK_PAL_PT_LEVELS);
	UK_ASSERT(UK_PAL_PAGE_Lx_HAS(level));
	UK_ASSERT(UK_PAL_PAGE_Lx_ALIGNED(vaddr, level));
	UK_ASSERT(UK_PAL_PAGE_Lx_ALIGNED(paddr, level));

	rc = uk_pal_pte_read(tbl_vaddr, level, idx, &pte);
	if (unlikely(rc))
		return rc;

	if (unlikely(UK_PAL_PT_Lx_PTE_PRESENT(pte, level)))
		return -EEXIST;

	pte = uk_pal_pte_create(paddr, attr, level, tmpl, tmpl_level);

	rc = uk_pal_pte_write(tbl_vaddr, level, idx, pte);
	if (unlikely(rc))
		return rc;

	uk_pt_flush_entry(pt, vaddr);

	return 0;
}

int uk_pt_map(struct uk_pt *pt, __vaddr_t vaddr, __paddr_t paddr,
	      unsigned int level, unsigned long attr, __pte_t tmpl,
	      unsigned int tmpl_level)
{
	__vaddr_t tbl_vaddr;
	int rc;

	rc = pt_walk_to(pt, vaddr, level, &tbl_vaddr);
	if (unlikely(rc))
		return rc;

	return uk_pt_map_at(pt, tbl_vaddr, level, vaddr, paddr, attr, tmpl,
			    tmpl_level);
}

int uk_pt_unmap_at(struct uk_pt *pt, __vaddr_t tbl_vaddr, unsigned int level,
		   __vaddr_t vaddr, __paddr_t *paddr)
{
	unsigned int idx = UK_PAL_PT_Lx_IDX(vaddr, level);
	__pte_t pte;
	int rc;

	UK_ASSERT(level < UK_PAL_PT_LEVELS);

	rc = uk_pal_pte_read(tbl_vaddr, level, idx, &pte);
	if (unlikely(rc))
		return rc;

	/* There must be a mapping here, not a page table */
	if (unlikely(!UK_PAL_PT_Lx_PTE_PRESENT(pte, level) ||
		     !UK_PAL_PAGE_Lx_IS(pte, level)))
		return -ENOENT;

	if (paddr)
		*paddr = UK_PAL_PT_Lx_PTE_PADDR(pte, level);

	rc = uk_pal_pte_write(tbl_vaddr, level, idx,
			      UK_PAL_PT_Lx_PTE_INVALID(level));
	if (unlikely(rc))
		return rc;

	uk_pt_flush_entry(pt, vaddr);

	return 0;
}

int uk_pt_unmap(struct uk_pt *pt, __vaddr_t vaddr, unsigned int level,
		__paddr_t *paddr)
{
	__vaddr_t tbl_vaddr;
	int rc;

	rc = pt_walk_to(pt, vaddr, level, &tbl_vaddr);
	if (unlikely(rc))
		return rc;

	return uk_pt_unmap_at(pt, tbl_vaddr, level, vaddr, paddr);
}

int uk_pt_set_attr_at(struct uk_pt *pt, __vaddr_t tbl_vaddr,
		      unsigned int level, __vaddr_t vaddr, unsigned long attr)
{
	unsigned int idx = UK_PAL_PT_Lx_IDX(vaddr, level);
	__pte_t pte;
	int rc;

	UK_ASSERT(level < UK_PAL_PT_LEVELS);

	rc = uk_pal_pte_read(tbl_vaddr, level, idx, &pte);
	if (unlikely(rc))
		return rc;

	if (unlikely(!UK_PAL_PT_Lx_PTE_PRESENT(pte, level) ||
		     !UK_PAL_PAGE_Lx_IS(pte, level)))
		return -ENOENT;

	/* Keep the attributes that are not covered by attr */
	pte = uk_pal_pte_create(UK_PAL_PT_Lx_PTE_PADDR(pte, level), attr,
				level, pte, level);

	rc = uk_pal_pte_write(tbl_vaddr, level, idx, pte);
	if (unlikely(rc))
		return rc;

	uk_pt_flush_entry(pt, vaddr);

	return 0;
}

int uk_pt_set_attr(struct uk_pt *pt, __vaddr_t vaddr, unsigned int level,
		   unsigned long attr)
{
	__vaddr_t tbl_vaddr;
	int rc;

	rc = pt_walk_to(pt, vaddr, level, &tbl_vaddr);
	if (unlikely(rc))
		return rc;

	return uk_pt_set_attr_at(pt, tbl_vaddr, level, vaddr, attr);
}

int uk_pt_table_split_at(struct uk_pt *pt, __vaddr_t parent_vaddr,
			 unsigned int level, __vaddr_t vaddr,
			 __vaddr_t tbl_vaddr, __paddr_t tbl_paddr)
{
	unsigned int idx = UK_PAL_PT_Lx_IDX(vaddr, level);
	unsigned int to_lvl = level - 1;
	unsigned int i;
	unsigned long attr;
	__paddr_t paddr;
	__pte_t pte, new_pte;
	int rc;

	UK_ASSERT(level > UK_PAL_PAGE_LEVEL);
	UK_ASSERT(level < UK_PAL_PT_LEVELS);

	if (unlikely(!UK_PAL_PAGE_Lx_HAS(to_lvl)))
		return -ENOTSUP;

	rc = uk_pal_pte_read(parent_vaddr, level, idx, &pte);
	if (unlikely(rc))
		return rc;

	/* There must be a mapping here that we can split */
	if (unlikely(!UK_PAL_PT_Lx_PTE_PRESENT(pte, level) ||
		     !UK_PAL_PAGE_Lx_IS(pte, level)))
		return -ENOENT;

	attr = uk_pal_attr_from_pte(pte, level);
	paddr = UK_PAL_PT_Lx_PTE_PADDR(pte, level);

	rc = uk_pt_table_create(tbl_vaddr, to_lvl);
	if (unlikely(rc))
		return rc;

	/* Establish the same mapping with the next smaller page size. The
	 * original page table entry serves as template so that we do not
	 * lose any attribute that is not covered by attr.
	 */
	for (i = 0; i < UK_PAL_PT_Lx_PTES(to_lvl); i++) {
		new_pte = uk_pal_pte_create(paddr, attr, to_lvl, pte, level);

		rc = uk_pal_pte_write(tbl_vaddr, to_lvl, i, new_pte);
		if (unlikely(rc))
			return rc;

		paddr += UK_PAL_PAGE_Lx_SIZE(to_lvl);
	}

	/* Replace the mapping with the new page table. Note that we must not
	 * go through uk_pt_table_link_at(), which expects the page table
	 * entry to be free.
	 */
	new_pte = uk_pal_pt_pte_create(tbl_paddr, level, pte, level);

	rc = uk_pal_pte_write(parent_vaddr, level, idx, new_pte);
	if (unlikely(rc))
		return rc;

	uk_pt_flush_entry(pt, vaddr);

	return 0;
}

int uk_pt_table_split(struct uk_pt *pt, __vaddr_t vaddr, unsigned int level,
		      __vaddr_t tbl_vaddr, __paddr_t tbl_paddr)
{
	__vaddr_t parent_vaddr;
	int rc;

	rc = pt_walk_to(pt, vaddr, level, &parent_vaddr);
	if (unlikely(rc))
		return rc;

	return uk_pt_table_split_at(pt, parent_vaddr, level, vaddr, tbl_vaddr,
				    tbl_paddr);
}

int uk_pt_pte_write_at(struct uk_pt *pt, __vaddr_t tbl_vaddr,
		       unsigned int level, unsigned int idx, __vaddr_t vaddr,
		       __pte_t pte)
{
	int rc;

	UK_ASSERT(level < UK_PAL_PT_LEVELS);
	UK_ASSERT(idx < UK_PAL_PT_Lx_PTES(level));

	rc = uk_pal_pte_write(tbl_vaddr, level, idx, pte);
	if (unlikely(rc))
		return rc;

	uk_pt_flush_entry(pt, vaddr);

	return 0;
}
