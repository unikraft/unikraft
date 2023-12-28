/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>
#include <signal.h>

#include <uk/essentials.h>
#include <uk/process.h>
#include <uk/syscall.h>

#include "signal.h"
#include "sigset.h"

UK_SYSCALL_R_DEFINE(int, rt_sigpending,
		    sigset_t *, set,
		    size_t, sigsetsize)
{
	struct posix_thread *thread;
	struct posix_process *proc;
	sigset_t sigset;

	if (unlikely(sigsetsize != sizeof(sigset_t)))
		return -EINVAL;

	thread = tid2pthread(uk_gettid());
	UK_ASSERT(thread);

	proc = tid2pprocess(uk_getpid());
	UK_ASSERT(proc);

	sigset = proc->signal->sigqueue.pending;
	uk_sigorset(&sigset, &thread->signal->sigqueue.pending);

	for (int i = 0; i < SIGRTMIN; i++)
		if (IS_MASKED(thread, i) && IS_IGNORED(proc, i))
			uk_sigdelset(&sigset, i);

	*set = sigset;

	return 0;
}
