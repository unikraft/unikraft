/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_PAL_LCPU_H__
#define __UK_PLAT_PAL_LCPU_H__

#include <uk/arch/types.h>
#include <uk/plat/native/arch/lcpu.h>

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

static inline __u64 uk_pal_lcpu_id(void)
{
	return uk_plat_native_lcpu_id();
}

static inline __u32 uk_pal_lcpu_idx(void)
{
	return uk_plat_native_lcpu_idx();
}

static inline int uk_pal_lcpu_init(struct uk_lcpu *this_lcpu)
{
	return uk_plat_native_lcpu_init(this_lcpu);
}

#if CONFIG_HAVE_SMP
static inline int uk_pal_lcpu_mp_init(void *arg)
{
	return uk_plat_native_lcpu_mp_init(arg);
}

static inline int uk_pal_send_ipi(__u64 id, unsigned long irq)
{
	return uk_plat_native_send_ipi(id, irq);
}
#endif /* CONFIG_HAVE_SMP */

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PLAT_PAL_LCPU_H__ */
