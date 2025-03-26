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
	(((_state) & WEXITED) && ((_pp)->state == POSIX_PROCESS_EXITED || \
				(_pp)->state == POSIX_PROCESS_KILLED))

static inline
struct posix_process *pprocess_state_changed(struct posix_process *pproc,
					     int state)
{
	UK_ASSERT(pproc);
	return (PPROCESS_STATE_CHANGED(pproc, state)) ? pproc : NULL;
}

static inline
struct posix_process *pprocess_child_state_changed(int state)
{
	struct posix_process *pchild, *pchildn;
	struct posix_process *pproc;

	uk_pprocess_foreach_child(uk_pprocess_current(), pchild, pchildn)
		if ((pproc = pprocess_state_changed(pchild, state)))
			return pproc;
	return NULL;
}

static int pprocess_reap(struct posix_process *pprocess, siginfo_t *si)
{
	pid_t pid;

	UK_ASSERT(pprocess);
	UK_ASSERT(uk_list_empty(&pprocess->threads));

	if (si)
		set_siginfo_wait(pprocess, si);

	pid = pprocess->pid;
	pprocess_release(pprocess);

	return pid;
}

/* Wait helper
 *
 * @param pid: child pid to wait for or -1 for any
 * @param opt: same as waitid()
 * @return child pid, zero if no waitable, negative value on error
 */
static int pprocess_wait(pid_t pid, siginfo_t *si, int opt)
{
	struct posix_process *this_process;
	struct posix_thread *this_thread;
	struct posix_process *waitable = NULL;
	struct posix_process *child = NULL;
	int ret;

	UK_ASSERT(pid > 0 || pid == -1);
	UK_ASSERT(si);

	this_thread = uk_pthread_current();
	UK_ASSERT(this_thread);

	this_process = this_thread->process;
	UK_ASSERT(this_process);

	if (uk_list_empty(&this_process->children))
		return -ECHILD;

	this_thread->state = POSIX_THREAD_BLOCKED_WAIT;

	if (pid > 0) {
		child = pid2pprocess(pid);
		/* Child does not exist or not our child. This also
		 * covers the case where the process chose to ignore
		 * SIGCHLD or set SA_NOCLDWAIT and therefore _exit()
		 * terminated it without further ado.
		 */
		if (unlikely(!child || child->parent != this_process)) {
			ret = -ECHILD;
			goto out;
		}

		/* If the process exited already, reap and return the pid */
		if (child->state != POSIX_PROCESS_RUNNING) {
			ret = pprocess_reap(child, si);
			goto out;
		}

		this_thread->wait_pid = pid;
	} else { /* any */
		this_thread->wait_pid = -1;
	}

	for (;;) {
		/* Find process with changed state. No change means that
		 * the state change happened to a different child, so we
		 * loop back to the semaphore.
		 */
		if (pid > 0)
			waitable = pprocess_state_changed(child, opt);
		else /* any */
			waitable = pprocess_child_state_changed(opt);

		if (waitable) {
			ret = waitable->pid;
			if (!(opt & WNOWAIT))
				pprocess_reap(waitable, si);
			break;
		}

		if (opt & WNOHANG) {
			ret = 0;
			break;
		}

		/* Wait for a state change on one of our children */
		uk_semaphore_down(&this_thread->process->wait_semaphore);
	}

out:
	this_thread->state = POSIX_THREAD_RUNNING;
	return ret;
}

UK_SYSCALL_R_DEFINE(pid_t, wait4, pid_t, pid,
		    int *, wstatus, int, options,
		    struct rusage *, rusage)
{
	siginfo_t si;
	int ret;

	if (options)
		if (unlikely(!(options & (WNOHANG | WUNTRACED | WCONTINUED))))
			return -EINVAL;

	if (pid < -1 || pid == 0) /* process groups */
		return -ENOTSUP;

	ret = pprocess_wait(pid, &si, options | WEXITED);

	if (ret > 0 && wstatus) {
		if (si.si_status == CLD_KILLED)
			*wstatus = si.si_code;
		else /* CLD_EXITED */
			*wstatus = si.si_code << 8;
	}

	return ret;
}

UK_LLSYSCALL_R_DEFINE(int, waitid, idtype_t, idtype, id_t, id,
		      siginfo_t *, infop, int, options,
		      struct rusage * __unused, rusage)
{
	int ret;

	if (options)
		if (unlikely(!(options & (WEXITED | WSTOPPED | WCONTINUED |
					  WNOHANG | WNOWAIT))))
			return -EINVAL;

	switch (idtype) {
	case P_PID:
		ret = pprocess_wait(id, infop, options);
		break;
	case P_PIDFD:
	case P_PGID:
		return -ENOTSUP;
	case P_ALL:
		ret = pprocess_wait(-1, infop, options);
		break;
	default:
		return -EINVAL;
	}

	/* POSIX.1-2008 requires that infop must not be NULL, yet Linux
	 * violates the standard and returns the pid of the waited child
	 * when that happens. wait(2) discourages applications to rely on
	 * that behavior, still for backwards compatibility we implement
	 * Linux's behavior nevertheless.
	 */
	return (infop && ret >= 0) ? 0 : ret;
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
