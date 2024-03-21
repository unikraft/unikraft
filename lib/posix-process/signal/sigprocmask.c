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

#include "process.h"
#include "signal.h"
#include "sigset.h"

UK_SYSCALL_R_DEFINE(int, rt_sigprocmask,
		    int, how,
		    const sigset_t *, set,
		    sigset_t *, oldset,
		    size_t, sigsetsize)
{
	struct posix_thread *thread;
	sigset_t tmpset;

	if (unlikely(sigsetsize != sizeof(sigset_t)))
		return -EINVAL;

	thread = tid2pthread(uk_gettid());
	UK_ASSERT(thread);

	if (oldset)
		*oldset = thread->signal->mask;

	if (!set)
		return 0;

	switch (how) {
	case SIG_BLOCK:
		uk_sigorset(&thread->signal->mask, set);
		break;
	case SIG_UNBLOCK:
		tmpset = *set;
		uk_sigreverseset(&tmpset);
		uk_sigandset(&thread->signal->mask, &tmpset);
		break;
	case SIG_SETMASK:
		uk_sigemptyset(&thread->signal->mask);
		uk_sigorset(&thread->signal->mask, set);
		break;
	default:
		return -EINVAL;
	}

	/* Ignore attempts to mask SIGKILL / SIGSTOP */
	uk_sigdelset(&thread->signal->mask, SIGKILL);
	uk_sigdelset(&thread->signal->mask, SIGSTOP);

	return 0;
}
