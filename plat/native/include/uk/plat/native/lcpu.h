/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_NATIVE_LCPU_H__
#define __UK_PLAT_NATIVE_LCPU_H__

 /* Logical CPU (LCPU) Management - Architecture-Independent Interface */

#include <uk/lcpu/core.h>
#include <uk/plat/native/arch/lcpu.h>

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_LIBUKPLAT_NATIVE_LCPU
/* LCPU identification (convenience wrappers) */

__u64 uk_plat_native_lcpu_id(void);
__u32 uk_plat_native_lcpu_idx(void);

/**
 * Initialize logical CPU structures and per-CPU state.
 * Must be called once per CPU during boot.
 *
 * @param this_lcpu  LCPU structure for the current CPU
 * @return 0 on success, negative errno on failure
 */
int uk_plat_native_lcpu_init(struct uk_lcpu *this_lcpu);

#if CONFIG_HAVE_SMP
/**
 * Detect and populate LCPU structures for all processors.
 * Enumerates available CPUs and creates corresponding LCPU structures.
 * Called once during boot on the bootstrap processor.
 *
 * @param arg  Reserved for future use
 * @return 0 on success, negative errno on failure
 */
int uk_plat_native_mp_init(void *arg __unused);
#endif /* CONFIG_HAVE_SMP */
#endif /* CONFIG_LIBUKPLAT_NATIVE_LCPU */

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PLAT_NATIVE_LCPU_H__ */
