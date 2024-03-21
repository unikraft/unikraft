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

UK_SYSCALL_R_DEFINE(int, rt_sigtimedwait,
		    const sigset_t *__unused, set,
		    siginfo_t *, info,
		    const struct timespec *__unused, timeout,
		    size_t __unused, sigsetsize)
{
	*info = (siginfo_t){0};
	info->si_signo = SIGINT;

	return 0;
}
