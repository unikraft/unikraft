/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *          Felipe Huici <felipe.huici@neclab.eu>
 *          Costin Lupu <costin.lupu@cs.pub.ro>
 *
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
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
#include <uk/config.h>
#include <sys/types.h>
#include <stddef.h>
#include <errno.h>
#include <uk/config.h>
#include <uk/syscall.h>

#if CONFIG_LIBPOSIX_PROCESS_PIDS
#include <uk/bitmap.h>
#include <uk/list.h>
#include <uk/alloc.h>
#include <uk/sched.h>
#include <uk/thread.h>
#include <uk/init.h>
#include <uk/errptr.h>
#include <uk/essentials.h>
#include <uk/process.h>

#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
#include "signal/signal.h"
#endif /* CONFIG_LIBPOSIX_PROCESS_SIGNAL */

#include "process.h"

#if CONFIG_LIBPOSIX_PROCESS_GRACEFUL_SHUTDOWN
#include <uk/semaphore.h>

#define GRACEFUL_SHUTDOWN_TIMEOUT_NSEC					\
	(CONFIG_LIBPOSIX_PROCESS_GRACEFUL_SHUTDOWN_TIMEOUT_MS * 1000000UL)
#define GRACEFUL_SHUTDOWN_SIGNAL					\
	CONFIG_LIBPOSIX_PROCESS_GRACEFUL_SHUTDOWN_SIGNAL
#endif /* CONFIG_LIBPOSIX_PROCESS_GRACEFUL_SHUTDOWN */

/**
 * System global lists
 * NOTE: We pre-allocate PID/TID 0 which is reserved by the kernel.
 *       An application should never get PID/TID 0 assigned.
 */
static struct posix_thread *tid_thread[TIDMAP_SIZE];
static unsigned long tid_map[UK_BITS_TO_LONGS(TIDMAP_SIZE)] = { [0] = 0x01UL };

/* Process Table */
struct posix_process *pid_process[TIDMAP_SIZE];

/**
 * Thread-local posix_thread reference
 */
static __uk_tls struct posix_thread *pthread_self = NULL;

/**
 * Helpers to find and reserve a `pid_t`
 */
static inline pid_t find_free_tid(void)
{
	static pid_t prev = 0;
	unsigned long found;

	/* search starting from last position */
	found = uk_find_next_zero_bit(tid_map, TIDMAP_SIZE, prev);
	if (found == TIDMAP_SIZE) {
		/* search again starting from the beginning */
		found = uk_find_first_zero_bit(tid_map, TIDMAP_SIZE);
	}
	if (found == TIDMAP_SIZE) {
		/* no free PID */
		return -1;
	}

	prev = found;
	return found;
}

static pid_t find_and_reserve_tid(void)
{
	pid_t tid;

	/* TODO: Mutex */
	tid = find_free_tid();
	if (tid > 0)
		uk_set_bit(tid, tid_map);
	return tid;
}

static void release_tid(pid_t tid)
{
	UK_ASSERT(tid > 0 && tid <= CONFIG_LIBPOSIX_PROCESS_MAX_PID);

	/* TODO: Mutex */
	uk_clear_bit(tid, tid_map);
}

/* Allocate a thread for a process */
static struct posix_thread *pprocess_create_pthread(
			struct posix_process *pprocess, struct uk_thread *th)
{
	struct posix_thread *pthread;
	struct uk_alloc *a;
	pid_t tid;
	int err;

	UK_ASSERT(pprocess);
	UK_ASSERT(pprocess->_a);

	/* Take allocator from process */
	a = pprocess->_a;

	tid = find_and_reserve_tid();
	if (tid < 0) {
		err = -EAGAIN;
		goto err_out;
	}

	pthread = uk_zalloc(a, sizeof(*pthread));
	if (!pthread) {
		err = -ENOMEM;
		goto err_free_tid;
	}

