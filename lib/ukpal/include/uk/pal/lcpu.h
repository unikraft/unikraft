/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PAL_LCPU_H__
#define __UK_PAL_LCPU_H__

/**
 * Logical CPU (LCPU) Management - Platform Abstraction Layer
 *
 * Provides initialization, control, and inter-processor operations for
 * logical CPUs across all platforms and architectures.
 */

#include <uk/arch/types.h>
#include <uk/lcpu/core.h>

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize logical CPU.
 * Sets up per-CPU state and prepares the CPU for operation.
 *
 * @param this_lcpu  LCPU structure for the current CPU
 * @return 0 on success, negative errno on failure
 */
int uk_pal_lcpu_init(struct uk_lcpu *this_lcpu);

#if CONFIG_HAVE_SMP
/**
 * Detect and populate LCPU structures for all processors.
 * Enumerates available CPUs and creates LCPU structures.
 *
 * @param arg  Platform-specific argument (may be unused)
 * @return 0 on success, negative errno on failure
 */
int uk_pal_lcpu_mp_init(void *arg);

/**
 * Send an Interprocessor Interrupt.
 *
 * @param id The CPU ID to send the IPI to
 * @param irq The IRQ to send to the CPU
 * @return 0 on success, negative errno on failure
 */
int uk_pal_send_ipi(__u64 id, unsigned long irq);
#endif /* CONFIG_HAVE_SMP */

/**
 * Get logical CPU hardware identifier.
 *
 * @return Platform-specific CPU ID
 */
__u64 uk_pal_lcpu_id(void);

/**
 * Get logical CPU index.
 *
 * @return Sequential CPU index (0..N-1)
 */
__u32 uk_pal_lcpu_idx(void);

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PAL_LCPU_H__ */
