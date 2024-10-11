/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#ifndef __UK_CLOCK_EVENT__
#define __UK_CLOCK_EVENT__

#ifdef __cplusplus
extern "C" {
#endif

#include <uk/plat/time.h>
#include <uk/list.h>

struct uk_clock_event;

typedef int (*clock_event_set_next_event_t)(struct uk_clock_event *ce,
					    __nsec at);

typedef int (*clock_event_disable_t)(struct uk_clock_event *ce);

typedef int (*clock_event_event_handler_t)(struct uk_clock_event *ce);

struct uk_clock_event {
	/** Clock list head. Internal to Unikraft */
	struct uk_list_head clock_list;
	/** Name of the clock. */
	const char *name;
	/**
	 * Priority of the clock event device. A higher number means a higher
	 * priority.
	 */
	int priority;
	/**
	 * Program the next expiry.
	 */
	clock_event_set_next_event_t set_next_event;
	/**
	 * Disable the clock event
	 */
	clock_event_disable_t disable;
	/**
	 * The event handler to call on expiry
	 */
	clock_event_event_handler_t event_handler;
	/** Clock implementation data */
	void *priv;
};

void uk_clock_event_register(struct uk_clock_event *ce);

extern struct uk_list_head uk_clock_event_list;

#define uk_clock_event_foreach(itr)					\
	uk_list_for_each_entry((itr), &uk_clock_event_list, clock_list)

#define UKPLAT_

#ifdef __cplusplus
}
#endif

#endif /* __UK_CLOCK_EVENT__ */