	pthread->_a = a;
	pthread->process = pprocess;
	pthread->tid = tid;
	pthread->thread = th;
	uk_list_add_tail(&pthread->thread_list_entry, &pprocess->threads);

#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
	err = pprocess_signal_tdesc_alloc(pthread);
	if (unlikely(err)) {
		uk_pr_err("Could not allocate signal descriptor\n");
		goto err_free_tid;
	}
	err = pprocess_signal_tdesc_init(pthread);
	if (unlikely(err)) {
		uk_pr_err("Could not initialize signal descriptor\n");
		goto err_free_tid;
	}
#endif /* CONFIG_LIBPOSIX_PROCESS_SIGNAL */

	/* Store reference to pthread with TID */
	tid_thread[tid] = pthread;

	uk_pr_debug("Process PID %d: New thread TID %d\n",
		    (int) pprocess->pid, (int) pthread->tid);
	return pthread;

err_free_tid:
	release_tid(tid);
err_out:
	return ERR2PTR(err);
}

static void pprocess_remove_from_parent(struct posix_process *pprocess)
{
	struct posix_process *pchild, *pchildn;

	if (pprocess->pid == UK_PID_INIT)
		return;

	uk_list_for_each_entry_safe(pchild, pchildn,
				    &pprocess->parent->children,
				    child_list_entry) {
		if (pchild->pid == pprocess->pid) {
			uk_list_del(&pchild->child_list_entry);
			break;
		}
	}
}

/* Free thread that is part of a process
 * NOTE: The process is not free'd here when its thread list
 *       becomes empty.
 */
static void pprocess_release_pthread(struct posix_thread *pthread)
{
	UK_ASSERT(pthread);
	UK_ASSERT(pthread->_a);

#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
	pprocess_signal_tdesc_free(pthread);
#endif /* CONFIG_LIBPOSIX_PROCESS_SIGNAL */

	/* remove from process' thread list */
	uk_list_del_init(&pthread->thread_list_entry);

	/* release TID */
	release_tid(pthread->tid);
	tid_thread[pthread->tid] = NULL;

	/* release memory */
	uk_free(pthread->_a, pthread);
}

static void pprocess_release(struct posix_process *pprocess);

/* Create a new posix process for a given thread */
int uk_posix_process_create(struct uk_alloc *a,
			    struct uk_thread *thread,
			    struct uk_thread *parent)
{
	struct posix_thread  *parent_pthread  = NULL;
	struct posix_process *parent_pprocess = NULL;
	struct posix_thread  **pthread;
	struct posix_thread  *_pthread;
	struct posix_process *pprocess;
	struct posix_process *orig_pprocess;
	int ret;

	/* Retrieve a reference to the `pthread_self` pointer on the remote TLS:
	 * Allows us changing the pointer value.
	 */
	pthread = &uk_thread_uktls_var(thread, pthread_self);

	if (parent)
		parent_pthread = uk_thread_uktls_var(parent, pthread_self);
	if (parent_pthread) {
		 /* if we have a parent pthread,
		  *  it must have a surrounding pprocess
		  */
		UK_ASSERT(parent_pthread->process);

		parent_pprocess = parent_pthread->process;
	}

	/* Allocate pprocess structure */
	pprocess = uk_zalloc(a, sizeof(*pprocess));
	if (!pprocess) {
		ret = -ENOMEM;
		goto err_out;
	}
	pprocess->_a = a;
	UK_INIT_LIST_HEAD(&pprocess->threads);
	UK_INIT_LIST_HEAD(&pprocess->children);

#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
	ret = pprocess_signal_pdesc_alloc(pprocess);
	if (unlikely(!pprocess->signal)) {
		uk_pr_err("Could not allocate signal descriptor\n");
		goto err_out;
	}
	ret = pprocess_signal_pdesc_init(pprocess);
	if (unlikely(ret)) {
		uk_pr_err("Could not initialize signal descriptor\n");
		goto err_out;
	}
#endif /* CONFIG_LIBPOSIX_PROCESS_SIGNAL */

