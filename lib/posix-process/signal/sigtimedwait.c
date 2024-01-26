/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <signal.h>
#include <stdlib.h>
#include <sys/time.h>

#include <uk/essentials.h>
#include <uk/syscall.h>
#include <uk/timeutil.h>

#include "process.h"
#include "signal.h"

UK_SYSCALL_R_DEFINE(int, rt_sigtimedwait,
		    const sigset_t *, set,
		    siginfo_t *, info,
		    const struct timespec *, timeout,
		    size_t, sigsetsize)
{
	struct posix_thread *pthread;
	struct uk_signal *sig;

	if (unlikely(!set))
		return -EINVAL;

	if (unlikely(sigsetsize != sizeof(sigset_t)))
		return -EINVAL;

	pthread = tid2pthread(uk_gettid());

	/* TODO protect concurrent access by deliver_pending_proc() */
	uk_sigcopyset(&pthread->signal->sigwait_set, set);

	/* If a signal in the set is already pending return immediately */
	if ((sig = pprocess_signal_next_pending_t(pthread)))
		goto out;

	if (timeout)
		uk_semaphore_down_to(&pthread->signal->pending_semaphore,
				     uk_time_spec_to_nsec(timeout));
	else
		uk_semaphore_down(&pthread->signal->pending_semaphore);

	if ((sig = pprocess_signal_next_pending_t(pthread)))
		goto out;

	return -EAGAIN;
out:
	if (info)
		*info = sig->siginfo;
	uk_sigemptyset(&pthread->signal->sigwait_set);
	return sig->siginfo.si_signo;
}
