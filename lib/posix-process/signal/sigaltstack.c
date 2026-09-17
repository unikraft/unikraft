/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <signal.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>

#include <uk/process.h>
#include <uk/syscall.h>

#include "signal.h"

#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
UK_SYSCALL_R_DEFINE(int, sigaltstack, const stack_t *, ss,
		    stack_t *, old_ss)
{
	struct posix_process *proc;
	stack_t old;

	proc = uk_pprocess_current();
	UK_ASSERT(proc);

	/* The state reported: what it was before this call, as on Linux,
	 * and SS_DISABLE alone for a record with no stack behind it -- such
	 * a record must never read as installed, or a program that asks
	 * before installing its own (the .NET runtime does) trusts the
	 * answer and leaves its SA_ONSTACK handlers with nowhere to run.
	 */
	old = proc->signal->altstack;
	if (!old.ss_sp)
		old = (stack_t){ .ss_flags = SS_DISABLE };

	if (ss) {
		if (unlikely(ss->ss_flags &&
			     !((unsigned int)ss->ss_flags == SS_AUTODISARM ||
			       (unsigned int)ss->ss_flags == SS_DISABLE)))
			return -EINVAL;

		/* See BUGS in SIGALTSTACK(2) */
		if (unlikely(ss->ss_flags & SS_ONSTACK))
			return -EPERM;

		if (unlikely(proc->signal->altstack.ss_flags & SS_ONSTACK))
			return -EPERM;

		if ((unsigned int)ss->ss_flags == SS_DISABLE) {
			/* Pointer and size are not looked at when disabling,
			 * as on Linux: callers pass zeros.
			 */
			proc->signal->altstack = (stack_t){ .ss_flags = SS_DISABLE };
		} else {
			if (unlikely(ss->ss_size < MINSIGSTKSZ))
				return -ENOMEM;

			if ((unsigned int)ss->ss_flags == SS_AUTODISARM)
				uk_pr_warn("SS_AUTODISARM stubbed\n");

			/* TODO Don't allow updating the altstack if we are
			 * executing on it already.
			 */
			proc->signal->altstack = *ss;
		}
	}

	if (old_ss)
		*old_ss = old;

	return 0;
}

#else /* !CONFIG_LIBPOSIX_PROCESS_SIGNAL */

UK_SYSCALL_R_DEFINE(int, sigaltstack, const stack_t *, ss,
		    stack_t *, old_ss)
{
	UK_WARN_STUBBED();
	return 0;
}
#endif /* !CONFIG_LIBPOSIX_PROCESS_SIGNAL */
