/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_NATIVE_ARCH_AUXSP_H__
#define __UK_PLAT_NATIVE_ARCH_AUXSP_H__

#include <uk/arch/types.h>
#include <uk/config.h>

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_LIBUKPLAT_NATIVE_AUXSP
/**
 * Get current auxiliary stack pointer.
 * The auxiliary stack is used for exception handling.
 *
 * @return Auxiliary stack pointer value
 */
__uptr uk_plat_native_get_auxsp(void);

/**
 * Set auxiliary stack pointer.
 * Configures the stack used for exception handling.
 *
 * @param auxsp  New auxiliary stack pointer
 */
void uk_plat_native_set_auxsp(__uptr auxsp);

/**
 * Get auxiliary stack pointer during exception handling.
 * ISR-safe variant for use within exception handlers.
 *
 * @return Current auxiliary stack pointer
 */
__isr __uptr uk_plat_native_get_auxsp_in_except(void);
#endif /* CONFIG_LIBUKPLAT_NATIVE_AUXSP */

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PLAT_NATIVE_ARCH_AUXSP_H__ */
