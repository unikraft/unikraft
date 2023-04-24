/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#include <uk/timer.h>
#include <uk/trace.h>
#include <uk/tick.h>

#include "timer_wheel.h"

UK_TRACEPOINT(uktimer_timer_added,
	      "timer=%#x expiry=%lu callback=%#x", void *, __u64, void *);

void uktimer_timer_add(struct uk_timer *timer) {
	uktimer_timer_added(timer, timer->expiry, timer->callback);
	uktimer_wheel_add(timer);
}
