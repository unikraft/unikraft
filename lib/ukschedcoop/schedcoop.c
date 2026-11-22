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
#include <uk/lcpu.h>
#include <uk/plat/memory.h>
#include <uk/plat/time.h>
#include <uk/sched_impl.h>
#include <uk/schedcoop.h>
#include <uk/essentials.h>
#include "schedcoop.h"

static void schedcoop_schedule(struct uk_sched *s)
{
	struct schedcoop *c = uksched2schedcoop(s);
	struct uk_thread *prev, *next, *thread, *tmp;
	__snsec now, min_wakeup_time;
	unsigned long flags;

	if (unlikely(uk_lcpu_irqs_disabled()))
		UK_CRASH("Must not call %s with IRQs disabled\n", __func__);

	now = ukplat_monotonic_clock();
	prev = uk_thread_current();
	ukplat_spin_lock_irqsave(&s->lock, flags);
	UK_ASSERT(c->curr_thread == prev);

	/* Update execution time of current thread (one scheduler per CPU). */
	prev->exec_time += now - c->ts_prev_switch;
	c->ts_prev_switch = now;

	/* Examine all sleeping threads.
	 * Wake up expired ones and find the time when the next timeout expires.
	 */
	min_wakeup_time = 0;
	UK_TAILQ_FOREACH_SAFE(thread, &c->sleep_queue,
			      queue, tmp) {
		if (likely(thread->wakeup_time)) {
			if (thread->wakeup_time <= now) {
				uk_thread_set_runnable(thread);
				thread->wakeup_time = 0LL;
				schedcoop_wake_locked(c, thread);
			} else if (!min_wakeup_time
				   || thread->wakeup_time < min_wakeup_time) {
				min_wakeup_time = thread->wakeup_time;
			}
		}
	}

	next = schedcoop_runq_pop_first(c);
	if (next) {
		UK_ASSERT(next != prev);
		UK_ASSERT(uk_thread_is_runnable(next));
		UK_ASSERT(!uk_thread_is_exited(next));

		/* Put previous thread on the end of the list */
		if ((prev != &c->idle)
		    && uk_thread_is_runnable(prev)
		    && !uk_thread_is_exited(prev))
			schedcoop_runq_push(c, prev);
	} else if ((prev != &c->idle)
		   && uk_thread_is_runnable(prev)
		   && !uk_thread_is_exited(prev)) {
		next = prev;
	} else {
		/*
		 * Schedule idle thread that will halt the CPU
		 * We select the idle thread only if we do not have anything
		 * else to execute
		 */
		c->idle_return_time = min_wakeup_time;
		next = &c->idle;
		uk_sched_stats_idle_count_incr(s);
	}

	c->curr_thread = next;
	uk_sched_stats_sched_count_incr(s);

	ukplat_spin_unlock_irqrestore(&s->lock, flags);

	/* Interrupting the switch is equivalent to having the next thread
	 * interrupted at the return instruction. And therefore at safe point.
	 */
	if (prev != next) {
		if (next != &c->idle)
			uk_sched_stats_next_count_incr(s);
		uk_sched_thread_switch(next);
	}
}

static void schedcoop_yield(struct uk_sched *s)
{
	uk_sched_stats_yield_count_incr(s);
	schedcoop_schedule(s);
}

static int schedcoop_thread_add(struct uk_sched *s, struct uk_thread *t)
{
	struct schedcoop *c = uksched2schedcoop(s);

	UK_ASSERT(t);
	UK_ASSERT(!uk_thread_is_exited(t));

	/* Add to run queue if runnable */
	if (uk_thread_is_runnable(t))
		schedcoop_runq_push(c, t);

	return 0;
}

static void schedcoop_thread_remove(struct uk_sched *s, struct uk_thread *t)
{
	struct schedcoop *c = uksched2schedcoop(s);

	schedcoop_runq_remove(c, t);
	schedcoop_sleepq_remove(c, t);
}

static void schedcoop_thread_blocked(struct uk_sched *s, struct uk_thread *t)
{
	struct schedcoop *c = uksched2schedcoop(s);

	UK_ASSERT(uk_lcpu_irqs_disabled());

	if (t != c->curr_thread)
		schedcoop_runq_remove(c, t);
	if (t->wakeup_time > 0)
		schedcoop_sleepq_add(c, t);
	else
		schedcoop_sleepq_remove(c, t);
}

