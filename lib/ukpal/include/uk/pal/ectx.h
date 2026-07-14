/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PAL_ECTX_H__
#define __UK_PAL_ECTX_H__

/**
 * Extended CPU Context (ECTX) - Platform Abstraction Layer
 *
 * Manages extended execution state beyond general-purpose registers.
 * Provides save/restore of floating-point, vector, and other specialized
 * execution units. Implementation is platform and architecture specific.
 */

#include <uk/arch/types.h>

/*
 * Platform must define size, alignment, and function symbols for extended
 * context operations. These constants are used by higher layers to allocate
 * and manage extended context storage.
 */

#ifndef UK_PAL_ECTX_SIZE
#error "UK_PAL_ECTX_SIZE undefined"
#endif

#ifndef UK_PAL_ECTX_ALIGN
#error "UK_PAL_ECTX_ALIGN undefined"
#endif

#ifndef UK_PAL_ECTX_LOAD_FNSYM
#error "UK_PAL_ECTX_LOAD_FNSYM undefined"
#endif

#ifndef UK_PAL_ECTX_STORE_FNSYM
#error "UK_PAL_ECTX_STORE_FNSYM undefined"
#endif

#ifndef UK_PAL_ECTX_SANITIZE_FNSYM
#error "UK_PAL_ECTX_SANITIZE_FNSYM undefined"
#endif

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Extended execution context (opaque).
 * Platform and architecture specific storage for extended CPU state.
 */
struct uk_pal_ectx;

/**
 * Prepare extended context memory for first use.
 * Must be called on newly allocated memory before any save/restore operations.
 *
 * @param state  Extended context memory (properly sized and aligned)
 */
__isr void uk_pal_ectx_sanitize(struct uk_pal_ectx *state);

/**
 * Initialize extended context to clean default state.
 * Creates a valid context with architectural reset values.
 *
 * @param state  Extended context to initialize
 */
void uk_pal_ectx_init(struct uk_pal_ectx *state);

/**
 * Save current extended execution state to memory.
 * Captures state for later restoration.
 *
 * @param state  Destination for saved state
 */
__isr void uk_pal_ectx_store(struct uk_pal_ectx *state);

/**
 * Restore extended execution state from memory.
 * Loads previously saved state.
 *
 * @param state  Extended context to restore
 */
__isr void uk_pal_ectx_load(struct uk_pal_ectx *state);

/**
 * Verify extended context matches current CPU state.
 * Debug helper that crashes on mismatch. Use to detect unexpected state
 * corruption.
 *
 * @param state  Extended context to compare
 */
__isr void uk_pal_ectx_assert_equal(struct uk_pal_ectx *state);

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PAL_ECTX_H__ */
