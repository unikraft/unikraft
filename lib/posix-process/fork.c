/* SPDX-License-Identifier: BSD-3-Clause */

#define _GNU_SOURCE

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <uk/crash.h>
#include <uk/errptr.h>
#include <uk/lcpu.h>
#include <uk/plat/config.h>
#include <uk/sched.h>
#include <uk/syscall.h>
#include <uk/vmem/process.h>

#include "process.h"

static void fork_child_release(struct uk_thread *thread)
{
	free(DECONST(char *, thread->name));
	thread->name = NULL;
}

static bool fork_private_word(struct uk_vas *vas, __u64 address)
{
	return address >= vas->range_start &&
		address <= vas->range_end - sizeof(__u32);
}

int pprocess_fork(struct clone_args *args, size_t args_len,
		  struct ukarch_execenv *execenv)
{
	const __u64 supported = CLONE_PARENT_SETTID | CLONE_CHILD_SETTID |
		CLONE_CHILD_CLEARTID;
	struct uk_thread *current = uk_thread_current(), *child;
	struct uk_sched *local = uk_sched_current(), *target;
	struct posix_process *parent = uk_pprocess_current(), *process;
	struct posix_process_clone_event_data event;
	struct uk_vas *vas;
	__uptr sp = uk_lcpu_regs_get(execenv->regs, SP);
	__uptr tls = uk_lcpu_sysctx_get(execenv->sysctx, TLSP);
	char *name;
	pid_t pid;
	int rc;

	if (!parent)
		return -ENOTSUP;
	vas = parent->vas;
	/* A kernel stack or UKTLS cannot be inherited as private user state. */
	if (!vas || !uk_vas_is_owned(vas) || uk_vas_get_active() != vas ||
	    sp < vas->range_start || sp >= vas->range_end ||
	    tls < vas->range_start || tls >= vas->range_end)
		return -ENOTSUP;
	if ((args->flags & ~supported) || args->stack || args->stack_size ||
	    args->tls || args->exit_signal != SIGCHLD)
		return -ENOTSUP;
	if ((args->flags & CLONE_PARENT_SETTID) &&
	    !fork_private_word(vas, args->parent_tid))
		return -EFAULT;
	if ((args->flags & CLONE_CHILD_SETTID) &&
	    !fork_private_word(vas, args->child_tid))
		return -EFAULT;
	if ((args->flags & CLONE_CHILD_CLEARTID) && args->child_tid &&
	    !fork_private_word(vas, args->child_tid))
		return -EFAULT;
	name = strdup(current->name ? current->name : "fork-child");
	if (!name)
		return -ENOMEM;
	child = uk_thread_create_container(local->a, NULL, 0,
					   local->a_auxstack, AUXSTACK_SIZE,
					   local->a_uktls, false, name, NULL,
					   fork_child_release);
	if (!child) {
		free(name);
		return -ENOMEM;
	}
	process = pprocess_prepare(local->a, child, current);
	if (PTRISERR(process)) {
		rc = PTR2ERR(process);
		goto release_child;
	}
	process->program_stack_size = pprocess_program_stack_size(parent);
	/* Only vfork needs a waiter link to the calling thread. */
	uk_thread_uktls_var(child, pthread_self)->parent = NULL;
	rc = pprocess_clone_prepared_vas(process, parent);
	if (rc)
		goto abort;
	pid = process->pid;
	process->exit_signal = SIGCHLD;
	event = (struct posix_process_clone_event_data) {
		.cl_args = args, .cl_args_len = args_len,
		.child = child, .parent = current, .ppid = parent->pid,
		.pid = pid, .tid = pid
	};
	rc = pprocess_raise_clone_event(&event);
	if (rc < 0)
		goto abort;
	/* Keep the new UKTLS for kernel entry and user TLS in the frame. */
	child->tlsp = 0;
	clone_setup_child_ctx(execenv, child, sp);
	if (args->flags & CLONE_CHILD_SETTID) {
		rc = uk_vas_store_u32(process->vas, args->child_tid, pid);
		if (rc)
			goto abort;
	}
	if (args->flags & CLONE_PARENT_SETTID) {
		rc = uk_vas_store_u32(vas, args->parent_tid, pid);
		if (rc)
			goto abort;
	}
	target = uk_sched_select_round_robin(local);
	rc = uk_sched_thread_add(target, child);
	if (rc)
		goto abort;
	rc = pprocess_publish(process);
	if (rc) {
		if (uk_sched_thread_remove(child))
			UK_CRASH("Cannot withdraw unpublished fork child\n");
		goto abort;
	}
	uk_thread_wake(child);
	return pid;
abort:
	pprocess_abort(process);
release_child:
	uk_thread_release(child);
	return rc;
}

UK_LLSYSCALL_R_E_DEFINE(pid_t, fork)
{
	struct clone_args args = { .exit_signal = SIGCHLD };

	return pprocess_fork(&args, sizeof(args), execenv);
}
