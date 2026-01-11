/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_NATIVE_ECTX_H__
#define __UK_PLAT_NATIVE_ECTX_H__

/**
 * Extended CPU Context (ECTX) - Architecture-Independent Interface
 *
 * Provides save/restore of extended CPU execution state beyond general-purpose
 * registers, such as floating-point, vector, and other specialized units.
 * Architecture-specific implementations define the actual state layout and
 * save/restore mechanisms.
 */

#include <uk/plat/native/arch/ectx.h>

/*
 * Verify that architecture-specific header defined required constants.
 * These are used by platform code to allocate properly sized and aligned
 * memory for extended context storage.
 */

#ifndef UK_PLAT_NATIVE_ECTX_SIZE
#error "UK_PLAT_NATIVE_ECTX_SIZE undefined"
#endif

#ifndef UK_PLAT_NATIVE_ECTX_ALIGN
#error "UK_PLAT_NATIVE_ECTX_ALIGN undefined"
#endif

#if CONFIG_LIBUKPLAT_NATIVE_ECTX
#ifndef UK_PLAT_NATIVE_ECTX_LOAD_FNSYM
#error "UK_PLAT_NATIVE_ECTX_LOAD_FNSYM undefined"
#endif

#ifndef UK_PLAT_NATIVE_ECTX_STORE_FNSYM
#error "UK_PLAT_NATIVE_ECTX_STORE_FNSYM undefined"
#endif

#ifndef UK_PLAT_NATIVE_ECTX_SANITIZE_FNSYM
#error "UK_PLAT_NATIVE_ECTX_SANITIZE_FNSYM undefined"
#endif
#endif /* CONFIG_LIBUKPLAT_NATIVE_ECTX */

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_LIBUKPLAT_NATIVE_ECTX
/**
 * Sanitize extended context memory for first use.
 * Initializes architecture-specific fields that must have valid values
 * before the first save/restore. Must be called on newly allocated memory.
 *
 * @param state  Extended context memory (properly aligned and sized)
 */
__isr void uk_plat_native_ectx_sanitize(struct uk_plat_native_ectx *state);

/**
 * Save current CPU's extended state to memory.
 * Captures the state of extended execution units (FPU, vector, etc.).
 *
 * @param state  Destination for saved state (must be sanitized)
 */
__isr void uk_plat_native_ectx_store(struct uk_plat_native_ectx *state);

/**
 * Restore extended state from memory to CPU.
 * Loads previously saved extended execution state.
 *
 * @param state  Extended context to restore (must contain valid saved state)
 */
__isr void uk_plat_native_ectx_load(struct uk_plat_native_ectx *state);

/**
 * Initialize extended context to clean default state.
 * Zeroes memory and saves a clean architectural state.
 *
 * @param state  Extended context to initialize (properly aligned and sized)
 */
__isr void uk_plat_native_ectx_init(struct uk_plat_native_ectx *state);

/**
 * Verify extended context matches current CPU state (debug only).
 * Crashes the kernel if mismatch detected. Used to catch unexpected
 * extended state corruption.
 *
 * @param state  Extended context to compare against current CPU state
 */
__isr void uk_plat_native_ectx_assert_equal(struct uk_plat_native_ectx *state);
#endif /* CONFIG_LIBUKPLAT_NATIVE_ECTX */

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PLAT_NATIVE_ECTX_H__ */
