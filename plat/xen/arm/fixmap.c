/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <common/fixmap.h>
#include <common/pt.h>
#include <xen-arm/mm.h>

#include <uk/arch/types.h>
#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/print.h>

static const struct {
	__vaddr_t vaddr;
	unsigned long attr;
} fixmap[] = {
	[UK_PLAT_XEN_FIXMAP_FDT] = {FIX_FDT_START, XENPLAT_PT_ATTR_MEM},
	[UK_PLAT_XEN_FIXMAP_CON] = {FIX_CON_START, XENPLAT_PT_ATTR_MEM},
	[UK_PLAT_XEN_FIXMAP_XS]  = {FIX_XS_START,  XENPLAT_PT_ATTR_MEM},
	[UK_PLAT_XEN_FIXMAP_GIC] = {FIX_GIC_START, XENPLAT_PT_ATTR_DEV},
	[UK_PLAT_XEN_FIXMAP_GNT] = {FIX_GNT_START, XENPLAT_PT_ATTR_MEM},
};

int uk_plat_xen_fixmap_init(void)
{
	return xenplat_pt_link(FIX_FDT_START, UK_PAGING_PT_LEVELS - 1,
			       fixmap_pgtable);
}

void *uk_plat_xen_fixmap_set(enum uk_plat_xen_fixmap win, __paddr_t paddr)
{
	int rc;

	UK_ASSERT(win < ARRAY_SIZE(fixmap));

	rc = xenplat_pt_map(fixmap[win].vaddr, paddr & L2_MASK, 1,
			    UK_PAGING_PAGE_LARGE_LEVEL, fixmap[win].attr);
	if (unlikely(rc)) {
		uk_pr_err("fixmap: could not map 0x%lx: %d\n",
			  (unsigned long)paddr, rc);
		return __NULL;
	}

	return (void *)(fixmap[win].vaddr + (paddr & L2_OFFSET));
}
