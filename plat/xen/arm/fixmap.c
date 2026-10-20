/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/essentials.h>
#include <uk/plat/xen/fixmap.h>
#include <uk/pt.h>
#include <xen-arm/mm.h>

/* A single page table, linked by _libxenplat_armentry(), covers the slots */
UK_CTASSERT(UK_PLAT_XEN_FIXMAP_NR <=
	    UK_PAL_PT_Lx_PTES(UK_PLAT_XEN_FIXMAP_LEVEL));
UK_CTASSERT(!(UK_PLAT_XEN_FIXMAP_TOP &
	      (UK_PAL_PAGE_Lx_SIZE(UK_PLAT_XEN_FIXMAP_LEVEL + 1) - 1)));
UK_CTASSERT(UK_PLAT_XEN_FIXMAP_TOP == PAGE_OFFSET);

unsigned long uk_plat_xen_fixmap_attr(unsigned int slot)
{
	if (slot == UK_PLAT_XEN_FIXMAP_IDX_GIC)
		return XENPLAT_PT_ATTR_DEV;

	return XENPLAT_PT_ATTR_MEM;
}

unsigned int uk_plat_xen_fixmap_window_slots(unsigned int slot)
{
	switch (slot) {
	case UK_PLAT_XEN_FIXMAP_IDX_FDT:
		return UK_PLAT_XEN_FIXMAP_FDT_NR;
	case UK_PLAT_XEN_FIXMAP_IDX_CON:
	case UK_PLAT_XEN_FIXMAP_IDX_XS:
	case UK_PLAT_XEN_FIXMAP_IDX_GIC:
	case UK_PLAT_XEN_FIXMAP_IDX_GNT:
		return 1;
	default:
		return 0;
	}
}
