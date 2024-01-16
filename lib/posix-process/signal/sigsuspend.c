/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <stddef.h>
#include <signal.h>
#include <sys/types.h>
#include <uk/syscall.h>

#include "signal.h"
#include "sigset.h"

UK_SYSCALL_R_DEFINE(int, rt_sigsuspend,
		    const uk_sigset_t *, mask,
		    size_t, sigsetsize)
{
	struct posix_thread *pthread;
	uk_sigset_t saved_mask;

	pthread = tid2pthread(uk_gettid());

	UK_ASSERT(pthread);

	if (unlikely(sigsetsize != sizeof(uk_sigset_t)))
		return -EINVAL;

	/* Temporarily replace mask */
	saved_mask = pthread->signal->mask;
	pthread->signal->mask = *mask;

	/* Wait for signal */
	pthread->state = POSIX_THREAD_BLOCKED_SIGNAL;
	uk_semaphore_down(&pthread->signal->deliver_semaphore);

	/* Restore mask */
	pthread->signal->mask = saved_mask;

	return -EINTR;
}
