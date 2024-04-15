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

UK_LLSYSCALL_R_E_DEFINE(pid_t, vfork,
			unsigned long __unused, a0,
			unsigned long __unused, a1)
{
	struct posix_process *child_proc;
	struct clone_args cl_args = {0};
	pid_t child_tid;

	cl_args.flags       = CLONE_VM | CLONE_VFORK;
	cl_args.exit_signal = SIGCHLD;

	child_tid = uk_clone(&cl_args, sizeof(cl_args), execenv);
	if (unlikely(child_tid < 0)) {
		uk_pr_err("Could not clone thread\n");
		return child_tid;
	}

	child_proc = tid2pprocess(child_tid);
	UK_ASSERT(child_proc);

	return child_proc->pid;
}
