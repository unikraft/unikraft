/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>
#include <sys/types.h>
#include <stddef.h>

#include <uk/config.h>
#include <uk/event.h>
#include <uk/process.h>
#include <uk/sched.h>
#include <uk/syscall.h>

#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
#include "signal/signal.h"
#include "signal.h"
#endif /* CONFIG_LIBPOSIX_PROCESS_SIGNAL */

#include "process.h"

#define PTHREAD_WAITING_FOR_PID(_pt, _pid)				\
	((_pt)->state == POSIX_THREAD_BLOCKED_WAIT &&			\
	 ((_pt)->wait_pid == (_pid) || (_pt)->wait_pid == -1))

#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
#define PTHREAD_BLOCKING_ON_SIGNAL(_pt)					\
	((_pt)->state == POSIX_THREAD_BLOCKED_SIGNAL)
#else /* !CONFIG_LIBPOSIX_PROCESS_SIGNAL */
#define PTHREAD_BLOCKING_ON_SIGNAL(_pt)		    0
#endif /* !CONFIG_LIBPOSIX_PROCESS_SIGNAL */

UK_EVENT(POSIX_PROCESS_EXIT_EVENT);

int pprocess_exit_status = PPROCESS_EXIT_STATUS_UNSET;

#if CONFIG_LIBPOSIX_PROCESS_MULTITHREADING

/* pprocess exit state to pthread exit state */
#define PP2PT_EXIT_STATE(_pp_state)					\
	((_pp_state) == POSIX_PROCESS_EXITED ? POSIX_THREAD_EXITED :	\
					      POSIX_THREAD_KILLED)

static void pprocess_reparent_children(struct posix_process *pprocess)
{
	struct posix_process *pchild, *pchildn;
	struct posix_process *pprocess_init;

	pprocess_init = pid2pprocess(UK_PID_INIT);

	uk_list_for_each_entry_safe(pchild, pchildn, &pprocess->children,
				    child_list_entry) {
		uk_list_del(&pchild->child_list_entry);
		if (pprocess_init && pprocess != pprocess_init) {
			pchild->parent = pprocess_init;
			uk_list_add(&pchild->child_list_entry,
				    &pprocess_init->children);
			uk_pr_debug("Process PID %d re-assigned to PID %d\n",
				    pchild->pid, pchild->parent->pid);
		} else {
			/* There is no parent, disconnect */
			pchild->parent = NULL;
			uk_pr_debug("Process PID %d loses its parent\n",
				    pchild->pid);
		}
	}
}

#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
static inline int signal_exit(struct posix_process *pprocess)
{
	struct kern_sigaction *ks;

	UK_ASSERT(pprocess);
	UK_ASSERT(pprocess->signal);

	ks = KERN_SIGACTION(pprocess, SIGCHLD);
	if (ks->ks_handler == SIG_IGN || ks->ks_flags & SA_NOCLDWAIT)
		return 1;

	return pprocess_signal_send(pprocess, SIGCHLD, NULL);
}
#endif /* CONFIG_LIBPOSIX_PROCESS_SIGNAL */

/* Exit AND release pthread.
 * Notice: Unlike pprocesses, pthreads are released upon exit.
 */
