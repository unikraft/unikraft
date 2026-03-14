/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_PAL_SYSCTX_H__
#define __UK_PLAT_PAL_SYSCTX_H__

/* Include arch-specific PAL items */
#include <uk/plat/pal/arch/sysctx.h>

/* Xen uses the same sysctx format as native */
#include <uk/plat/native/sysctx.h>
/* But provides its own operations */
#include <uk/plat/xen/sysctx.h>

#define UK_PAL_SYSCTX_LOAD_FNSYM					\
	UK_PLAT_XEN_SYSCTX_LOAD_FNSYM
#define UK_PAL_SYSCTX_STORE_FNSYM					\
	UK_PLAT_XEN_SYSCTX_STORE_FNSYM

#define UK_PAL_SYSCTX_SIZE						\
	UK_PLAT_NATIVE_SYSCTX_SIZE
#define UK_PAL_SYSCTX_OFFSETOF_TLSP					\
	UK_PLAT_NATIVE_SYSCTX_OFFSETOF_TLSP

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

struct uk_pal_sysctx;

static inline
__u64 uk_pal_sysctx_get(const struct uk_pal_sysctx *sc, __sz offset)
{
	return uk_plat_native_sysctx_get(
			(const struct uk_plat_native_sysctx *)sc,
			offset);
}

static inline
void uk_pal_sysctx_set(struct uk_pal_sysctx *sc, __sz offset, __u64 val)
{
	uk_plat_native_sysctx_set((struct uk_plat_native_sysctx *)sc,
				  offset, val);
}

__isr static inline void uk_pal_sysctx_store(struct uk_pal_sysctx *sysctx)
{
	uk_plat_xen_sysctx_store((struct uk_plat_native_sysctx *)sysctx);
}

__isr static inline void uk_pal_sysctx_load(struct uk_pal_sysctx *sysctx)
{
	uk_plat_xen_sysctx_load((struct uk_plat_native_sysctx *)sysctx);
}

static inline __uptr uk_pal_tlsp_get(void)
{
	return uk_plat_xen_tlsp_get();
}

static inline void uk_pal_tlsp_set(__uptr tlsp)
{
	uk_plat_xen_tlsp_set(tlsp);
}

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */

#endif /* __UK_PLAT_PAL_SYSCTX_H__ */
