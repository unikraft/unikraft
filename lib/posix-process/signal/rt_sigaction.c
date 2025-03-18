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

#include "process.h"
#include "signal.h"
#include "sigset.h"

#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
UK_LLSYSCALL_R_DEFINE(int, rt_sigaction, int, signum,
		      const struct kern_sigaction *, act,
		      struct kern_sigaction *, oldact,
		      size_t, sigsetsize)
{
	struct posix_process *proc;

	if (unlikely(!IS_VALID(signum)))
		return -EINVAL;

	if (unlikely(signum == SIGKILL || signum == SIGSTOP))
		return -EINVAL;

	if (unlikely(sigsetsize != sizeof(uk_sigset_t)))
		return -EINVAL;

	proc = uk_pprocess_current();
	UK_ASSERT(proc);

	if (oldact)
		memcpy(oldact, KERN_SIGACTION(proc, signum),
		       sizeof(struct kern_sigaction));

	if (!act)
		return 0;

	if (act->ks_flags & SA_NOCLDSTOP)
		uk_pr_warn("ks_flags: SA_NOCLDSTOP not supported, ignoring\n");

	if (act->ks_flags & SA_RESTART)
		uk_pr_warn("ks_flags: SA_RESTART not supported, ignoring\n");

	/* TODO Don't update sigaction while executing a signal handler */

	memcpy(KERN_SIGACTION(proc, signum), act,
	       sizeof(struct kern_sigaction));

	/* Clear pending signals if the action has been changed to SIG_IGN */
	if (IS_IGNORED(proc, signum))
		pprocess_signal_clear_pending(proc, signum);

	return 0;
}
#else /* !CONFIG_LIBPOSIX_PROCESS_SIGNAL */
UK_LLSYSCALL_R_DEFINE(int, rt_sigaction, int, signum,
		      const struct kern_sigaction *, act,
		      struct kern_sigaction *, oldact,
		      size_t, sigsetsize)
{
	UK_WARN_STUBBED();
	if (oldact)
		*oldact = (struct kern_sigaction){0};

	return 0;
}
#endif /* !CONFIG_LIBPOSIX_PROCESS_SIGNAL */