void pprocess_exit_pthread(struct posix_thread *pthread,
			   enum posix_thread_state state,
			   int exit_status __unused)
{
	struct posix_process_exit_event_data event_data;
	struct posix_process *pprocess;
	struct posix_thread *parent_pthread;
	struct uk_thread *thread;
	int ret;

	UK_ASSERT(state == POSIX_THREAD_EXITED ||
		  state == POSIX_THREAD_KILLED);

	UK_ASSERT(pthread);
	UK_ASSERT(pthread->state != POSIX_THREAD_EXITED &&
		  pthread->state != POSIX_THREAD_KILLED);

	pprocess = pthread->process;
	UK_ASSERT(pprocess);
	UK_ASSERT(pprocess->state == POSIX_PROCESS_RUNNING);

	uk_pr_debug("pid %d: exit tid %d\n", pprocess->pid, pthread->tid);

	/* May be NULL if init terminated early */
	parent_pthread = pthread->parent;

	/* Update state */
	pthread->state = state;

	/* Raise event */
	event_data.thread = pthread->thread;
	event_data.tid = pthread->tid;
	event_data.pid = pprocess->pid;
	ret = uk_raise_event(POSIX_PROCESS_EXIT_EVENT, &event_data);
	if (unlikely(ret < 0))
		UK_CRASH("POSIX_PROCESS_EXIT_EVENT failed with %d\n", ret);

	/* Wake up parent if it was blocking on vfork */
	if (parent_pthread &&
	    parent_pthread->state == POSIX_THREAD_BLOCKED_VFORK) {
		uk_thread_wake(parent_pthread->thread);
		parent_pthread->state = POSIX_THREAD_RUNNING;
	}

	/* Release pthread and terminate the underlying uk_thread
	 * unless it's the current one or if it hasn't been associated
	 * with a scheduler yet (may happen if a thread is released
	 * before being added to the scheduler, e.g. on some error path
	 * that cleans up created threads that didn't get the chance to
	 * be added).
	 */
	thread = pthread->thread;
	pprocess_release_pthread(pthread);
	if (thread != uk_thread_current() && thread->sched)
		uk_sched_thread_terminate(thread);
}

void pprocess_exit(struct posix_process *pprocess,
		   enum posix_process_state state,
		   int exit_status)
{
	struct posix_process_exit_event_data event_data;
	struct posix_process *parent_process;
	struct posix_thread *pt, *ptn;
	__bool nowait = false;
	int ret;

	UK_ASSERT(state == POSIX_PROCESS_EXITED ||
		  state == POSIX_PROCESS_KILLED);

	UK_ASSERT(pprocess);
	UK_ASSERT(pprocess->state == POSIX_PROCESS_RUNNING);

	uk_pr_debug("pid %d: exit process\n", pprocess->pid);

	/* May be NULL if init terminated early */
	parent_process = pprocess->parent;

#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
	/* As this will be signaling the parent on behalf of the current
	 * pthread, we must do this before we terminate the pthreads, in
	 * case we are called from _exit(), exit_group() (i.e. operate on
	 * current).
	 */
	if (parent_process) {
		ret = signal_exit(parent_process);
		if (ret > 0) {
			/* From exit(2): "If the parent has set SA_NOCLDWAIT, or
			 * has set the SIGCHLD handler to SIG_IGN, the status is
			 * discarded and the child dies immediately."
			 */
			uk_pr_info("Parent ignores SIGHLD, terminating\n");
			nowait = true;
		} else if (unlikely(ret < 0)) { /* no choice here but crash */
			UK_CRASH("Could not signal parent (%d)\n", ret);
		}
	}
#endif /* CONFIG_LIBPOSIX_PROCESS_SIGNAL */

	/* Terminate pthreads.
	 *
	 * This must happen before we unblock wait(), so that threads
	 * have already been released when we reap the process.
	 */
	uk_pprocess_foreach_pthread(pprocess, pt, ptn)
		pprocess_exit_pthread(pt, PP2PT_EXIT_STATE(state), exit_status);

	/* ---> No ops at the posix level on behalf of the current <---
	 * ---> thread from this point                             <---
	 */
	UK_ASSERT(uk_list_empty(&pprocess->threads));

	/* Update process state & exit_status*/
	if (state == POSIX_PROCESS_EXITED)
		pprocess->exit_status = exit_status & 0xff;
	else /* POSIX_PROCESS_KILLED */
		pprocess->exit_status = exit_status;
	pprocess->state = state;

	/* Reparent child processes to init */
	pprocess_reparent_children(pprocess);

	/* Notify handlers */
	event_data.thread = NULL;
	event_data.pid = pprocess->pid;
	ret = uk_raise_event(POSIX_PROCESS_EXIT_EVENT, &event_data);
	if (unlikely(ret < 0))
		UK_CRASH("POSIX_PROCESS_EXIT_EVENT handler returned error\n");

	uk_semaphore_up(&pprocess->exit_semaphore);

	/* Unblock wait */
	if (parent_process && !nowait) {
		uk_pprocess_foreach_pthread(parent_process, pt, ptn) {
			if (PTHREAD_WAITING_FOR_PID(pt, uk_sys_getpid()) ||
			    PTHREAD_BLOCKING_ON_SIGNAL(pt)) {
				uk_semaphore_up(&parent_process->wait_semaphore);
				break;
			}
		}
	} else if (nowait) { /* release now */
		pprocess_release(pprocess);
	}
}

