/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *          Sairaj Kodilkar <skodilkar7@gmail.com>
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
#include <uk/preempt.h>

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

#define UK_MUTEX_CONFIG_RECURSE 0x01 /* Allow recursive locking */

/*
 * Mutex that relies on a scheduler
 * uses wait queues for threads
 */
struct uk_mutex {
	__uptr lock;
	int lock_count;
	unsigned int flags;
	struct uk_waitq wait;
};

static inline int uk_mutex_is_recursive(const struct uk_mutex *m)
{
	return (m->flags & UK_MUTEX_CONFIG_RECURSE);
}

#define _UK_MUTEX_UNOWNED	0x1
#define _UK_MUTEX_CONTESTED	0x2
#define _UK_MUTEX_STATE_MASK	(_UK_MUTEX_UNOWNED | _UK_MUTEX_CONTESTED)

#if CONFIG_LIBUKLOCK_MUTEX_ATOMIC
#define _uk_mutex_lock_fetch(m, v, tid)					\
	__atomic_compare_exchange_n(&(m)->lock,				\
		v, tid, 0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)
#define _uk_mutex_unlock_fetch(m, v)					\
	__atomic_compare_exchange_n(&(m)->lock,				\
		v, _UK_MUTEX_UNOWNED, 0,				\
		__ATOMIC_RELEASE, __ATOMIC_RELAXED)
#else
#define _uk_mutex_lock_fetch(m, v, tid)					\
	({								\
		int _success;						\
		uk_preempt_disable();					\
		if ((m)->lock == *(v)) {				\
			(m)->lock = tid;				\
			_success = 1;					\
		} else {						\
			*(v) = (m)->lock;				\
			_success = 0;					\
		}							\
		uk_preempt_enable();					\
		_success;						\
	})

#define _uk_mutex_unlock_fetch(m, v)					\
	({								\
		int _success;						\
		uk_preempt_disable();					\
		if ((m)->lock == *(v)) {				\
			(m)->lock = _UK_MUTEX_UNOWNED;			\
			_success = 1;					\
		} else {						\
			*(v) = (m)->lock;				\
			_success = 0;					\
		}							\
		uk_preempt_enable();					\
		_success;						\
	})

#endif

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
	{ _UK_MUTEX_UNOWNED, 0, 0, __WAIT_QUEUE_INITIALIZER((name).wait) }

#define	UK_MUTEX_INITIALIZER_RECURSIVE(name)			\
	{ _UK_MUTEX_UNOWNED, 0, UK_MUTEX_CONFIG_RECURSE,	\
	__WAIT_QUEUE_INITIALIZER((name).wait) }

void uk_mutex_init_config(struct uk_mutex *m, unsigned int flags);
void uk_mutex_get_metrics(struct uk_mutex_metrics *dst);

#define uk_mutex_init(m) uk_mutex_init_config(m, 0)

void _uk_mutex_lock_wait(struct uk_mutex *m, __uptr tid, __uptr v);
int _uk_mutex_trylock_wait(struct uk_mutex *m, __uptr tid, __uptr v);
void _uk_mutex_unlock_wait(struct uk_mutex *m);

static inline void uk_mutex_lock(struct uk_mutex *m)
{
	__uptr tid, v;

	UK_ASSERT(m);

	v = _UK_MUTEX_UNOWNED;
	tid = (__uptr)uk_thread_current();

	if (_uk_mutex_lock_fetch(m, &v, tid)) {
		UK_ASSERT(m->lock_count == 0);
		m->lock_count = 1;
	} else {
		_uk_mutex_lock_wait(m, tid, v);
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
	__uptr tid, v;

	UK_ASSERT(m);

	v = _UK_MUTEX_UNOWNED;
	tid = (__uptr)uk_thread_current();

	if (_uk_mutex_lock_fetch(m, &v, tid)) {
		UK_ASSERT(m->lock_count == 0);
		m->lock_count = 1;
#ifdef CONFIG_LIBUKLOCK_MUTEX_METRICS
		ukarch_spin_lock(&_uk_mutex_metrics_lock);
			_uk_mutex_metrics.active_locked++;
			_uk_mutex_metrics.active_unlocked--;
			_uk_mutex_metrics.total_ok_trylocks++;
			ukarch_spin_unlock(&_uk_mutex_metrics_lock);
#endif /* CONFIG_LIBUKLOCK_MUTEX_METRICS */

		return 1;
	}
	return _uk_mutex_trylock_wait(m, tid, v);
}

static inline int uk_mutex_is_locked(struct uk_mutex *m)
{
	__uptr tid;

	UK_ASSERT(m);
	tid = (__uptr)uk_thread_current();

	return (m->lock & ~_UK_MUTEX_STATE_MASK) ==
	       (tid & ~_UK_MUTEX_STATE_MASK);
}


static inline void uk_mutex_unlock(struct uk_mutex *m)
{
	__uptr v;

	v = (__uptr)uk_thread_current();

	UK_ASSERT(m);
	UK_ASSERT(m->lock_count > 0);
	UK_ASSERT((m->lock & ~_UK_MUTEX_STATE_MASK) ==
		  (v & ~_UK_MUTEX_STATE_MASK));

	if (--m->lock_count == 0) {
		/* Make sure lock_count is visible before resetting the
		 * owner. The lock can be acquired afterwards.
		 */
		if (!_uk_mutex_unlock_fetch(m, &v))
			_uk_mutex_unlock_wait(m);
	}

#ifdef CONFIG_LIBUKLOCK_MUTEX_METRICS
	ukarch_spin_lock(&_uk_mutex_metrics_lock);
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

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_LIBUKLOCK_MUTEX */

#endif /* __UK_MUTEX_H__ */
