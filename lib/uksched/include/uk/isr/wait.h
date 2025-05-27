/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#ifndef __UK_SCHED_WAIT_ISR_H__
#define __UK_SCHED_WAIT_ISR_H__

#include <uk/isr/thread.h>
#include <uk/wait.h>

/* ISR variant of `uk_waitq_wake_up()` */
static inline void uk_waitq_wake_up_isr(struct uk_waitq *wq)
{
	struct uk_waitq_ticket *t;
	unsigned long flags;

	_uk_waitq_lock(wq, flags);
	uk_list_for_each_entry(t, &wq->waiters, link)
		uk_thread_wake_isr(uk_thread_of_wait_ticket(t));
	_uk_waitq_unlock(wq, flags);
}

#endif /* __UK_SCHED_WAIT_ISR_H__ */
