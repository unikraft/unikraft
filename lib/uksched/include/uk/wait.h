/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Unikraft wait queues */

/* This is a reimplementation of the API of waitqueues from Xen MiniOS, whose
 * BSD-2 licensed implementation served as the first waitqueues in Unikraft.
 */

#ifndef __UK_SCHED_WAIT_H__
#define __UK_SCHED_WAIT_H__

#include <uk/essentials.h>
#include <uk/plat/lcpu.h>
#include <uk/plat/time.h>
#include <uk/sched.h>
#include <uk/thread.h>
#include <uk/wait_types.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline
void uk_waitq_init(struct uk_waitq *wq)
{
	UK_INIT_LIST_HEAD(&wq->waiters);
	ukarch_spin_init(&wq->lock);
}

static inline
int uk_waitq_empty(struct uk_waitq *wq)
{
	return uk_list_empty(&wq->waiters);
}

/**
 * INTERNAL. Add the thread `thread` to waitqueue `wq`.
 *
 * `thread` must not be in a waitqueue and the lock of `wq` must be acquired.
 */
static inline
void _uk_waitq_add(struct uk_waitq *wq, struct uk_thread *thread)
{
	struct uk_waitq_ticket *t = &thread->wait_ticket;

	UK_ASSERT(!t->wq);
	t->wq = wq;
	uk_list_add_tail(&t->link, &wq->waiters);
}

/**
 * INTERNAL. Remove `thread` from the waitqueue it's in.
 *
 * `thread` must be linked in a waitqueue and that queue's lock must be held.
 */
static inline
void _uk_waitq_remove(struct uk_thread *thread)
{
	struct uk_waitq_ticket *t = &thread->wait_ticket;

	UK_ASSERT(t->wq);
	uk_list_del(&t->link);
	t->wq = __NULL;
}

/**
 * INTERNAL. Block thread until deadline without safeguards.
 *
 * We know what we're doing.
 */
static inline
void _uk_waitq_block_until(struct uk_thread *thread, __snsec deadline)
{
	uk_thread_set_wakeup(thread, deadline);
	uk_thread_set_blocked(thread);
	uk_sched_thread_blocked(thread);
}

/**
 * INTERNAL. Acquire lock on `wq`, using `flags` to store irqflags.
 */
#define _uk_waitq_lock(wq, flags) \
	ukplat_spin_lock_irqsave(&((wq)->lock), (flags))

/**
 * INTERNAL. Release lock on `wq`, restoring irqflags from `flags`.
 */
#define _uk_waitq_unlock(wq, flags) \
	ukplat_spin_unlock_irqrestore(&((wq)->lock), (flags))

/**
 * INTERNAL. Wait using `wq` until `condition` is met or until a deadline.
 *
 * Expression returns non-zero if timed out.
 *
 * Depending on `condition` may wait 0 or multiple times in the queue.
 *
 * If `deadline` is non-zero, wait at most until that point in time;
 * after wakeup, if `condition` is still unmet, compare `deadline` with the
 * value supplied by `clock_expr` to determine whether a timeout has occurred.
 *
 * If `lock` evaluates true, it will be released before sleeping with
 * `unlock_fn` and re-acquired after wakeup with `lock_fn`.
 */
#define _uk_waitq_wait_until(wq, condition, deadline, clock_expr,	\
			     lock_fn, unlock_fn, lock)			\
({									\
	int _uk_waitq_timedout = 0;					\
									\
	while (unlikely(!(condition))) {				\
		struct uk_thread *_uk_waitq_current = uk_thread_current();\
		unsigned long _uk_waitq_flags;				\
									\
		_uk_waitq_lock((wq), _uk_waitq_flags);			\
		if ((condition)) {					\
			/* Condition changed in the meantime */		\
			_uk_waitq_unlock((wq), _uk_waitq_flags);	\
			break;						\
		}							\
		/* Need to wait; add self to waitqueue & block */	\
		_uk_waitq_add((wq), _uk_waitq_current);			\
		for (;;) {						\
			_uk_waitq_block_until(_uk_waitq_current, (deadline));\
			_uk_waitq_unlock((wq), _uk_waitq_flags);	\
			if ((lock))					\
				unlock_fn((lock));			\
			uk_sched_yield();				\
			/* Awoken */					\
			if ((lock))					\
				lock_fn((lock));			\
			_uk_waitq_lock((wq), _uk_waitq_flags);		\
			if ((condition))				\
				break;					\
			if ((deadline) && ((clock_expr) >= (deadline))) {\
				_uk_waitq_timedout = 1;			\
				break;					\
			}						\
			/* Still need to wait; back to sleep */		\
		}							\
		/* No more waiting, remove from queue & exit */		\
		_uk_waitq_remove(_uk_waitq_current);			\
		_uk_waitq_unlock((wq), _uk_waitq_flags);		\
		break;							\
	}								\
	_uk_waitq_timedout;						\
})

