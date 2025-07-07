/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
 * Copyright (c) 2022, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
 * Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/config.h>
#include <sys/types.h>
#include <stddef.h>
#include <errno.h>
#include <uk/config.h>
#include <uk/syscall.h>

struct uk_thread *pprocess_thread_main;

#if CONFIG_LIBPOSIX_PROCESS_MULTITHREADING
#include <signal.h> /* SIGCHLD */

#include <uk/bitops/bitmap.h>
#include <uk/list.h>
#include <uk/alloc.h>
#include <uk/sched.h>
#include <uk/thread.h>
#include <uk/init.h>
#include <uk/errptr.h>
#include <uk/essentials.h>
#include <uk/plat/config.h>
#include <uk/process.h>

#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
#include "signal/signal.h"
#endif /* CONFIG_LIBPOSIX_PROCESS_SIGNAL */

#include "process.h"

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
__uk_tls struct posix_thread *pthread_self = NULL;

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
		uk_pr_err("Could not allocate TID: Out of TIDs (configured max: %d)\n",
			  CONFIG_LIBPOSIX_PROCESS_MAX_PID);
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
struct posix_thread *pprocess_create_pthread(struct posix_process *pprocess,
					     struct uk_thread *th)
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
	if (unlikely(tid < 0)) {
		err = -EAGAIN;
		goto err_out;
	}

	pthread = uk_zalloc(a, sizeof(*pthread));
	if (unlikely(!pthread)) {
		err = -ENOMEM;
		goto err_free_tid;
	}

	pthread->_a = a;
	pthread->process = pprocess;
	pthread->parent = uk_pthread_current();
	pthread->tid = tid;
	pthread->thread = th;

	uk_thread_uktls_var(th, pthread_self) = pthread;

#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
	err = pprocess_signal_tdesc_alloc(pthread);
	if (unlikely(err)) {
		uk_pr_err("Could not allocate signal descriptor\n");
		/* Handle rollback manually to avoid adding more
		 * signal conditionals.
		 */
		uk_free(a, pthread);
		goto err_free_tid;
	}
	err = pprocess_signal_tdesc_init(pthread);
	if (unlikely(err)) {
		uk_pr_err("Could not initialize signal descriptor\n");
		/* Handle rollback manually to avoid adding more
		 * signal conditionals.
		 */
		pprocess_signal_tdesc_free(pthread);
		uk_free(a, pthread);
		goto err_free_tid;
	}
#endif /* CONFIG_LIBPOSIX_PROCESS_SIGNAL */

	/* Store reference to pthread with TID */
	tid_thread[tid] = pthread;

	/* Add to parent's list of threads */
	uk_list_add_tail(&pthread->thread_list_entry, &pprocess->threads);

	uk_pr_debug("Process PID %d: New thread TID %d\n",
		    (int) pprocess->pid, (int) pthread->tid);
	return pthread;

err_free_tid:
	release_tid(tid);
err_out:
	return ERR2PTR(err);
}

/* Free thread that is part of a process
 * NOTE: The process is not released here when its thread list
 *       becomes empty.
 */
void pprocess_release_pthread(struct posix_thread *pthread)
{
	UK_ASSERT(pthread);
	UK_ASSERT(pthread->_a);
	UK_ASSERT(pthread->process);

	uk_pr_debug("pid %d: Release tid %d\n",
		    pthread->process->pid, pthread->tid);

#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
	pprocess_signal_tdesc_free(pthread);
#endif /* CONFIG_LIBPOSIX_PROCESS_SIGNAL */

	uk_thread_uktls_var(pthread->thread, pthread_self) = NULL;

	/* remove from process' thread list */
	uk_list_del_init(&pthread->thread_list_entry);

	/* release TID */
	release_tid(pthread->tid);
	tid_thread[pthread->tid] = NULL;

	/* release memory */
	uk_free(pthread->_a, pthread);
}

int uk_posix_process_create_pthread(struct uk_thread *thread)
{
	struct posix_process *pprocess;
	struct posix_thread *pthread;

	pprocess = uk_pprocess_current();
	UK_ASSERT(pprocess);

	pthread = pprocess_create_pthread(pprocess, thread);
	if (unlikely(PTRISERR(pthread)))
		return PTR2ERR(pthread);

	return 0;
}

