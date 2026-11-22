/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <signal.h>
#include <stdlib.h>
#include <sys/time.h>

#include <uk/essentials.h>
#include <uk/plat/time.h>
#include <uk/syscall.h>
#include <uk/timeutil.h>

#if !CONFIG_LIBPOSIX_PROCESS_SIGNAL
#include <uk/sched.h>
#endif /* !CONFIG_LIBPOSIX_PROCESS_SIGNAL */

#include "process.h"
#include "signal.h"

#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
UK_LLSYSCALL_R_DEFINE(int, rt_sigtimedwait,
		      const uk_sigset_t *, set,
		      siginfo_t *, info,
		      const struct timespec *, timeout,
		      size_t, sigsetsize)
{
	struct posix_thread *pthread;
	__snsec deadline = 0;

	if (unlikely(!set || sigsetsize != sizeof(*set)))
		return -EINVAL;
	if (timeout) {
		__nsec now = ukplat_monotonic_clock();
		__nsec remaining = __SNSEC_MAX - now;
		__nsec seconds = timeout->tv_sec;
		__nsec interval;

		if (unlikely(!(uk_time_spec_canonical(timeout) &&
			       uk_time_spec_positive(timeout))))
			return -EINVAL;
		if (seconds > remaining / UKARCH_NSEC_PER_SEC) {
			deadline = __SNSEC_MAX;
		} else {
			interval = ukarch_time_sec_to_nsec(seconds);
			interval += timeout->tv_nsec;
			deadline = now + MIN(remaining, interval);
			if (!deadline)
				deadline = 1;
		}
	}

	pthread = uk_pthread_current();
	UK_ASSERT(pthread);
	return pprocess_signal_wait_set(pthread, *set, deadline, info);
}
#else /* !CONFIG_LIBPOSIX_PROCESS_SIGNAL */
UK_LLSYSCALL_R_DEFINE(int, rt_sigtimedwait,
		      const uk_sigset_t *, set,
		      siginfo_t *, info,
		      const struct timespec *, timeout,
		      size_t, sigsetsize)
{
	struct uk_thread *current = uk_thread_current();
	__nsec timeout_ns;
	int rc;

	if (timeout) {
		if (unlikely(!(uk_time_spec_canonical(timeout) &&
			       uk_time_spec_positive(timeout))))
			return -EINVAL;
		timeout_ns = ukplat_monotonic_clock() +
			ukarch_time_sec_to_nsec(timeout->tv_sec) +
			timeout->tv_nsec;
		uk_thread_block_until(current, (__snsec)timeout_ns);
		uk_sched_yield();
		rc = -EAGAIN;
	} else {
		uk_thread_block(current);
		uk_sched_yield();
		/* If this ever returns then just return an EINTR to not
		 * confuse the application
		 */
		rc = -EINTR;
	}

	/* We currently never return a signal */
	*info = (siginfo_t){0};
	return rc;
}
#endif /* !CONFIG_LIBPOSIX_PROCESS_SIGNAL */
