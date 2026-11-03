/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_XEN_FIXMAP_H__
#define __UK_PLAT_XEN_FIXMAP_H__

#include <uk/config.h>
#include <uk/plat/xen/page.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Each slot is a 2 MiB block */
#define UK_PLAT_XEN_FIXMAP_LEVEL	UK_PLAT_XEN_PAGE_LARGE_LEVEL
#define UK_PLAT_XEN_FIXMAP_SLOT_SIZE	UK_PLAT_XEN_PAGE_LARGE_SIZE

/* A device tree of up to 2 MiB spans at most 2 blocks */
#define UK_PLAT_XEN_FIXMAP_FDT_NR	2

/* The slots end where the kernel is linked (see link64.lds.S) */
#define UK_PLAT_XEN_FIXMAP_TOP		0xffffffc000000000UL

#if !__ASSEMBLY__

enum uk_plat_xen_fixmap_idx {
	UK_PLAT_XEN_FIXMAP_IDX_FDT,
	UK_PLAT_XEN_FIXMAP_IDX_CON =
		UK_PLAT_XEN_FIXMAP_IDX_FDT + UK_PLAT_XEN_FIXMAP_FDT_NR,
	UK_PLAT_XEN_FIXMAP_IDX_XS,
	UK_PLAT_XEN_FIXMAP_IDX_GIC,
	UK_PLAT_XEN_FIXMAP_IDX_GNT,
	UK_PLAT_XEN_FIXMAP_NR
};

#define UK_PLAT_XEN_FIXMAP_BASE						\
	(UK_PLAT_XEN_FIXMAP_TOP -					\
	 UK_PLAT_XEN_FIXMAP_NR * UK_PLAT_XEN_FIXMAP_SLOT_SIZE)

/**
 * Returns the page attributes a fixmap slot is mapped with
 *
 * @param slot the slot [0..UK_PLAT_XEN_FIXMAP_NR - 1]
 *
 * @return the page attributes
 */
unsigned long uk_plat_xen_fixmap_attr(unsigned int slot);

/**
 * Returns the number of slots of the window that starts at a slot
 *
 * @param slot the first slot of the window
 *
 * @return the number of slots in the window, 0 if no window starts at slot
 */
unsigned int uk_plat_xen_fixmap_window_slots(unsigned int slot);

#endif /* !__ASSEMBLY__ */

#ifdef __cplusplus
}
#endif

#endif /* __UK_PLAT_XEN_FIXMAP_H__ */
