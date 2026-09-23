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

/* FIXME The directmap area is only known at runtime, this
 * is just a placeholder to allow including uk/pal/addr.h
 */
#define UK_PLAT_XEN_DIRECTMAP_AREA_START	0x0
#define UK_PLAT_XEN_DIRECTMAP_AREA_END		0x0

#endif /* CONFIG_HAVE_PAGING */

#ifdef __cplusplus
}
#endif
#endif /* __UK_PLAT_XEN_ADDR_H__ */
