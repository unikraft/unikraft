/* SPDX-License-Identifier: MIT */
/*
 * Authors: Grzegorz Milos
 *          Robert Kaiser
 *          Costin Lupu <costin.lupu@cs.pub.ro>
 *
 * Copyright (c) 2005, Intel Research Cambridge
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
/*
 * The scheduler is non-preemptive (cooperative), and schedules according
 * to Round Robin algorithm.
 */
#include <uk/plat/config.h>
#include <uk/plat/lcpu.h>
#include <uk/plat/memory.h>
#include <uk/plat/time.h>
#include <uk/sched.h>
#include <uk/schedcoop.h>
#include <uk/essentials.h>

struct schedcoop {
	struct uk_sched sched;

	struct uk_thread_list thread_list;
	struct uk_thread_list sleeping_threads;

	struct uk_thread idle;
};

static inline struct schedcoop *uksched2schedcoop(struct uk_sched *s)
{
	UK_ASSERT(s);

	return __containerof(s, struct schedcoop, sched);
}

#ifdef SCHED_DEBUG
static void print_runqueue(struct uk_sched *s)
{
	struct schedcoop *c = uksched2schedcoop(s);
	struct uk_thread *th;

	UK_TAILQ_FOREACH(th, &c->thread_list, thread_list) {
		uk_pr_debug("   Thread \"%s\", runnable=%d\n",
			    th->name, is_runnable(th));
	}
}
#endif

static void schedcoop_schedule(struct uk_sched *s)
{
	struct schedcoop *c = uksched2schedcoop(s);
	struct uk_thread *prev, *next, *thread, *tmp;
	unsigned long flags;

	if (ukplat_lcpu_irqs_disabled())
		UK_CRASH("Must not call %s with IRQs disabled\n", __func__);

	prev = uk_thread_current();
	flags = ukplat_lcpu_save_irqf();

#if 0 //TODO
	if (in_callback)
		UK_CRASH("Must not call %s from a callback\n", __func__);
#endif

	do {
		/* Examine all threads.
		 * Find a runnable thread, but also wake up expired ones and
		 * find the time when the next timeout expires, else use
		 * 10 seconds.
		 */
		__snsec now = ukplat_monotonic_clock();
		__snsec min_wakeup_time = now + ukarch_time_sec_to_nsec(10);

		/* wake some sleeping threads */
		UK_TAILQ_FOREACH_SAFE(thread, &c->sleeping_threads,
				      thread_list, tmp) {

			if (thread->wakeup_time && thread->wakeup_time <= now)
				uk_thread_wake(thread);

			else if (thread->wakeup_time < min_wakeup_time)
				min_wakeup_time = thread->wakeup_time;
		}

		next = UK_TAILQ_FIRST(&c->thread_list);
		if (next) {
			UK_ASSERT(next != prev);
			UK_ASSERT(is_runnable(next));
			UK_ASSERT(!is_exited(next));
			UK_TAILQ_REMOVE(&c->thread_list, next,
					thread_list);
			/* Put previous thread on the end of the list */
			if (is_runnable(prev))
				UK_TAILQ_INSERT_TAIL(&c->thread_list, prev,
						thread_list);
			else
				set_queueable(prev);
			clear_queueable(next);
			break;
		} else if (is_runnable(prev)) {
			next = prev;
			break;
		}

		/* block until the next timeout expires, or for 10 secs,
		 * whichever comes first
		 */
		ukplat_lcpu_halt_to(min_wakeup_time);
		/* handle pending events if any */
		ukplat_lcpu_irqs_handle_pending();

	} while (1);

	ukplat_lcpu_restore_irqf(flags);

	/* Interrupting the switch is equivalent to having the next thread
	 * interrupted at the return instruction. And therefore at safe point.
	 */
	if (prev != next)
		uk_sched_thread_switch(next);

	UK_TAILQ_FOREACH_SAFE(thread, &s->exited_threads, thread_list, tmp) {
		if (!thread->detached)
			/* someone will eventually wait for it */
			continue;

		if (thread != prev)
			uk_sched_thread_destroy(s, thread);
	}
}

