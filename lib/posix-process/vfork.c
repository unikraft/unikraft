/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#define _GNU_SOURCE /* struct clone_args */

#include <sched.h>
#include <signal.h>

#include <uk/essentials.h>
#include <uk/process.h>
#include <uk/sched.h>
#include <uk/syscall.h>

#include "process.h"

UK_LLSYSCALL_R_E_DEFINE(pid_t, vfork)
{
	struct clone_args cl_args = {0};
	pid_t pid; /* child */

	cl_args.flags       = CLONE_VM | CLONE_VFORK;
	cl_args.exit_signal = SIGCHLD;

	pid = uk_clone(&cl_args, sizeof(cl_args), execenv);
	if (unlikely(pid < 0)) {
		uk_pr_err("vfork error (%d)\n", pid);
		return pid;
	}

	return pid;
}
