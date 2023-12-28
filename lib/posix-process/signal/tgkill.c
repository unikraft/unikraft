/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>

#include <uk/process.h>
#include <uk/syscall.h>

#include "signal.h"

UK_SYSCALL_R_DEFINE(int, tgkill, int, tgid,
		    int, tid, int, signum)
{
	struct posix_process *pproc;

	/* Chack tid is part of my tgid */
	pproc = tid2pprocess(tid);
	if (unlikely(!pproc || pproc->pid != tgid))
		return -ESRCH;

	return pprocess_signal_thread_do(tid, signum, NULL);
}