static int schedcoop_thread_add(struct uk_sched *s, struct uk_thread *t,
	const uk_thread_attr_t *attr __unused)
{
	struct schedcoop *c = uksched2schedcoop(s);
	unsigned long flags;

	flags = ukplat_lcpu_save_irqf();
	UK_TAILQ_INSERT_TAIL(&c->thread_list, t, thread_list);
	ukplat_lcpu_restore_irqf(flags);

	return 0;
}

static void schedcoop_thread_remove(struct uk_sched *s, struct uk_thread *t)
{
	struct schedcoop *c = uksched2schedcoop(s);
	unsigned long flags;

	flags = ukplat_lcpu_save_irqf();

	/* Remove from the thread list */
	if (t != uk_thread_current())
		UK_TAILQ_REMOVE(&c->thread_list, t, thread_list);
	clear_runnable(t);

	uk_thread_exit(t);

	/* Put onto exited list */
	UK_TAILQ_INSERT_HEAD(&s->exited_threads, t, thread_list);

	ukplat_lcpu_restore_irqf(flags);

	/* Schedule only if current thread is exiting */
	if (t == uk_thread_current()) {
		schedcoop_schedule(s);
		uk_pr_warn("schedule() returned! Trying again\n");
	}
}

static void schedcoop_thread_blocked(struct uk_sched *s, struct uk_thread *t)
{
	struct schedcoop *c = uksched2schedcoop(s);

	UK_ASSERT(ukplat_lcpu_irqs_disabled());

	if (t != uk_thread_current())
		UK_TAILQ_REMOVE(&c->thread_list, t, thread_list);
	if (t->wakeup_time > 0)
		UK_TAILQ_INSERT_TAIL(&c->sleeping_threads, t, thread_list);
}

static void schedcoop_thread_woken(struct uk_sched *s, struct uk_thread *t)
{
	struct schedcoop *c = uksched2schedcoop(s);

	UK_ASSERT(ukplat_lcpu_irqs_disabled());

	if (t->wakeup_time > 0)
		UK_TAILQ_REMOVE(&c->sleeping_threads, t, thread_list);
	if (t != uk_thread_current() || is_queueable(t)) {
		UK_TAILQ_INSERT_TAIL(&c->thread_list, t, thread_list);
		clear_queueable(t);
	}
}

static void idle_thread_fn(void) __noreturn;

static void idle_thread_fn(void)
{
	struct uk_thread *current = uk_thread_current();
	struct uk_sched *s = current->sched;

	while (1) {
		uk_thread_block(current);
		schedcoop_schedule(s);
	}
}

static void schedcoop_yield(struct uk_sched *s)
{
	schedcoop_schedule(s);
}

static int schedcoop_start(struct uk_sched *s, struct uk_thread *main_thread)
{
	UK_ASSERT(main_thread);
	UK_ASSERT(main_thread->sched == s);
	UK_ASSERT(uk_thread_is_runnable(main_thread));
	UK_ASSERT(!uk_thread_is_exited(main_thread));
	UK_ASSERT(uk_thread_current() == main_thread);

	/* NOTE: We do not put `main_thread` into the thread list.
	 *       Current running threads will be added as soon as
	 *       a different thread is scheduled.
	 */

	s->threads_started = true;
	ukplat_lcpu_enable_irq();

	return 0;
}

struct uk_sched *uk_schedcoop_create(struct uk_alloc *a)
{
	struct schedcoop *c = NULL;
	int rc;

	uk_pr_info("Initializing cooperative scheduler\n");
	c = uk_zalloc(a, sizeof(struct schedcoop));
	if (!c)
		return NULL;

	UK_TAILQ_INIT(&c->thread_list);
	UK_TAILQ_INIT(&c->sleeping_threads);

	rc = uk_thread_init_fn0(&c->idle,
				idle_thread_fn,
				a, STACK_SIZE,
				a,
				false, NULL,
				"idle",
				NULL,
				NULL);
	if (rc < 0) {
		 /* FIXME: Do not crash on failure */
		UK_CRASH("Failed to initialize `idle` thread\n");
	}

	c->idle.sched = &c->sched;

	uk_sched_init(&c->sched,
		        schedcoop_start,
			schedcoop_yield,
			schedcoop_thread_add,
			schedcoop_thread_remove,
			schedcoop_thread_blocked,
			schedcoop_thread_woken,
		        NULL, NULL, NULL, NULL,
		        a);

	return &c->sched;
}