	/* Initialize semaphore for wait() */
	uk_semaphore_init(&pprocess->wait_semaphore, 0);
	uk_semaphore_init(&pprocess->exit_semaphore, 0);

	/* Check if we have a pthread structure already for this thread
	 * or if we need to allocate one
	 */
	if (!(*pthread)) {
		_pthread = pprocess_create_pthread(pprocess, thread);
		if (PTRISERR(_pthread)) {
			ret = PTR2ERR(_pthread);
			goto err_free_pprocess;
		}
		/* take thread id for process id */
		pprocess->pid = _pthread->tid;
		*pthread = _pthread;
	} else {
		/* Re-use existing pthread, re-link it and re-use its TID */
		UK_ASSERT((*pthread)->thread == thread);

		/* Remove thread from original process */
		orig_pprocess = (*pthread)->process;
		uk_list_del(&(*pthread)->thread_list_entry);

		/* Re-assign thread to current process */
		(*pthread)->process = pprocess;
		pprocess->pid = (*pthread)->tid;
		uk_list_add_tail(&(*pthread)->thread_list_entry,
				 &pprocess->threads);

#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
		/* Reset signal state of this thread */
		ret = pprocess_signal_tdesc_init(*pthread);
		if (unlikely(ret)) {
			uk_pr_err("Could not initialize signal descriptor\n");
			goto err_out;
		}
#endif /* CONFIG_LIBPOSIX_PROCESS_SIGNAL */

		/* Release original process if it became empty of threads */
		if (uk_list_empty(&orig_pprocess->threads))
			pprocess_release(orig_pprocess);
	}

	pprocess->parent = parent_pprocess;
	if (parent_pprocess) {
		uk_list_add_tail(&pprocess->child_list_entry,
				 &parent_pprocess->children);
	}

	/* Add to process table */
	if ((unsigned long)pprocess->pid >= ARRAY_SIZE(pid_process)) {
		uk_pr_err("Process limit reached, could not create new process\n");
		ret = -EAGAIN;
		goto err_free_pprocess;
	}
	pid_process[pprocess->pid] = pprocess;

	uk_pr_debug("Process PID %d created (parent PID: %d)\n",
		    (int) pprocess->pid,
		    (int) ((pprocess->parent) ? pprocess->parent->pid : 0));
	return 0;

err_free_pprocess:
	uk_free(a, pprocess);
err_out:
	return ret;
}

/* Releases pprocess memory.
 * NOTE: All related threads must be removed already from this pprocess
 */
static void pprocess_release(struct posix_process *pprocess)
{
	UK_ASSERT(uk_list_empty(&pprocess->threads));

	pprocess_remove_from_parent(pprocess);

#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
	pprocess_signal_pdesc_free(pprocess);
#endif /* CONFIG_LIBPOSIX_PROCESS_SIGNAL */

	pid_process[pprocess->pid] = NULL;

	uk_pr_debug("Process PID %d released\n",
		    pprocess->pid);
	uk_free(pprocess->_a, pprocess);
}

static void pprocess_kill(struct posix_process *pprocess)
{
	struct posix_thread *pthread, *pthreadn, *pthread_self = NULL;

	/* Kill all remaining threads of the process */
	uk_list_for_each_entry_safe(pthread, pthreadn,
				    &pprocess->threads, thread_list_entry) {
		/* Double-check that this thread is part of this process */
		UK_ASSERT(pthread->process == pprocess);

		if (pthread->thread == uk_thread_current()) {
			/* Self-destruct this thread as last work of this
			 * function. The reason is that nothing of this
			 * function is executed anymore as soon as the
			 * thread killed itself.
			 */
			pthread_self = pthread;
			continue;
		}
		if (uk_thread_is_exited(pthread->thread)) {
			/* Thread already exited, might wait for getting
			 * garbage collected.
			 */
			continue;
		}

		uk_pr_debug("Terminating PID %d: Killing TID %d: thread %p (%s)...\n",
			    pprocess->pid, pthread->tid,
			    pthread->thread, pthread->thread->name);

		/* Terminating the thread will lead to calling
		 * `posix_thread_fini()` which will clean-up the related
		 * pthread resources and pprocess resources on the last
		 * thread
		 */
		uk_sched_thread_terminate(pthread->thread);
	}

	if (pthread_self) {
		uk_pr_debug("Terminating PID %d: Self-killing TID %d...\n",
			    pprocess->pid, pthread_self->tid);
		uk_sched_thread_terminate(uk_thread_current());

		/* NOTE: Nothing will be executed from here on */
	}
}

