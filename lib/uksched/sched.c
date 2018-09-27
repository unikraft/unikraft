/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Costin Lupu <costin.lupu@cs.pub.ro>
 *
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#include <stdlib.h>
#include <uk/plat/config.h>
#include <uk/plat/thread.h>
#include <uk/alloc.h>
#include <uk/sched.h>
#if CONFIG_LIBUKSCHEDCOOP
#include <uk/schedcoop.h>
#endif

struct uk_sched *uk_sched_head;

/* FIXME Support for external schedulers */
struct uk_sched *uk_sched_default_init(struct uk_alloc *a)
{
	struct uk_sched *s = NULL;

#if CONFIG_LIBUKSCHEDCOOP
	s = uk_schedcoop_init(a);
#endif

	return s;
}

int uk_sched_register(struct uk_sched *s)
{
	struct uk_sched *this = uk_sched_head;

	if (!uk_sched_head) {
		uk_sched_head = s;
		s->next = NULL;
		return 0;
	}

	while (this && this->next)
		this = this->next;
	this->next = s;
	s->next = NULL;
	return 0;
}

struct uk_sched *uk_sched_get_default(void)
{
	return uk_sched_head;
}

int uk_sched_set_default(struct uk_sched *s)
{
	struct uk_sched *head, *this, *prev;

	head = uk_sched_get_default();

	if (s == head)
		return 0;

	if (!head) {
		uk_sched_head = s;
		return 0;
	}

	this = head;
	while (this->next) {
		prev = this;
		this = this->next;
		if (s == this) {
			prev->next = this->next;
			this->next = head->next;
			head = this;
			return 0;
		}
	}

	/* s is not registered yet. Add in front of the queue. */
	s->next = head;
	uk_sched_head = s;
	return 0;
}

void uk_sched_start(struct uk_sched *sched)
{
	UK_ASSERT(sched != NULL);
	ukplat_thread_ctx_start(&sched->plat_ctx_cbs, sched->idle.ctx);
}

static void *create_stack(struct uk_alloc *allocator)
{
	void *stack;

	stack = uk_palloc(allocator, STACK_SIZE_PAGE_ORDER);
	if (stack == NULL) {
		uk_pr_warn("Error allocating thread stack.");
		return NULL;
	}

	return stack;
}

void uk_sched_idle_init(struct uk_sched *sched,
		void *stack, void (*function)(void *))
{
	struct uk_thread *idle;
	int rc;

	UK_ASSERT(sched != NULL);

	if (stack == NULL)
		stack = create_stack(sched->allocator);
	UK_ASSERT(stack != NULL);

	idle = &sched->idle;

	rc = uk_thread_init(idle,
			&sched->plat_ctx_cbs, sched->allocator,
			"Idle", stack, function, NULL);
	if (rc)
		UK_CRASH("Error initializing idle thread.");

	idle->sched = sched;
}

struct uk_thread *uk_sched_thread_create(struct uk_sched *sched,
		const char *name, void (*function)(void *), void *arg)
{
	struct uk_thread *thread = NULL;
	void *stack = NULL;
	int rc;

	thread = uk_malloc(sched->allocator, sizeof(struct uk_thread));
	if (thread == NULL) {
		uk_pr_warn("Error allocating memory for thread.");
		goto err;
	}

	/* We can't use lazy allocation here
	 * since the trap handler runs on the stack
	 */
	stack = create_stack(sched->allocator);
	if (stack == NULL)
		goto err;

	rc = uk_thread_init(thread,
			&sched->plat_ctx_cbs, sched->allocator,
			name, stack, function, arg);
	if (rc)
		goto err;

	uk_sched_thread_add(sched, thread);

	return thread;

err:
	if (stack)
		uk_free(sched->allocator, stack);
	if (thread)
		uk_free(sched->allocator, thread);

	return NULL;
}

void uk_sched_thread_destroy(struct uk_sched *sched, struct uk_thread *thread)
{
	UK_ASSERT(sched != NULL);
	UK_ASSERT(thread != NULL);
	uk_thread_fini(thread, sched->allocator);
	uk_pfree(sched->allocator, thread->stack, STACK_SIZE_PAGE_ORDER);
	uk_free(sched->allocator, thread);
}

void uk_sched_thread_sleep(__nsec nsec)
{
	struct uk_thread *thread;

	thread = uk_thread_current();
	uk_thread_block_timeout(thread, nsec);
	uk_sched_yield();
}

void uk_sched_thread_exit(void)
{
	struct uk_thread *thread;

	thread = uk_thread_current();

	uk_pr_info("Thread \"%s\" exited.\n", thread->name);

	UK_ASSERT(thread->sched);
	uk_sched_thread_remove(thread->sched, thread);
	UK_CRASH("Error stopping thread.");
}
