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

#include "signal.h"

UK_LLSYSCALL_R_DEFINE(int, rt_sigqueueinfo,
		      int, tgid,
		      int, signum,
		      siginfo_t *, siginfo)
{
	if (unlikely(!siginfo))
		return -EFAULT;

	/* The si_code must be valid when targeting a
	 * different process.
	 */
	if (unlikely(tgid != uk_getpid() && siginfo->si_code != SI_QUEUE))
		return -EPERM;

	return pprocess_signal_process_do(tgid, signum, siginfo);
}