void uk_posix_process_kill_siblings(struct uk_thread *thread)
{
	struct posix_thread *pthread, *pthreadn;
	struct posix_thread *this_thread;
	struct posix_process *pprocess;
	pid_t this_tid;

	this_tid = ukthread2tid(thread);
	this_thread = tid2pthread(this_tid);

	pprocess = this_thread->process;
	UK_ASSERT(pprocess);

	/* Kill all remaining threads of the process */
	uk_list_for_each_entry_safe(pthread, pthreadn,
				    &pprocess->threads, thread_list_entry) {
		if (pthread->tid == this_tid)
			continue;

		if (uk_thread_is_exited(pthread->thread)) {
			/* Thread already exited, might wait for getting
			 * garbage collected.
			 */
			continue;
		}

		uk_pr_debug("Terminating siblings of tid: %d (pid: %d): Killing TID %d: thread %p (%s)...\n",
			    this_thread->tid, pprocess->pid,
			    pthread->tid, pthread->thread,
			    pthread->thread->name);

		/* Terminating the thread will lead to calling
		 * `posix_thread_fini()` which will clean-up the related
		 * pthread resources and pprocess resources on the last
		 * thread
		 */
		uk_sched_thread_terminate(pthread->thread);
	}
}

void uk_posix_process_kill(struct uk_thread *thread)
{
	struct posix_thread  **pthread;
	struct posix_process *pprocess;

	pthread = &uk_thread_uktls_var(thread, pthread_self);

	UK_ASSERT(*pthread);
	UK_ASSERT((*pthread)->process);

	pprocess = (*pthread)->process;
	pprocess_kill(pprocess);
}

#if CONFIG_LIBPOSIX_PROCESS_PIDS
static int posix_process_init(struct uk_init_ctx *ictx __unused)
{
	/* Create a POSIX process without parent ("init" process) */
	return uk_posix_process_create(uk_alloc_get_default(),
				       uk_thread_current(), NULL);
}

uk_late_initcall(posix_process_init, 0x0);
#endif /* CONFIG_LIBPOSIX_PROCESS_PIDS */

/* Thread initialization: Assign posix thread only if parent is part of a
 * process
 */
static int posix_thread_init(struct uk_thread *child, struct uk_thread *parent)
{
	struct posix_thread *parent_pthread = NULL;
	struct posix_thread *pthread;

	if (parent) {
		parent_pthread = uk_thread_uktls_var(parent,
						     pthread_self);
	}
	if (!parent_pthread) {
		/* parent has no posix thread, do not setup one for the child */
		uk_pr_debug("thread %p (%s): Parent %p (%s) without process context, skipping...\n",
			    child, child->name, parent,
			    parent ? parent->name : "<n/a>");
		pthread_self = NULL;
		return 0;
	}

	UK_ASSERT(parent_pthread->process);

	pthread = pprocess_create_pthread(parent_pthread->process,
					  child);
	if (PTRISERR(pthread))
		return PTR2ERR(pthread);

	pthread_self = pthread;

	uk_pr_debug("thread %p (%s): New thread with TID: %d (PID: %d)\n",
		    child, child->name, (int) pthread->tid,
		    (int) pthread->process->pid);
	return 0;
}

