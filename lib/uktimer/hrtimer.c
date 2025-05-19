/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#include <uk/hrtimer.h>
#include <uk/init.h>
#include <uk/clock_event.h>
#include <uk/plat/lcpu.h>
#include <uk/print.h>
#include <uk/assert.h>
#include <errno.h>

static int expiry_cmp(struct uk_hr_timer *e1, struct uk_hr_timer *e2)
{
	/* First order the elements according to their expiry */
	if (e1->expiry != e2->expiry)
		return e1->expiry < e2->expiry ? -1 : 1;
	/* If the expiry is the same, then use the pointer value for ordering.
	 * This works because the pointer value will not change for an
	 * individual timer.
	 */
	if ((__uptr)e1 != (__uptr)e2)
		return (__uptr)e1 < (__uptr)e2 ? -1 : 1;

	return 0;
}

/*
 * Tree structure used to manage active hr_timers.
 */
UK_RB_HEAD(timer_tree, uk_hr_timer)
timer_tree_head = UK_RB_INITIALIZER(&timer_tree_head);
UK_RB_GENERATE(timer_tree, uk_hr_timer, entry, expiry_cmp);

static struct uk_clock_event *hrtimer_clock_event;
static int hrtimer_clock_event_active;

static unsigned long hrtimer_crit_begin(void)
{
	unsigned long flags;
	flags = ukplat_lcpu_save_irqf();
	__atomic_signal_fence(__ATOMIC_ACQUIRE);
	return flags;
}

static void hrtimer_crit_end(unsigned long flags)
{
	__atomic_signal_fence(__ATOMIC_RELEASE);
	ukplat_lcpu_restore_irqf(flags);
}

/**
 * Update the clock_event for the next expiry.
 * @param new A expiry time that was just added, NULL if it isn't known.
 * @return a non-zero value if the clock_event was programmed.
 */
static int hrtimer_clock_event_update(const __nsec *new)
{
	struct uk_hr_timer *hrt = NULL;
	__nsec prog = 0;

	if (!hrtimer_clock_event_active && new) {
		/* The clock event was previously not active, and we received a
		 * timestamp => use that timestamp
		 */
		prog = *new;
	} else {
		/* Either the clock event was active *or* we didn't receive a
		 * timestamp => fetch the minimum expiry time (which could also
		 * be *new)
		 */
		/* TODO: Introduce a range and allow batching */
		hrt = UK_RB_MIN(timer_tree, &timer_tree_head);
		if (hrt)
			prog = hrt->expiry;
	}

	if (prog) {
		hrtimer_clock_event->set_next_event(hrtimer_clock_event, prog);
		hrtimer_clock_event_active = 1;
	} else {
		hrtimer_clock_event_active = 0;
	}
	return hrtimer_clock_event_active;
}

static void uktimer_hr_do_callback_noupdate(struct uk_hr_timer *hrt)
{
	enum uk_hr_timer_rearm rearm;

	if (hrt->registered) {
		UK_RB_REMOVE(timer_tree, &timer_tree_head, hrt);
		hrt->registered = 0;
	}

	rearm = hrt->callback(hrt);
	if (rearm == UK_HR_TIMER_REARM) {
		UK_RB_INSERT(timer_tree, &timer_tree_head, hrt);
		hrt->registered = 1;
	}
}

static void uktimer_hr_do_callback(struct uk_hr_timer *hrt)
{
	uktimer_hr_do_callback_noupdate(hrt);
	hrtimer_clock_event_update(hrt->registered ? &hrt->expiry : 0);
}

void uktimer_hr_add(struct uk_hr_timer *hrt)
{
	struct uk_hr_timer *prev;
	unsigned long flags;

	/* Ignore repeated add */
	if (hrt->registered)
		return;

	flags = hrtimer_crit_begin();

	if (hrt->expiry < ukplat_monotonic_clock()) {
		/* If the timer already expired just do the callback directly */
		uktimer_hr_do_callback(hrt);
		goto exit;
	}

	prev = UK_RB_INSERT(timer_tree, &timer_tree_head, hrt);
	/* The hrt->registered check should catch this case */
	UK_ASSERT(!prev);
	hrt->registered = 1;
	hrtimer_clock_event_update(&hrt->expiry);
exit:
	hrtimer_crit_end(flags);
}

void uktimer_hr_update(struct uk_hr_timer *hrt)
{
	unsigned long flags;

	UK_ASSERT(hrt->registered);

	flags = hrtimer_crit_begin();

	if (hrt->expiry < ukplat_monotonic_clock()) {
		/* If the timer already expired just do the callback directly */
		uktimer_hr_do_callback(hrt);
		goto exit;
	}

	UK_RB_REINSERT(timer_tree, &timer_tree_head, hrt);
	hrtimer_clock_event_update(&hrt->expiry);
exit:
	hrtimer_crit_end(flags);
}

int uktimer_hr_try_remove(struct uk_hr_timer *hrt)
{
	unsigned long flags;

	if (!hrt->registered)
		return 0;

	flags = hrtimer_crit_begin();

	UK_RB_REMOVE(timer_tree, &timer_tree_head, hrt);
	hrt->registered = 0;
	hrtimer_clock_event_update(NULL);

	hrtimer_crit_end(flags);
	return 0;
}

void uktimer_hr_remove(struct uk_hr_timer *hrt)
{
	/* Equivalent to try_remove in the non-SMP case */
	uktimer_hr_try_remove(hrt);
}

static int uktimer_hr_event_handler(struct uk_clock_event *ce __unused)
{
	struct uk_hr_timer *hrt;
	__nsec now, next;

	next = 0;
	now = ukplat_monotonic_clock();

	for (hrt = UK_RB_MIN(timer_tree, &timer_tree_head); hrt != NULL;
	     hrt = UK_RB_NEXT(timer_tree, &timer_tree_head, hrt)) {
		if (hrt->expiry > now) {
			next = hrt->expiry;
			break;
		}
		uktimer_hr_do_callback_noupdate(hrt);
	}
	hrtimer_clock_event_update(next ? &next : NULL);

	return 0;
}

void uktimer_hr_dump(void)
{
	struct uk_hr_timer *hrt;
	unsigned long flags;

	flags = hrtimer_crit_begin();

	for (hrt = UK_RB_MIN(timer_tree, &timer_tree_head); hrt != NULL;
	     hrt = UK_RB_NEXT(timer_tree, &timer_tree_head, hrt)) {
		uk_pr_info("timer=%p expiry=%lu\n", hrt, hrt->expiry);
	}

	hrtimer_crit_end(flags);
}

static int uktimer_hr_libinit(struct uk_init_ctx * ctx __unused)
{
	struct uk_clock_event *ce, *prio = NULL;

	uk_clock_event_foreach(ce) {
		if (prio == __NULL || prio->priority < ce->priority)
			prio = ce;
	}
	if (!prio)
		return -ENOENT;

	uk_pr_info("Using '%s' for high-resolution timers\n", prio->name);
	prio->event_handler = uktimer_hr_event_handler;

	hrtimer_clock_event = prio;
	hrtimer_clock_event_update(__NULL);

	return 0;
}

uk_initcall_class_prio(uktimer_hr_libinit,
		       0,
		       UK_HRTIMER_INIT_CLASS,
		       UK_HRTIMER_INIT_PRIO);
