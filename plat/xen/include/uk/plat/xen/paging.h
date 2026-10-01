/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_XEN_PAGING_H__
#define __UK_PLAT_XEN_PAGING_H__

#include <uk/config.h>
#include <uk/arch/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !__ASSEMBLY__

#if CONFIG_HAVE_PAGING

/**
 * Platform-specific initialization
 *
 * @return zero on success, negative value on error
 */
int uk_plat_xen_paging_init(void);

#endif /* CONFIG_HAVE_PAGING */

#endif /* !__ASSEMBLY__ */

#ifdef __cplusplus
}
#endif
#endif /* __UK_PLAT_XEN_PAGING_H__ */
