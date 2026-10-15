/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>
#include <sys/types.h>
#include <stddef.h>

#include <uk/config.h>
#include <uk/process.h>
#include <uk/sched.h>
#include <uk/syscall.h>

#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
#include "signal/signal.h"
#include "signal.h"
#endif /* CONFIG_LIBPOSIX_PROCESS_SIGNAL */

#include "process.h"

#define PTHREAD_WAITING_FOR(_pt, _pproc)				\
	((_pt)->state == POSIX_THREAD_BLOCKED_WAIT &&			\
	 ((_pt)->wait_pid == (_pproc)->pid ||				\
	  ((_pt)->wait_pid == -1 &&					\
	   (!(_pt)->wait_pgid || (_pt)->wait_pgid == (_pproc)->pgid))))

#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
#define PTHREAD_BLOCKING_ON_SIGNAL(_pt)					\
	((_pt)->state == POSIX_THREAD_BLOCKED_SIGNAL)
#else /* !CONFIG_LIBPOSIX_PROCESS_SIGNAL */
#define PTHREAD_BLOCKING_ON_SIGNAL(_pt)		    0
#endif /* !CONFIG_LIBPOSIX_PROCESS_SIGNAL */

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
			/* Its exit is for init to hear of as SIGCHLD, whatever
			 * signal it was to send its parent, as on Linux: init
			 * has no handler for most signals and would die of one
			 */
			pchild->exit_signal = SIGCHLD;
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
/* Signal @parent that @child exits, with the child's exit signal (the one
 * its clone() named), not the parent's own.  Returns 1 when the parent
 * discards its children's status (SIGCHLD ignored, or SA_NOCLDWAIT).
 */
static inline int signal_exit(struct posix_process *parent,
			      struct posix_process *child)
{
	struct kern_sigaction *ks;
	__u64 exit_signal;
	int discard = 0;
	int rc;

	UK_ASSERT(parent);
	UK_ASSERT(parent->signal);
	UK_ASSERT(child);

	exit_signal = child->exit_signal ? child->exit_signal : SIGCHLD;

	if (exit_signal == SIGCHLD) {
		ks = KERN_SIGACTION(parent, SIGCHLD);
		if (ks->ks_handler == SIG_IGN)
			return 1;
		/* Reaped at once, but a handler still hears of it */
		if (ks->ks_flags & SA_NOCLDWAIT)
			discard = 1;
	}

	/* clone() takes any number for the exit signal: one that is not a
	 * signal sends nothing, as on Linux; nor do SIGSTOP and SIGCONT,
	 * which kill() ignores too (stop and resume are not supported)
	 */
	if (!IS_VALID(exit_signal) ||
	    exit_signal == SIGSTOP || exit_signal == SIGCONT)
		return discard;

	/* As kill() sends it: dropped if ignored, init spared, the parent
	 * ended by a signal that ends it.  One the parent cannot take (a
	 * full queue of realtime signals, no memory) is lost, as on Linux:
	 * the child stays to be waited for, which is what the parent needs.
	 */
	rc = pprocess_signal_notify_parent(parent, (int)exit_signal);
	if (unlikely(rc < 0))
		uk_pr_warn("pid %d: could not signal parent %d of its exit (%d)\n",
			   child->pid, parent->pid, rc);
	return discard;
}
#endif /* CONFIG_LIBPOSIX_PROCESS_SIGNAL */

/* The threads of @pprocess in wait() share one semaphore: up it once for
 * each, so each looks again.  One up could go to a thread waiting for
 * another child, which sleeps again, while the one it was for sleeps on.
 */
void pprocess_wake_waiters(struct posix_process *pprocess)
{
	struct posix_thread *pt, *ptn;
	int waiters = 0;

	uk_pprocess_foreach_pthread(pprocess, pt, ptn) {
		if (pt->state == POSIX_THREAD_BLOCKED_WAIT)
			waiters++;
	}
	/* A thread paused for a signal looks too (see pprocess_exit()) */
	if (!waiters)
		waiters = 1;
	while (waiters--)
		uk_semaphore_up(&pprocess->wait_semaphore);
}

/* Wait until no thread of @pprocess, the caller's, lends its stack to a
 * vfork child: the child runs on it until it execs or exits, which gives
 * it back.  Ending such a thread first would free the stack, and the
 * thread the child wakes, from under the child.
 */
