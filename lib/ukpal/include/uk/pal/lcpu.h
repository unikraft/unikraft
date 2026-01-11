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
 * Start a logical CPU.
 * Brings the CPU out of reset and begins execution.
 *
 * @param lcpu   LCPU to start
 * @param flags  Platform-specific startup flags
 * @return 0 on success, negative errno on failure
 */
int uk_pal_lcpu_start(struct uk_lcpu *lcpu, unsigned long flags);

#if CONFIG_HAVE_CPU_MULTI_PHASE_STARTUP
/**
 * Finalize LCPU startup.
 * Performs post-start operations and synchronization.
 *
 * @param lcpuidx  Array of LCPU indices (NULL for all)
 * @param num      [IN] Number of indices, [OUT] number processed
 * @return 0 on success, negative errno on failure
 */
int uk_pal_lcpu_post_start(const __u32 lcpuidx[], unsigned int *num);
#endif /* CONFIG_HAVE_CPU_MULTI_PHASE_STARTUP */

/**
 * Execute a function on a remote CPU.
 * Sends an inter-processor request to run the function.
 *
 * @param lcpu   Target LCPU
 * @param fn     Function to execute
 * @param flags  Execution flags
 * @return 0 on success, negative errno on failure
 */
int uk_pal_lcpu_run(struct uk_lcpu *lcpu, const struct uk_lcpu_func *fn,
		    unsigned long flags);

/**
 * Wake up a halted CPU.
 * Resumes execution of a halted processor.
 *
 * @param lcpu  LCPU to wake
 * @return 0 on success, negative errno on failure
 */
int uk_pal_lcpu_wakeup(struct uk_lcpu *lcpu);
#endif /* CONFIG_HAVE_SMP */

/**
 * Halt the current CPU.
 * Stops execution until interrupted or reset.
 */
void uk_pal_halt(void);

/**
 * Halt the current CPU and wait for an interrupt.
 * Atomically enables interrupts and halts. Disables interrupts on wake.
 */
void uk_pal_halt_irq(void);

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
