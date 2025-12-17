/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, Karlsruhe Institute of Technology (KIT).
 *                     All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>

#include <uk/fallocbuddy.h>
#include <uk/paging/arch.h>
#include <uk/paging.h>

int pgarch_pt_add_mem(struct uk_pagetable *pt, __paddr_t start, __sz len)
{
	unsigned long pages = len >> UK_PAL_PAGE_SHIFT;
	void *fa_meta;
	__sz fa_meta_size;
	__vaddr_t dm_off;

	UK_ASSERT(start <= __PADDR_MAX - len);
	UK_ASSERT(uk_pal_paddr_range_isvalid(start, len));

	/* Reserve space for the metadata at the beginning of the area. Note
	 * that the metadata area will be a bit too large because we eat away
	 * from the frames by placing the metadata in the first frames.
	 */
	fa_meta = (void *)pgarch_directmap_paddr_to_vaddr(start);
	fa_meta_size = uk_fallocbuddy_metadata_size(pages);

	if (unlikely(fa_meta_size >= len))
		return -ENOMEM;

	start = UK_PAL_PAGE_ALIGN_UP(start + fa_meta_size);
	pages = (len - fa_meta_size) >> UK_PAL_PAGE_SHIFT;

	dm_off = pgarch_directmap_paddr_to_vaddr(start);

	UK_ASSERT(pt->fa->addmem);

	return pt->fa->addmem(pt->fa, fa_meta, start, pages, dm_off);
}

int pgarch_pt_init(struct uk_pagetable *pt, __paddr_t start, __sz len)
{
	__sz fa_size;
	int rc;

	/* Reserve space for the allocator at the beginning of the area. */
	pt->fa = (struct uk_falloc *)pgarch_directmap_paddr_to_vaddr(start);

	fa_size = ALIGN_UP(uk_fallocbuddy_size(), 8);
	if (unlikely(fa_size >= len))
		return -ENOMEM;

	rc = uk_fallocbuddy_init(pt->fa);
	if (unlikely(rc))
		return rc;

	return pgarch_pt_add_mem(pt, start + fa_size, len - fa_size);
}
