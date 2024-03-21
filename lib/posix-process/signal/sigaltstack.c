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

	if (ss && ss->ss_flags && (unsigned int)ss->ss_flags != SS_AUTODISARM)
		return -EINVAL;

	if (ss && ss->ss_size < MINSIGSTKSZ)
		return -ENOMEM;

	if (ss && (unsigned int)ss->ss_flags == SS_AUTODISARM)
		uk_pr_warn("SS_AUTODISARM stubbed\n");

	/* A thread with an altstck configures switches its
	 * altstack state to SS_ONSTACK while handing a signal.
	 * During that time it is not allowed to modify the altstack.
	 */
	uk_mutex_lock(&proc->signal->lock);

	if (ss && proc->signal->altstack.ss_flags == SS_ONSTACK) {
		uk_mutex_unlock(&proc->signal->lock);
		return -EPERM;
	}

	if (old_ss)
		memcpy(old_ss, &proc->signal->altstack, sizeof(*old_ss));

	if (ss)
		memcpy(&proc->signal->altstack, ss, sizeof(*ss));

	uk_mutex_unlock(&proc->signal->lock);

	return 0;
}
