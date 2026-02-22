/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
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

#include <uk/arch.h>
#include <uk/arch/types.h>
#include <uk/pcpuvar.h>
#include <uk/plat/pal/lcpu.h>
#include <uk/pal/lcpu.h>
#include <uk/lcpu/ectx.h>
#include <uk/lcpu/except.h>
#include <uk/lcpu/auxsp.h>
#include <uk/lcpu/regs.h>
#include <uk/lcpu/start.h>
#include <uk/lcpu/sysctx.h>
#include <uk/lcpu/pm.h>

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_HAVE_SMP
#define UK_LCPU_FUNC_SIZE		0x10

#if !__ASSEMBLY__
struct uk_lcpu_regs;

struct uk_lcpu_func {
	/**
	 * Function to execute.
	 *
	 * @param regs pointer to a snapshot of the current CPU register state.
	 *    Changes to the registers are applied after the RUN IRQ handler
	 *    returns. The parameter might be NULL if the platform does not
	 *    support supplying a register snapshot.
	 * @param arg user-supplied argument
	 */
	void (*fn)(struct uk_lcpu_regs *regs, void *arg);

	/* Optional user-supplied argument. */
	void *user;
};

UK_CTASSERT(sizeof(struct uk_lcpu_func) == UK_LCPU_FUNC_SIZE);
#endif /* !__ASSEMBLY__ */
#endif /* CONFIG_HAVE_SMP */

/*
 * Logical CPU (LCPU) Structure
 */
#define UK_LCPU_STATE_OFFSET		0x00
#define UK_LCPU_ERR_OFFSET		(UK_LCPU_STATE_OFFSET + 0x4)
#if CONFIG_HAVE_SMP
#define UK_LCPU_FN_OFFSET		(UK_LCPU_ERR_OFFSET + 0x4)

#define UK_LCPU_MEMBERS_SIZE						\
	(UK_LCPU_FN_OFFSET + UK_LCPU_FUNC_SIZE)
#else /* !CONFIG_HAVE_SMP */
#define UK_LCPU_MEMBERS_SIZE						\
	(UK_LCPU_ERR_OFFSET + 4)
#endif /* !CONFIG_HAVE_SMP */
#define UK_LCPU_SIZE							\
	(ALIGN_UP(UK_LCPU_MEMBERS_SIZE, UK_ARCH_CACHE_LINE_SIZE))

/**
 * UK_LCPU States
 * The following state transitions are safe to execute.
 *
 *                         lcpu_init
 *                   ┌───────────────────┐lcpu_run
 *        lcpu_start │          ┌──────┐ │ ┌─────┐   ┌────
 * ┌─────────┐   ┌───┴──┐   ┌───┴──┐ ┌─▼─▼─┴─┐ ┌─▼───┴─┐
 * │ OFFLINE ├──►│ INIT │   │ IDLE │ │ BUSY0 │ │ BUSY1 │
 * └─────────┘   └───┬──┘   └─┬─▲──┘ └─┬─┬─▲─┘ └─┬─┬─▲─┘
 *                   │        │ └──────┘ │ └─────┘ │ └────
 * ┌────────┐        │        │          │ RUN_IRQ │
 * │ HALTED │◄───────┴────────┴──────────┴─────────┴──────
 * └────────┘        lcpu_halt (ONLY ALLOWED FOR THIS CPU)
 */
#define UK_LCPU_STATE_HALTED		-1 /* CPU stopped, needs reset */
#define UK_LCPU_STATE_OFFLINE		0 /* CPU not started */
#define UK_LCPU_STATE_INIT		1 /* CPU started, init not finished */
#define UK_LCPU_STATE_IDLE		2 /* CPU is idle */
#define UK_LCPU_STATE_BUSY0		3 /* >= CPU is busy */

#if !__ASSEMBLY__
struct __align(UK_ARCH_CACHE_LINE_SIZE) uk_lcpu {
	/* Current CPU state (UK_LCPU_STATE_*).
	 * Typically accessed with atomic instructions - must be 8-byte aligned
	 */
	volatile int state __align(8);

	/* Error code indicating the halt reason
	 * Only valid in UK_LCPU_STATE_HALTED
	 */
	int error_code;

#if CONFIG_HAVE_SMP
	/* Remote function to execute
	 * Only valid in UK_LCPU_STATE_IDLE and busy states
	 */
	struct uk_lcpu_func fn;
#endif /* CONFIG_HAVE_SMP */
};

UK_CTASSERT(__offsetof(struct uk_lcpu, state) == UK_LCPU_STATE_OFFSET);
UK_CTASSERT(__offsetof(struct uk_lcpu, error_code) == UK_LCPU_ERR_OFFSET);
#if CONFIG_HAVE_SMP
UK_CTASSERT(__offsetof(struct uk_lcpu, fn) == UK_LCPU_FN_OFFSET);
#endif /* CONFIG_HAVE_SMP */

UK_CTASSERT(sizeof(struct uk_lcpu) == UK_LCPU_SIZE);
UK_CTASSERT(UK_LCPU_MEMBERS_SIZE <= UK_LCPU_SIZE);
#endif /* !__ASSEMBLY__ */

#if !__ASSEMBLY__
#include <uk/arch/time.h>
#include <uk/essentials.h>

/**
 * Return the UK_LCPU structure for the CPU executing this function
 */
struct uk_lcpu *uk_lcpu_get_current(void);

extern __uk_pcpuvar struct uk_lcpu uk_lcpus;

/**
 * Return the UK_LCPU structure for the bootstraping CPU
 */
static inline struct uk_lcpu *uk_lcpu_get_bsp(void)
{
	/* The BSP is always index 0 */
	return &uk_pcpuvar_lval(0, uk_lcpus);
}

/**
 * Return 1 if the supplied UK_LCPU is the boottrap processor, 0 otherwise
 */
static inline __bool uk_lcpu_is_bsp(struct uk_lcpu *lcpu)
{
	return (lcpu == uk_lcpu_get_bsp());
}

/**
 * Return 1 if the executed on the bootstrap processor, 0 otherwise
 */
static inline __bool uk_lcpu_current_is_bsp(void)
{
	return uk_lcpu_is_bsp(uk_lcpu_get_current());
}

#if CONFIG_HAVE_SMP
/* The IRQ vectors passed to lcpu_mp_init */
extern const unsigned long * const uk_lcpu_run_irqv;
extern const unsigned long * const uk_lcpu_wakeup_irqv;
#endif /* CONFIG_HAVE_SMP */

/**
 * Return 1 if the given UK_LCPU is online, 0 otherwise
 */
static inline __bool uk_lcpu_state_is_online(int state)
{
	return (state >= UK_LCPU_STATE_IDLE);
}

/**
 * Return 1 if the given UK_LCPU is busy, 0 otherwise.
 * NOTE: The negation (i.e., the UK_LCPU is idle) does not have be true!
 */
static inline __bool uk_lcpu_state_is_busy(int state)
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
 *
 * @return 0 on success, -errno otherwise
 */
int uk_lcpu_mp_init(unsigned long run_irq, unsigned long wakeup_irq);

/**
 * Default entry function for secondary logical CPUs. Calls uk_lcpu_init() and
 * if the logical CPU's startup arguments supply an entry function, the
 * original stack pointer is restored and execution continues in the
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

#endif /* !__ASSEMBLY__ */
#ifdef __cplusplus
}
#endif
#endif /* __UK_LCPU_H__*/
