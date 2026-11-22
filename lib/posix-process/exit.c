/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <stddef.h>

#include <uk/config.h>
#include <uk/process.h>
#include <uk/sched.h>
#include <uk/syscall.h>

#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
#include "signal/signal.h"
#include "signal/siginfo.h"
#include "signal.h"
#endif /* CONFIG_LIBPOSIX_PROCESS_SIGNAL */

#include "process.h"

#define PTHREAD_WAITING_FOR_PID(_pt, _pid)				\
	((_pt)->state == POSIX_THREAD_BLOCKED_WAIT &&			\
	 ((_pt)->wait_pid == (_pid) || (_pt)->wait_pid == -1))

int pprocess_exit_status = PPROCESS_EXIT_STATUS_UNSET;

#if CONFIG_LIBPOSIX_PROCESS_MULTITHREADING

/* pprocess exit state to pthread exit state */
#define PP2PT_EXIT_STATE(_pp_state)					\
	((_pp_state) == POSIX_PROCESS_EXITED ? POSIX_THREAD_EXITED :	\
					      POSIX_THREAD_KILLED)

struct child_exit_notice {
	struct posix_process *parent;
	bool reap;
#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
	int signum;
	struct uk_signal *signal;
	bool queued;
#endif
};

/* All helpers ending in _locked require pprocess_lock. */
static struct posix_process *reparent_target_locked(struct posix_process *child)
{
	struct posix_process *init = pid2pprocess(UK_PID_INIT);

	return init && init != child && !init->exiting &&
	       init->state == POSIX_PROCESS_RUNNING ? init : NULL;
}

static void reparent_child_locked(struct posix_process *child,
				  struct posix_process *parent)
{
	uk_list_del_init(&child->child_list_entry);
	child->parent = parent;
	child->orphaned = !parent;
	if (parent) {
		child->exit_signal = SIGCHLD;
		uk_list_add_tail(&child->child_list_entry, &parent->children);
	}
}

static struct posix_process *child_parent_locked(struct posix_process *child)
{
	struct posix_process *parent = child->parent;

	if (parent && parent->exiting) {
		parent = reparent_target_locked(child);
		reparent_child_locked(child, parent);
	}
	return parent;
}

/* Reserve outside the publication lock; parent is pinned by the caller. */
static void allocate_child_notice(struct child_exit_notice *notice __unused)
{
#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
	if (notice->parent) {
		notice->signal = uk_signal_alloc(notice->parent->_a);
		if (!notice->signal)
			UK_CRASH("Could not allocate child notification\n");
	}
#endif
}

static void discard_child_notice(struct child_exit_notice *notice)
{
#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
	if (notice->signal && !notice->queued)
		uk_signal_free(notice->signal->alloc, notice->signal);
#endif
	if (notice->parent)
		pprocess_put(notice->parent);
}

static int prepare_child_notice_locked(struct posix_process *child,
				       struct child_exit_notice *notice)
{
	struct posix_process *parent = child->parent;
	struct posix_thread *thread, *next;
	int rc = 0;

	if (child->reap_claimed)
		return 0;
	UK_ASSERT(parent == notice->parent);
	if (!parent) {
		/* Explicitly parentless kernel processes keep their owner. */
		if (child->orphaned) {
			child->reap_claimed = true;
			notice->reap = true;
		}
		return 0;
	}
#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
	{
		const struct kern_sigaction *action;

		notice->signum = child->exit_signal ?
			child->exit_signal : SIGCHLD;
		action = KERN_SIGACTION(parent, notice->signum);
		if (notice->signum == SIGCHLD &&
		    (action->ks_handler == SIG_IGN ||
		     (action->ks_flags & SA_NOCLDWAIT))) {
			child->reap_claimed = true;
			notice->reap = true;
			uk_list_del_init(&child->child_list_entry);
			child->parent = NULL;
		}
		if (action->ks_handler == SIG_IGN)
			notice->signum = 0;
		if (notice->signum) {
			UK_ASSERT(notice->signal);
			set_siginfo_wait(child, &notice->signal->siginfo);
			notice->signal->siginfo.si_signo = notice->signum;
			rc = pprocess_signal_queue_locked(parent, NULL,
							  notice->signal,
							  &notice->queued);
		}
	}
#endif
	/* Also wake auto-reap waiters so they can observe ECHILD. */
	uk_pprocess_foreach_pthread(parent, thread, next) {
		if (PTHREAD_WAITING_FOR_PID(thread, child->pid))
			uk_semaphore_up(&thread->wait_semaphore);
	}
	return rc;
}

/* Queue publication is complete; only callbacks and release remain. */
static void finish_child_notice(struct posix_process *child,
				struct child_exit_notice *notice)
{
#if CONFIG_LIBPOSIX_PROCESS_SIGNALFD
	if (notice->signum)
		pprocess_signal_files_notify(notice->parent, notice->signum);
#endif
	if (notice->reap)
		pprocess_release(child);
	discard_child_notice(notice);
}

