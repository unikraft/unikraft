/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_LCPU_PM_H__
#define __UK_LCPU_PM_H__

#include <uk/arch/types.h>
#include <uk/lcpu/core.h>

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_HAVE_SMP
typedef int (*uk_lcpu_pm_start_t)(struct uk_lcpu *lcpu);

#if CONFIG_HAVE_CPU_MULTI_PHASE_STARTUP
typedef int (*uk_lcpu_pm_post_start_t)(const __u32 lcpuidx[],
				       unsigned int *num);
#endif /* CONFIG_HAVE_CPU_MULTI_PHASE_STARTUP */
#endif /* CONFIG_HAVE_SMP */

typedef void (*uk_lcpu_pm_halt_t)(void);
typedef void (*uk_lcpu_pm_halt_irq_t)(void);

struct uk_lcpu_pm_ops {
#if CONFIG_HAVE_SMP
	/**
	 * Starts a secondary logical CPU.
	 *
	 * @param lcpu
	 *   The secondary logical CPU to start
	 * @return
	 *   0 on success, !=0 on error
	 */
	uk_lcpu_pm_start_t start;
#if CONFIG_HAVE_CPU_MULTI_PHASE_STARTUP
	/**
	 * Allows the driver to do operations post-start/boot of secondary CPUs.
	 * It is immediately run after ->start() on the successfully booted
	 * CPUs.
	 *
	 * @param lcpuidx
	 *   The secondary CPU linear indexes that have successfully passed
	 *   the ->start() function.
	 * @param num
	 *   Pointer to the size of the idxs array - can be modified to let the
	 *   caller know on how many of the secondary CPUs the post-boot
	 *   operation was successful in case of error.
	 * @return
	 *   0 on success, !=0 on error
	 */
	uk_lcpu_pm_post_start_t post_start;
#endif /* CONFIG_HAVE_CPU_MULTI_PHASE_STARTUP */
#endif /* CONFIG_HAVE_SMP */
	/**
	 * Halt the current CPU. Typically involves some power management
	 * operation that may put the calling CPU into a more efficient
	 * power state.
	 *
	 * NOTE: This is called with IRQs disabled to completely stop execution
	 * on calling CPU (e.g. useful when crashing), BUT should still be
	 * able to be returned from in case of non-maskable asynchronous
	 * exceptions.
	 */
	uk_lcpu_pm_halt_t halt;
	/**
	 * Halt the current CPU but with IRQs enabled.
	 * Typically involves some power management operation that may put the
	 * calling CPU into a more efficient power state while also having
	 * interrupts enabled.
	 *
	 * NOTE: This is called with IRQs disabled and has the responsibility
	 * to put the current CPU in a lower power state but still be able to
	 * resume execution if some IRQ happens (e.g. useful for idle waiting
	 * and checking for a condition in an idle thread).
	 */
	uk_lcpu_pm_halt_irq_t halt_irq;
};

/**
 * Register a driver's implementation of the LCPU power management operations.
 *
 * @param ops Pointer to the driver's interface implementation
 * @return 0 on success, an errno-type error value otherwise
 */
int uk_lcpu_pm_ops_register(const struct uk_lcpu_pm_ops *ops);

#if CONFIG_HAVE_SMP
/**
 * Starts multiple logical CPUs and assigns them the given stacks. The logical
 * CPUs execute the entry functions if supplied or enter a low-power wait state
 * otherwise. CPUs that have already been started are ignored.
 *
 * @param lcpuidx array with the indices of the logical CPUs that are to be
 *   started. CPUs are started in the order specified in the array. Can be NULL
 *   to include all logical CPUs except the one executing the function, in which
 *   case CPUs are started in sequential order according to their CPU index
 * @param[inout] num if lcpuidx is not NULL, provides [IN] the number of
 *   elements in lcpuidx, and [OUT] the number of successfully started CPUs in
 *   sequential order of lcpuidx. If the call succeeds, input and output values
 *   are equal. Must be NULL if lcpuidx is NULL
 * @param sp array of stack pointers, one for each logical CPU to start. If
 *   lcpuidx is NULL, must be uk_lcpu_count() - 1 stack pointers. The
 *   stacks may be specifically prepared to contain arguments for the entry
 *   function (e.g., cdecl calling convention). The platform may use the
 *   following stack space to execute initialization routines
 * @param entry array of entry functions, one for each logical CPU to start.
 *   Can be NULL, otherwise if lcpuidx is NULL, must contain
 *   uk_lcpu_count() - 1 function pointers. Provided functions must not
 *   return. If the parameter or individual function pointers are NULL the
 *   respective logical CPUs enter a low-power wait state after startup
 * @param flags (architecture-dependent) flags that specify how to start the
 *   CPUs (see UK_LCPU_SFLG_* flags if available)
 *
 * @return 0 on success, an errno-type error value otherwise
 */
int uk_lcpu_start(const __u32 lcpuidx[],
		  unsigned int *num,
		  __u64 sp[], __u64 entry[],
		  unsigned long flags);
#endif /* CONFIG_HAVE_SMP */

/**
 * Halts the current logical CPU.
 */
__noreturn void uk_lcpu_halt(void);

/**
 * Halts the current logical CPU. Execution is resumed when an interrupt/signal
 * arrives.
 */
void uk_lcpu_halt_irq(void);

/**
 * Halts the current logical CPU. Execution is resumed when an interrupt/signal
 * arrives or the specified deadline expires
 *
 * NOTE: This must be called with IRQ's disabled. On return, IRQ's are not
 *        re-enabled.
 *
 * @param until deadline in nanoseconds
 */
void uk_lcpu_halt_irq_until(__nsec until);

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_LCPU_PM_H__ */
