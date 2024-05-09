/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#ifndef __UK_LOCK_SEMAPHORE_ISR_H__
#define __UK_LOCK_SEMAPHORE_ISR_H__

#include <uk/isr/wait.h>
#include <uk/semaphore.h>

/*
 * NOTE: uk_semaphore_down_try() is a static inline function, so it will
 * use the same compilation flags as the unit where it is used
 */
#define uk_semaphore_down_try_isr(x) uk_semaphore_down_try(x)

static inline void uk_semaphore_up_isr(struct uk_semaphore *s)
{
	unsigned long irqf;

	UK_ASSERT(s);

	uk_spin_lock_irqsave(&(s->sl), irqf);
	++s->count;
#if UK_SEMAPHORE_DEBUG
	uk_pr_debug("Increased semaphore %p to %ld\n",
		    s, s->count);
#endif /* UK_SEMAPHORE_DEBUG */
	uk_waitq_wake_up_isr(&s->wait);
	uk_spin_unlock_irqrestore(&(s->sl), irqf);
}

#endif /* __UK_LOCK_SEMAPHORE_ISR_H__ */
