/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#include "schedcoop.h"

void schedcoop_thread_woken_isr(struct uk_sched *s, struct uk_thread *t)
{
	struct schedcoop *c = uksched2schedcoop(s);

	UK_ASSERT(ukplat_lcpu_irqs_disabled());

	if (uk_thread_is_queueable(t) && uk_thread_is_runnable(t)) {
		UK_TAILQ_INSERT_TAIL(&c->run_queue, t, queue);
		uk_thread_clear_queueable(t);
		UK_WRITE_ONCE(c->have_pending_events, 1);
	}
}