/**
 * INTERNAL. Wait exactly once using `wq` until awoken or a deadline.
 *
 * Expression returns non-zero if timed out.
 *
 * If `deadline` is non-zero, wait at most until that point in time;
 * after wakeup compare `deadline` with the value supplied by `clock_expr` to
 * determine whether a timeout has occurred.
 *
 * If `lock` evaluates true, it will be released before sleeping with
 * `unlock_fn` and re-acquired after wakeup with `lock_fn`.
 */
#define _uk_waitq_wait(wq, deadline, clock_expr, lock_fn, unlock_fn, lock)\
({									\
	struct uk_thread *_uk_waitq_current = uk_thread_current();	\
	int _uk_waitq_timedout = 0;					\
	unsigned long _uk_waitq_flags;					\
									\
	_uk_waitq_lock((wq), _uk_waitq_flags);				\
	_uk_waitq_add((wq), _uk_waitq_current);				\
	_uk_waitq_block_until(_uk_waitq_current, (deadline));		\
	_uk_waitq_unlock((wq), _uk_waitq_flags);			\
	if ((lock))							\
		unlock_fn((lock));					\
	uk_sched_yield();						\
	/* Awoken */							\
	if ((lock))							\
		lock_fn((lock));					\
	_uk_waitq_lock((wq), _uk_waitq_flags);				\
	if ((deadline) && ((clock_expr) >= (deadline)))			\
		_uk_waitq_timedout = 1;					\
	_uk_waitq_remove(_uk_waitq_current);				\
	_uk_waitq_unlock((wq), _uk_waitq_flags);			\
									\
	_uk_waitq_timedout;						\
})

#define uk_waitq_wait_locked(wq, lock_fn, unlock_fn, lock) \
	_uk_waitq_wait((wq), 0, 0, lock_fn, unlock_fn, (lock))

static inline void _uk_waitq_noplock(int lock __unused) {}

#define uk_waitq_wait_event(wq, condition)				\
	_uk_waitq_wait_until((wq), (condition), 0, 0,			\
			     _uk_waitq_noplock, _uk_waitq_noplock, 0)

#define uk_waitq_wait_event_locked(wq, condition, lock_fn, unlock_fn, lock) \
	_uk_waitq_wait_until((wq), (condition), 0, 0,			\
			     lock_fn, unlock_fn, (lock))

#define uk_waitq_wait_event_deadline(wq, condition, deadline)		\
	_uk_waitq_wait_until((wq), (condition), (deadline),		\
			     ukplat_monotonic_clock(),			\
			     _uk_waitq_noplock, _uk_waitq_noplock, 0)

#define uk_waitq_wait_event_deadline_locked(wq, condition, deadline,	\
					    lock_fn, unlock_fn, lock)	\
	_uk_waitq_wait_until((wq), (condition), (deadline),		\
			     ukplat_monotonic_clock(),			\
			     lock_fn, unlock_fn, (lock))

/**
 * Forcefully cancel the wait ticket of `thread` and remove it from the queue.
 *
 * `thread` must be waiting on a queue and will not be awoken; continuing its
 * execution after forceful cancellation is undefined.
 */
static inline
void uk_waitq_cancel(struct uk_thread *thread)
{
	struct uk_waitq *wq = thread->wait_ticket.wq;
	unsigned long flags;

	UK_ASSERT(wq);
	_uk_waitq_lock(wq, flags);
	_uk_waitq_remove(thread);
	_uk_waitq_unlock(wq, flags);
}

/**
 * INTERNAL. Wake thread of ticket `t` from `wq`.
 */
static inline
void _uk_waitq_wake(struct uk_waitq *wq __maybe_unused,
		    struct uk_waitq_ticket *t)
{
	UK_ASSERT(t->wq == wq);
	uk_thread_wake(uk_thread_of_wait_ticket(t));
}

/**
 * Wake all threads waiting on `wq`
 */
static inline
void uk_waitq_wake_up(struct uk_waitq *wq)
{
	struct uk_waitq_ticket *t;
	unsigned long flags;

	_uk_waitq_lock(wq, flags);
	uk_list_for_each_entry(t, &wq->waiters, link)
		_uk_waitq_wake(wq, t);
	_uk_waitq_unlock(wq, flags);
}

/**
 * Wake at most one thread waiting on `wq`.
 */
static inline
void uk_waitq_wake_up_one(struct uk_waitq *wq)
{
	struct uk_waitq_ticket *t;
	unsigned long flags;

	_uk_waitq_lock(wq, flags);
	t = uk_list_first_entry_or_null(&wq->waiters,
					struct uk_waitq_ticket, link);
	if (t)
		_uk_waitq_wake(wq, t);
	_uk_waitq_unlock(wq, flags);
}

#ifdef __cplusplus
}
#endif

#endif /* __UK_SCHED_WAIT_H__ */
