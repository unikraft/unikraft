/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __UKPLAT_LCPU_H__
#define __UKPLAT_LCPU_H__

#include <uk/arch/time.h>
#include <uk/arch/lcpu.h>
#include <uk/essentials.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Enables interrupts
 */
void ukplat_lcpu_enable_irq(void);

/**
 * Disables interrupts
 */
void ukplat_lcpu_disable_irq(void);

/**
 * Returns current interrupt flags and disables them
 *
 * @return interrupt flags (the format is unspecified)
 */
unsigned long ukplat_lcpu_save_irqf(void);

/**
 * Loads interrupt flags
 *
 * @param flags interrupt flags (the format is unspecified)
 */
void ukplat_lcpu_restore_irqf(unsigned long flags);

/**
 * Checks if interrupts are disabled
 *
 * @return non-zero value if interrupts are disabled
 */
int ukplat_lcpu_irqs_disabled(void);

/**
 * TODO: This is Xen specific. Remove from public API?
 */
void ukplat_lcpu_irqs_handle_pending(void);

/**
 * Halts the current logical CPU execution
 */
void ukplat_lcpu_halt(void);

/**
 * Halts the current logical CPU. Execution is resumed when an interrupt/signal
 * arrives or the specified deadline expires
 *
 * @param until deadline in nanoseconds
 */
void ukplat_lcpu_halt_to(__nsec until);

/**
 * Halts the current logical CPU. Execution is resumed when an interrupt/signal
 * arrives.
 */
void ukplat_lcpu_halt_irq(void);

#ifdef CONFIG_HAVE_SMP
#include <uk/list.h>

typedef void (*ukplat_lcpu_entry_t)(void) __noreturn;
typedef __u32 __lcpuid;

struct ukplat_lcpu_func {
	struct uk_list_head lentry;

	/**
	 * Function to execute.
	 *
	 * @param regs pointer to a snapshot of the current CPU state. Changes
	 *   to the state are applied after the function returns
	 * @param fn pointer to this structure. The structure is not touched
	 *   after function invocation and can be freed if necessary during
	 *   function execution
	 */
	void (*fn)(struct __regs *regs, struct ukplat_lcpu_func *fn);

	/* Optional user-supplied pointer. */
	void *user;
};

/**
 * Returns the ID of the current logical CPU
 */
__lcpuid ukplat_lcpu_id(void);

/**
 * Returns the number of logical CPUs present on the system
 */
__lcpuid ukplat_lcpu_count(void);

/**
 * Starts multiple logical CPUs and assigns them the given stacks. The logical
 * CPUs execute the entry functions if supplied or enter a low-power wait state
 * otherwise.
 *
 * @param lcpuid array with the IDs of the logical CPUs that are to be started
 * @param sp array of stack pointers, one for each specified logical CPU
 * @param entry optional array of entry function pointers, one for each
 *   specified logical CPU. Provided functions must not return. If the
 *   parameter or individual elements are NULL the respective logical CPUs
 *   enter a low-power wait state after startup
 * @param num number of logical CPU IDs given
 *
 * @return 0 on success, an errno-type error value otherwise
 */
int ukplat_lcpu_start(__lcpuid lcpuid[], void *sp[],
		      ukplat_lcpu_entry_t entry[], unsigned int num);

/**
 * Waits for the specified logical CPUs to enter idle state, or until the
 * timeout expires.
 *
 * @param lcpuid array with the IDs of the logical CPUs to wait for. Can be
 *   NULL to include all logical CPUs except the one executing the function
 * @param num number of logical CPU IDs given
 * @param timeout timeout in nanoseconds for the wait to be satisfied. Can be
 *   0 to wait indefinitely
 *
 * @return 0 if the wait for all specified logical CPUs has been satisfied,
 *   an errno-type error value otherwise (e.g., timeout)
 */
int ukplat_lcpu_wait(__lcpuid lcpuid[], unsigned int num, __nsec timeout);

/**
 * Executes a function on the specified logical CPUs. The run function does not
 * wait for the execution to start or complete. Multiple functions can be run
 * at the same time without having to wait for their completion.
 *
 * @param lcpuid array with the IDs of the logical CPUs that should execute the
 *   function. Can be NULL to execute the function on all logical CPUs except
 *   the current one
 * @param num number of logical CPU IDs given
 * @param fn the function to be executed
 * @param flags (architecture-dependent) flags that specify how the function
 *   should be executed (see UKPLAT_LCPU_RFLG_* flags)
 *
 * @return 0 on success, an errno-type error value otherwise
 */
int ukplat_lcpu_run(__lcpuid lcpuid[], unsigned int num,
		    struct ukplat_lcpu_func *fn, int flags);

/**
 * Wakes up the specified logical CPUs from a halt or low-power sleep state.
 *
 * @return 0 on success, an errno-type error value otherwise
 */
int ukplat_lcpu_wakeup(__lcpuid lcpuid[], unsigned int num);

#else /* CONFIG_HAVE_SMP */
#define ukplat_lcpu_id()	(0)
#define ukplat_lcpu_count()	(1)
#endif /* CONFIG_HAVE_SMP */

#ifdef __cplusplus
}
#endif

#endif /* __UKPLAT_LCPU_H__ */
