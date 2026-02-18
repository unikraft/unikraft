/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_NATIVE_ARCH_AUXSP_H__
#define __UK_PLAT_NATIVE_ARCH_AUXSP_H__

#include <uk/arch/types.h>
#include <uk/config.h>
#include <uk/pcpuvar.h>

#if CONFIG_LIBUKPLAT_NATIVE_AUXSP
#define UK_PLAT_NATIVE_AUXSP_SYM					\
	uk_plat_native_auxsp
#endif /* CONFIG_LIBUKPLAT_NATIVE_AUXSP */

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_LIBUKPLAT_NATIVE_AUXSP
extern __uk_pcpuvar __uptr uk_plat_native_auxsp;

void uk_plat_native_set_auxsp(__uptr auxsp);
#endif /* CONFIG_LIBUKPLAT_NATIVE_AUXSP */

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PLAT_NATIVE_ARCH_AUXSP_H__ */
