/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
 * Copyright (c) 2021, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
 * Copyright (c) 2022, OpenSynergy GmbH All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_NATIVE_ARCH_ECTX_H__
#define __UK_PLAT_NATIVE_ARCH_ECTX_H__

#include <uk/arch/types.h>
#include <uk/arch/util.h>
#include <uk/config.h>

#define UK_PLAT_NATIVE_ECTX_SIZE			520
#define UK_PLAT_NATIVE_ECTX_ALIGN			16

#if CONFIG_LIBUKPLAT_NATIVE_ECTX
/* Assembly-visible function symbols */
#define UK_PLAT_NATIVE_ECTX_LOAD_FNSYM					\
	uk_plat_native_ectx_load
#define UK_PLAT_NATIVE_ECTX_STORE_FNSYM					\
	uk_plat_native_ectx_store
#define UK_PLAT_NATIVE_ECTX_SANITIZE_FNSYM				\
	uk_plat_native_ectx_sanitize
#endif /* CONFIG_LIBUKPLAT_NATIVE_ECTX */

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_LIBUKPLAT_NATIVE_ECTX
/* --- Extended Context --- */
struct uk_plat_native_ectx {
	/* NEON / FP registers */
	__u8 fpregs[512];
	/* Floating Point Status Register */
	__u32 fpsr;
	/* Floating Point Control Register */
	__u32 fpcr;
};

UK_CTASSERT(sizeof(struct uk_plat_native_ectx) == UK_PLAT_NATIVE_ECTX_SIZE);
UK_CTASSERT(__offsetof(struct uk_plat_native_ectx, fpregs) == 0);
UK_CTASSERT(__offsetof(struct uk_plat_native_ectx, fpsr) == 512);
UK_CTASSERT(__offsetof(struct uk_plat_native_ectx, fpcr) == 516);

/**
 * Sanitize state.
 *
 * @param state  16-byte aligned ectx memory to sanitize
 */
__isr void uk_plat_native_ectx_sanitize(struct uk_plat_native_ectx *state);

/**
 * Save CPU extended state (FP/NEON) to memory.
 *
 * @param state  16-byte aligned destination (must be sanitized)
 */
__isr void uk_plat_native_ectx_store(struct uk_plat_native_ectx *state);

/**
 * Restore extended state from memory to CPU.
 *
 * @param state  16-byte aligned ectx with valid saved data
 */
__isr void uk_plat_native_ectx_load(struct uk_plat_native_ectx *state);

/**
 * Initialize ectx to clean default state.
 *
 * @param state  16-byte aligned ectx to initialize
 */
__isr void uk_plat_native_ectx_init(struct uk_plat_native_ectx *state);

/**
 * Assert ectx matches current CPU state (debug only).
 *
 * @param state  ectx to compare against current CPU state
 */
__isr void uk_plat_native_ectx_assert_equal(struct uk_plat_native_ectx *state);
#endif /* CONFIG_LIBUKPLAT_NATIVE_ECTX */

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PLAT_NATIVE_ARCH_ECTX_H__ */
