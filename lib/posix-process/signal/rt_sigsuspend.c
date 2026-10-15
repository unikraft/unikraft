/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <stddef.h>
#include <signal.h>
#include <sys/types.h>
#include <uk/process.h>
#include <uk/syscall.h>

#include "signal.h"
#include "sigset.h"

#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
int uk_sys_rt_sigsuspend(const void *sigmask, size_t sigsetsize)
{
	const uk_sigset_t *mask = sigmask;
	struct posix_thread *pthread;
	uk_sigset_t suspend_mask;
	uk_sigset_t saved_mask;

	if (unlikely(sigsetsize != sizeof(uk_sigset_t)))
		return -EINVAL;

	pthread = uk_pthread_current();
	if (unlikely(!pthread))
		return -ENOTSUP;

	/* It is not possible to block SIGKILL & SIGSTOP */
	uk_sigcopyset(&suspend_mask, mask);
	uk_sigdelset(&suspend_mask, SIGKILL);
	uk_sigdelset(&suspend_mask, SIGSTOP);

	/* Temporarily replace mask */
	saved_mask = pthread->signal->mask;
	pthread->signal->mask = suspend_mask;

	/* Wait for signal */
	pthread_signal_pause(pthread);

	/* The signal that woke us is delivered on the way out, under the
	 * suspend mask, which unblocks it: the old mask comes back after
	 * that, as on Linux.  Put back here, it would block the signal
	 * again, and it would stay queued with its handler never run.
	 */
	pthread->signal->sigsuspend_saved = saved_mask;
	pthread->signal->sigsuspend_restore = true;

	return -EINTR;
}
#else /* !CONFIG_LIBPOSIX_PROCESS_SIGNAL */
int uk_sys_rt_sigsuspend(const void *sigmask __unused,
			 size_t sigsetsize __unused)
{
	UK_WARN_STUBBED();
	return -EINTR;
}
#endif /* !CONFIG_LIBPOSIX_PROCESS_SIGNAL */

UK_LLSYSCALL_R_DEFINE(int, rt_sigsuspend,
		      const uk_sigset_t *, mask,
		      size_t, sigsetsize)
{
	return uk_sys_rt_sigsuspend(mask, sigsetsize);
}
