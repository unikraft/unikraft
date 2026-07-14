/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_LCPU_SYSCTX_H__
#define __UK_LCPU_SYSCTX_H__

#include <uk/arch/types.h>
#include <uk/plat/pal/sysctx.h>
#include <uk/pal/sysctx.h>
#include <uk/lcpu/arch/sysctx.h>

#define UK_LCPU_SYSCTX_SIZE						\
	UK_PAL_SYSCTX_SIZE
#define UK_LCPU_SYSCTX_OFFSETOF_TLSP					\
	UK_PAL_SYSCTX_OFFSETOF_TLSP

#define UK_LCPU_SYSCTX_LOAD_FNSYM					\
	UK_PAL_SYSCTX_LOAD_FNSYM
#define UK_LCPU_SYSCTX_STORE_FNSYM					\
	UK_PAL_SYSCTX_STORE_FNSYM

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

struct uk_lcpu_sysctx;

__isr static inline void uk_lcpu_sysctx_load(struct uk_lcpu_sysctx *sysctx)
{
	uk_pal_sysctx_load((struct uk_pal_sysctx *)sysctx);
}

__isr static inline void uk_lcpu_sysctx_store(struct uk_lcpu_sysctx *sysctx)
{
	uk_pal_sysctx_store((struct uk_pal_sysctx *)sysctx);
}

__isr static inline __uptr uk_lcpu_tlsp_get(void)
{
	return uk_pal_tlsp_get();
}

__isr static inline void uk_lcpu_tlsp_set(__uptr tlsp)
{
	uk_pal_tlsp_set(tlsp);
}

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_LCPU_SYSCTX_H__ */