/* Thread release: Release TID and posix_thread */
static void posix_thread_fini(struct uk_thread *child)
{
	struct posix_process *pprocess;

	if (!pthread_self)
		return; /* no posix thread was assigned */

	pprocess = pthread_self->process;

	UK_ASSERT(pprocess);

	uk_pr_debug("thread %p (%s): Releasing thread with TID: %d (PID: %d)\n",
		    child, child->name, (int) pthread_self->tid,
		    (int) pprocess->pid);
	pprocess_release_pthread(pthread_self);
	pthread_self = NULL;

	/* Release process if it became empty of threads */
	if (uk_list_empty(&pprocess->threads))
		pprocess_release(pprocess);
}

UK_THREAD_INIT_PRIO(posix_thread_init, posix_thread_fini, UK_PRIO_EARLIEST);

struct posix_process *pid2pprocess(pid_t pid)
{
	UK_ASSERT((size_t)pid < ARRAY_SIZE(pid_process));

	return pid_process[pid];
}

struct posix_thread *tid2pthread(pid_t tid)
{
	if (tid >= CONFIG_LIBPOSIX_PROCESS_MAX_PID || tid < 0)
		return NULL;
	return tid_thread[tid];
}

struct posix_process *tid2pprocess(pid_t tid)
{
	struct posix_thread *pthread;

	pthread = tid2pthread(tid);
	if (!pthread)
		return NULL;

	return pthread->process;
}

struct uk_thread *tid2ukthread(pid_t tid)
{
	struct posix_thread *pthread;

	pthread = tid2pthread(tid);
	if (!pthread)
		return NULL;

	return pthread->thread;
}

pid_t ukthread2tid(struct uk_thread *thread)
{
	struct posix_thread *pthread;

	pthread = uk_thread_uktls_var(thread, pthread_self);
	if (!pthread)
		return -ENOTSUP;

	return pthread->tid;
}

pid_t ukthread2pid(struct uk_thread *thread)
{
	struct posix_thread *pthread;

	pthread = uk_thread_uktls_var(thread, pthread_self);
	if (!pthread)
		return -ENOTSUP;

	UK_ASSERT(pthread->process);

	return pthread->process->pid;
}

UK_SYSCALL_R_DEFINE(pid_t, getpid)
{
	if (!pthread_self)
		return -ENOTSUP;

	UK_ASSERT(pthread_self->process);
	return pthread_self->process->pid;
}

UK_SYSCALL_R_DEFINE(pid_t, gettid)
{
	if (!pthread_self)
		return -ENOTSUP;

	return pthread_self->tid;
}

/* PID of parent process  */
UK_SYSCALL_R_DEFINE(pid_t, getppid)
{
	if (!pthread_self)
		return -ENOTSUP;

	UK_ASSERT(pthread_self->process);

	if (!pthread_self->process->parent) {
		 /* no parent, return 0 */
		return 0;
	}

	return pthread_self->process->parent->pid;
}

