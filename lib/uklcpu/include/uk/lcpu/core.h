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

#define UK_LCPU_IS_LCPU_PTR(ptr)					\
	(IN_RANGE((__uptr)(ptr),					\
		  (__uptr)uk_lcpu_get(0),				\
		  (__uptr)CONFIG_UKPLAT_CPU_MAXCOUNT * sizeof(struct uk_lcpu)))

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
#define UK_LCPU_IDX_OFFSET		(UK_LCPU_STATE_OFFSET + 0x04)
#define UK_LCPU_ID_OFFSET		(UK_LCPU_IDX_OFFSET   + 0x04)
#define UK_LCPU_ENTRY_OFFSET		(UK_LCPU_ID_OFFSET    + 0x08)
#define UK_LCPU_STACKP_OFFSET		(UK_LCPU_ENTRY_OFFSET + 0x08)
#define UK_LCPU_ERR_OFFSET		(UK_LCPU_ENTRY_OFFSET + 0x00)
/* TODO: See comment from syscall_prologue.h architecture specific files */
#ifdef UK_LCPU_AUXSP_OFFSET
#undef UK_LCPU_AUXSP_OFFSET
#endif /* UK_LCPU_AUXSP_OFFSET */
#define UK_LCPU_AUXSP_OFFSET		(UK_LCPU_ENTRY_OFFSET + 0x10)
#define UK_LCPU_ARCH_OFFSET		(UK_LCPU_ENTRY_OFFSET + 0x18)

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
	ALIGN_UP(UK_LCPU_MEMBERS_SIZE, UK_ARCH_CACHE_LINE_SIZE)

#if !__ASSEMBLY__
struct __align(UK_ARCH_CACHE_LINE_SIZE) uk_lcpu {
	/* Current CPU state (UK_LCPU_STATE_*).
	 * Working on it with atomic instructions - must be 8-byte aligned
	 */
	volatile int state __align(8);

	__u32 idx;
	__u64 id;

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

	/*
	 * Auxiliary stack pointer of the thread currently executing on
	 * UK_LCPU
	 */
	__uptr auxsp;

	/* Architecture-dependent part */
	struct uk_lcpu_arch arch;
};

UK_CTASSERT(__offsetof(struct uk_lcpu, state) == UK_LCPU_STATE_OFFSET);
UK_CTASSERT(__offsetof(struct uk_lcpu, idx) == UK_LCPU_IDX_OFFSET);
UK_CTASSERT(__offsetof(struct uk_lcpu, id) == UK_LCPU_ID_OFFSET);
UK_CTASSERT(__offsetof(struct uk_lcpu, s_args.entry) == UK_LCPU_ENTRY_OFFSET);
UK_CTASSERT(__offsetof(struct uk_lcpu, s_args.stackp) == UK_LCPU_STACKP_OFFSET);
UK_CTASSERT(__offsetof(struct uk_lcpu, error_code) == UK_LCPU_ERR_OFFSET);
UK_CTASSERT(__offsetof(struct uk_lcpu, auxsp) == UK_LCPU_AUXSP_OFFSET);
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
 * Return the UK_LCPU structure for the logical CPU with the given index.
 *
 * @param idx the index of the requested UK_LCPU. The index must be less than
 *    the value returned by uk_lcpu_count(), otherwise behavior is
 *    undefined
 * @return pointer to the requested UK_LCPU structure
 */
struct uk_lcpu *uk_lcpu_get(__u32 idx);

#define _lcpu_lcpuidx_list_entry(list, i, n)				\
	(((i) < (n)) ?							\
	  uk_lcpu_get((list) ? (list)[i] : (__u32)(i))		\
	  : __NULL)

#define uk_lcpu_lcpuidx_list_foreach(list, num, n, i, lcpu)		\
	if ((num) == __NULL) {						\
		UK_ASSERT(!(list));					\
		(n) = uk_lcpu_count();					\
	} else	{							\
		UK_ASSERT((*num) <= uk_lcpu_count());			\
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
 * Return the UK_LCPU structure for the CPU executing this function
 */
struct uk_lcpu *uk_lcpu_get_current(void);

/**
 * Return the UK_LCPU structure for the bootstraping CPU
 */
static inline struct uk_lcpu *uk_lcpu_get_bsp(void)
{
	/* The BSP is always index 0 */
	return uk_lcpu_get(0);
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

/**
 * Returns the number of logical CPUs present in the system
 */
__u32 uk_lcpu_count(void);
#else /* !CONFIG_HAVE_SMP */
#define uk_lcpu_count()	(1)
#endif /* !CONFIG_HAVE_SMP */

/* Per-LCPU variable definition */

#define UK_PER_LCPU_DEFINE(var_type, var_name)				\
	var_type var_name[CONFIG_UKPLAT_CPU_MAXCOUNT]
#define uk_per_lcpu(var_name, lcpu_idx)					\
	var_name[lcpu_idx]
#define uk_per_lcpu_current(var_name)					\
	uk_per_lcpu(var_name, uk_pal_lcpu_idx())

#define UK_PER_LCPU_ARRAY_DEFINE(var_type, var_name, size)		\
	var_type var_name[CONFIG_UKPLAT_CPU_MAXCOUNT][size]
#define uk_per_lcpu_array(var_name, lcpu_idx, idx)			\
	var_name[lcpu_idx][idx]
#define uk_per_lcpu_array_current(var_name, idx)			\
	uk_per_lcpu_array(var_name, uk_pal_lcpu_idx(), idx)

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_LCPU_CORE_H__ */
