/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __UK_MUTEX_H__
#define __UK_MUTEX_H__

#include <uk/config.h>

#if CONFIG_LIBUKLOCK_MUTEX
#include <uk/assert.h>
#include <uk/plat/lcpu.h>
#include <uk/thread.h>
#include <uk/wait.h>
#include <uk/wait_types.h>
#include <uk/plat/time.h>

#ifdef CONFIG_LIBUKLOCK_MUTEX_METRICS
#include <uk/plat/spinlock.h>
#include <string.h>
#endif /* CONFIG_LIBUKLOCK_MUTEX_METRICS */

#ifdef __cplusplus
extern "C" {
#endif

#include <uk/lock-common.h>

/*
 * Mutex that relies on a scheduler
 * uses wait queues for threads
 */
struct uk_mutex {
	int lock_count;
#define UK_MUTEX_CONFIG_WRITE_RECURSE 0x01
	uint8_t config_flags;
	struct uk_thread *owner;
	struct uk_waitq wait;
};

#define __IS_CONFIG_WRITE_RECURSE(flag)	(flag & UK_MUTEX_CONFIG_WRITE_RECURSE)

/*
 * Mutex statistics for ukstore.
 */
struct uk_mutex_metrics {
	/** Mutex objects currently locked */
	size_t active_locked;
	/** Mutex objects currently unlocked */
	size_t active_unlocked;

	/** Successful blocking lock operations since startup */
	size_t total_locks;
	/** Successful non-blocking (trylock) operations since startup */
	size_t total_ok_trylocks;
	/** Failed non-blocking (trylock) operations since startup */
	size_t total_failed_trylocks;
	/** Successful unlock operations since startup */
	size_t total_unlocks;
};

#ifdef CONFIG_LIBUKLOCK_MUTEX_METRICS
/*
 * Metric storage (see mutex.c).
 */
extern struct uk_mutex_metrics _uk_mutex_metrics;
extern __spinlock              _uk_mutex_metrics_lock;
#endif /* CONFIG_LIBUKLOCK_MUTEX_METRICS */

#define	UK_MUTEX_INITIALIZER(name)				\
	{ 0, 0, NULL, __WAIT_QUEUE_INITIALIZER((name).wait) }

void __uk_mutex_init(struct uk_mutex *m, uint8_t config_flags);
void uk_mutex_get_metrics(struct uk_mutex_metrics *dst);

#define uk_mutex_init(m) __uk_mutex_init(m, 0)
#define uk_mutex_init_config(m, config_flags) __uk_mutex_init(m, config_flags)

static inline void uk_mutex_lock(struct uk_mutex *m)
{
	struct uk_thread *current;

	UK_ASSERT(m);

	current = uk_thread_current();

	/* if the owner is current thread then it definitely can't unlock during
	 * uk_mutex_lock, hence if the owner is current thread then we can simply
	 * increment the lock count
	 */
	if (__IS_CONFIG_WRITE_RECURSE(m->config_flags)
			&& unlikely(m->owner == current)) {
		ukarch_inc(&(m->lock_count));
		return;
	} else {
		UK_ASSERT(m->owner != current);
	}


	for (;;) {
		uk_waitq_wait_event(&m->wait, m->owner == NULL);

		/* If there is no owner for the lock we can grab it and increment the
		 * lock count
		 *
		 * The owner act as the lock
		 * We dont need to disable interrupts because the compare exchange
		 * is atomic even on the single processor
		 */
		if (ukarch_compare_exchange_sync(&m->owner, 0, current) == current) {
			ukarch_inc(&(m->lock_count));
			break;
		}
	}
#ifdef CONFIG_LIBUKLOCK_MUTEX_METRICS
	ukarch_spin_lock(&_uk_mutex_metrics_lock);

	_uk_mutex_metrics.active_locked   += (m->lock_count == 1);
	_uk_mutex_metrics.active_unlocked -= (m->lock_count == 1);
	_uk_mutex_metrics.total_locks++;

	ukarch_spin_unlock(&_uk_mutex_metrics_lock);
#endif /* CONFIG_LIBUKLOCK_MUTEX_METRICS */
}

static inline int uk_mutex_trylock(struct uk_mutex *m)
{
	struct uk_thread *current;
	int ret = 0;

	UK_ASSERT(m);

	current = uk_thread_current();

	if (__IS_CONFIG_WRITE_RECURSE(m->config_flags) && m->owner == current)
		ret = 1;
	if (!ret && ukarch_compare_exchange_sync(&m->owner, 0, current) == current)
		ret = 1;

#ifdef CONFIG_LIBUKLOCK_MUTEX_METRICS
	_uk_mutex_metrics.active_locked += (ret == 1) && (m->lock_count == 1);
	_uk_mutex_metrics.active_unlocked += (ret == 1) && (m->lock_count == 1);
	_uk_mutex_metrics.total_ok_trylocks += ret;
	_uk_mutex_metrics.total_failed_trylocks += !ret;
#endif /* CONFIG_LIBUKLOCK_MUTEX_METRICS */

	if (ret)
		ukarch_inc(&(m->lock_count));

	return ret;
}

static inline int uk_mutex_is_locked(struct uk_mutex *m)
{
	UK_ASSERT(m);
	return m->owner != NULL;
}

static inline void uk_mutex_unlock(struct uk_mutex *m)
{

	UK_ASSERT(m);

#ifdef CONFIG_LIBUKLOCK_MUTEX_METRICS
	ukarch_spin_lock(&_uk_mutex_metrics_lock);
#endif /* CONFIG_LIBUKLOCK_MUTEX_METRICS */

	UK_ASSERT(m->lock_count > 0);

	if (ukarch_sub_fetch(&(m->lock_count), 1) == 0) {
		m->owner = NULL;
		uk_waitq_wake_up(&m->wait);
	}

#ifdef CONFIG_LIBUKLOCK_MUTEX_METRICS
	_uk_mutex_metrics.active_locked   -= (m->lock_count == 0);
	_uk_mutex_metrics.active_unlocked += (m->lock_count == 0);
	_uk_mutex_metrics.total_unlocks++;

	ukarch_spin_unlock(&_uk_mutex_metrics_lock);
#endif /* CONFIG_LIBUKLOCK_MUTEX_METRICS */

}

#define uk_waitq_wait_event_mutex(wq, condition, mutex) \
	uk_waitq_wait_event_locked(wq, condition, uk_mutex_lock, \
				   uk_mutex_unlock, mutex)

#define uk_waitq_wait_event_deadline_mutex(wq, condition, deadline, mutex) \
	uk_waitq_wait_event_deadline_locked(wq, condition, deadline, \
					    uk_mutex_lock, uk_mutex_unlock, \
					    mutex)

_LOCK_IRQF(struct uk_mutex *, uk_mutex_lock)

_TRYLOCK_IRQF(struct uk_mutex *, uk_mutex_trylock)

_UNLOCK_IRQF(struct uk_mutex *, uk_mutex_unlock)

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_LIBUKLOCK_MUTEX */

#endif /* __UK_MUTEX_H__ */
