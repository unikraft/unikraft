/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2022, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
 * Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <uk/syscall.h>

#if CONFIG_LIBPOSIX_PROCESS_MULTIPROCESS
#include <uk/process.h>
#include <uk/semaphore.h>

#include "process.h"
#include "signal/siginfo.h"

#define PPROCESS_STATE_CHANGED(_pp, _state)				  \
	(!(_pp)->reap_claimed && ((_state) & WEXITED) &&		 \
	 ((_pp)->state == POSIX_PROCESS_EXITED ||			 \
				(_pp)->state == POSIX_PROCESS_KILLED))

static inline
struct posix_process *pprocess_state_changed(struct posix_process *pproc,
					     int state)
{
	UK_ASSERT(pproc);
	return (PPROCESS_STATE_CHANGED(pproc, state)) ? pproc : NULL;
}

static void pprocess_reap(struct posix_process *pprocess)
{
	UK_ASSERT(pprocess);
	UK_ASSERT(uk_list_empty(&pprocess->threads));

	pprocess_release(pprocess);
}

/* Wait helper
 *
 * @param pid: child pid to wait for or -1 for any
 * @param pgid: restrict any-child selection to this group, or zero
 * @param opt: same as waitid()
 * @return child pid, zero if no waitable, negative value on error
 */
static int pprocess_wait(pid_t pid, pid_t pgid, siginfo_t *si, int opt)
{
	struct posix_process *this_process;
	struct posix_thread *this_thread;
	struct posix_process *waitable = NULL;
	struct posix_process *child, *next, *reap = NULL;
	bool matched;
	unsigned long flags;
	int ret;

	UK_ASSERT(pid > 0 || pid == -1);
	UK_ASSERT(si);

	this_thread = uk_pthread_current();
	UK_ASSERT(this_thread);

	this_process = this_thread->process;
	UK_ASSERT(this_process);

	for (;;) {
		flags = pprocess_lock_acquire();
		if (pprocess_terminating_signal(this_process)) {
			ret = -EINTR;
			goto out;
		}
		this_thread->wait_pid = pid;
		this_thread->wait_pgid = pgid;
		this_thread->state = POSIX_THREAD_BLOCKED_WAIT;
		if (uk_list_empty(&this_process->children)) {
			ret = -ECHILD;
			goto out;
		}
		if (pid > 0) {
			child = pid2pprocess(pid);
			if (!child || child->parent != this_process ||
			    child->reap_claimed) {
				ret = -ECHILD;
				goto out;
			}
			waitable = pprocess_state_changed(child, opt);
		} else {
			matched = false;
			waitable = NULL;
			uk_pprocess_foreach_child(this_process, child, next) {
				if (child->reap_claimed ||
				    (pgid && child->pgid != pgid))
					continue;
				matched = true;
				waitable = pprocess_state_changed(child, opt);
				if (waitable)
					break;
			}
			if (!matched) {
				ret = -ECHILD;
				goto out;
			}
		}

		if (waitable) {
			ret = waitable->pid;
			set_siginfo_wait(waitable, si);
			if (!(opt & WNOWAIT)) {
				/* Claim the child before unlocking. */
				waitable->reap_claimed = true;
				uk_list_del_init(&waitable->child_list_entry);
				waitable->parent = NULL;
				reap = waitable;
			}
			goto out;
		}

		if (opt & WNOHANG) {
			ret = 0;
			goto out;
		}

		pprocess_lock_release(flags);
		uk_semaphore_down(&this_thread->wait_semaphore);
	}

out:
	this_thread->state = POSIX_THREAD_RUNNING;
	pprocess_lock_release(flags);
	if (reap)
		pprocess_reap(reap);
	return ret;
}

/* Capture PID-zero group selection once, before the first blocking wait. */
static pid_t current_group(void)
{
	unsigned long flags = pprocess_lock_acquire();
	pid_t pgid = uk_pthread_current()->process->pgid;

	pprocess_lock_release(flags);
	return pgid;
}

UK_SYSCALL_R_DEFINE(pid_t, wait4, pid_t, pid,
		    int *, wstatus, int, options,
		    struct rusage *, rusage)
{
	siginfo_t si;
	pid_t pgid = 0;
	int ret;

	if (options & ~(WNOHANG | WUNTRACED | WCONTINUED))
		return -EINVAL;
	if (pid < -CONFIG_LIBPOSIX_PROCESS_MAX_PID)
		return -ECHILD;
	if (pid <= 0 && pid != -1) {
		pgid = pid ? -pid : current_group();
		pid = -1;
	}
	ret = pprocess_wait(pid, pgid, &si, options | WEXITED);

	if (ret > 0 && wstatus) {
		if (si.si_code == CLD_KILLED)
			*wstatus = si.si_status;
		else /* CLD_EXITED */
			*wstatus = si.si_status << 8;
	}

	return ret;
}

UK_LLSYSCALL_R_DEFINE(int, waitid, idtype_t, idtype, id_t, id,
		      siginfo_t *, infop, int, options,
		      struct rusage * __unused, rusage)
{
	siginfo_t si = { 0 };
	int ret;

	if (unlikely(options & ~(WEXITED | WSTOPPED | WCONTINUED |
				 WNOHANG | WNOWAIT)))
		return -EINVAL;
	if (unlikely(!(options & (WEXITED | WSTOPPED | WCONTINUED))))
		return -EINVAL;

	switch (idtype) {
	case P_PID:
		if ((pid_t)id <= 0)
			return -EINVAL;
		ret = pprocess_wait(id, 0, &si, options);
		break;
	case P_PGID:
		if ((pid_t)id < 0)
			return -EINVAL;
		ret = pprocess_wait(-1, id ? (pid_t)id : current_group(),
				    &si, options);
		break;
	case P_PIDFD:
		return -ENOTSUP;
	case P_ALL:
		ret = pprocess_wait(-1, 0, &si, options);
		break;
	default:
		return -EINVAL;
	}

	if (ret < 0)
		return ret;
	/* Linux also permits a NULL result pointer and returns zero. */
	if (infop)
		*infop = si;
	return 0;
}
#else /* !CONFIG_LIBPOSIX_PROCESS_MULTIPROCESS */
UK_SYSCALL_R_DEFINE(pid_t, wait4, pid_t, pid,
		    int *, wstatus, int, options,
		    struct rusage *, rusage)
{
	return -ECHILD;
}

UK_LLSYSCALL_R_DEFINE(int, waitid, idtype_t, idtype, id_t, id,
		      siginfo_t *, infop, int, options,
		      struct rusage *, rusage)
{
	return -ECHILD;
}
#endif /* !CONFIG_LIBPOSIX_PROCESS_MULTIPROCESS */

#if UK_LIBC_SYSCALLS
pid_t wait3(int *wstatus, int options, struct rusage *rusage)
{
	return uk_syscall_e_wait4((long)-1, (long)wstatus,
				  (long)options, (long)rusage);
}

int waitpid(pid_t pid, int *wstatus, int options)
{
	return uk_syscall_e_wait4((long)pid, (long)wstatus,
				  (long)options, (long)NULL);
}

int wait(int *wstatus)
{
	return uk_syscall_e_wait4((long)-1, (long)wstatus,
				  (long)0x0, (long)NULL);
}

int waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options)
{
	return uk_syscall_e_waitid((long)idtype, (long)id, (long)infop,
				   (long)options, (long)NULL);
}
#endif /* !UK_LIBC_SYSCALLS */
