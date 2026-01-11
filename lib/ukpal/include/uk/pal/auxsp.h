/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PAL_AUXSP_H__
#define __UK_PAL_AUXSP_H__

#include <uk/arch/types.h>

/**
 * Auxiliary stack - Platform Abstraction Layer
 *
 * Provides accessors to the auxiliary stack: an emergency per-kernel thread
 * stack that is prefaulted and at all times available to switch to.
 */

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Set auxiliary stack pointer.
 * Configures the stack used for exception handling.
 *
 * @param auxsp  New auxiliary stack pointer
 */
void uk_pal_set_auxsp(__uptr auxsp);

/**
 * Get current auxiliary stack pointer.
 *
 * @return Auxiliary stack pointer value
 */
__uptr uk_pal_get_auxsp(void);

/**
 * Get auxiliary stack pointer during exception handling.
 *
 * @return Current auxiliary stack pointer (ISR-safe variant)
 */
__isr __uptr uk_pal_get_auxsp_in_except(void);

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PAL_AUXSP_H__ */
