/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#ifndef __UK_SCHEDCOOP_SCHEDCOOP_H__
#define __UK_SCHEDCOOP_SCHEDCOOP_H__

#include <uk/schedcoop.h>

#define SCHEDCOOP_THREADF_IN_RUNQ   (0x100U)
#define SCHEDCOOP_THREADF_IN_SLEEPQ (0x200U)

struct schedcoop {
	struct uk_sched sched;
	struct uk_thread *curr_thread;
	struct uk_thread_list run_queue;
	struct uk_thread_list sleep_queue;

	struct uk_thread idle;
	__nsec idle_return_time;
	__nsec ts_prev_switch;
};

static inline struct schedcoop *uksched2schedcoop(struct uk_sched *s)
{
	UK_ASSERT(s);

	return __containerof(s, struct schedcoop, sched);
}

static inline void schedcoop_runq_push(struct schedcoop *c, struct uk_thread *t)
{
	if (!(t->flags & SCHEDCOOP_THREADF_IN_RUNQ)) {
		UK_ASSERT(!(t->flags & SCHEDCOOP_THREADF_IN_SLEEPQ));
		UK_TAILQ_INSERT_TAIL(&c->run_queue, t, queue);
		t->flags |= SCHEDCOOP_THREADF_IN_RUNQ;
	}
}

static inline void schedcoop_runq_remove(struct schedcoop *c,
					 struct uk_thread *t)
{
	if (t->flags & SCHEDCOOP_THREADF_IN_RUNQ) {
		UK_TAILQ_REMOVE(&c->run_queue, t, queue);
		t->flags &= ~SCHEDCOOP_THREADF_IN_RUNQ;
	}
}

static inline struct uk_thread *schedcoop_runq_pop_first(struct schedcoop *c)
{
	struct uk_thread *t = UK_TAILQ_FIRST(&c->run_queue);

	if (t) {
		UK_ASSERT(t->flags & SCHEDCOOP_THREADF_IN_RUNQ);
		UK_TAILQ_REMOVE(&c->run_queue, t, queue);
		t->flags &= ~SCHEDCOOP_THREADF_IN_RUNQ;
	}
	return t;
}

static inline void schedcoop_sleepq_add(struct schedcoop *c,
					struct uk_thread *t)
{
	if (!(t->flags & SCHEDCOOP_THREADF_IN_SLEEPQ)) {
		UK_ASSERT(!(t->flags & SCHEDCOOP_THREADF_IN_RUNQ));
		UK_TAILQ_INSERT_TAIL(&c->sleep_queue, t, queue);
		t->flags |= SCHEDCOOP_THREADF_IN_SLEEPQ;
	}
}

static inline void schedcoop_sleepq_remove(struct schedcoop *c,
					   struct uk_thread *t)
{
	if (t->flags & SCHEDCOOP_THREADF_IN_SLEEPQ) {
		UK_TAILQ_REMOVE(&c->sleep_queue, t, queue);
		t->flags &= ~SCHEDCOOP_THREADF_IN_SLEEPQ;
	}
}

static inline void schedcoop_wake_locked(struct schedcoop *c,
					 struct uk_thread *t)
{
	schedcoop_sleepq_remove(c, t);
	if (uk_thread_is_runnable(t) && t != c->curr_thread && t != &c->idle)
		schedcoop_runq_push(c, t);
}

void schedcoop_thread_woken_isr(struct uk_sched *s, struct uk_thread *t);

#endif /* __UK_SCHEDCOOP_SCHEDCOOP_H__ */
