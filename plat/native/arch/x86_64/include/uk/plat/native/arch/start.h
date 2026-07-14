/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_NATIVE_ARCH_START_H__
#define __UK_PLAT_NATIVE_ARCH_START_H__

#include <uk/arch/types.h>
#include <uk/config.h>
#include <uk/pcpuvar.h>

#if CONFIG_LIBUKPLAT_NATIVE_START
#define UK_PLAT_NATIVE_SENTRY_SYM					\
	uk_plat_native_sentry
#define UK_PLAT_NATIVE_SSTACKP_SYM					\
	uk_plat_native_sstackp
#define UK_PLAT_NATIVE_SARG_SYM						\
	uk_plat_native_sarg
#endif /* CONFIG_LIBUKPLAT_NATIVE_START */

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_LIBUKPLAT_NATIVE_START
extern __uk_pcpuvar __uptr uk_plat_native_sentry;
extern __uk_pcpuvar __uptr uk_plat_native_sstackp;
extern __uk_pcpuvar __uptr uk_plat_native_sarg;
#endif /* CONFIG_LIBUKPLAT_NATIVE_START */

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PLAT_NATIVE_ARCH_START_H__ */
