/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_XEN_ADDR_H__
#define __UK_PLAT_XEN_ADDR_H__

#include <uk/config.h>
#include <uk/plat/native/addr.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UK_PLAT_XEN_VADDR_INV	UK_PLAT_NATIVE_VADDR_INV
#define UK_PLAT_XEN_PADDR_INV	UK_PLAT_NATIVE_PADDR_INV

#if CONFIG_HAVE_PAGING

#if !__ASSEMBLY__

extern __vaddr_t uk_plat_xen_directmap_start;
extern __vaddr_t uk_plat_xen_directmap_end;

#define UK_PLAT_XEN_DIRECTMAP_AREA_START	uk_plat_xen_directmap_start
#define UK_PLAT_XEN_DIRECTMAP_AREA_END		uk_plat_xen_directmap_end

/**
 * Describes the direct-mapped area, which is not known until boot
 *
 * @param start the virtual address at which physical address zero is mapped
 * @param end the last virtual address of the area
 */
void uk_plat_xen_directmap_set(__vaddr_t start, __vaddr_t end);

#endif /* !__ASSEMBLY__ */

#endif /* CONFIG_HAVE_PAGING */

#ifdef __cplusplus
}
#endif
#endif /* __UK_PLAT_XEN_ADDR_H__ */
