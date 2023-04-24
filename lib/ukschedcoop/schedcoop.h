/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#ifndef __UK_SCHEDCOOP_SCHEDCOOP_H__
#define __UK_SCHEDCOOP_SCHEDCOOP_H__

#include <uk/schedcoop.h>

struct schedcoop {
	struct uk_sched sched;
	struct uk_thread_list run_queue;
	struct uk_thread_list sleep_queue;
	unsigned long have_pending_events;

	struct uk_thread idle;
	__nsec idle_return_time;
	__nsec ts_prev_switch;
};

static inline struct schedcoop *uksched2schedcoop(struct uk_sched *s)
{
	UK_ASSERT(s);

	return __containerof(s, struct schedcoop, sched);
}

void schedcoop_thread_woken_isr(struct uk_sched *s, struct uk_thread *t);

#endif /* __UK_SCHEDCOOP_SCHEDCOOP_H__ */