/* Create a new posix process for a given thread */
int pprocess_create(struct uk_alloc *a,
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

	pprocess->state = POSIX_PROCESS_RUNNING;

#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
	ret = pprocess_signal_pdesc_alloc(pprocess);
	if (unlikely(ret)) {
		uk_pr_err("Could not allocate signal descriptor\n");
		/* Free manually as we can jump to err_free_pprocess
		 * after this allocation is successful.
		 */
		uk_free(a, pprocess);
		goto err_out;
	}
	ret = pprocess_signal_pdesc_init(pprocess);
	if (unlikely(ret)) {
		uk_pr_err("Could not initialize signal descriptor\n");
		goto err_free_pprocess;
	}
#endif /* CONFIG_LIBPOSIX_PROCESS_SIGNAL */

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

		/* Update parent */
		(*pthread)->parent = parent_pthread;
#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
		/* Reset signal state of this thread */
		ret = pprocess_signal_tdesc_init(*pthread);
		if (unlikely(ret)) {
			uk_pr_err("Could not initialize signal descriptor\n");
			goto err_free_pprocess;
		}
#endif /* CONFIG_LIBPOSIX_PROCESS_SIGNAL */

		/* Release original process if it became empty of threads */
		if (uk_list_empty(&orig_pprocess->threads))
			pprocess_release(orig_pprocess);
	}

	/* Add to process table. No failure past this point. */
	if (unlikely((unsigned long)pprocess->pid >= ARRAY_SIZE(pid_process))) {
		uk_pr_err("Process limit reached, could not create new process\n");
		ret = -EAGAIN;
		goto err_free_pprocess;
	}

	pprocess->parent = parent_pprocess;
	if (parent_pprocess) {
		uk_list_add_tail(&pprocess->child_list_entry,
				 &parent_pprocess->children);
	}
	pid_process[pprocess->pid] = pprocess;

	uk_pr_debug("Process PID %d created (parent PID: %d)\n",
		    (int) pprocess->pid,
		    (int) ((pprocess->parent) ? pprocess->parent->pid : 0));
	return 0;

err_free_pprocess:
#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
	pprocess_signal_pdesc_free(pprocess);
#endif /* CONFIG_LIBPOSIX_PROCESS_SIGNAL */
	uk_free(a, pprocess);
err_out:
	return ret;
}

#if CONFIG_LIBPOSIX_PROCESS_MULTIPROCESS
pid_t uk_posix_process_run(uk_posix_process_mainlike_func fn,
			   int argc, const char *argv[])
{
	struct posix_process_execve_event_data event_data;
	struct uk_sched *s = uk_sched_current();
	struct clone_args cl_args;
	struct uk_thread *thread;
	struct uk_thread *parent;
	pid_t parent_tid;
	pid_t tid;
	int ret;

	UK_ASSERT(s);

	parent = uk_thread_current();
	parent_tid = ukthread2tid(parent);
	UK_ASSERT(parent_tid > 0);

	/* Create container thread */
	thread = uk_thread_create_container(uk_alloc_get_default(),
					    s->a_stack,
					    STACK_SIZE,
					    s->a_auxstack, 0, s->a_uktls,
					    false, "application", NULL, NULL);
	if (unlikely(!thread)) {
		uk_pr_err("Could not create thread\n");
		return -ENOMEM;
	}

	/* Create new process */
	ret = pprocess_create(uk_alloc_get_default(), thread, parent);
	if (unlikely(ret)) {
		uk_pr_err("Could not create process (%d)\n", ret);
		goto err_free_thread;
	}

	tid = ukthread2tid(thread);

	/* Iterate clonetab. We pass the flags used when creating
	 * a new process, i.e. CLONE_VM | CLONE_VFORK | SIGCHLD.
	 */
	cl_args = (struct clone_args) {
		.flags       = CLONE_VM | CLONE_VFORK,
		.child_tid   = tid,
		.parent_tid  = parent_tid,
		.exit_signal = SIGCHLD,
	};
	ret = pprocess_clonetab_init(&cl_args, sizeof(cl_args), 0,
				     thread, parent);
	if (unlikely(ret)) {
		uk_pr_err("clonetab execution error (%d)\n", ret);
		goto err_free_process;
	}

	/* Raise the execve event */
	event_data.thread = thread;
	ret = pprocess_raise_execve_event(&event_data);
	if (unlikely(ret < 0)) {
		uk_pr_err("exeve event error (%d)\n", ret);
		goto err_term_clonetab;
	}

	/* Schedule the process */
	uk_thread_container_init_fn2(thread,
				     (uk_thread_fn2_t)fn,
				     (void *)(unsigned long)argc,
				     (void *)argv);
	uk_sched_thread_add(s, thread);

	return ukthread2pid(thread);

err_term_clonetab:
	pprocess_clonetab_term(thread);

err_free_process:
	pprocess_release(tid2pprocess(tid));

err_free_thread:
	uk_thread_release(thread);

	return ret;
}
#endif /* CONFIG_LIBPOSIX_PROCESS_MULTIPROCESS */

