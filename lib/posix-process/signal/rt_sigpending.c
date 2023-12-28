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

UK_LLSYSCALL_R_DEFINE(int, rt_sigpending,
		      uk_sigset_t *, set,
		      size_t, sigsetsize)
{
	struct posix_thread *pthread;
	struct posix_process *proc;
	uk_sigset_t sigset;

	if (unlikely(sigsetsize != sizeof(*set)))
		return -EINVAL;

	if (unlikely(!set))
		return -EFAULT;

	pthread = uk_pthread_current();
	UK_ASSERT(pthread);

	proc = uk_pprocess_current();
	UK_ASSERT(proc);

	sigset = proc->signal->sigqueue.pending;
	uk_sigorset(&sigset, &pthread->signal->sigqueue.pending);

	for (int i = 0; i < SIGRTMIN; i++)
		if (IS_MASKED(pthread, i) && IS_IGNORED(proc, i))
			uk_sigdelset(&sigset, i);

	*set = sigset;

	return 0;
}