static __noreturn void idle_thread_fn(void *argp)
{
	struct schedcoop *c = (struct schedcoop *) argp;
	__nsec now, wake_up_time;
	unsigned long flags;

	UK_ASSERT(c);

	for (;;) {
		if (uk_sched_thread_gc(&c->sched) > 0) {
			schedcoop_yield(&c->sched);
			continue;
		}

		ukplat_spin_lock_irqsave(&c->sched.lock, flags);
		if (!UK_TAILQ_EMPTY(&c->run_queue) ||
		    !UK_TAILQ_EMPTY(&c->sched.exited_threads)) {
			ukplat_spin_unlock_irqrestore(&c->sched.lock, flags);
			schedcoop_yield(&c->sched);
			continue;
		}

		/* Read return time under sched.lock, then release the spinlock
		 * while keeping local IRQs disabled until halt atomically re-enables
		 * them so a remote wakeup IPI cannot pass before hlt.
		 */
		wake_up_time = c->idle_return_time;
		ukarch_spin_unlock(&c->sched.lock);

		now = ukplat_monotonic_clock();
		if (!wake_up_time || wake_up_time > now) {
			if (wake_up_time)
				uk_lcpu_halt_irq_until(wake_up_time);
			else
				uk_lcpu_halt_irq();

			/* handle pending events if any */
			uk_lcpu_irqs_handle_pending();
		}

		uk_lcpu_restore_irqf(flags);

		schedcoop_yield(&c->sched);
	}
}

static int schedcoop_start(struct uk_sched *s,
			   struct uk_thread *main_thread __maybe_unused)
{
	struct schedcoop *c = uksched2schedcoop(s);

	UK_ASSERT(main_thread);
	UK_ASSERT(main_thread->sched == s);
	UK_ASSERT(uk_thread_is_runnable(main_thread));
	UK_ASSERT(!uk_thread_is_exited(main_thread));
	UK_ASSERT(uk_thread_current() == main_thread);

	c->curr_thread = main_thread;

	/* Since we are now starting to schedule, we save the current timestamp
	 * as the start time for the first time slice.
	 */
	c->ts_prev_switch = ukplat_monotonic_clock();

	/* NOTE: We do not put `main_thread` into the thread list.
	 *       Current running threads will be added as soon as
	 *       a different thread is scheduled.
	 */

	uk_lcpu_enable_irq();

	return 0;
}

static const struct uk_thread *schedcoop_idle_thread(struct uk_sched *s,
						     unsigned int proc_id)
{
	struct schedcoop *c = uksched2schedcoop(s);

	/* NOTE: We only support one processing LCPU (for now) */
	if (proc_id > 0)
		return NULL;

	return &(c->idle);
}

struct uk_sched *uk_schedcoop_create(struct uk_alloc *a,
				     struct uk_alloc *sa,
				     struct uk_alloc *auxsa,
				     struct uk_alloc *tls_a)
{
	struct schedcoop *c = NULL;
	int rc;

	uk_pr_info("Initializing cooperative scheduler\n");
	c = uk_zalloc(a, sizeof(struct schedcoop));
	if (!c)
		goto err_out;

	UK_TAILQ_INIT(&c->run_queue);
	UK_TAILQ_INIT(&c->sleep_queue);

	/* Create idle thread */
	rc = uk_thread_init_fn1(&c->idle,
				idle_thread_fn, (void *) c,
				sa, STACK_SIZE,
				auxsa, AUXSTACK_SIZE,
				a, false,
				NULL,
				"idle",
				NULL,
				NULL);
	if (rc < 0)
		goto err_free_c;

	c->idle.sched = &c->sched;

	uk_sched_init(&c->sched,
			schedcoop_start,
			schedcoop_yield,
			schedcoop_thread_add,
			schedcoop_thread_remove,
			schedcoop_thread_blocked,
			schedcoop_thread_woken_isr,
			schedcoop_thread_woken_isr,
			schedcoop_idle_thread,
			a, sa, auxsa, tls_a);

	/* Add idle thread to the scheduler's thread list */
	UK_TAILQ_INSERT_TAIL(&c->sched.thread_list, &c->idle, thread_list);

	return &c->sched;

err_free_c:
	uk_free(a, c);
err_out:
	return NULL;
}
