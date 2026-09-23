/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>

#include <common/pt.h>

#include <uk/assert.h>
#include <uk/config.h>
#include <uk/essentials.h>
#include <uk/plat/pal/paging.h>
#include <uk/pt.h>

static struct uk_pt xenplat_pt;

/* Walks to the page table that holds the entry for vaddr at the given level */
static int pt_walk_to(__vaddr_t vaddr, unsigned int level,
		      __vaddr_t *tbl_vaddr)
{
	unsigned int lvl = level;
	int rc;

	rc = uk_pt_walk(&xenplat_pt, vaddr, &lvl, tbl_vaddr, __NULL);
	if (unlikely(rc))
		return rc;

	/* A page table on the way is missing */
	if (unlikely(lvl != level))
		return -ENOENT;

	return 0;
}

int xenplat_pt_init(__paddr_t pt_pbase, __vaddr_t frame_off)
{
	int rc;

	rc = uk_pal_paging_init();
	if (unlikely(rc))
		return rc;

	return uk_pt_init(&xenplat_pt, pt_pbase, frame_off);
}

int xenplat_pt_link(__vaddr_t vaddr, unsigned int level, void *tbl)
{
	__vaddr_t tbl_vaddr = (__vaddr_t)tbl;
	int rc;

	UK_ASSERT(UK_PAL_PAGE_ALIGNED(tbl_vaddr));

	rc = uk_pt_table_create(tbl_vaddr, level - 1);
	if (unlikely(rc))
		return rc;

	return uk_pt_table_link(&xenplat_pt, vaddr, level,
				tbl_vaddr - xenplat_pt.pt_frame_off,
				UK_PT_TMPL_NONE, UK_PT_TMPL_LEVEL_NONE);
}

int xenplat_pt_map(__vaddr_t vaddr, __paddr_t paddr, unsigned long pages,
		   unsigned int level, unsigned long attr)
{
	__sz pgsize = UK_PAL_PAGE_Lx_SIZE(level);
	__vaddr_t tbl_vaddr;
	unsigned long i;
	__pte_t pte;
	int rc;

	UK_ASSERT(UK_PAL_PAGE_Lx_ALIGNED(vaddr, level));
	UK_ASSERT(UK_PAL_PAGE_Lx_ALIGNED(paddr, level));

	for (i = 0; i < pages; i++) {
		rc = pt_walk_to(vaddr, level, &tbl_vaddr);
		if (unlikely(rc))
			return rc;

		pte = uk_pal_pte_create(paddr, attr, level, UK_PT_TMPL_NONE,
					UK_PT_TMPL_LEVEL_NONE);

		rc = uk_pt_pte_write_at(&xenplat_pt, tbl_vaddr, level,
					UK_PAL_PT_Lx_IDX(vaddr, level),
					vaddr, pte);
		if (unlikely(rc))
			return rc;

		vaddr += pgsize;
		paddr += pgsize;
	}

	return 0;
}

int xenplat_pt_set_attr(__vaddr_t vaddr, unsigned long pages,
			unsigned int level, unsigned long attr)
{
	__sz pgsize = UK_PAL_PAGE_Lx_SIZE(level);
	unsigned long i;
	int rc;

	UK_ASSERT(UK_PAL_PAGE_Lx_ALIGNED(vaddr, level));

	for (i = 0; i < pages; i++) {
		rc = uk_pt_set_attr(&xenplat_pt, vaddr, level, attr);
		if (unlikely(rc))
			return rc;

		vaddr += pgsize;
	}

	return 0;
}