static void pprocess_reparent_children(struct posix_process *process)
{
	for (;;) {
		struct child_exit_notice notice = { 0 };
		struct posix_process *child;
		unsigned long flags = pprocess_lock_acquire();
		bool waitable;
		int rc = 0;

		child = uk_list_first_entry_or_null(&process->children,
						    struct posix_process,
						    child_list_entry);
		if (!child) {
			pprocess_lock_release(flags);
			break;
		}
		pprocess_get(child);
		notice.parent = reparent_target_locked(child);
		if (notice.parent)
			pprocess_get(notice.parent);
		pprocess_lock_release(flags);
		allocate_child_notice(&notice);
		flags = pprocess_lock_acquire();
		if (child->parent != process || child->reap_claimed ||
		    reparent_target_locked(child) != notice.parent) {
			pprocess_lock_release(flags);
			discard_child_notice(&notice);
			pprocess_put(child);
			continue;
		}
		reparent_child_locked(child, notice.parent);
		waitable = child->state != POSIX_PROCESS_RUNNING;
		if (waitable)
			rc = prepare_child_notice_locked(child, &notice);
		pprocess_lock_release(flags);
		if (unlikely(rc))
			UK_CRASH("Could not notify adopted child (%d)\n", rc);
		if (waitable)
			finish_child_notice(child, &notice);
		else
			discard_child_notice(&notice);
		pprocess_put(child);
	}
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

bool pprocess_try_exit(struct posix_process *pprocess,
		       enum posix_process_state state,
		       int exit_status)
{
	struct posix_process_ppexit_event_data event_data;
	struct posix_thread *pt;

	UK_ASSERT(state == POSIX_PROCESS_EXITED ||
		  state == POSIX_PROCESS_KILLED);

	UK_ASSERT(pprocess);
	if (!pprocess_claim_process_exit(pprocess))
		return false;

	uk_pr_debug("pid %d: exit process\n", pprocess->pid);

	/* Claim and terminate remaining pthreads one by one without holding
	 * pprocess_lock across exit events or scheduler termination.
	 */
	for (;;) {
		unsigned long pflags = pprocess_lock_acquire();

		pt = uk_list_first_entry_or_null(&pprocess->threads,
						 struct posix_thread,
						 thread_list_entry);
		if (pt)
			uk_list_del_init(&pt->thread_list_entry);
		pprocess_lock_release(pflags);

		if (!pt)
			break;
		pprocess_exit_pthread(pt, PP2PT_EXIT_STATE(state), exit_status);
	}

	/* Unlinked threads may still be completing their exit events. */
	for (;;) {
		unsigned long pflags = pprocess_lock_acquire();
		bool complete = !pprocess->exiting_threads;

		pprocess_lock_release(pflags);
		if (complete)
			break;
		uk_semaphore_down(&pprocess->thread_exit_semaphore);
	}

	/* ---> No ops at the posix level on behalf of the current <---
	 * ---> thread from this point                             <---
	 */
	UK_ASSERT(uk_list_empty(&pprocess->threads));

	/* Reparent child processes to init */
	pprocess_reparent_children(pprocess);

	/* Notify handlers */
	event_data.pid = pprocess->pid;
	pprocess_raise_ppexit_event(&event_data);

	/* Allocation stays outside the atomic state/queue publication. */
	for (;;) {
		struct child_exit_notice notice = { 0 };
		unsigned long flags = pprocess_lock_acquire();
		int rc;

		if (!pprocess->reap_claimed) {
			notice.parent = child_parent_locked(pprocess);
			if (notice.parent)
				pprocess_get(notice.parent);
		}
		pprocess_lock_release(flags);
		allocate_child_notice(&notice);
		flags = pprocess_lock_acquire();
		if (!pprocess->reap_claimed &&
		    child_parent_locked(pprocess) != notice.parent) {
			pprocess_lock_release(flags);
			discard_child_notice(&notice);
			continue;
		}
		pprocess->exit_status = state == POSIX_PROCESS_EXITED ?
			exit_status & 0xff : exit_status;
		pprocess->state = state;
		rc = prepare_child_notice_locked(pprocess, &notice);
		pprocess_lock_release(flags);
		if (unlikely(rc))
			UK_CRASH("Could not signal parent (%d)\n", rc);
		uk_semaphore_up(&pprocess->exit_semaphore);
		finish_child_notice(pprocess, &notice);
		break;
	}
	pprocess_put(pprocess);
	return true;
}

void pprocess_exit(struct posix_process *pprocess,
		   enum posix_process_state state,
		   int exit_status)
{
	(void)pprocess_try_exit(pprocess, state, exit_status);
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
	struct posix_process *pprocess;
	struct posix_thread *this_pthread;
	struct uk_thread *this_thread;
	bool became_empty = false;

	this_thread = uk_thread_current();
	this_pthread = pprocess_claim_pthread_exit(this_thread, &became_empty);
	if (this_pthread) {
		pprocess = this_pthread->process;
		pprocess_exit_pthread(this_pthread, POSIX_THREAD_EXITED,
				      status);
		pprocess_complete_pthread_exit(pprocess);
		if (became_empty)
			pprocess_exit(pprocess, POSIX_PROCESS_EXITED, status);
		pprocess_put(pprocess);
	}
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
