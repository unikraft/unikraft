/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors. */
#ifndef __UKSCHED_THREAD_STATE_H__
#define __UKSCHED_THREAD_STATE_H__

#include <uk/lcpu.h>
#include <uk/sched.h>
#include <uk/thread.h>

/* Shared transition serialized under the owning scheduler lock when bound.
 * Caller retains its normal/ISR notification path and local IRQ guard.
 */
static inline bool _uk_thread_wake_transition(struct uk_thread *thread,
					      void (*notify)(struct uk_thread *))
{
	struct uk_sched *s;
	bool woken = false;

	UK_ASSERT(uk_lcpu_irqs_disabled());
	s = __atomic_load_n(&thread->sched, __ATOMIC_ACQUIRE);
	if (s)
		ukarch_spin_lock(&s->lock);

	if (!uk_thread_is_runnable(thread)) {
		uk_thread_set_runnable(thread);
		if (s && thread->sched == s) {
			notify(thread);
			woken = true;
		}
	}
	thread->wakeup_time = 0LL;

	if (s)
		ukarch_spin_unlock(&s->lock);
	if (woken)
		uk_sched_kick_cpu(s);
	return woken;
}

#endif /* __UKSCHED_THREAD_STATE_H__ */
