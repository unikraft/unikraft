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

	proc = uk_pprocess_current();
	UK_ASSERT(proc);

	if (ss) {
		if (unlikely(ss->ss_flags &&
			     !((unsigned int)ss->ss_flags == SS_AUTODISARM ||
			       (unsigned int)ss->ss_flags == SS_DISABLE)))
			return -EINVAL;

		if (unlikely(ss->ss_size < MINSIGSTKSZ))
			return -ENOMEM;

		/* See BUGS in SIGALTSTACK(2) */
		if (unlikely(ss->ss_flags & SS_ONSTACK))
			return -EPERM;

		if (unlikely(proc->signal->altstack.ss_flags == SS_ONSTACK))
			return -EPERM;

		if ((unsigned int)ss->ss_flags == SS_AUTODISARM)
			uk_pr_warn("SS_AUTODISARM stubbed\n");

		if ((unsigned int)ss->ss_flags == SS_DISABLE) {
			proc->signal->altstack.ss_flags |= SS_DISABLE;
			return 0;
		}

		/* TODO Don't allow updating the altstack if we are executing
		 * on it already.
		 */

		proc->signal->altstack = *ss;
	}

	if (old_ss)
		*old_ss = proc->signal->altstack;

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
