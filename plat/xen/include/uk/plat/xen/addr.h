/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_XEN_ADDR_H__
#define __UK_PLAT_XEN_ADDR_H__

/* Xen uses the same address format as native */
#include <uk/plat/native/addr.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UK_PLAT_XEN_VADDR_INV	UK_PLAT_NATIVE_VADDR_INV
#define UK_PLAT_XEN_PADDR_INV	UK_PLAT_NATIVE_PADDR_INV

/* Xen platform does not support paging; do not declare addr functions */

#ifdef __cplusplus
}
#endif
#endif /* __UK_PLAT_XEN_ADDR_H__ */
