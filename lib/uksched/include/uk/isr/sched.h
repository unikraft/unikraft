/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#ifndef __UK_SCHED_SCHED_ISR_H__
#define __UK_SCHED_SCHED_ISR_H__

#include <uk/isr/thread.h>
#include <uk/sched.h>

/* ISR variant of `uk_sched_thread_woken()` */
static inline void uk_sched_thread_woken_isr(struct uk_thread *t)
{
	struct uk_sched *s;

	UK_ASSERT(t);
	UK_ASSERT(t->sched);
	UK_ASSERT(uk_thread_is_runnable(t));

	s = t->sched;
	s->thread_woken_isr(s, t);
}
#endif /* __UK_SCHED_SCHED_ISR_H__ */
