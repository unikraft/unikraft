/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *          Răzvan Vîrtan <virtanrazvan@gmail.com>
 *          Cristian Vijelie <cristianvijelie@gmail.com>
 *
 * Copyright (c) 2022, Karlsruhe Institute of Technology (KIT)
 *                     All rights reserved.
 * Copyright (c) 2022, University Politehnica of Bucharest.
 *                     All rights reserved.
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

#ifndef __PLAT_CMN_LCPU_H__
#define __PLAT_CMN_LCPU_H__

#include <uk/config.h>
#ifndef __ASSEMBLY__
#include <uk/essentials.h>
#include <uk/arch/types.h>
#include <uk/plat/lcpu.h>
#include <uk/plat/spinlock.h>
#include <uk/list.h>
#else /* !__ASSEMBLY__ */
#define ALIGN_UP(v, a) (((v) + (a)-1) & ~((a)-1))
#endif /* __ASSEMBLY__ */

#if defined(__X86_64__)
#include <x86/lcpu_defs.h>
#endif /* __X86_64__ */

/* Provide empty architecture-dependent LCPU part as default */
#ifndef LCPU_ARCH_SIZE
#define LCPU_ARCH_SIZE			0x00

#ifndef __ASSEMBLY__
struct lcpu_arch { };
#endif /* !__ASSEMBLY__ */
#endif /* !LCPU_ARCH_SIZE */

#define IS_LCPU_PTR(ptr)                                               \
	(IN_RANGE((__uptr)(ptr),                                        \
		  (__uptr)lcpu_get(0),                                  \
		  (__uptr)CONFIG_UKPLAT_LCPU_MAXCOUNT *                 \
		  sizeof(struct lcpu)))

/*
 * LCPU Startup Arguments
 */
#define LCPU_SARGS_ENTRY_OFFSET		0x00
#define LCPU_SARGS_STACKP_OFFSET	(LCPU_SARGS_ENTRY_OFFSET + 0x08)

#define LCPU_SARGS_SIZE			0x10

#ifndef __ASSEMBLY__
struct lcpu_sargs {
	ukplat_lcpu_entry_t entry;
	void *stackp;
};

UK_CTASSERT(__offsetof(struct lcpu_sargs, entry)  == LCPU_SARGS_ENTRY_OFFSET);
UK_CTASSERT(__offsetof(struct lcpu_sargs, stackp) == LCPU_SARGS_STACKP_OFFSET);

UK_CTASSERT(sizeof(struct lcpu_sargs) == LCPU_SARGS_SIZE);
#endif /* !__ASSEMBLY__ */

/*
 * Logical CPU (LCPU) Structure
 */
#define LCPU_STATE_OFFSET		0x00
#define LCPU_IDX_OFFSET			(LCPU_STATE_OFFSET + 0x04)
#define LCPU_ID_OFFSET			(LCPU_IDX_OFFSET   + 0x04)
#define LCPU_ENTRY_OFFSET		(LCPU_ID_OFFSET    + 0x08)
#define LCPU_STACKP_OFFSET		(LCPU_ENTRY_OFFSET + 0x08)
#define LCPU_ERR_OFFSET			(LCPU_ENTRY_OFFSET + 0x00)
#define LCPU_ARCH_OFFSET		(LCPU_ENTRY_OFFSET + 0x10)

#ifdef CONFIG_HAVE_SMP
#define LCPU_FUNC_SIZE			0x10
#endif /* CONFIG_HAVE_SMP */

#define LCPU_MEMBERS_SIZE		(LCPU_ARCH_OFFSET  + LCPU_ARCH_SIZE)
#define LCPU_SIZE						\
	ALIGN_UP(LCPU_MEMBERS_SIZE, CACHE_LINE_SIZE)

#ifndef __ASSEMBLY__
struct __align(CACHE_LINE_SIZE) lcpu {
	/* Current CPU state (LCPU_STATE_*).
	 * Working on it with atomic instructions - must be 8-byte aligned
	 */
	volatile int state __align(8);

	__lcpuidx idx;
	__lcpuid id;

	union {
		/* Startup arguments
		 * Only valid in LCPU_STATE_INIT
		 */
		struct lcpu_sargs s_args;

		/* Remote function to execute
		 * Only valid in LCPU_STATE_IDLE and busy states
		 */
#ifdef CONFIG_HAVE_SMP
		struct ukplat_lcpu_func fn;
#endif /* CONFIG_HAVE_SMP */

		/* Error code indicating the halt reason
		 * Only valid in LCPU_STATE_HALTED
		 */
		int error_code;
	};

	/* Architecture-dependent part */
	struct lcpu_arch arch;
};

#ifdef CONFIG_HAVE_SMP
UK_CTASSERT(sizeof(struct ukplat_lcpu_func)        == LCPU_FUNC_SIZE);
#endif /* CONFIG_HAVE_SMP */

