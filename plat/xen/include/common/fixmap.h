/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef _FIXMAP_H_
#define _FIXMAP_H_

#include <uk/arch/types.h>
#include <uk/config.h>

#ifdef __cplusplus
extern "C" {
#endif

enum uk_plat_xen_fixmap {
	UK_PLAT_XEN_FIXMAP_FDT,		/* Device tree */
	UK_PLAT_XEN_FIXMAP_CON,		/* Console ring */
	UK_PLAT_XEN_FIXMAP_XS,		/* Xenstore ring */
	UK_PLAT_XEN_FIXMAP_GIC,		/* Interrupt controller */
	UK_PLAT_XEN_FIXMAP_GNT,		/* Grant table */
};

/**
 * Prepares the fixmap windows for use
 *
 * @return 0 on success, a non-zero error value otherwise
 */
int uk_plat_xen_fixmap_init(void);

/**
 * Points a fixmap window at the given physical address
 *
 * @param win the window to point
 * @param paddr the physical address to reach, not necessarily aligned
 *
 * @return the virtual address of paddr, __NULL on error
 */
void *uk_plat_xen_fixmap_set(enum uk_plat_xen_fixmap win, __paddr_t paddr);

#ifdef __cplusplus
}
#endif
#endif /* _FIXMAP_H_ */
