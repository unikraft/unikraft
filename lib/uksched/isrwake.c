/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#include <uk/isr/sched.h>
#include <uk/isr/thread.h>
#include "thread_state.h"

void uk_thread_wake_isr(struct uk_thread *thread)
{
	unsigned long flags;

	flags = uk_lcpu_save_irqf();
	_uk_thread_wake_transition(thread, UK_THREAD_WAKE_NORMAL,
				   uk_sched_thread_woken_isr);
	uk_lcpu_restore_irqf(flags);
}
