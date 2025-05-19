/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#ifndef __UKTIMER_HRTIMER_H__
#define __UKTIMER_HRTIMER_H__

#include <uk/plat/time.h>
#include <uk/tree.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum uk_hr_timer_rearm {
	/**
	 * Don't rearm the timer. The timer structure won't be used by uktimer
	 * anymore.
	 */
	UK_HR_TIMER_NO_REARM,
	/**
	 * Rearm the timer with the updated expiry time.
	 */
	UK_HR_TIMER_REARM,
};

struct uk_hr_timer {
	/** RB-Tree entry. uktimer internal */
	UK_RB_ENTRY(uk_hr_timer) entry;
	/** Whether the timer is registered. uktimer internal */
	__u8 registered;

	/**
	 * The expiry time of the timer. If the expiry is in the past, then the
	 * timer will expire immediately.
	 */
	__nsec expiry;
	/**
	 * The function to call when the timer expires. Depending on the return
	 * value the timer will be re-armed after the function returns.
	 * The function must be callable in a ISR context, but can also be
	 * called from a non-ISR context.
	 */
	enum uk_hr_timer_rearm (*callback)(struct uk_hr_timer *);
	/**
	 * Additional data that can be used by the uktimer client.
	 */
	void *priv;
};

static inline void uktimer_hr_init(struct uk_hr_timer *hrt)
{
	hrt->registered = 0;
	hrt->expiry = 0;
	hrt->callback = __NULL;
	hrt->priv = __NULL;
}

/**
 * Register a initialized timer structure.
 * @param hrt the timer to add
 */
void uktimer_hr_add(struct uk_hr_timer *hrt);

/**
 * Update the internal data structure after the expiry of hrt was changed.
 * @param hrt the timer that was updated
 */
void uktimer_hr_update(struct uk_hr_timer *hrt);

/**
 * Try to remove the timer from the uktimer system. The timer structure can be
 * still in use (for example the interrupt handler scheduled an execution of the
 * callback)
 * @param hrt the timer to remove
 * @return zero if no immediate error was encountered during the removal, or a
 * negative error code if an error was encountered.
 */
int uktimer_hr_try_remove(struct uk_hr_timer *hrt);

/**
 * Remove the timer from the uktimer system. When this function returns the
 * timer structure will not be used by uktimer anymore.
 * @param hrt the timer to remove
 */
void uktimer_hr_remove(struct uk_hr_timer *hrt);

void uktimer_hr_dump(void);

#define UK_HRTIMER_INIT_CLASS UK_INIT_CLASS_EARLY
#define UK_HRTIMER_INIT_PRIO  1

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __UKTIMER_HRTIMER_H__ */
