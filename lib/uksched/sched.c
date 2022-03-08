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
 */

#include <stdlib.h>
#include <string.h>
#include <uk/plat/config.h>
#include <uk/alloc.h>
#include <uk/plat/lcpu.h>
#include <uk/sched.h>
#if CONFIG_LIBUKSCHEDCOOP
#include <uk/schedcoop.h>
#endif
#if CONFIG_LIBUKSIGNAL
#include <uk/uk_signal.h>
#endif
#include <uk/syscall.h>

struct uk_sched *uk_sched_head;

/* TODO: Define per CPU */
struct uk_thread *__uk_sched_thread_current;

/* FIXME Support for external schedulers */
struct uk_sched *uk_sched_default_init(struct uk_alloc *a)
{
	struct uk_sched *s = NULL;

#if CONFIG_LIBUKSIGNAL
	uk_proc_sig_init(&uk_proc_sig);
#endif

#if CONFIG_LIBUKSCHEDCOOP
	s = uk_schedcoop_create(a);
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
			// Update the default scheduler to head
			uk_sched_head = head;
			return 0;
		}
	}

	/* s is not registered yet. Add in front of the queue. */
	s->next = head;
	uk_sched_head = s;
	return 0;
}

struct uk_thread *uk_sched_thread_create(struct uk_sched *s,
					 uk_thread_fn1_t fn,
					 void * argp,
					 const char *name)
{
	struct uk_thread *t;
	int rc;

	UK_ASSERT(s);

	t = uk_thread_create_fn1(s->allocator,
				 fn, argp,
				 s->allocator, 0, /* TODO: stack allocator */
				 s->allocator, /* TODO: TLS allocator */
				 false,
				 name,
				 NULL,
				 NULL);
	if (!t)
		goto err_out;

	rc = uk_sched_thread_add(s, t);
	if (rc < 0)
		goto err_free_t;

	return t;

err_free_t:
	uk_thread_release(t);
err_out:
	return NULL;
}

int uk_sched_start(struct uk_sched *s)
{
	struct uk_thread *main_thread;
	uintptr_t tlsp;
	int ret;

	UK_ASSERT(s);
	UK_ASSERT(s->sched_start);
	UK_ASSERT(!s->is_started);
	UK_ASSERT(!uk_thread_current()); /* No other thread runs */

	/* Allocate an `uk_thread` instance for current context
	 * NOTE: We assume that if we have a TLS pointer, it points to
	 *       an TLS that is derived from the Unikraft TLS template.
	 */
	tlsp = ukplat_tlsp_get();
	main_thread = uk_thread_create_bare(s->a,
					    0x0, 0x0, tlsp, !(!tlsp), false,
					    "init", NULL, NULL);
	if (!main_thread) {
		ret = -ENOMEM;
		goto err_out;
	}
	main_thread->sched = s;

	/* Because `main_thread` acts as container for storing the current
	 * context, it does not have IP and SP set. We have to manually mark
	 * the thread as RUNNABLE.
	 */
	main_thread->flags |= UK_THREADF_RUNNABLE;

	/* Set main_thread as current scheduled thread */
	__uk_sched_thread_current = main_thread;

	/* Add main to the scheduler's thread list */
	UK_TAILQ_INSERT_TAIL(&s->thread_list, main_thread, thread_list);

	/* Enable scheduler, like time slicing, etc. and notify that `s`
	 * has an (already) scheduled thread
	 */
	ret = s->sched_start(s, main_thread);
	if (ret < 0)
		goto err_unset_thread_current;
	s->is_started = true;
	return 0;

err_unset_thread_current:
	__uk_sched_thread_current = NULL;
	uk_thread_release(main_thread);
err_out:
	return ret;
}

unsigned int uk_sched_thread_gc(struct uk_sched *sched)
{
	struct uk_thread *thread, *tmp;
	unsigned int num = 0;

	/* Cleanup finished threads */
	UK_TAILQ_FOREACH_SAFE(thread, &sched->exited_threads,
			      thread_list, tmp) {
		UK_ASSERT(thread != uk_thread_current());

		uk_pr_debug("%p: garbage collect thread %p (%s)\n",
			    sched, thread,
			    thread->name ? thread->name : "<unnamed>");

		UK_TAILQ_REMOVE(&sched->exited_threads, thread, thread_list);
		uk_thread_release(thread);
		++num;
	}

	return num;
}

