/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_SIGNAL_SIGINFO_H__
#define __UK_SIGNAL_SIGINFO_H__

#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <uk/process.h>

#include "process.h"

static inline void set_siginfo_kill(int signum, siginfo_t *si)
{
	si->si_signo = signum;
	si->si_code  = SI_USER;
	si->si_pid   = uk_getpid();
	si->si_uid   = 0;
}

static inline void set_siginfo_sigqueue(int signum, siginfo_t *si,
					siginfo_t *si_usr)
{
	si->si_signo = signum;

	/* The remaining fields are set by the caller
	 * without verification.
	 */
	si->si_code  = si_usr->si_code;
	si->si_pid   = si_usr->si_pid;
	si->si_uid   = si_usr->si_uid;
	si->si_value = si_usr->si_value;
}

#endif /* __UK_SIGNAL_SIGINFO_H__ */