/* Whether @self is @lender's vfork child, or one down a chain of them:
 * a vfork child that shares its process (CLONE_THREAD) gives the stack
 * back with its own execve or exit, so it cannot wait for its lenders.
 */
static __bool pthread_lends_to(struct posix_thread *lender,
			       struct posix_thread *self)
{
	struct posix_thread *q;

	for (q = lender; q && q->state == POSIX_THREAD_BLOCKED_VFORK;
	     q = q->vfork_child) {
		if (q->vfork_child == self)
			return true;
	}
	return false;
}

static __bool pprocess_lends_to_vfork(struct posix_process *pprocess)
{
	struct posix_thread *self = uk_pthread_current();
	struct posix_thread *pt, *ptn;

	uk_pprocess_foreach_pthread(pprocess, pt, ptn) {
		if (pt->state == POSIX_THREAD_BLOCKED_VFORK &&
		    !pthread_lends_to(pt, self))
			return true;
	}
	return false;
}

void pprocess_wait_vfork(struct posix_process *pprocess)
{
	/* Blocked, not yielding: the guest goes idle, so what the child
	 * waits for (the host, another thread) can come.
	 */
	uk_waitq_wait_event(&pprocess->vfork_waitq,
			    !pprocess_lends_to_vfork(pprocess));
}

/* Exit AND release pthread.
 * Notice: Unlike pprocesses, pthreads are released upon exit.
 */
void pprocess_exit_pthread(struct posix_thread *pthread,
			   enum posix_thread_state state,
			   int exit_status __unused)
{
	struct posix_process_ptexit_event_data event_data;
	struct posix_process *pprocess;
	struct posix_thread *parent_pthread;
	struct uk_thread *thread;

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
	pprocess_raise_ptexit_event(&event_data);

	/* Wake up parent if it was blocking on vfork for this child */
	if (parent_pthread &&
	    parent_pthread->state == POSIX_THREAD_BLOCKED_VFORK &&
	    parent_pthread->vfork_child == pthread) {
		parent_pthread->vfork_child = NULL;
		uk_thread_wake(parent_pthread->thread);
		parent_pthread->state = POSIX_THREAD_RUNNING;
		uk_waitq_wake_up(&parent_pthread->process->vfork_waitq);
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
	struct posix_process_ppexit_event_data event_data;
	struct posix_process *parent_process;
	struct posix_thread *pt, *ptn;
	__bool nowait = false;
	int ret __maybe_unused;

	UK_ASSERT(state == POSIX_PROCESS_EXITED ||
		  state == POSIX_PROCESS_KILLED);

	UK_ASSERT(pprocess);
	UK_ASSERT(pprocess->state == POSIX_PROCESS_RUNNING);

	/* The shutdown path ends processes from a thread that is none's */
	if (uk_pthread_current() &&
	    uk_pthread_current()->process == pprocess) {
		pprocess_wait_vfork(pprocess);
		/* Another of its threads may have ended it meanwhile */
		if (pprocess->state != POSIX_PROCESS_RUNNING)
			uk_sched_thread_exit();
	}

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
		ret = signal_exit(parent_process, pprocess);
		if (ret > 0) {
			/* From exit(2): "If the parent has set SA_NOCLDWAIT, or
			 * has set the SIGCHLD handler to SIG_IGN, the status is
			 * discarded and the child dies immediately."
			 */
			uk_pr_info("Parent ignores SIGHLD, terminating\n");
			nowait = true;
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
	event_data.pid = pprocess->pid;
	pprocess_raise_ppexit_event(&event_data);

	uk_semaphore_up(&pprocess->exit_semaphore);

	/* Unblock wait. Match on the exiting process, not the caller: the
	 * caller's pthread is gone by now (getpid() fails), and with kill(2)
	 * the caller is not the exiting process at all.
	 */
	if (parent_process && !nowait) {
		__bool wake = false;

		uk_pprocess_foreach_pthread(parent_process, pt, ptn) {
			if (PTHREAD_WAITING_FOR(pt, pprocess) ||
			    PTHREAD_BLOCKING_ON_SIGNAL(pt))
				wake = true;
		}
		if (wake)
			pprocess_wake_waiters(parent_process);
	} else if (nowait) { /* release now */
		pprocess_release(pprocess);
		/* Gone from its parent's children: a wait() of the parent's
		 * may now have none left (ECHILD)
		 */
		if (parent_process)
			pprocess_wake_waiters(parent_process);
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
__noreturn void pprocess_exit_stub(int status)
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