void pprocess_exit(struct uk_thread *thread, enum posix_thread_state state,
		   int exit_status)
{
	struct posix_thread *ptchild, *ptchildn;
	struct posix_process *pchild, *pchildn;
	struct posix_process *parent_process;
	struct posix_process *pprocess_init;
	struct posix_thread *pthread;
	int ret;

	UK_ASSERT(state == POSIX_THREAD_EXITED ||
		  state == POSIX_THREAD_KILLED);

	pprocess_init = pid2pprocess(UK_PID_INIT);
	UK_ASSERT(pprocess_init);

	pthread = uk_thread_uktls_var(thread, pthread_self);
	UK_ASSERT(pthread);
	UK_ASSERT(!pthread->process->terminated);

	parent_process = pthread->process->parent;

	/* Update process status */
	if (state == POSIX_THREAD_EXITED) {
		pthread->state = POSIX_THREAD_EXITED;
		pthread->process->exit_status = exit_status & 0xff;
	} else { /* POSIX_THREAD_KILLED */
		pthread->state = POSIX_THREAD_KILLED;
		pthread->process->exit_status = exit_status;
	}
	pthread->process->terminated = true;

	pprocess_remove_from_parent(pthread->process);

	uk_posix_process_kill_siblings(pthread->thread);

	/* Reparent child processes to init */
	uk_list_for_each_entry_safe(pchild, pchildn,
				    &pthread->process->children,
				    child_list_entry) {
		uk_list_del(&pchild->child_list_entry);
		pchild->parent = pprocess_init;
		uk_list_add(&pchild->child_list_entry,
			    &pprocess_init->children);

		uk_pr_debug("pid %d reparented to init\n", pchild->pid);
	}

#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
	ret = pprocess_signal_send(parent_process, SIGCHLD, NULL);
	if (unlikely(ret))
		UK_CRASH("Could not signal parent\n");

	/* If the parent has set the disposition of SIGCHLD to SIG_IGN,
	 * or has set SA_NOCLDWAIT, then terminate the child right away.
	 */
	if (parent_process->signal->sigaction[SIGCHLD].ks_handler == SIG_IGN ||
	    parent_process->signal->sigaction[SIGCHLD].ks_flags & SA_NOCLDWAIT) {
		uk_pr_err("Parent ignores SIGHLD, terminating\n");
		pprocess_kill(pthread->process);
	}
#endif /* CONFIG_LIBPOSIX_PROCESS_SIGNAL */

	/* If the parent is waiting, write the status and notify the
	 * parent that the process is terminated. We will let wait()
	 * reap the child so that it can obtain its exit_status first.
	 *
	 * Finally, if the parent is not waiting, and the parent did not
	 * set the disposition of SIGCHLD to SIG_IGN or set SA_NOCLDWAIT,
	 * this process becomes a zombie. Keep the process struct so that
	 * the parent can retrieve basic info later via waitpid().
	 */
	uk_process_foreach_pthread(parent_process, ptchild, ptchildn) {
		if (ptchild->state == POSIX_THREAD_BLOCKED_SIGNAL ||
		    (ptchild->state == POSIX_THREAD_BLOCKED_WAIT &&
		     (ptchild->wait_pid == uk_getpid() ||
		      ptchild->wait_pid == UK_PID_WAIT_ANY))) {
			uk_semaphore_up(&parent_process->wait_semaphore);
			break;
		}
	}

	uk_semaphore_up(&pthread->process->exit_semaphore);

	/* This process is now a zombie. Remove from the scheduler */
	uk_thread_block(pthread->thread);
	if (pthread->thread == uk_thread_current())
		uk_sched_yield();
}

#if CONFIG_LIBPOSIX_PROCESS_GRACEFUL_SHUTDOWN
static bool try_graceful_shutdown(struct posix_process *pproc)
{
	__nsec timeout;

	UK_ASSERT(pproc);

	timeout = GRACEFUL_SHUTDOWN_TIMEOUT_NSEC;

	uk_pr_info("pid %d sending termination signal\n", pproc->pid);
	pprocess_signal_send(pproc, GRACEFUL_SHUTDOWN_SIGNAL, NULL);
	uk_semaphore_down_to(&pproc->exit_semaphore, timeout);

	return pproc->terminated;
}
#endif /* CONFIG_LIBPOSIX_PROCESS_GRACEFUL_SHUTDOWN */

static void
pprocess_system_shutdown(const struct uk_term_ctx *ctx __unused)
{
	struct posix_process *pproc_init, *pproc;
	struct posix_thread *pthread;
	int rc;

	pproc_init = pid2pprocess(UK_PID_INIT);
	UK_ASSERT(pproc_init);

	while ((pproc = uk_list_first_entry_or_null(&pproc_init->children,
						    struct posix_process,
						    child_list_entry))) {
		/* Skip zombies */
		if (pproc->terminated)
			continue;

#if CONFIG_LIBPOSIX_PROCESS_GRACEFUL_SHUTDOWN
		if (try_graceful_shutdown(pproc) == true) {
			uk_pr_info("pid %d terminated gracefully\n",
				   pproc->pid);
			continue;
		}
		uk_pr_info("pid %d still running\n", pproc->pid);
#endif /* CONFIG_LIBPOSIX_PROCESS_GRACEFUL_SHUTDOWN */

		pthread = uk_list_first_entry_or_null(&pproc->threads,
						      struct posix_thread,
						      thread_list_entry);
		UK_ASSERT(pthread);
		pprocess_exit(pthread->thread, POSIX_THREAD_KILLED, 1);
		uk_pr_info("pid %d killed\n", pproc->pid);
	}

	/* Now reap any children */
	do {
		rc = uk_posix_process_wait();
	} while (rc != -ECHILD);
}

