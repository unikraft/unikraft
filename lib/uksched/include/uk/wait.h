/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/* Ported from Mini-OS */

#ifndef __UK_SCHED_WAIT_H__
#define __UK_SCHED_WAIT_H__

#include <uk/essentials.h>
#include <uk/plat/lcpu.h>
#include <uk/plat/time.h>
#include <uk/sched.h>
#include <uk/wait_types.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline
void uk_waitq_init(struct uk_waitq *wq)
{
	ukarch_spin_init(&(wq->sl));
	UK_STAILQ_INIT(&(wq->wait_list));
}

static inline
void uk_waitq_entry_init(struct uk_waitq_entry *entry,
		struct uk_thread *thread)
{
	entry->thread = thread;
	entry->waiting = 0;
}

static inline
int uk_waitq_empty(struct uk_waitq *wq)
{
	return UK_STAILQ_EMPTY(&(wq->wait_list));
}

static inline
void uk_waitq_add(struct uk_waitq *wq,
		struct uk_waitq_entry *entry)
{
	if (!entry->waiting) {
		UK_STAILQ_INSERT_TAIL(&(wq->wait_list), entry, thread_list);
		entry->waiting = 1;
	}
}

static inline
void uk_waitq_remove(struct uk_waitq *wq,
		struct uk_waitq_entry *entry)
{
	if (entry->waiting) {
		UK_STAILQ_REMOVE(&(wq->wait_list), entry,
				 struct uk_waitq_entry, thread_list);
		entry->waiting = 0;
	}
}

#define uk_waitq_add_waiter(wq, w) \
do { \
	unsigned long flags; \
	ukplat_spin_lock_irqsave(&((wq)->sl), flags); \
	uk_waitq_add(wq, w); \
	ukplat_spin_unlock_irqrestore(&((wq)->sl), flags); \
	uk_thread_block(uk_thread_current()); \
} while (0)

#define uk_waitq_remove_waiter(wq, w) \
do { \
	unsigned long flags; \
	ukplat_spin_lock_irqsave(&((wq)->sl), flags); \
	uk_waitq_remove(wq, w); \
	ukplat_spin_unlock_irqrestore(&((wq)->sl), flags); \
} while (0)

#define __wq_wait_event_deadline(wq, condition, deadline, deadline_condition, \
				 lock_fn, unlock_fn, lock) \
({ \
	struct uk_thread *__current; \
	unsigned long flags; \
	int timedout = 0; \
	DEFINE_WAIT(__wait); \
	if (!(condition)) { \
		__current = uk_thread_current(); \
		for (;;) { \
			/* protect the list */ \
			ukplat_spin_lock_irqsave(&((wq)->sl), flags); \
			if (condition) { \
				ukplat_spin_unlock_irqrestore(&((wq)->sl), \
							      flags); \
				break; \
			} \
			uk_waitq_add(wq, &__wait); \
			__current->wakeup_time = deadline; \
			uk_thread_set_blocked(__current); \
			uk_sched_thread_blocked(__current); \
			ukplat_spin_unlock_irqrestore(&((wq)->sl), flags); \
			if (lock) \
				unlock_fn(lock); \
			uk_sched_yield(); \
			if (lock) \
				lock_fn(lock); \
			if (condition) \
				break; \
			if (deadline_condition) { \
				timedout = 1; \
				break; \
			} \
		} \
		ukplat_spin_lock_irqsave(&((wq)->sl), flags); \
		/* need to wake up */ \
		uk_thread_wake(__current); \
		uk_waitq_remove(wq, &__wait); \
		ukplat_spin_unlock_irqrestore(&((wq)->sl), flags); \
	} \
	timedout; \
})

#define uk_waitq_wait_deadline(wq, lock_fn, unlock_fn, lock, deadline, \
			       deadline_condition) \
({ \
	struct uk_thread *__current; \
	unsigned long flags; \
	int timedout = 0; \
	DEFINE_WAIT(__wait); \
	__current = uk_thread_current(); \
	ukplat_spin_lock_irqsave(&((wq)->sl), flags); \
	uk_waitq_add(wq, &__wait); \
	__current->wakeup_time = deadline; \
	uk_thread_set_blocked(__current); \
	uk_sched_thread_blocked(__current); \
	ukplat_spin_unlock_irqrestore(&((wq)->sl), flags); \
	if (lock) \
		unlock_fn(lock); \
	uk_sched_yield(); \
	if (lock) \
		lock_fn(lock); \
	if (deadline_condition) \
		timedout = 1; \
	ukplat_spin_lock_irqsave(&((wq)->sl), flags); \
	/* need to wake up */ \
	uk_thread_wake(__current); \
	uk_waitq_remove(wq, &__wait); \
	ukplat_spin_unlock_irqrestore(&((wq)->sl), flags); \
	timedout; \
})

#define uk_waitq_wait_locked(wq, lock_fn, unlock_fn, lock) \
	uk_waitq_wait_deadline(wq, lock_fn, unlock_fn, lock, 0, 0)

static inline void __lock_dummy(void *lock __unused) {}

#define uk_waitq_wait_event(wq, condition) \
	__wq_wait_event_deadline(wq, (condition), 0, 0, \
				 __lock_dummy, __lock_dummy, NULL)

#define uk_waitq_wait_event_locked(wq, condition, lock_fn, unlock_fn, lock) \
	__wq_wait_event_deadline(wq, (condition), 0, 0, \
				 lock_fn, unlock_fn, lock)

#define uk_waitq_wait_event_deadline(wq, condition, deadline) \
	__wq_wait_event_deadline(wq, (condition), \
		(deadline), \
		(deadline) && ukplat_monotonic_clock() >= (deadline), \
		__lock_dummy, __lock_dummy, NULL)

#define uk_waitq_wait_event_deadline_locked(wq, condition, deadline, \
					    lock_fn, unlock_fn, lock) \
	__wq_wait_event_deadline(wq, (condition), \
		(deadline), \
		(deadline) && ukplat_monotonic_clock() >= (deadline), \
		lock_fn, unlock_fn, lock)

static inline
void uk_waitq_wake_up(struct uk_waitq *wq)
{
	struct uk_waitq_entry *curr, *tmp;
	unsigned long flags;

	ukplat_spin_lock_irqsave(&(wq->sl), flags);
	UK_STAILQ_FOREACH_SAFE(curr, &(wq->wait_list), thread_list, tmp)
		uk_thread_wake(curr->thread);
	ukplat_spin_unlock_irqrestore(&(wq->sl), flags);
}

static inline
void uk_waitq_wake_up_one(struct uk_waitq *wq)
{
	struct uk_waitq_entry *head;
	unsigned long flags;

	ukplat_spin_lock_irqsave(&(wq->sl), flags);
	head = UK_STAILQ_FIRST(&wq->wait_list);
	if (head)
		uk_thread_wake(head->thread);
	ukplat_spin_unlock_irqrestore(&(wq->sl), flags);
}

#ifdef __cplusplus
}
#endif

#endif /* __UK_SCHED_WAIT_H__ */
