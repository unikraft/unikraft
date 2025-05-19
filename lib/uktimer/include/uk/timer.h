/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#ifndef __UKTIMER_TIMER_H__
#define __UKTIMER_TIMER_H__

#include <uk/list.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct uk_timer {
	/** List entry. uktimer internal */
	struct uk_hlist_node timer_list;
	/** Whether the timer is registered. uktimer internal */
	__u8 registered;

	/** Expiry in terms of uk_jiffies */
	__u64 expiry;
	/**
	 * The function to call when the timer expires.
	 */
	void (*callback)(struct uk_timer *);
	/**
	 * Additional data that can be used by the uktimer client.
	 */
	void *priv;
};

void uktimer_timer_add(struct uk_timer *timer);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __UKTIMER_TIMER_H__ */
