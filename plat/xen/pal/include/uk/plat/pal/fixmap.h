/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_PAL_FIXMAP_H__
#define __UK_PLAT_PAL_FIXMAP_H__

#include <uk/config.h>
#include <uk/plat/xen/fixmap.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UK_PAL_FIXMAP_LEVEL		UK_PLAT_XEN_FIXMAP_LEVEL
#define UK_PAL_FIXMAP_BASE		UK_PLAT_XEN_FIXMAP_BASE
#define UK_PAL_FIXMAP_NR_SLOTS		UK_PLAT_XEN_FIXMAP_NR

#if !__ASSEMBLY__

static inline
unsigned long uk_pal_fixmap_slot_attr(unsigned int slot)
{
	return uk_plat_xen_fixmap_attr(slot);
}

static inline
unsigned int uk_pal_fixmap_window_slots(unsigned int slot)
{
	return uk_plat_xen_fixmap_window_slots(slot);
}

#define UK_PAL_FIXMAP_FDT		UK_PLAT_XEN_FIXMAP_IDX_FDT
#define UK_PAL_FIXMAP_CON		UK_PLAT_XEN_FIXMAP_IDX_CON
#define UK_PAL_FIXMAP_XS		UK_PLAT_XEN_FIXMAP_IDX_XS
#define UK_PAL_FIXMAP_GIC		UK_PLAT_XEN_FIXMAP_IDX_GIC
#define UK_PAL_FIXMAP_GNT		UK_PLAT_XEN_FIXMAP_IDX_GNT

#endif /* !__ASSEMBLY__ */

#ifdef __cplusplus
}
#endif

#endif /* __UK_PLAT_PAL_FIXMAP_H__ */
