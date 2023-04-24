/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#include <uk/tick.h>
#include <uk/hrtimer.h>
#include <uk/init.h>
#include <uk/print.h>
#include <uk/trace.h>

UK_TRACEPOINT(uktimer_tick_skipped, "%lu", unsigned long);

static int tick_idle;
static struct uk_hr_timer tick_timer;

__u64 uk_jiffies;

static enum uk_hr_timer_rearm
uktimer_tick_callback(struct uk_hr_timer *hrt)
{
	__u64 ticks;
	__nsec now;

	now = ukplat_monotonic_clock();
	ticks = (now - hrt->expiry) / UKPLAT_TIME_TICK_NSEC + 1;
	if (ticks > 1)
		uktimer_tick_skipped(ticks - 1);

	uk_jiffies += ticks;

	/* Reset timer */
	hrt->expiry += UKPLAT_TIME_TICK_NSEC * ticks;
	return tick_idle ? UK_HR_TIMER_NO_REARM : UK_HR_TIMER_REARM;
}

void uktimer_set_idle(int idle)
{
	idle = idle != 0;

	if (idle == tick_idle)
		return;

	tick_idle = idle;
	if (tick_idle) {
		uktimer_hr_remove(&tick_timer);
	} else {
		/* The callback will take care of the catchup ticks */
		uktimer_hr_add(&tick_timer);
	}
}

static int uktimer_tick_init(struct uk_init_ctx *ctx __unused)
{
	tick_timer.callback = uktimer_tick_callback;
	tick_timer.expiry = ukplat_monotonic_clock() + UKPLAT_TIME_TICK_NSEC;
	uktimer_hr_add(&tick_timer);

	return 0;
}

uk_early_initcall_prio(uktimer_tick_init, 0, UK_PRIO_AFTER(UK_HRTIMER_INIT_PRIO));
