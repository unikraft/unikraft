/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, The Unikraft Authors. All rights reserved. */

#include <errno.h>
#include <uk/process/sigowner.h>
#include <uk/syscall.h>

#include "process.h"

#if CONFIG_LIBPOSIX_PROCESS_MULTITHREADING

void pprocess_groups_init_locked(struct posix_process *process)
{
	struct posix_process *parent = process->parent;

	UK_ASSERT(!process->pgid && !process->sid);
	process->pgid = parent ? parent->pgid : process->pid;
	process->sid = parent ? parent->sid : process->pid;
	pprocess_id_get_locked(process->pgid);
	pprocess_id_get_locked(process->sid);
}

static struct posix_process *group_member_locked(pid_t pgid)
{
	struct posix_process *member;

	uk_pprocess_foreach(member)
		if (member->pgid == pgid)
			return member;
	return NULL;
}

#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
int uk_sigowner_acquire(pid_t owner)
{
	struct posix_process *process;
	unsigned long flags;
	pid_t id;

	if (!owner)
		return 0;
	if (owner > CONFIG_LIBPOSIX_PROCESS_MAX_PID ||
	    owner < -CONFIG_LIBPOSIX_PROCESS_MAX_PID)
		return -ESRCH;
	id = owner < 0 ? -owner : owner;
	flags = pprocess_lock_acquire();
	process = owner < 0 ? group_member_locked(id) : pid2pprocess(id);
	if (process)
		pprocess_id_get_locked(id);
	pprocess_lock_release(flags);
	return process ? 0 : -ESRCH;
}

void uk_sigowner_release(pid_t owner)
{
	unsigned long flags;

	if (!owner)
		return;
	flags = pprocess_lock_acquire();
	pprocess_id_put_locked(owner < 0 ? -owner : owner);
	pprocess_lock_release(flags);
}
#endif /* CONFIG_LIBPOSIX_PROCESS_SIGNAL */

static void group_change_locked(struct posix_process *process, pid_t pgid)
{
	pprocess_id_get_locked(pgid);
	pprocess_id_put_locked(process->pgid);
	process->pgid = pgid;
	if (process->parent) {
		struct posix_thread *thread, *next;

		/* A blocked group wait may now have no matching children. */
		uk_pprocess_foreach_pthread(process->parent, thread, next)
			if (thread->state == POSIX_THREAD_BLOCKED_WAIT)
				uk_semaphore_up(&thread->wait_semaphore);
	}
}

static pid_t process_identity(pid_t pid, bool session)
{
	struct posix_process *process = uk_pthread_current()->process;
	unsigned long flags;
	pid_t result = -ESRCH;

	flags = pprocess_lock_acquire();
	if (pid) {
		process = pid2pprocess(pid);
		if (!process)
			process = tid2pprocess(pid);
	}
	if (process)
		result = session ? process->sid : process->pgid;
	pprocess_lock_release(flags);
	return result;
}

UK_SYSCALL_R_DEFINE(pid_t, getpgid, pid_t, pid)
{
	return process_identity(pid, false);
}

UK_SYSCALL_R_DEFINE(pid_t, getpgrp)
{
	return process_identity(0, false);
}

UK_SYSCALL_R_DEFINE(pid_t, getsid, pid_t, pid)
{
	return process_identity(pid, true);
}

UK_SYSCALL_R_DEFINE(int, setpgid, pid_t, pid, pid_t, pgid)
{
	struct posix_process *self = uk_pthread_current()->process;
	struct posix_process *target, *member;
	unsigned long flags;
	int result = -ESRCH;

	if (pgid < 0)
		return -EINVAL;
	if (!pid)
		pid = self->pid;
	if (!pgid)
		pgid = pid;
	flags = pprocess_lock_acquire();
	target = pid2pprocess(pid);
	if (!target) {
		if (tid2pprocess(pid))
			result = -EINVAL;
		goto out;
	}
	if (target->parent == self) {
		result = -EPERM;
		if (target->sid != self->sid)
			goto out;
		result = -EACCES;
		if (target->execed)
			goto out;
	} else if (target != self) {
		goto out;
	}
	result = -EPERM;
	if (target->pid == target->sid)
		goto out;
	if (pgid != pid) {
		member = group_member_locked(pgid);
		if (!member || member->sid != self->sid)
			goto out;
	}
	group_change_locked(target, pgid);
	result = 0;
out:
	pprocess_lock_release(flags);
	return result;
}

UK_SYSCALL_R_DEFINE(pid_t, setsid)
{
	struct posix_process *self = uk_pthread_current()->process;
	unsigned long flags;
	pid_t result = -EPERM;

	flags = pprocess_lock_acquire();
	if (self->sid == self->pid || group_member_locked(self->pid))
		goto out;
	pprocess_id_get_locked(self->pid);
	pprocess_id_put_locked(self->sid);
	self->sid = self->pid;
	group_change_locked(self, self->pid);
	result = self->sid;
out:
	pprocess_lock_release(flags);
	return result;
}

#else /* !CONFIG_LIBPOSIX_PROCESS_MULTITHREADING */

/* Preserve the single-process ABI when no process namespace is enabled. */
UK_SYSCALL_R_DEFINE(pid_t, setsid)
{
	return -EPERM;
}

UK_SYSCALL_R_DEFINE(pid_t, getsid, pid_t, pid)
{
	return pid ? -ESRCH : 0;
}

UK_SYSCALL_R_DEFINE(int, setpgid, pid_t, pid, pid_t, pgid)
{
	if (pid)
		return -ESRCH;
	return pgid ? -EPERM : 0;
}

UK_SYSCALL_R_DEFINE(pid_t, getpgid, pid_t, pid)
{
	return pid ? -ESRCH : 0;
}

UK_SYSCALL_R_DEFINE(pid_t, getpgrp)
{
	return 0;
}

#endif /* CONFIG_LIBPOSIX_PROCESS_MULTITHREADING */
