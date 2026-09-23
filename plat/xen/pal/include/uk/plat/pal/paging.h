/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_PAL_PAGING_H__
#define __UK_PLAT_PAL_PAGING_H__

#include <uk/config.h>
#include <uk/plat/xen/paging.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !__ASSEMBLY__

#if CONFIG_HAVE_PAGING

static inline
int uk_pal_paging_init(void)
{
	return uk_plat_xen_paging_init();
}

#endif /* CONFIG_HAVE_PAGING */

#endif /* !__ASSEMBLY__ */

#ifdef __cplusplus
}
#endif

#endif /* __UK_PLAT_PAL_PAGING_H__ */
