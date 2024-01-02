/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <stddef.h>
#include <string.h>
#include <sys/types.h>

#include <uk/mutex.h>
#include <uk/process.h>
#include <uk/syscall.h>

#include "signal.h"

UK_SYSCALL_R_DEFINE(int, sigaltstack, const stack_t *, ss,
		    stack_t *, old_ss)
{
	struct posix_process *proc;

	proc = pid2pprocess(uk_getpid());
	UK_ASSERT(proc);

	if (unlikely(ss && ss->ss_flags &&
		     !((unsigned int)ss->ss_flags == SS_AUTODISARM ||
		       (unsigned int)ss->ss_flags == SS_DISABLE)))
		return -EINVAL;

	if (ss && (unsigned int)ss->ss_flags == SS_DISABLE) {
		proc->signal->altstack.ss_flags |= SS_DISABLE;
		return 0;
	}

	if (ss && (unsigned int)ss->ss_flags == SS_AUTODISARM)
		uk_pr_warn("SS_AUTODISARM stubbed\n");

	if (ss && ss->ss_size < MINSIGSTKSZ)
		return -ENOMEM;

	/* See BUGS in SIGALTSTACK(2) */
	if (ss && (ss->ss_flags & SS_ONSTACK))
		return -EPERM;

	/* TODO Don't allow updating the altstack if we are executing
	 * on it already.
	 */

	if (ss && proc->signal->altstack.ss_flags == SS_ONSTACK) {
		uk_mutex_unlock(&proc->signal->lock);
		return -EPERM;
	}

	if (old_ss)
		*old_ss = proc->signal->altstack;

	if (ss)
		proc->signal->altstack = *ss;

	return 0;
}
