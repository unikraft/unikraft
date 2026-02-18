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

#ifndef __UK_LCPU_CORE_H__
#define __UK_LCPU_CORE_H__

#include <uk/arch.h>
#include <uk/essentials.h>

#if !__ASSEMBLY__
#include <uk/arch/time.h>

#ifdef __cplusplus
extern "C" {
#endif

#endif /* !__ASSEMBLY__ */

/* Provide empty architecture-dependent UK_LCPU part as default */
#ifndef UK_LCPU_ARCH_SIZE
#define UK_LCPU_ARCH_SIZE		0x00

#if !__ASSEMBLY__
struct uk_lcpu_arch { };
#endif /* !__ASSEMBLY__ */

#endif /* !UK_LCPU_ARCH_SIZE */

/*
 * UK_LCPU Startup Arguments
 */
#define UK_LCPU_SARGS_ENTRY_OFFSET	0x00
#define UK_LCPU_SARGS_STACKP_OFFSET					\
	(UK_LCPU_SARGS_ENTRY_OFFSET + 0x08)

#define UK_LCPU_SARGS_SIZE		0x10

#if !__ASSEMBLY__
struct uk_lcpu_sargs {
	__uptr entry;
	__uptr stackp;
};

UK_CTASSERT(__offsetof(struct uk_lcpu_sargs, entry)  ==
	    UK_LCPU_SARGS_ENTRY_OFFSET);
UK_CTASSERT(__offsetof(struct uk_lcpu_sargs, stackp) ==
	    UK_LCPU_SARGS_STACKP_OFFSET);

UK_CTASSERT(sizeof(struct uk_lcpu_sargs) == UK_LCPU_SARGS_SIZE);
#endif /* !__ASSEMBLY__ */

/*
 * Logical CPU (LCPU) Structure
 */
#define UK_LCPU_STATE_OFFSET		0x00
#define UK_LCPU_ENTRY_OFFSET		(UK_LCPU_STATE_OFFSET + 0x08)
#define UK_LCPU_STACKP_OFFSET		(UK_LCPU_ENTRY_OFFSET + 0x08)
#define UK_LCPU_ERR_OFFSET		(UK_LCPU_ENTRY_OFFSET + 0x0)
#define UK_LCPU_ARCH_OFFSET		(UK_LCPU_ENTRY_OFFSET + 0x10)

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

#define UK_LCPU_MEMBERS_SIZE						\
	(UK_LCPU_ARCH_OFFSET  + UK_LCPU_ARCH_SIZE)
#define UK_LCPU_SIZE							\
	(ALIGN_UP(UK_LCPU_MEMBERS_SIZE, UK_ARCH_CACHE_LINE_SIZE))

#if !__ASSEMBLY__
struct __align(UK_ARCH_CACHE_LINE_SIZE) uk_lcpu {
	/* Current CPU state (UK_LCPU_STATE_*).
	 * Working on it with atomic instructions - must be 8-byte aligned
	 */
	volatile int state __align(8);

	union {
		/* Startup arguments
		 * Only valid in UK_LCPU_STATE_INIT
		 */
		struct uk_lcpu_sargs s_args;

		/* Remote function to execute
		 * Only valid in UK_LCPU_STATE_IDLE and busy states
		 */
#if CONFIG_HAVE_SMP
		struct uk_lcpu_func fn;
#endif /* CONFIG_HAVE_SMP */

		/* Error code indicating the halt reason
		 * Only valid in UK_LCPU_STATE_HALTED
		 */
		int error_code;
	};

	/* Architecture-dependent part */
	struct uk_lcpu_arch arch;
};

UK_CTASSERT(__offsetof(struct uk_lcpu, state) == UK_LCPU_STATE_OFFSET);
UK_CTASSERT(__offsetof(struct uk_lcpu, s_args.entry) == UK_LCPU_ENTRY_OFFSET);
UK_CTASSERT(__offsetof(struct uk_lcpu, s_args.stackp) == UK_LCPU_STACKP_OFFSET);
UK_CTASSERT(__offsetof(struct uk_lcpu, error_code) == UK_LCPU_ERR_OFFSET);
UK_CTASSERT(__offsetof(struct uk_lcpu, arch) == UK_LCPU_ARCH_OFFSET);

UK_CTASSERT(sizeof(struct uk_lcpu) == UK_LCPU_SIZE);
UK_CTASSERT(UK_LCPU_MEMBERS_SIZE <= UK_LCPU_SIZE);
#endif /* !__ASSEMBLY__ */

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
static inline int uk_lcpu_is_bsp(struct uk_lcpu *lcpu)
{
	return (lcpu == uk_lcpu_get_bsp());
}

/**
 * Return 1 if the executed on the bootstrap processor, 0 otherwise
 */
static inline int uk_lcpu_current_is_bsp(void)
{
	return uk_lcpu_is_bsp(uk_lcpu_get_current());
}

#if CONFIG_HAVE_SMP
/* The IRQ vectors passed to lcpu_mp_init */
extern const unsigned long * const uk_lcpu_run_irqv;
extern const unsigned long * const uk_lcpu_wakeup_irqv;
#endif /* CONFIG_HAVE_SMP */

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_LCPU_CORE_H__ */