UK_CTASSERT(__offsetof(struct lcpu, state)         == LCPU_STATE_OFFSET);
UK_CTASSERT(__offsetof(struct lcpu, idx)           == LCPU_IDX_OFFSET);
UK_CTASSERT(__offsetof(struct lcpu, id)            == LCPU_ID_OFFSET);
UK_CTASSERT(__offsetof(struct lcpu, s_args.entry)  == LCPU_ENTRY_OFFSET);
UK_CTASSERT(__offsetof(struct lcpu, s_args.stackp) == LCPU_STACKP_OFFSET);
UK_CTASSERT(__offsetof(struct lcpu, error_code)    == LCPU_ERR_OFFSET);
UK_CTASSERT(__offsetof(struct lcpu, arch)          == LCPU_ARCH_OFFSET);

UK_CTASSERT(sizeof(struct lcpu) == LCPU_SIZE);
UK_CTASSERT(LCPU_MEMBERS_SIZE <= LCPU_SIZE);
#endif /* !__ASSEMBLY__ */

/**
 * LCPU States
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
#define LCPU_STATE_HALTED		-1 /* CPU stopped, needs reset */
#define LCPU_STATE_OFFLINE		 0 /* CPU not started */
#define LCPU_STATE_INIT			 1 /* CPU started, init not finished */
#define LCPU_STATE_IDLE			 2 /* CPU is idle */
#define LCPU_STATE_BUSY0		 3 /* >= CPU is busy */

#ifndef __ASSEMBLY__
/**
 * Return 1 if the given LCPU is online, 0 otherwise
 */
static inline int lcpu_state_is_online(int state)
{
	return (state >= LCPU_STATE_IDLE);
}

/**
 * Return 1 if the given LCPU is busy, 0 otherwise.
 * NOTE: The negation (i.e., the LCPU is idle) does not have be true!
 */
static inline int lcpu_state_is_busy(int state)
{
	return (state >= LCPU_STATE_BUSY0);
}

#ifdef CONFIG_HAVE_SMP
/**
 * Allocate a logical CPU and assign the provided CPU ID. This function may
 * only be called from one thread running on the bootstrap processor before
 * secondary CPUs are started.
 *
 * @param id ID of the CPU for which to allocate an LCPU structure
 * @return an LCPU on success, which is in OFFLINE state with the lock, id, and
 *    idx initialized; NULL on failure.
 */
struct lcpu *lcpu_alloc(__lcpuid id);
#endif /* CONFIG_HAVE_SMP */

/**
 * Return the LCPU structure for the logical CPU with the given index.
 *
 * @param idx the index of the requested LCPU. The index must be less than
 *    the value returned by ukplat_lcpu_count(), otherwise behavior is
 *    undefined
 * @return pointer to the requested LCPU structure
 */
struct lcpu *lcpu_get(__lcpuidx idx);

#define _lcpu_lcpuidx_list_entry(list, i, n)				\
	(((i) < (n)) ?							\
	  lcpu_get((list) ? (list)[i] : (__lcpuidx) (i))		\
	  : NULL)

#define lcpu_lcpuidx_list_foreach(list, num, n, i, lcpu)		\
	if ((num) == NULL) {						\
		UK_ASSERT(!(list));					\
		(n) = ukplat_lcpu_count();				\
	} else	{							\
		UK_ASSERT((*num) <= ukplat_lcpu_count());		\
		(n) = *(num);						\
	}								\
	for ((i) = 0,							\
	     ({ if (num) *num = i; }),					\
	     (lcpu) = _lcpu_lcpuidx_list_entry(list, i, n);		\
	     (i) < (n);							\
	     (i)++,							\
	     ({ if (num) *num = i; }),					\
	     (lcpu) = _lcpu_lcpuidx_list_entry(list, i, n))

/**
 * Return the LCPU structure for the CPU executing this function
 */
struct lcpu *lcpu_get_current(void);

/**
 * Return the LCPU structure for the bootstraping CPU
 */
static inline struct lcpu *lcpu_get_bsp(void)
{
	/* The BSP is always index 0 */
	return lcpu_get(0);
}

/**
 * Return 1 if the supplied LCPU is the boottrap processor, 0 otherwise
 */
static inline int lcpu_is_bsp(struct lcpu *lcpu)
{
	return (lcpu == lcpu_get_bsp());
}

/**
 * Return 1 if the executed on the bootstrap processor, 0 otherwise
 */
static inline int lcpu_current_is_bsp(void)
{
	return lcpu_is_bsp(lcpu_get_current());
}

/**
 * Initialize a logical CPU. The function must be executed on the CPU
 * represented by the LCPU as early as possible after startup.
 *
 * @param this_lcpu pointer to the LCPU structure representing the CPU
 *    executing this function
 * @return 0 on success, -errno otherwise
 */
int lcpu_init(struct lcpu *this_lcpu);

#ifdef CONFIG_HAVE_SMP
/* The IRQ vectors passed to lcpu_mp_init */
extern const unsigned long * const lcpu_run_irqv;
extern const unsigned long * const lcpu_wakeup_irqv;

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
int lcpu_mp_init(unsigned long run_irq, unsigned long wakeup_irq, void *arg);

