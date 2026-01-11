/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_LCPU_ECTX_H__
#define __UK_LCPU_ECTX_H__

#include <uk/arch/types.h>
#include <uk/plat/pal/ectx.h>
#include <uk/pal/ectx.h>

#define UK_LCPU_ECTX_SIZE						\
	UK_PAL_ECTX_SIZE
#define UK_LCPU_ECTX_ALIGN						\
	UK_PAL_ECTX_ALIGN

#define UK_LCPU_ECTX_LOAD_FNSYM						\
	UK_PAL_ECTX_LOAD_FNSYM
#define UK_LCPU_ECTX_STORE_FNSYM					\
	UK_PAL_ECTX_STORE_FNSYM
#define UK_LCPU_ECTX_SANITIZE_FNSYM					\
	UK_PAL_ECTX_SANITIZE_FNSYM

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

struct uk_lcpu_ectx;

__isr static inline void uk_lcpu_ectx_load(struct uk_lcpu_ectx *state)
{
	uk_pal_ectx_load((struct uk_pal_ectx *)state);
}

__isr static inline void uk_lcpu_ectx_store(struct uk_lcpu_ectx *state)
{
	uk_pal_ectx_store((struct uk_pal_ectx *)state);
}

__isr static inline void uk_lcpu_ectx_sanitize(struct uk_lcpu_ectx *state)
{
	uk_pal_ectx_sanitize((struct uk_pal_ectx *)state);
}

__isr static inline void uk_lcpu_ectx_assert_equal(struct uk_lcpu_ectx *state)
{
	uk_pal_ectx_assert_equal((struct uk_pal_ectx *)state);
}

__isr static inline void uk_lcpu_ectx_init(struct uk_lcpu_ectx *state)
{
	uk_pal_ectx_init((struct uk_pal_ectx *)state);
}

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_LCPU_ECTX_H__ */
