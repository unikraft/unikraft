/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_SYSRETTAB_H__
#define __UK_SYSRETTAB_H__

#include <uk/assert.h>

struct uk_sysrettab_ctx {
	struct uk_syscall_ctx *usc;
};

typedef int (*uk_sysrettab_func_t)(struct uk_sysrettab_ctx *);

struct uk_sysrettab_entry {
	uk_sysrettab_func_t func;
};

extern const struct uk_sysrettab_entry uk_sysrettab_start[];
extern const struct uk_sysrettab_entry uk_sysrettab_end;

extern __thread unsigned long uk_syscallchain_count;

/**
 * Helper macro for iterating over system call return hooked functions.
 * Please note that the table may contain NULL pointer entries and that a
 * Unikraft system call return function is only called during the
 * exiting of the first called system call in the callchain.
 *
 * @param itr
 *   Iterator variable (struct uk_sysrettab_entry *) which points to the
 *   individual table entries during iteration
 * @param sysrettab_start
 *   Start address of table (type: const struct uk_sysrettab_entry[])
 * @param sysrettab_end
 *   End address of table (type: const struct uk_sysrettab_entry)
 */
#define uk_sysrettab_foreach(itr, sysrettab_start, sysrettab_end)	\
	for ((itr) = DECONST(struct uk_sysrettab_entry *, sysrettab_start);\
	     (itr) < &(sysrettab_end);					\
	     (itr)++)

/**
 * Register a Unikraft system call return function that is called during the
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
 *   System call return function to be called
 * @param prio
 *   Priority level (0 (earliest) to 9 (latest)), must be a constant.
 *   Use the UK_PRIO_AFTER()/UK_PRIO_BEFORE() helper macro for computing
 *   priority dependencies.
 *   Note: Any other value for level will be ignored
 */
#define __UK_SYSRETTAB_ENTRY(fn, prio)					\
	static const struct uk_sysrettab_entry				\
	__used __section(".uk_sysrettab" #prio) __align(8)		\
		__uk_sysrettab ## prio ## _ ## fn = {			\
		.func = (fn),						\
	}

#define _UK_SYSRETTAB(fn, prio)						\
	__UK_SYSRETTAB_ENTRY(fn, prio)

#define uk_sysretcall_prio(fn, prio)					\
	_UK_SYSRETTAB(fn, prio)

static inline void uk_sysrettab_run(struct uk_sysrettab_ctx *sysret_ctx)
{
	struct uk_sysrettab_entry *sysret_entry;
	int rc = 0;

	UK_ASSERT(sysret_ctx);

	/* This cannot happen, caller must have increased
	 * `uk_syscallchain_count` before calling `uk_sysrettab_run` to signal
	 * the addition of a syscall being added in the current thread's
	 * callchain.
	 */
	UK_ASSERT(uk_syscallchain_count);

	/* Only run this if the first system call in the callchain */
	if (uk_syscallchain_count != 1)
		return;

	uk_pr_debug_once("Sysret Table @ %p - %p\n",
			 &uk_sysrettab_start[0], &uk_sysrettab_end);

	uk_sysrettab_foreach(sysret_entry,
			     uk_sysrettab_start, uk_sysrettab_end) {
		UK_ASSERT(sysret_entry);

		if (!sysret_entry->func)
			continue;

		uk_pr_debug("Call sysret function: %p(%p)...\n",
			    sysret_entry->func, &sysret_ctx);
		rc = (*sysret_entry->func)(sysret_ctx);
		if (unlikely(rc < 0))
			UK_CRASH("Sysret function at %p returned error %d\n",
				 sysret_entry->func, rc);
	}
}

#endif /* __UK_SYSRETTAB_H__ */
