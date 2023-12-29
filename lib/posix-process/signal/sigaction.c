/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>
#include <signal.h>
#include <string.h>

#include <uk/essentials.h>
#include <uk/process.h>
#include <uk/syscall.h>

#include "signal.h"
#include "sigset.h"

UK_SYSCALL_R_DEFINE(int, rt_sigaction, int, signum,
		    const struct uk_sigaction *, act,
		    struct uk_sigaction *, oldact,
		    size_t, sigsetsize)
{
	struct posix_process *proc;

	if (unlikely(!IS_VALID(signum)))
		return -EINVAL;

	if (unlikely(signum == SIGKILL || signum == SIGSTOP))
		return -EINVAL;

	if (unlikely(sigsetsize != sizeof(uk_sigset_t)))
		return -EINVAL;

	proc = tid2pprocess(uk_gettid());
	UK_ASSERT(proc);

	if (oldact)
		*oldact = proc->signal->sigaction[signum];

	if (!act)
		return 0;

	if (act->ks_flags & SA_NOCLDSTOP)
		uk_pr_warn("ks_flags: SA_NOCLDSTOP not supported, ignoring\n");

	if (act->ks_flags & SA_RESTART)
		uk_pr_warn("ks_flags: SA_RESTART not supported, ignoring\n");

	/* TODO Don't update sigaction while executing a signal handler */

	proc->signal->sigaction[signum] = *act;

	/* Clear pending signals if the action has been changed to SIG_IGN */
	if (IS_IGNORED(proc, signum))
		pprocess_signal_clear_pending(proc, signum);

	return 0;
}