/* NOTE: The man pages of _exit(2) say:
 *       "In glibc up to version 2.3, the _exit() wrapper function invoked
 *        the kernel system call of the same name.  Since glibc 2.3, the
 *        wrapper function invokes exit_group(2), in order to terminate all
 *        of the threads in a process.
 *        The raw _exit() system call terminates only the calling thread,
 *        and actions such as reparenting child processes or sending
 *        SIGCHLD to the parent process are performed only if this is the
 *        last thread in the thread group."
 */
UK_LLSYSCALL_R_DEFINE(int, exit, int, status)
{
	struct posix_thread *this_pthread;
	struct uk_thread *this_thread;

	this_thread = uk_thread_current();
	this_pthread = uk_thread_uktls_var(this_thread, pthread_self);

	UK_ASSERT(this_pthread);
	UK_ASSERT(this_pthread->process);

	/* Last thread, exit the process */
	if (uk_list_is_singular(&this_pthread->process->threads)) {
		pprocess_exit(this_pthread->process, POSIX_PROCESS_EXITED,
			      status);
		uk_sched_thread_exit();
	}

	pprocess_exit_pthread(this_pthread, POSIX_THREAD_EXITED, status);
	uk_sched_thread_exit();
}

UK_LLSYSCALL_R_DEFINE(int, exit_group, int, status)
{
	struct posix_process *pprocess;

	pprocess = uk_pprocess_current();
	UK_ASSERT(pprocess);

	pprocess_exit(pprocess, POSIX_PROCESS_EXITED, status);
	uk_sched_thread_exit();
}
#else /* !CONFIG_LIBPOSIX_PROCESS_MULTITHREADING */
void pprocess_exit_stub(int status)
{
	if (pprocess_thread_main) {
		pprocess_exit_status = status;
		uk_sched_thread_exit(); /* noreturn */
	}

	UK_CRASH("Invalid system configuration:\n"
		 "Process support without multithreading does not support _exit() / exit_group()\n"
		 "Select LIBPOSIX_PROCESS_MULTITHREADING, or at minimum LIBUKBOOT_MAINTHREAD\n");
}

UK_LLSYSCALL_R_DEFINE(int, exit, int, status)
{
	pprocess_exit_stub(status);
	UK_BUG(); /* noreturn */
}

UK_LLSYSCALL_R_DEFINE(int, exit_group, int, status)
{
	pprocess_exit_stub(status);
	UK_BUG(); /* noreturn */
}
#endif /* !CONFIG_LIBPOSIX_PROCESS_MULTITHREADING */

#if UK_LIBC_SYSCALLS
__noreturn void exit(int status)
{
	/* According to _exit(2): "Since glibc 2.3, the wrapper function invokes
	 * exit_group(2), in order to terminate all of the threads in a
	 * process."
	 */
	uk_syscall_e_exit_group(status);
	UK_BUG(); /* noreturn */
}

__noreturn void exit_group(int status)
{
	uk_syscall_e_exit_group(status);
	UK_BUG(); /* noreturn */
}
#endif /* UK_LIBC_SYSCALLS */
