/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_SYSENTERTAB_H__
#define __UK_SYSENTERTAB_H__

#include <uk/assert.h>

struct uk_sysentertab_ctx {
	struct ukarch_execenv *execenv;
};

typedef void (*uk_sysentertab_func_t)(struct uk_sysentertab_ctx *);

struct uk_sysentertab_entry {
	uk_sysentertab_func_t func;
};

extern const struct uk_sysentertab_entry uk_sysentertab_start[];
extern const struct uk_sysentertab_entry uk_sysentertab_end;

extern __thread unsigned long uk_syscallchain_count;

/**
 * Helper macro for iterating over system call enter hooked functions.
 * Please note that the table may contain NULL pointer entries and that a
 * Unikraft system call enter function is only called during the
 * entering of the first called system call in the callchain (i.e. if a system
 * call calls another system call in Unikraft context (although it shouldn't)
 * the routines in this table is iterated upon only when entering the first
 * called system call).
 *
 * @param itr
 *   Iterator variable (struct uk_sysentertab_entry *) which points to the
 *   individual table entries during iteration
 * @param sysentertab_start
 *   Start address of table (type: const struct uk_sysentertab_entry[])
 * @param sysentertab_end
 *   End address of table (type: const struct uk_sysentertab_entry)
 */
#define uk_sysentertab_foreach(itr, sysentertab_start, sysentertab_end)	\
	for ((itr) = DECONST(struct uk_sysentertab_entry *, sysentertab_start);\
	     (itr) < &(sysentertab_end);					\
	     (itr)++)

/**
 * Register a Unikraft system call enter function that is called during the
 * entering of the first called system call in the callchain. For example,
 * if a binary system call is called and during its handling some other
 * native system calls (just the function calls, no `syscall`/`svc`/etc
 * instructions involved) are called during the handling of the binary system
 * call, then this entry is executed only once, during the enter of the binary
 * system call. Likewise, if in the current callchain there are only
 * native system calls, then the entry will get executed during the entering
 * of the first called system call.
 *
 * @param fn
 *   System call enter function to be called
 * @param prio
 *   Priority level (0 (earliest) to 9 (latest)), must be a constant.
 *   Use the UK_PRIO_AFTER()/UK_PRIO_BEFORE() helper macro for computing
 *   priority dependencies.
 *   Note: Any other value for level will be ignored
 */
#define __UK_SYSENTERTAB_ENTRY(fn, prio)					\
	static const struct uk_sysentertab_entry				\
	__used __section(".uk_sysentertab" #prio) __align(8)		\
		__uk_sysentertab ## prio ## _ ## fn = {			\
		.func = (fn),						\
	}

#define _UK_SYSENTERTAB(fn, prio)						\
	__UK_SYSENTERTAB_ENTRY(fn, prio)

#define uk_sysentercall_prio(fn, prio)					\
	_UK_SYSENTERTAB(fn, prio)

static inline void uk_sysentertab_run(struct uk_sysentertab_ctx *sysenter_ctx)
{
	struct uk_sysentertab_entry *sysenter_entry;

	UK_ASSERT(sysenter_ctx);

	/* This cannot happen, caller must have increased
	 * `uk_syscallchain_count` before calling `uk_sysentertab_run` to signal
	 * the addition of a syscall being added in the current thread's
	 * callchain.
	 */
	UK_ASSERT(uk_syscallchain_count);

	/* Only run this if the first system call in the callchain */
	if (uk_syscallchain_count != 1)
		return;

	uk_pr_debug_once("Sysenter Table @ %p - %p\n",
			 &uk_sysentertab_start[0], &uk_sysentertab_end);

	uk_sysentertab_foreach(sysenter_entry,
			      uk_sysentertab_start, uk_sysentertab_end) {
		UK_ASSERT(sysenter_entry);

		if (!sysenter_entry->func)
			continue;

		uk_pr_debug("Call sysenter function: %p(%p)...\n",
			    sysenter_entry->func, sysenter_ctx);
		(*sysenter_entry->func)(sysenter_ctx);
	}
}

#endif /* __UK_SYSENTERTAB_H__ */
