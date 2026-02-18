/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_PAL_AUXSP_H__
#define __UK_PLAT_PAL_AUXSP_H__

#include <uk/arch/types.h>
#include <uk/plat/native/arch/auxsp.h>

#define UK_PAL_AUXSP_SYM						\
	UK_PLAT_NATIVE_AUXSP_SYM

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

static inline void uk_pal_set_auxsp(__uptr auxsp)
{
	uk_plat_native_set_auxsp(auxsp);
}

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PLAT_PAL_AUXSP_H__ */
