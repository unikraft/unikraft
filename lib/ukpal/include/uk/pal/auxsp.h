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

/**
 * The platform must define a per-CPU variable for the auxiliary stack pointer
 * and export its symbol through the PAL by aliasing this macro to it.
 */
#ifndef UK_PAL_AUXSP_SYM
#error "UK_PAL_AUXSP_SYM undefined"
#endif

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

void uk_pal_set_auxsp(__uptr auxsp);

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PAL_AUXSP_H__ */
