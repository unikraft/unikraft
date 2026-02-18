/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2022, Karlsruhe Institute of Technology (KIT)
 *                     All rights reserved.
 * Copyright (c) 2022, University Politehnica of Bucharest.
 *                     All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_LCPU_H__
#define __UK_LCPU_H__

#include <uk/arch/types.h>
#include <uk/pcpuvar.h>
#include <uk/lcpu/core.h>
#include <uk/plat/pal/lcpu.h>
#include <uk/pal/lcpu.h>
#include <uk/lcpu/ectx.h>
#include <uk/lcpu/except.h>
#include <uk/lcpu/auxsp.h>
#include <uk/lcpu/regs.h>
#include <uk/lcpu/sysctx.h>
#include <uk/lcpu/pm.h>

#if !__ASSEMBLY__
#include <uk/essentials.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Return 1 if the given UK_LCPU is online, 0 otherwise
 */
static inline int uk_lcpu_state_is_online(int state)
{
	return (state >= UK_LCPU_STATE_IDLE);
}

/**
 * Return 1 if the given UK_LCPU is busy, 0 otherwise.
 * NOTE: The negation (i.e., the UK_LCPU is idle) does not have be true!
 */
static inline int uk_lcpu_state_is_busy(int state)
{
	return (state >= UK_LCPU_STATE_BUSY0);
}

/**
 * Initialize a logical CPU. The function must be executed on the CPU
 * represented by the UK_LCPU as early as possible after startup.
 *
 * @param this_lcpu pointer to the UK_LCPU structure representing the CPU
 *    executing this function
 * @return 0 on success, -errno otherwise
 */
int uk_lcpu_init(struct uk_lcpu *this_lcpu);

#if CONFIG_HAVE_SMP
/**
 * Initialize multi-processor functions. Must only be executed once on the
 * bootstrap processor (BSP)
 *
 * @param run_irq the IRQ vector to use for running remote functions
 * @param wakeup_irq the IRQ vector to use for waking up CPUs
 * @param arg an optional parameter from the boot code that is passed to the
 *    architectural initialization
 *
 * @return 0 on success, -errno otherwise
 */
int uk_lcpu_mp_init(unsigned long run_irq, unsigned long wakeup_irq, void *arg);

/**
 * Default entry function for secondary logical CPUs. Will call lcpu_init() and
 * If the logical CPU's startup arguments supply an entry function, the
 * original stack pointer will be restored and execution continues in the
 * supplied entry function with interrupts still disabled. Otherwise, interrupts
 * are enabled and the CPU enters a low-power state to wait for interrupts and
 * calls of uk_lcpu_run() that are destined for this CPU.
 *
 * NOTE: The function may be replaced with a custom implementation by
 *    overriding the function symbol.
 *
 * NOTE: The architecture's CPU startup code (typically an assembler trampoline)
 *    must jump to this function with interrupts disabled and prepare the stack
 *    and/or registers according to the respective calling convention to
 *    provide the following parameters:
 *
 * @param this_lcpu pointer to the UK_LCPU structure representing the CPU
 *    executing this function
 */
void __weak __noreturn uk_lcpu_entry_default(struct uk_lcpu *this_lcpu);

/**
 * Enqueue a function to the supplied UK_LCPU
 *
 * @param lcpu the UK_LCPU to enqueue the function to
 * @param fn the function to enqueue
 *
 * @return 0 on success, -errno otherwise
 */
int uk_lcpu_fn_enqueue(struct uk_lcpu *lcpu, const struct uk_lcpu_func *fn);

/**
 * Waits for the specified logical CPUs to enter idle state, or until the
 * timeout expires.
 *
 * @param lcpuidx array with the indices of the logical CPUs to wait for.
 * @param[inout] num provides [IN] the number of elements in lcpuidx,
 *   and [OUT] the number of CPUs in idle state until the timeout expired
 *   in sequential order of lcpuidx. If the call succeeds, input and output
 *   values are equal
 * @param timeout timeout in nanoseconds for the wait to be satisfied. Can be
 *   0 to wait indefinitely
 *
 * @return 0 if the wait for all specified logical CPUs has been satisfied,
 *   an errno-type error value otherwise (e.g., timeout)
 */
int uk_lcpu_wait(const __u64 lcpuidx[], unsigned int *num,
		 __nsec timeout);

/**
 * Executes a function on the specified logical CPUs. The run function does not
 * wait for the execution to start or complete. Multiple functions can be run
 * at the same time without having to wait for their completion.
 *
 * @param lcpuidx array with the indices of the logical CPUs that should
 *   execute the function.
 * @param[inout] num provides [IN] the number of elements in lcpuidx,
 *   and [OUT] the number of CPUs on which the function has
 *   been successfully queued in sequential order of lcpuidx. If the call
 *   succeeds, input and output values are equal.
 * @param fn the function to be executed
 * @param flags (architecture-dependent) flags that specify how the function
 *   should be executed (see UK_LCPU_RFLG_* flags)
 *
 * @return 0 on success, an errno-type error value otherwise
 */
int uk_lcpu_run(const __u64 lcpuidx[], unsigned int *num,
		const struct uk_lcpu_func *fn, unsigned long flags);

/* Do not block while trying to queue the function to the remote core */
#define UK_LCPU_RFLG_DONOTBLOCK	0x1

/**
 * Wakes up the specified logical CPUs from a halt or low-power sleep state.
 *
 * @param lcpuidx array with the indices of the logical CPUs that should be
 *   woken up
 * @param[inout] num provides [IN] the number of elements in lcpuidx,
 *   and [OUT] the number of successfully woken up CPUs in sequential
 *   order of lcpuidx. If the call succeeds, input and output values
 *   are equal.
 *
 * @return 0 on success, an errno-type error value otherwise
 */
int uk_lcpu_wakeup(const __u64 lcpuidx[], unsigned int *num);
#endif /* CONFIG_HAVE_SMP */

struct uk_lcpu *uk_lcpu_get_current(void);

/**
 * Assuming we are in an exception handler on an exception stack, fetch the
 * current CPU index in such a manner that we do not assume any register
 * state.
 *
 * WARNING: Again, only use in exception context (e.g. trap (non-syscall),
 *          IRQ)!
 */
__isr __u64 uk_lcpu_get_current_idx_in_except(void);

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_LCPU_H__*/
