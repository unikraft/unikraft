/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Process groups and sessions, as setsid(2) and setpgid(2) describe them.
 * There is no controlling terminal: a session only groups process groups,
 * which is what a supervisor needs to signal (kill(-pgid)) or reap
 * (wait4(-pgid)) everything a command started.
 */

#include <errno.h>
#include <sys/types.h>

#include <uk/process.h>
#include <uk/syscall.h>

#include "process.h"

/* @p leaves its group: wake its parent's threads waiting for a child of
 * that group, which may have no child left in it (ECHILD).
 */
static void pgroup_leave(struct posix_process *p)
{
	struct posix_thread *pt, *ptn;

	if (!p->parent)
		return;
	uk_pprocess_foreach_pthread(p->parent, pt, ptn) {
		if (pt->state == POSIX_THREAD_BLOCKED_WAIT &&
		    pt->wait_pid == -1 && pt->wait_pgid == p->pgid) {
			pprocess_wake_waiters(p->parent);
			return;
		}
	}
}

/* Whether a process of @sid has @pgid as its group */
static bool pgroup_exists(pid_t pgid, pid_t sid)
{
	struct posix_process *p;

	uk_pprocess_foreach(p) {
		if (p->pgid == pgid && p->sid == sid)
			return true;
	}
	return false;
}

UK_SYSCALL_R_DEFINE(pid_t, setsid)
{
	struct posix_process *this = uk_pprocess_current();
	struct posix_process *p;

	UK_ASSERT(this);

	/* A group leader cannot leave: its group would span two sessions */
	uk_pprocess_foreach(p) {
		if (p->pgid == this->pid)
			return -EPERM;
	}

	pgroup_leave(this);
	this->sid = this->pid;
	this->pgid = this->pid;
	return this->sid;
}

UK_SYSCALL_R_DEFINE(pid_t, getsid, pid_t, pid)
{
	struct posix_process *p;

	p = pid ? pid2pprocess(pid) : uk_pprocess_current();
	if (!p)
		return -ESRCH;
	return p->sid;
}

UK_SYSCALL_R_DEFINE(int, setpgid, pid_t, pid, pid_t, pgid)
{
	struct posix_process *this = uk_pprocess_current();
	struct posix_process *p;

	UK_ASSERT(this);

	if (pgid < 0)
		return -EINVAL;

	/* Only the caller or one of its children can be moved */
	p = pid ? pid2pprocess(pid) : this;
	if (!p || (p != this && p->parent != this))
		return -ESRCH;

	if (p->sid == p->pid || p->sid != this->sid)
		return -EPERM;

	if (!pgid)
		pgid = p->pid;

	/* Join a group of this session, or start one named after p */
	if (pgid != p->pid && !pgroup_exists(pgid, this->sid))
		return -EPERM;

	if (p->pgid != pgid)
		pgroup_leave(p);
	p->pgid = pgid;
	return 0;
}

UK_SYSCALL_R_DEFINE(pid_t, getpgid, pid_t, pid)
{
	struct posix_process *p;

	p = pid ? pid2pprocess(pid) : uk_pprocess_current();
	if (!p)
		return -ESRCH;
	return p->pgid;
}

UK_SYSCALL_R_DEFINE(pid_t, getpgrp)
{
	return uk_pprocess_current()->pgid;
}