/* init last, term first */
uk_late_initcall(0, pprocess_system_shutdown);

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

	if (uk_list_is_singular(&this_pthread->process->threads)) {
		pprocess_exit(this_thread, POSIX_THREAD_EXITED, status);
		/* noreturn when called by current process */
		UK_BUG();
	}

	uk_sched_thread_exit(); /* noreturn */
	UK_CRASH("sys_exit() unexpectedly returned\n");
	return -EFAULT;
}

UK_LLSYSCALL_R_DEFINE(int, exit_group, int, status)
{
	pprocess_exit(uk_thread_current(), POSIX_THREAD_EXITED, status);
	UK_CRASH("sys_exit_group() unexpectedly returned\n");
	return -EFAULT;
}

#if UK_LIBC_SYSCALLS
__noreturn void exit(int status)
{
	uk_syscall_r_exit_group(status);
	UK_CRASH("sys_exit_group() unexpectedly returned\n");
}

__noreturn void exit_group(int status)
{
	uk_syscall_r_exit_group(status);
	UK_CRASH("sys_exit_group() unexpectedly returned\n");
}
#endif /* UK_LIBC_SYSCALLS */

#if CONFIG_LIBPOSIX_PROCESS_CLONE
/* Store child PID at given location for parent */
static int pprocess_parent_settid(const struct clone_args *cl_args,
				  size_t cl_args_len __unused,
				  struct uk_thread *child,
				  struct uk_thread *parent __unused)
{
	pid_t child_tid = ukthread2tid(child);

	UK_ASSERT(child_tid > 0);

	if (!cl_args->parent_tid)
		return -EINVAL;

	*((pid_t *) cl_args->parent_tid) = child_tid;
	return 0;
}
UK_POSIX_CLONE_HANDLER(CLONE_PARENT_SETTID, true, pprocess_parent_settid, 0x0);

/* Store child PID at given location in child */
static int pprocess_child_settid(const struct clone_args *cl_args,
				 size_t cl_args_len __unused,
				 struct uk_thread *child,
				 struct uk_thread *parent __unused)
{
	pid_t child_tid = ukthread2tid(child);

	UK_ASSERT(child_tid > 0);

	if (!cl_args->child_tid)
		return -EINVAL;

	*((pid_t *) cl_args->child_tid) = child_tid;
	return 0;
}
UK_POSIX_CLONE_HANDLER(CLONE_CHILD_SETTID, true, pprocess_child_settid, 0x0);

static int pprocess_clone_thread(const struct clone_args *cl_args __unused,
				 size_t cl_args_len __unused,
				 struct uk_thread *child __unused,
				 struct uk_thread *parent __unused)
{
	UK_WARN_STUBBED();

	return 0;
}
UK_POSIX_CLONE_HANDLER(CLONE_THREAD, false, pprocess_clone_thread, 0x0);
#endif /* CONFIG_LIBPOSIX_PROCESS_CLONE */
#else  /* !CONFIG_LIBPOSIX_PROCESS_PIDS */

#define UNIKRAFT_PID      1
#define UNIKRAFT_TID      1
#define UNIKRAFT_PPID     0

UK_SYSCALL_R_DEFINE(int, getpid)
{
	return UNIKRAFT_PID;
}

UK_SYSCALL_R_DEFINE(int, gettid)
{
	return UNIKRAFT_TID;
}

UK_SYSCALL_R_DEFINE(pid_t, getppid)
{
	return UNIKRAFT_PPID;
}

#endif /* !CONFIG_LIBPOSIX_PROCESS_PIDS */
