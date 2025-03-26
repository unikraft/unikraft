/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2022, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
 * Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#ifndef __PROCESS_H_INTERNAL__
#define __PROCESS_H_INTERNAL__

#include <sys/types.h>
#include <uk/config.h>

#if CONFIG_LIBPOSIX_PROCESS_MULTITHREADING
#include <linux/sched.h>
#include <uk/arch/ctx.h>
#include <uk/semaphore.h>
#include <uk/thread.h>
#endif /* CONFIG_LIBPOSIX_PROCESS_MULTITHREADING */

extern struct uk_thread *pprocess_thread_main;

extern int pprocess_exit_status;

#if CONFIG_LIBPOSIX_PROCESS_MULTITHREADING

#define UK_PID_INIT		1
#define TIDMAP_SIZE		(CONFIG_LIBPOSIX_PROCESS_MAX_PID + 1)

/* Notice: The RUNNING state is not necessarily in sync with the state
 * of the underlying uk_thread (may be blocked by the scheduler).
 * On the other hand, the BLOCKED state implies that the underlying
 * uk_thread is also blocked. Use RUNNING only as a means to check
 * whether a posix-thread is neither terminated or blocked.
 */
enum posix_thread_state {
	POSIX_THREAD_RUNNING,
	POSIX_THREAD_BLOCKED_VFORK,  /* waiting for child to call execve */
	POSIX_THREAD_BLOCKED_WAIT,   /* waiting for process state change */
	POSIX_THREAD_BLOCKED_SIGNAL, /* waiting for signal */
	POSIX_THREAD_EXITED,         /* terminated normally */
	POSIX_THREAD_KILLED,         /* terminated by signal */
};

enum posix_process_state {
	POSIX_PROCESS_RUNNING,        /* not terminated */
	POSIX_PROCESS_EXITED,         /* terminated normally */
	POSIX_PROCESS_KILLED,         /* terminated by signal */
};

struct posix_process {
	pid_t pid;
	struct posix_process *parent;
	struct uk_list_head children; /* child processes */
	struct uk_list_head child_list_entry;
	struct uk_list_head threads;
	struct uk_alloc *_a;
#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
	struct uk_signal_pdesc *signal;
#endif /* CONFIG_LIBPOSIX_PROCESS_SIGNAL */
	struct uk_semaphore wait_semaphore;
	struct uk_semaphore exit_semaphore;
	enum posix_process_state state;
	int exit_status;

	/* TODO: Mutex */
};

struct posix_thread {
	pid_t tid;
	struct posix_process *process;
	struct posix_thread *parent;
	struct uk_list_head thread_list_entry;
	struct uk_thread *thread;
	struct uk_alloc *_a;
	enum posix_thread_state state;
#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
	struct uk_signal_tdesc *signal;
#endif /* CONFIG_LIBPOSIX_PROCESS_SIGNAL */
	pid_t wait_pid;

	/* TODO: Mutex */
};

extern struct posix_process *pid_process[TIDMAP_SIZE];

extern __uk_tls struct posix_thread *pthread_self;

#define uk_pprocess_foreach(_p)						\
	for (int _j = 1, _i = 0; _i != ARRAY_SIZE(pid_process);		\
		_j = !_j, _i++)						\
			for ((_p) = pid_process[_i]; _j; _j = !_j)	\
				if ((_p))

#define uk_pprocess_foreach_pthread(_proc, _pthread, _pthreadn)		\
	uk_list_for_each_entry_safe((_pthread), (_pthreadn),		\
				    &(_proc)->threads, thread_list_entry)

#define uk_pprocess_foreach_child(_proc, _pchild, _pchildn)		\
	uk_list_for_each_entry_safe((_pchild), (_pchildn),		\
				    &(_proc)->children, child_list_entry)

#define uk_pthread_current()						\
	uk_thread_uktls_var(uk_thread_current(), pthread_self)

#define uk_pprocess_current()						\
	uk_pthread_current()->process

struct posix_process *pid2pprocess(pid_t pid);
struct uk_thread *tid2ukthread(pid_t tid);
struct posix_thread *tid2pthread(pid_t tid);
struct posix_process *tid2pprocess(pid_t tid);
pid_t ukthread2tid(struct uk_thread *thread);
pid_t ukthread2pid(struct uk_thread *thread);

/**
 * INTERNAL. Exit and release a pthread
 *
 * Performs termination tasks and raises the POSIX_PROCESS_EXIT_EVENT.
 * Unlike processes, pthreads are released upon exit. Besides the pthread,
 * this function terminates the underlying uk_thread, unless called on
 * uk_thread_current()'s pthread. In this last case, the caller should
 * take care of terminating the underlying uk_thread, as needed.
 *
 * @param pthread      The terminating thread.
 * @param state	       The terminating thread's state. Must be either
 *                     POSIX_THREAD_STATE_EXITED if terminated volunatrily, or
 *                     POSIX_THREAD_STATE_KILLED if terminated by signal.
 * @param exit_status  The exit status of the terminating thread.
 */
void pprocess_exit_pthread(struct posix_thread *pthread,
			   enum posix_thread_state state,
			   int exit_status);
/**
 * INTERNAL. Exit process
 *
 * Performs process termination tasks and raises POSIX_PROCESS_EXIT_EVENT.
 * Does not release the process. Upon completion, if the process has not been
 * reaped by an existing wait, the process becomes a zombie and its resources
 * remain allocated until reaped by a subsequent call to wait().
 *
 * Notice: It's the caller's responsibility to terminate the current uk_thread
 *         upon return, if the calling thread is a member of the terminating
 *         process.
 *
 * @param pprocess     The terminating process.
 * @param state        The new process state. Must be either
 *                     POSIX_PROCESS_STATE_EXITED if terminated volunatrily, or
 *                     POSIX_PROCESS_STATE_KILLED if terminated by signal.
 * @param exit_status  The exit status of the terminating process.
 */
void pprocess_exit(struct posix_process *process,
		   enum posix_process_state state,
		   int exit_status);

/**
 * INTERNAL. Release pthread
 *
 * Releases thread resources.
 *
 * @param pthread The thread to release.
 */
void pprocess_release_pthread(struct posix_thread *pthread);

/**
 * INTERNAL. Release process
 *
 * Releases process resources and removes from process table.
 * Requires that all pthreads are already terminated.
 *
 * @param pprocess  The process to release.
 */
void pprocess_release(struct posix_process *pprocess);

/**
 * INTERNAL. Create pthread
 *
 * @param pprocess process to assign thread to
 * @param thread   backing uk_thread to create pthread from
 * @return pthread on success or negative value on failure
 */
struct posix_thread *pprocess_create_pthread(struct posix_process *pprocess,
					     struct uk_thread *thread);
/**
 * INTERNAL. Create process
 *
 * @param alloc  allocator to assign process
 * @param thread backing uk_thread to create process from
 * @param parent parent processs
 * @return process on success or negative value on failure
 */
int pprocess_create(struct uk_alloc *a, struct uk_thread *thread,
		    struct uk_thread *parent);

int uk_clone(struct clone_args *cl_args, size_t cl_args_len,
	     struct ukarch_execenv *execenv);
#endif /* CONFIG_LIBPOSIX_PROCESS_MULTITHREADING */

#endif /* __PROCESS_H_INTERNAL__ */
