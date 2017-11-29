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
#include <uk/alloc.h>
#include <uk/sched.h>

struct uk_sched *uk_sched_head;

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

struct uk_thread *uk_sched_thread_create(struct uk_sched *sched,
		char *name, void (*function)(void *), void *data)
{
	struct uk_thread *thread;

	thread = uk_malloc(sched->allocator, sizeof(struct uk_thread));
	if (thread == NULL) {
		uk_printd(DLVL_WARN, "Error allocating memory for thread.");
		goto out;
	}

	/* We can't use lazy allocation here
	 * since the trap handler runs on the stack
	 */
	thread->stack = uk_palloc(sched->allocator, STACK_SIZE_PAGE_ORDER);
	if (thread->stack == NULL) {
		uk_printd(DLVL_WARN, "Error allocating thread stack.");
		free(thread);
		thread = NULL;
		goto out;
	}

	thread->name = name;
	uk_printd(DLVL_EXTRA, "Thread \"%s\": pointer: %p, stack: %p\n",
			name, thread, thread->stack);

	/* Not runnable, not exited, not sleeping */
	thread->flags = 0;
	thread->wakeup_time = 0LL;

	/* Call platform specific setup. */
	ukplat_thread_ctx_init(&thread->plat_ctx, thread->stack,
			       function, data);
#ifdef HAVE_LIBC
	//TODO _REENT_INIT_PTR(&thread->reent);
#endif

	thread->sched = sched;

out:
	return thread;
}

void uk_sched_thread_destroy(struct uk_sched *sched, struct uk_thread *thread)
{
	uk_pfree(sched->allocator, thread->stack, STACK_SIZE_PAGE_ORDER);
	uk_free(sched->allocator, thread);
}

void uk_sched_sleep(uint32_t millis)
{
	struct uk_thread *thread;

	thread = uk_thread_current();
	uk_thread_block_millis(thread, millis);
	uk_sched_yield();
}
