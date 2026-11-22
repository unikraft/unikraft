/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors. */
#ifndef __UKSCHED_THREAD_STATE_H__
#define __UKSCHED_THREAD_STATE_H__

#include <uk/lcpu.h>
#include <uk/thread.h>

/* Shared transition; the caller retains its normal/ISR notification path
 * and its IRQ guard. Cross-CPU serialization is a separate change.
 */
static inline void _uk_thread_wake_transition(struct uk_thread *thread,
					     void (*notify)(struct uk_thread *))
{
	UK_ASSERT(uk_lcpu_irqs_disabled());
	if (!uk_thread_is_runnable(thread)) {
		uk_thread_set_runnable(thread);
		if (thread->sched)
			notify(thread);
	}
	thread->wakeup_time = 0LL;
}

#endif /* __UKSCHED_THREAD_STATE_H__ */
