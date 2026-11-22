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

#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
UK_LLSYSCALL_R_DEFINE(int, rt_sigsuspend,
		      const uk_sigset_t *, mask,
		      size_t, sigsetsize)
{
	struct posix_thread *pthread;
	uk_sigset_t suspend_mask;

	pthread = uk_pthread_current();
	UK_ASSERT(pthread);

	if (unlikely(!mask || sigsetsize != sizeof(*mask)))
		return -EINVAL;

	/* It is not possible to block SIGKILL & SIGSTOP */
	uk_sigcopyset(&suspend_mask, mask);
	uk_sigdelset(&suspend_mask, SIGKILL);
	uk_sigdelset(&suspend_mask, SIGSTOP);

	pprocess_signal_suspend(pthread, suspend_mask);
	return -EINTR;
}
#else /* !CONFIG_LIBPOSIX_PROCESS_SIGNAL */
UK_LLSYSCALL_R_DEFINE(int, rt_sigsuspend,
		      const uk_sigset_t *, mask,
		      size_t, sigsetsize)
{
	UK_WARN_STUBBED();
	return -EINTR;
}
#endif /* !CONFIG_LIBPOSIX_PROCESS_SIGNAL */
