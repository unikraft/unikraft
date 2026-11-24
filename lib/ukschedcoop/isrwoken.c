/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#include "schedcoop.h"

void schedcoop_thread_woken_isr(struct uk_sched *s, struct uk_thread *t)
{
	struct schedcoop *c = uksched2schedcoop(s);

	UK_ASSERT(uk_lcpu_irqs_disabled());
	schedcoop_wake_locked(c, t);
}
