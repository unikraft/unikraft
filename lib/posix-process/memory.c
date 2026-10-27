/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>
#include <uk/alloc.h>
#include <uk/process.h>
#include <uk/refcount.h>
#include <uk/vmem.h>
#include "process.h"

struct process_mmap_owner {
	__atomic references;
};

static struct process_mmap_owner *owner_new(void)
{
	struct process_mmap_owner *owner;

	owner = uk_malloc(uk_alloc_get_default(), sizeof(*owner));
	if (owner)
		uk_refcount_init(&owner->references, 1);
	return owner;
}

static void owner_put(struct process_mmap_owner *owner)
{
	if (!owner || !uk_refcount_release(&owner->references))
		return;
	uk_vma_unmap_owner(uk_vas_get_active(), owner);
	uk_free(uk_alloc_get_default(), owner);
}

/* The ELF loader maps the new image before exec commits, under an owner
 * of the exec'ing thread's own: the process's other threads go on mapping
 * into the old image until they are terminated.
 */
const void *uk_process_mmap_owner(void)
{
	struct posix_thread *thread = uk_pthread_current();

	if (!thread)
		return __NULL;
	if (thread->mmap_exec_owner)
		return thread->mmap_exec_owner;
	return thread->process->mmap_owner;
}

static int memory_clone(void *arg)
{
	struct posix_process_clone_event_data *event = arg;
	struct posix_process *process = pid2pprocess(event->pid);
	struct posix_thread *parent = __NULL;

	if (event->cl_args->flags & CLONE_THREAD)
		return UK_EVENT_HANDLED_CONT;
	/* A process started by no process's thread (the init process)
	 * owns its mappings alone
	 */
	if (event->parent)
		parent = uk_thread_uktls_var(event->parent, pthread_self);
	if (parent) {
		process->mmap_owner = parent->process->mmap_owner;
		UK_ASSERT(process->mmap_owner);
		uk_refcount_acquire(&process->mmap_owner->references);
	} else {
		process->mmap_owner = owner_new();
		if (!process->mmap_owner)
			return -ENOMEM;
	}
	return UK_EVENT_HANDLED_CONT;
}

POSIX_PROCESS_CLONE_HANDLER(0, memory_clone);

static int memory_exit(void *arg)
{
	struct posix_process_ppexit_event_data *event = arg;
	struct posix_process *process = pid2pprocess(event->pid);
	struct process_mmap_owner *owner = process->mmap_owner;

	process->mmap_owner = __NULL;
	owner_put(owner);
	return UK_EVENT_HANDLED_CONT;
}

POSIX_PROCESS_PPEXIT_HANDLER(memory_exit);

/* The image a thread's execve() was loading goes when the exec fails,
 * and when the thread ends before the exec completes (another thread's
 * exit_group(), a signal).
 */
static void exec_owner_drop(struct posix_thread *thread)
{
	struct process_mmap_owner *owner = thread->mmap_exec_owner;

	thread->mmap_exec_owner = __NULL;
	owner_put(owner);
}

static int memory_thread_exit(void *arg)
{
	struct posix_process_ptexit_event_data *event = arg;

	exec_owner_drop(uk_thread_uktls_var(event->thread, pthread_self));
	return UK_EVENT_HANDLED_CONT;
}

POSIX_PROCESS_PTEXIT_HANDLER(memory_thread_exit);

int uk_process_mmap_exec_begin(void)
{
	struct posix_thread *thread = uk_pthread_current();

	UK_ASSERT(thread && !thread->mmap_exec_owner);
	thread->mmap_exec_owner = owner_new();
	return thread->mmap_exec_owner ? 0 : -ENOMEM;
}

void uk_process_mmap_exec_commit(void)
{
	struct posix_thread *thread = uk_pthread_current();
	struct posix_process *process = thread->process;
	struct process_mmap_owner *old = process->mmap_owner;

	UK_ASSERT(thread->mmap_exec_owner);
	process->mmap_owner = thread->mmap_exec_owner;
	thread->mmap_exec_owner = __NULL;
	owner_put(old);
}

void uk_process_mmap_exec_abort(void)
{
	exec_owner_drop(uk_pthread_current());
}