void uk_sched_thread_kill(struct uk_thread *thread)
{
	struct uk_sched *sched;

	UK_ASSERT(thread);
	 /* NOTE: The following assertion fails for instance on a double-kill.
	  *       This can happen when the thread was put on the exited_thread
	  *       list and waits for getting garbage collected.
	  *       Double-kill can be avoided by testing for the exit flag.
	  */
	UK_ASSERT(thread->sched);

	sched = thread->sched;

	uk_pr_debug("%p: thread %p (%s) killed\n",
		    sched, thread, thread->name ? thread->name : "<unnamed>");

	/* remove from scheduling queue */
	uk_sched_thread_remove(thread);

	set_exited(thread);
	clear_runnable(thread);

	if (thread == uk_thread_current()) {
		/* enqueue thread for garbage collecting */
		uk_pr_debug("%p: thread %p (%s) on gc list\n",
			    sched, thread, thread->name ? thread->name : "<unnamed>");
		UK_TAILQ_INSERT_TAIL(&sched->exited_threads, thread,
				     thread_list);

		/* leave this thread */
		sched->yield(sched); /* we won't return */
		UK_CRASH("Unexpectedly returned to exited thread %p\n", thread);
	} else {
		/* free thread resources immediately */
		uk_thread_release(thread);
	}
}

/* This function has the __noreturn attribute set */
void uk_sched_thread_exit(void)
{
	struct uk_thread *t = uk_thread_current();

	uk_sched_thread_kill(t);
	UK_CRASH("Unexpectedly returned to exited thread %p\n", t);
}

void uk_sched_thread_sleep(__nsec nsec)
{
	struct uk_thread *thread;

	thread = uk_thread_current();
	uk_thread_block_timeout(thread, nsec);
	uk_sched_yield();
}

int uk_sched_thread_add(struct uk_sched *s, struct uk_thread *t)
{
	unsigned long flags;
	int rc;

	UK_ASSERT(s);
	UK_ASSERT(t);
	UK_ASSERT(!t->sched);

	flags = ukplat_lcpu_save_irqf();

	rc = s->thread_add(s, t);
	if (rc < 0)
		goto out;

	t->sched = s;
	UK_TAILQ_INSERT_TAIL(&s->thread_list, t, thread_list);
out:
	ukplat_lcpu_restore_irqf(flags);
	return rc;
}

int uk_sched_thread_remove(struct uk_thread *t)
{
	unsigned long flags;
	struct uk_sched *s;

	UK_ASSERT(t);
	UK_ASSERT(t->sched);

	flags = ukplat_lcpu_save_irqf();
	s = t->sched;
	s->thread_remove(s, t);
	t->sched = NULL;
	UK_TAILQ_REMOVE(&s->thread_list, t, thread_list);
	ukplat_lcpu_restore_irqf(flags);
	return 0;
}

UK_SYSCALL_R_DEFINE(int, sched_yield)
{
	uk_sched_yield();
	return 0;
}

void uk_sched_dumpk_threads(int klvl, struct uk_sched *s)
{
	struct uk_thread *t, *tmp;

	uk_sched_foreach_thread_safe(s, t, tmp) {
		uk_printk(klvl,
			  "sched %p: thread %p (%s), ctx: %p, flags: %c%c%c%c\n",
			  s,
			  t, t->name ? t->name : "<unnamed>",
			  &t->ctx,
			  (t->flags & UK_THREADF_RUNNABLE) ? 'R' : '-',
			  (t->flags & UK_THREADF_EXITED)   ? 'D' : '-',
			  (t->flags & UK_THREADF_ECTX)     ? 'E' : '-',
			  (t->flags & UK_THREADF_UKTLS)    ? 'T' : '-');
	}
}
