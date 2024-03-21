/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2022, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <errno.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "process.h"
#include "signal/siginfo.h"

#include <uk/process.h>
#include <uk/semaphore.h>
#include <uk/syscall.h>

#define PTHREAD_TERMINATED(_thread)					\
			((_thread)->state == POSIX_THREAD_EXITED ||	\
			 (_thread)->state == POSIX_THREAD_KILLED)

static int reap_pprocess(struct posix_thread *pthread, siginfo_t *si)
{
	pid_t pid = pthread->process->pid;

	if (si)
		set_siginfo_wait(pthread, si);

	uk_pr_debug("Reaping pid %d\n", pid);
	uk_posix_process_kill(pthread->thread);

	return pid;
}

static int wait_pid_any(siginfo_t *si, int options)
{
	struct posix_thread *pthread, *pthreadn;
	struct posix_thread *this_thread;
	struct posix_process *proc;
	int ret;

	this_thread = tid2pthread(uk_gettid());

	this_thread->wait_pid = UK_PID_WAIT_ANY;
	this_thread->state = POSIX_THREAD_BLOCKED_WAIT;

	for (;;) {
		/* Check first whether any children already terminated */
		uk_process_foreach_child(pid2pprocess(uk_getpid()), proc) {
			uk_process_foreach_pthread(proc, pthread, pthreadn) {
				if ((options & WEXITED) &&
				    PTHREAD_TERMINATED(pthread)) {
					ret = reap_pprocess(pthread, si);
					goto out;
				}
			}
		}

		if (options & WNOHANG) {
			ret = 0;
			break;
		}

		if (uk_list_empty(&this_thread->process->children)) {
			ret = -ECHILD;
			break;
		}

		/* Wait for a state change on one of our children*/
		uk_semaphore_down(&this_thread->process->wait_semaphore);
	}

out:
	this_thread->state = POSIX_THREAD_RUNNING;
	return ret;
}

static int wait_pid_one(pid_t pid, siginfo_t *si, int options)
{
	struct posix_thread *pthread, *pthreadn;
	struct posix_thread *this_thread;
	struct posix_process *child_process;
	int ret;

	this_thread = tid2pthread(uk_gettid());

	this_thread->wait_pid = pid;
	this_thread->state = POSIX_THREAD_BLOCKED_WAIT;

	child_process = pid2pprocess(pid);

	/* Process does not exist. This also covers the case
	 * where the process chose to ignore SIGCHLD or set
	 * SA_NOCLDWAIT and therefore _exit() terminated it
	 * without further ado.
	 */
	if (unlikely(!child_process)) {
		ret = -ECHILD;
		goto out;
	}

	if (unlikely(child_process->parent != this_thread->process)) {
		ret = -ECHILD;
		goto out;
	}

	for (;;) {
		/* Find thread with changed state. No change means that
		 * the state change happened to a different child, so we
		 * loop back to the semaphore.
		 */
		uk_process_foreach_pthread(child_process, pthread, pthreadn) {
			if ((options & WEXITED) &&
			    PTHREAD_TERMINATED(pthread)) {
				ret = reap_pprocess(pthread, si);
				goto out;
			}
		}

		if (options & WNOHANG) {
			ret = 0;
			break;
		}

		if (uk_list_empty(&this_thread->process->children)) {
			ret = -ECHILD;
			break;
		}

		/* Wait for a state change on one of our children*/
		uk_semaphore_down(&this_thread->process->wait_semaphore);
	}

out:
	if (!(options & WNOWAIT))
		this_thread->state = POSIX_THREAD_RUNNING;
	return ret;
}

pid_t uk_posix_process_wait(void)
{
	siginfo_t si;
	int ret;

	if (uk_list_empty(&pid2pprocess(uk_getpid())->children))
		return -ECHILD;

	ret = wait_pid_any(&si, WEXITED);

	return ret >= 0 ? si.si_pid : ret;
}

UK_SYSCALL_R_DEFINE(pid_t, wait4, pid_t, pid,
		    int *, wstatus, int, options,
		    struct rusage *, rusage)
{
	siginfo_t si;
	int ret;

	if (uk_list_empty(&pid2pprocess(uk_getpid())->children))
		return -ECHILD;

	if (pid < -1 || pid == 0) {
		uk_pr_warn("Process groups not supported\n");
		return -ENOTSUP;
	}

	if (options && !(options & (WNOHANG | WUNTRACED | WCONTINUED)))
		return -EINVAL;

	if (options & WUNTRACED)
		uk_pr_warn("WUNTRACED stubbed\n");

	if (options & WCONTINUED)
		uk_pr_warn("WCONTINUED stubbed\n");

	if (rusage)
		uk_pr_warn("rusage stubbed\n");

	if (pid == UK_PID_WAIT_ANY)
		ret = wait_pid_any(&si, options);
	else
		ret = wait_pid_one(pid, &si, options);

	if (ret >= 0 && wstatus) {
		if (si.si_status == CLD_KILLED)
			*wstatus = si.si_code;
		else /* CLD_EXITED */
			*wstatus = si.si_code << 8;
	}

	return ret >= 0 ? si.si_pid : ret;
}

UK_LLSYSCALL_R_DEFINE(int, waitid, idtype_t, idtype, id_t, id,
		      siginfo_t *, infop, int, options,
		      struct rusage *, rusage)
{
	int ret;

	if (uk_list_empty(&pid2pprocess(uk_getpid())->children))
		return -ECHILD;

	if (options && !(options & (WEXITED | WSTOPPED | WCONTINUED |
				    WNOHANG | WNOWAIT)))
		return -EINVAL;

	if (options & WSTOPPED)
		uk_pr_warn("WSTOPPED stubbed");

	if (options & WCONTINUED)
		uk_pr_warn("WCONTINUED stubbed");

	if (rusage)
		uk_pr_warn("rusage stubbed");

	switch (idtype) {
	case P_PID:
		ret = wait_pid_one(id, infop, options);
		break;
	case P_PIDFD:
		uk_pr_warn("P_PIDFD not supported\n");
		ret = -ENOTSUP;
		break;
	case P_PGID:
		uk_pr_warn("P_PGID not supported\n");
		ret = -ENOTSUP;
		break;
	case P_ALL:
		ret = wait_pid_any(infop, options);
		break;
	default:
		return -EINVAL;
	}

	/* POSIX.1-2008 requires that infop must not be NULL, yet Linux
	 * violates the standard and returns the pid of the waited child
	 * when that happens. wait(2) discourages applications to rely on
	 * that behavior, still for backwards compatibility we implement
	 * Linux's behavior and issue a warning.
	 */
	if (!infop)
		uk_pr_warn("infop set to NULL, falling back to Linux behavior");

	return (infop && ret >= 0) ? 0 : ret;
}

#if UK_LIBC_SYSCALLS
pid_t wait3(int *wstatus, int options, struct rusage *rusage)
{
	return uk_syscall_e_wait4((long) -1, (long) wstatus,
				  (long) options, (long) rusage);
}

int waitpid(pid_t pid, int *wstatus, int options)
{
	return uk_syscall_e_wait4((long) pid, (long) wstatus,
				  (long) options, (long) NULL);
}

int wait(int *wstatus)
{
	return uk_syscall_e_wait4((long) -1, (long) wstatus,
				  (long) 0x0, (long) NULL);
}
#endif /* !UK_LIBC_SYSCALLS */