/* Releases pprocess memory and other resources.
 * NOTE: All pthreads must be removed already
 *       from this pprocess. All chilren must
 *       be already reparented.
 */
void pprocess_release(struct posix_process *pprocess)
{
	pid_t pid;

	UK_ASSERT(pprocess);
	UK_ASSERT(uk_list_empty(&pprocess->threads));
	UK_ASSERT(uk_list_empty(&pprocess->children));

	/* Unlink this process from its parent */
	if (pprocess->parent)
		uk_list_del(&pprocess->child_list_entry);

#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
	pprocess_signal_pdesc_free(pprocess);
#endif /* CONFIG_LIBPOSIX_PROCESS_SIGNAL */

	pid = pprocess->pid;

	pid_process[pid] = NULL;
	uk_free(pprocess->_a, pprocess);

	uk_pr_debug("pid %d released\n", pid);
}

static int posix_process_init(struct uk_init_ctx *ictx)
{
	struct uk_thread *t;

	UK_ASSERT(ictx);

	/* If ictx->tmain in set, main() executes on a
	 * separate uk_thread. Instantiate PID_INIT from
	 * that thread, and set pprocess_thread_main, as
	 * we will need that information later.
	 */
	if (ictx->tmain) {
		t = ictx->tmain;
		pprocess_thread_main = ictx->tmain;
	} else {
		t = uk_thread_current();
	}

	/* Create a POSIX process without parent ("init" process) */
	return pprocess_create(uk_alloc_get_default(), t, NULL);
}

uk_late_initcall(posix_process_init, 0x0);

/* Thread release: Release TID and posix_thread */
static void posix_thread_fini(struct uk_thread *thread)
{
	struct posix_process *pprocess;
	struct posix_thread *pthread;

	pthread = uk_thread_uktls_var(thread, pthread_self);

	if (!pthread)
		return; /* no posix thread was assigned */

	pprocess = pthread->process;
	UK_ASSERT(pprocess);

	/* Perform thread termination and relase the thread */
	pprocess_exit_pthread(pthread_self, POSIX_THREAD_EXITED, 0);

	/* If last thread, also release the process */
	if  (uk_list_empty(&pprocess->threads)) {
		pprocess_exit(pprocess, POSIX_PROCESS_EXITED, 0);
		/* UK_PID_INIT cannot be waited, release here */
		if (pprocess->pid == UK_PID_INIT)
			pprocess_release(pprocess);
	}
}

UK_THREAD_INIT_PRIO(0, posix_thread_fini, UK_PRIO_EARLIEST);

struct posix_process *pid2pprocess(pid_t pid)
{
	UK_ASSERT((__sz)pid < ARRAY_SIZE(pid_process));

	return pid_process[pid];
}

struct posix_thread *tid2pthread(pid_t tid)
{
	if ((__sz)tid >= ARRAY_SIZE(tid_thread) || tid < 0)
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

	UK_ASSERT(thread);

	pthread = uk_thread_uktls_var(thread, pthread_self);
	if (!pthread)
		return -ENOTSUP;

	return pthread->tid;
}

pid_t ukthread2pid(struct uk_thread *thread)
{
	struct posix_thread *pthread;

	UK_ASSERT(thread);

	pthread = uk_thread_uktls_var(thread, pthread_self);
	if (!pthread)
		return -ENOTSUP;

	UK_ASSERT(pthread->process);

	return pthread->process->pid;
}

pid_t uk_sys_getpid(void)
{
	if (!pthread_self)
		return -ENOTSUP;

	UK_ASSERT(pthread_self->process);
	return pthread_self->process->pid;
}

pid_t uk_sys_gettid(void)
{
	if (!pthread_self)
		return -ENOTSUP;

	return pthread_self->tid;
}

/* PID of parent process  */
pid_t uk_sys_getppid(void)
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
#else  /* !CONFIG_LIBPOSIX_PROCESS_MULTITHREADING */

#define UNIKRAFT_PID      1
#define UNIKRAFT_TID      1
#define UNIKRAFT_PPID     0

pid_t uk_sys_getpid(void)
{
	return UNIKRAFT_PID;
}

pid_t uk_sys_gettid(void)
{
	return UNIKRAFT_TID;
}

pid_t uk_sys_getppid(void)
{
	return UNIKRAFT_PPID;
}

#endif /* !CONFIG_LIBPOSIX_PROCESS_MULTITHREADING */

UK_SYSCALL_R_DEFINE(pid_t, gettid)
{
	return uk_sys_gettid();
}

UK_SYSCALL_R_DEFINE(pid_t, getppid)
{
	return uk_sys_getppid();
}

UK_SYSCALL_R_DEFINE(pid_t, getpid)
{
	return uk_sys_getpid();
}
