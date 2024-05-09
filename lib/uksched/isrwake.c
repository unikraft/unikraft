/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#include <uk/isr/sched.h>
#include <uk/isr/thread.h>

void uk_thread_wake_isr(struct uk_thread *thread)
{
	unsigned long flags;

	flags = ukplat_lcpu_save_irqf();
	if (!uk_thread_is_runnable(thread)) {
		uk_thread_set_runnable(thread);
		if (thread->sched)
			uk_sched_thread_woken_isr(thread);
	}
	thread->wakeup_time = 0LL;
	ukplat_lcpu_restore_irqf(flags);
}
