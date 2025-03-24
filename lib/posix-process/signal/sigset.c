/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>
#include <signal.h>

#include <uk/arch/lcpu.h> /* unlikely */

#include "sigset.h"

#if !HAVE_LIBC
int sigaddset(sigset_t *set, int signo)
{
	if (unlikely(signo <= 0 || signo >= NSIG)) {
		errno = EINVAL;
		return -1;
	}
	uk_sigaddset(set, signo);
	return 0;
}

int sigdelset(sigset_t *set, int signo)
{
	if (unlikely(signo <= 0 || signo >= NSIG)) {
		errno = EINVAL;
		return -1;
	}
	uk_sigdelset(set, signo);
	return 0;
}

int sigemptyset(sigset_t *set)
{
	uk_sigemptyset(set);
	return 0;
}

int sigfillset(sigset_t *set)
{
	uk_sigfillset(set);
	return 0;
}

int sigismember(const sigset_t *set, int signo)
{
	if (unlikely(signo <= 0 || signo >= NSIG)) {
		errno = EINVAL;
		return -1;
	}

	return uk_sigismember(set, signo);
}
#endif /* !HAVE_LIBC */
