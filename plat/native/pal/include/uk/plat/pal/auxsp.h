/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_PAL_AUXSP_H__
#define __UK_PLAT_PAL_AUXSP_H__

#include <uk/arch/types.h>
#include <uk/plat/native/arch/auxsp.h>

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

static inline __uptr uk_pal_get_auxsp(void)
{
	return uk_plat_native_get_auxsp();
}

static inline __isr __uptr uk_pal_get_auxsp_in_except(void)
{
	return uk_plat_native_get_auxsp_in_except();
}

static inline void uk_pal_set_auxsp(__uptr auxsp)
{
	uk_plat_native_set_auxsp(auxsp);
}

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PLAT_PAL_AUXSP_H__ */
