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

#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
UK_LLSYSCALL_R_DEFINE(int, rt_sigprocmask,
		      int, how,
		      const uk_sigset_t *, set,
		      uk_sigset_t *, oldset,
		      size_t, sigsetsize)
{
	struct posix_thread *pthread;
	uk_sigset_t tmpset;

	if (unlikely(sigsetsize != sizeof(uk_sigset_t)))
		return -EINVAL;

	pthread = uk_pthread_current();
	UK_ASSERT(pthread);

	if (oldset)
		*oldset = pthread->signal->mask;

	if (!set)
		return 0;

	/* copy to mutable set */
	uk_sigcopyset(&tmpset, set);

	/* ignore attempts to mask SIGKILL / SIGSTOP */
	uk_sigdelset(&tmpset, SIGKILL);
	uk_sigdelset(&tmpset, SIGSTOP);

	/* update */
	switch (how) {
	case SIG_BLOCK:
		uk_sigorset(&pthread->signal->mask, &tmpset);
		break;
	case SIG_UNBLOCK:
		uk_sigreverseset(&tmpset);
		uk_sigandset(&pthread->signal->mask, &tmpset);
		break;
	case SIG_SETMASK:
		uk_sigcopyset(&pthread->signal->mask, &tmpset);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}
#else /* !CONFIG_LIBPOSIX_PROCESS_SIGNAL */
UK_LLSYSCALL_R_DEFINE(int, rt_sigprocmask,
		      int, how,
		      const uk_sigset_t *, set,
		      uk_sigset_t *, oldset,
		      size_t, sigsetsize)
{
	UK_WARN_STUBBED();
	return 0;
}
#endif /* !CONFIG_LIBPOSIX_PROCESS_SIGNAL */

#if UK_LIBC_SYSCALLS
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
{
	return uk_syscall_e_rt_sigprocmask(how, (long)set, (long)oldset,
					   sizeof(uk_sigset_t));
}
#endif /* UK_LIBC_SYSCALLS */