/**
 * Default entry function for secondary logical CPUs. Will call lcpu_init() and
 * If the logical CPU's startup arguments supply an entry function, the
 * original stack pointer will be restored and execution continues in the
 * supplied entry function with interrupts still disabled. Otherwise, interrupts
 * are enabled and the CPU enters a low-power state to wait for interrupts and
 * calls of ukplat_lcpu_run() that are destined for this CPU.
 *
 * NOTE: The function may be replaced with a custom implementation by
 *    overriding the function symbol.
 *
 * NOTE: The architecture's CPU startup code (typically an assembler trampoline)
 *    must jump to this function with interrupts disabled and prepare the stack
 *    and/or registers according to the respective calling convention to
 *    provide the following parameters:
 *
 * @param this_lcpu pointer to the LCPU structure representing the CPU
 *    executing this function
 */
void __weak __noreturn lcpu_entry_default(struct lcpu *this_lcpu);

/**
 * Enqueue a function to the supplied LCPU
 *
 * @param lcpu the LCPU to enqueue the function to
 * @param fn the function to enqueue
 *
 * @return 0 on success, -errno otherwise
 */
int lcpu_fn_enqueue(struct lcpu *lcpu, const struct ukplat_lcpu_func *fn);
#endif /* CONFIG_HAVE_SMP */

/*
 * Definitions that must be satisfied by the architectural implementation.
 * DO NOT CALL DIRECTLY. Use the ukplat_* and lcpu_* non-architectural versions.
 */

/**
 * Return the hardware ID of the CPU executing this function. Must be able to
 * return the ID of the bootstrap processor without initialization of the MP
 * functions.
 */
__lcpuid lcpu_arch_id(void);

/**
 * Initialize the architectural part of the LCPU. The function is
 * executed on the CPU represented by the LCPU as part of lcpu_init().
 *
 * @param this_lcpu pointer to the LCPU structure representing the CPU
 *    executing this function
 * @return 0 on success, -errno otherwise
 */
int lcpu_arch_init(struct lcpu *this_lcpu);

/**
 * Switch to the specified stack and jump to the entry function
 *
 * @param sp new stack pointer
 * @param entry the function to jump to
 */
void __noreturn lcpu_arch_jump_to(void *sp, ukplat_lcpu_entry_t entry);

#ifdef CONFIG_HAVE_SMP
/**
 * Initialize the architectural part of the multi-processor functions. This
 * should perform CPU discovery and call lcpu_alloc() for each discovered CPU.
 * The bootstrap processor is already allocated with index 0 and must not be
 * added.
 *
 * @param arg an optional parameter from the boot code. Can be NULL
 * @return 0 on success, -errno otherwise
 */
int lcpu_arch_mp_init(void *arg);

/**
 * Start the given logical CPU. The CPU should execute the entry function
 * with the supplied stack. The CPU will be in INIT state.
 *
 * @param lcpu logical CPU to start
 * @param flags flags for controling how to start the given CPU
 *    (see UKPLAT_LCPU_SFLG_* if available)
 *
 * @return 0 on success, -errno otherwise
 */
int lcpu_arch_start(struct lcpu *lcpu, unsigned long flags);

#ifdef LCPU_ARCH_MULTI_PHASE_STARTUP
/**
 * An optional post start routine that is invoked by ukplat_lcpu_start() after
 * issuing a start command to all specified logical CPUs. This can be used on
 * architectures that have a multi-phase startup sequence like x86.
 *
 * @param lcpuidx the list of logical CPU indices specified in the call to
 *	ukplat_lcpu_start()
 * @param num the number of entries in the list
 *
 * @return 0 on success, -errno otherwise
 */
int lcpu_arch_post_start(const __lcpuidx lcpuidx[], unsigned int *num);
#endif /* LCPU_ARCH_MULTI_PHASE_STARTUP */

/**
 * Queue a function to the given logical CPU and send a run IRQ. The
 * implementation may also choose to handle the execution of the function
 * differently, for example, if certain flags are applied.
 *
 * @param lcpu the target logical CPU which should execute the function
 * @param fn the function to execute on the remote CPU
 * @param flags flags that control how the function should be run
 *    (see UKPLAT_LCPU_RFLG_* if available)
 *
 * @return 0 on success, -errno otherwise
 */
int lcpu_arch_run(struct lcpu *lcpu, const struct ukplat_lcpu_func *fn,
		  unsigned long flags);

/**
 * Send a wakeup IRQ to the specified logical CPU. The wakeup IRQ may be
 * implemented in such a way that the IRQ handler just acknowledges the IRQ and
 * immediately returns to keep the overhead minimal. However, in that case, it
 * must be guaranteed that no other IRQs (e.g., for devices) use the same
 * vector.
 *
 * @param lcpu pointer to the LCPU structure of the CPU to wake up
 * @return 0 on success, -errno otherwise
 */
int lcpu_arch_wakeup(struct lcpu *lcpu);
#endif /* CONFIG_HAVE_SMP */

#endif /* !__ASSEMBLY__ */

#if !CONFIG_PLAT_KVM
#define lcpu_get_current()		lcpu_get(0)
#endif /* !CONFIG_PLAT_KVM */

#endif /* __PLAT_CMN_LCPU_H__ */
