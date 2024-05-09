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
	struct uk_waitq_entry *curr, *tmp;
	unsigned long flags;

	ukplat_spin_lock_irqsave(&wq->sl, flags);
	UK_STAILQ_FOREACH_SAFE(curr, &wq->wait_list, thread_list, tmp)
		uk_thread_wake_isr(curr->thread);
	ukplat_spin_unlock_irqrestore(&wq->sl, flags);
}

#endif /* __UK_SCHED_WAIT_ISR_H__ */
