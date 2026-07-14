/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_PAL_ECTX_H__
#define __UK_PLAT_PAL_ECTX_H__

/* Xen handles ectx identically to a native platform; reuse native ops */
#include <uk/plat/native/ectx.h>

#define UK_PAL_ECTX_LOAD_FNSYM						\
	UK_PLAT_NATIVE_ECTX_LOAD_FNSYM
#define UK_PAL_ECTX_STORE_FNSYM						\
	UK_PLAT_NATIVE_ECTX_STORE_FNSYM
#define UK_PAL_ECTX_SANITIZE_FNSYM					\
	UK_PLAT_NATIVE_ECTX_SANITIZE_FNSYM

#define UK_PAL_ECTX_SIZE						\
	UK_PLAT_NATIVE_ECTX_SIZE
#define UK_PAL_ECTX_ALIGN						\
	UK_PLAT_NATIVE_ECTX_ALIGN

#if !__ASSEMBLY__
struct uk_pal_ectx;

static inline void uk_pal_ectx_sanitize(struct uk_pal_ectx *state)
{
	uk_plat_native_ectx_sanitize((struct uk_plat_native_ectx *)state);
}

static inline void uk_pal_ectx_store(struct uk_pal_ectx *state)
{
	uk_plat_native_ectx_store((struct uk_plat_native_ectx *)state);
}

static inline void uk_pal_ectx_load(struct uk_pal_ectx *state)
{
	uk_plat_native_ectx_load((struct uk_plat_native_ectx *)state);
}

static inline void uk_pal_ectx_init(struct uk_pal_ectx *state)
{
	uk_plat_native_ectx_init((struct uk_plat_native_ectx *)state);
}

static inline void uk_pal_ectx_assert_equal(struct uk_pal_ectx *state)
{
	uk_plat_native_ectx_assert_equal((struct uk_plat_native_ectx *)state);
}

#endif /* !__ASSEMBLY__ */
#endif /* __UK_PLAT_PAL_ECTX_H__ */
