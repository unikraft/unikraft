/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_SYSEXITTAB_H__
#define __UK_SYSEXITTAB_H__

#include <uk/assert.h>

struct uk_sysexittab_ctx {
	struct ukarch_execenv *execenv;
};

typedef void (*uk_sysexittab_func_t)(struct uk_sysexittab_ctx *);

struct uk_sysexittab_entry {
	uk_sysexittab_func_t func;
};

extern const struct uk_sysexittab_entry uk_sysexittab_start[];
extern const struct uk_sysexittab_entry uk_sysexittab_end;

extern __thread unsigned long uk_syscallchain_count;

/**
 * Helper macro for iterating over system call exit hooked functions.
 * Please note that the table may contain NULL pointer entries and that a
 * Unikraft system call exit function is only called during the
 * exiting of the first called system call in the callchain (i.e. if a system
 * call calls another system call in Unikraft context (although it shouldn't)
 * the routines in this table is iterated upon only when exiting the first
 * called system call).
 *
 * @param itr
 *   Iterator variable (struct uk_sysexittab_entry *) which points to the
 *   individual table entries during iteration
 * @param sysexittab_start
 *   Start address of table (type: const struct uk_sysexittab_entry[])
 * @param sysexittab_end
 *   End address of table (type: const struct uk_sysexittab_entry)
 */
#define uk_sysexittab_foreach(itr, sysexittab_start, sysexittab_end)	\
	for ((itr) = DECONST(struct uk_sysexittab_entry *, sysexittab_start);\
	     (itr) < &(sysexittab_end);					\
	     (itr)++)

/**
 * Register a Unikraft system call exit function that is called during the
 * exiting of the first called system call in the callchain. For example,
 * if a binary system call is called and during its handling some other
 * native system calls (just the function calls, no `syscall`/`svc`/etc
 * instructions involved) are called during the handling of the binary system
 * call, then this entry is executed only once, during the exit of the binary
 * system call's exit. Likewise, if in the current callchain there are only
 * native system calls, then the entry will get executed during the exit of the
 * first called system call.
 *
 * @param fn
 *   System call exit function to be called
 * @param prio
 *   Priority level (0 (earliest) to 9 (latest)), must be a constant.
 *   Use the UK_PRIO_AFTER()/UK_PRIO_BEFORE() helper macro for computing
 *   priority dependencies.
 *   Note: Any other value for level will be ignored
 */
#define __UK_SYSEXITTAB_ENTRY(fn, prio)					\
	static const struct uk_sysexittab_entry				\
	__used __section(".uk_sysexittab" #prio) __align(8)		\
		__uk_sysexittab ## prio ## _ ## fn = {			\
		.func = (fn),						\
	}

#define _UK_SYSEXITTAB(fn, prio)						\
	__UK_SYSEXITTAB_ENTRY(fn, prio)

#define uk_sysexitcall_prio(fn, prio)					\
	_UK_SYSEXITTAB(fn, prio)

static inline void uk_sysexittab_run(struct uk_sysexittab_ctx *sysexit_ctx)
{
	struct uk_sysexittab_entry *sysexit_entry;

	UK_ASSERT(sysexit_ctx);

	/* This cannot happen, caller must have increased
	 * `uk_syscallchain_count` before calling `uk_sysexittab_run` to signal
	 * the addition of a syscall being added in the current thread's
	 * callchain.
	 */
	UK_ASSERT(uk_syscallchain_count);

	/* Only run this if the first system call in the callchain */
	if (uk_syscallchain_count != 1)
		return;

	uk_pr_debug_once("Sysexit Table @ %p - %p\n",
			 &uk_sysexittab_start[0], &uk_sysexittab_end);

	uk_sysexittab_foreach(sysexit_entry,
			      uk_sysexittab_start, uk_sysexittab_end) {
		UK_ASSERT(sysexit_entry);

		if (!sysexit_entry->func)
			continue;

		uk_pr_debug("Call sysexit function: %p(%p)...\n",
			    sysexit_entry->func, sysexit_ctx);
		(*sysexit_entry->func)(sysexit_ctx);
	}
}

#endif /* __UK_SYSEXITTAB_H__ */
