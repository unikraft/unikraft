/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_PAL_PAGING_H__
#define __UK_PLAT_PAL_PAGING_H__

#include <uk/arch/types.h>
#include <uk/plat/native/paging.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !__ASSEMBLY__

#if CONFIG_LIBUKPLAT_NATIVE_PAGING

static inline
int uk_pal_paging_init(void)
{
	return uk_plat_native_paging_init();
}

#endif /* CONFIG_LIBUKPLAT_NATIVE_PAGING */

#endif /* !__ASSEMBLY__ */

#ifdef __cplusplus
}
#endif
#endif /* __UK_PLAT_PAL_PAGING_H__ */
