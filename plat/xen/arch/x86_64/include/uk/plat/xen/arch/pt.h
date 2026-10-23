/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_XEN_ARCH_PT_H__
#define __UK_PLAT_XEN_ARCH_PT_H__

#include <uk/config.h>
#include <uk/plat/native/pt.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UK_PLAT_XEN_ARCH_PT_LEVELS	UK_PLAT_NATIVE_PT_LEVELS

/* No implementation here. In x86_64 we implement PV guest where
 * the page tables are owned by the hypervisor, and are mapped
 * read-only on the guest.
 */

#ifdef __cplusplus
}
#endif
#endif /* __UK_PLAT_XEN_ARCH_PT_H__ */
