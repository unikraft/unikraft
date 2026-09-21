/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_PAL_ADDR_H__
#define __UK_PLAT_PAL_ADDR_H__

#include <uk/arch/types.h>
#include <uk/config.h>
#include <uk/plat/xen/addr.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !__ASSEMBLY__

#define UK_PAL_VADDR_INV	UK_PLAT_XEN_VADDR_INV
#define UK_PAL_PADDR_INV	UK_PLAT_XEN_PADDR_INV

#if CONFIG_HAVE_PAGING

/* See uk/plat/xen/addr.h: we have no direct-mapped area, these only
 * satisfy <uk/pal/addr.h>
 */
#define UK_PAL_DIRECTMAP_AREA_START	UK_PLAT_XEN_DIRECTMAP_AREA_START
#define UK_PAL_DIRECTMAP_AREA_END	UK_PLAT_XEN_DIRECTMAP_AREA_END

#endif /* CONFIG_HAVE_PAGING */

#endif /* !__ASSEMBLY__ */

#ifdef __cplusplus
}
#endif

#endif /* __UK_PLAT_PAL_ADDR_H__ */
