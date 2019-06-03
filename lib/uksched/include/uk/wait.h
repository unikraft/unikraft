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
	UK_STAILQ_INIT(wq);
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
	return UK_STAILQ_EMPTY(wq);
}

static inline
void uk_waitq_add(struct uk_waitq *wq,
		struct uk_waitq_entry *entry)
{
	if (!entry->waiting) {
		UK_STAILQ_INSERT_TAIL(wq, entry, thread_list);
		entry->waiting = 1;
	}
}

static inline
void uk_waitq_remove(struct uk_waitq *wq,
		struct uk_waitq_entry *entry)
{
	if (entry->waiting) {
		UK_STAILQ_REMOVE(wq, entry, struct uk_waitq_entry, thread_list);
		entry->waiting = 0;
	}
}

#define uk_waitq_add_waiter(wq, w) \
do { \
	unsigned long flags; \
	flags = ukplat_lcpu_save_irqf(); \
	uk_waitq_add(wq, w); \
	uk_thread_block(uk_thread_current()); \
	ukplat_lcpu_restore_irqf(flags); \
} while (0)

#define uk_waitq_remove_waiter(wq, w) \
do { \
	unsigned long flags; \
	flags = ukplat_lcpu_save_irqf(); \
	uk_waitq_remove(wq, w); \
	ukplat_lcpu_restore_irqf(flags); \
} while (0)

#define __wq_wait_event_deadline(wq, condition, deadline, deadline_condition) \
do { \
	struct uk_thread *__current; \
	unsigned long flags; \
	DEFINE_WAIT(__wait); \
	if (condition) \
		break; \
	for (;;) { \
		__current = uk_thread_current(); \
		/* protect the list */ \
		flags = ukplat_lcpu_save_irqf(); \
		uk_waitq_add(wq, &__wait); \
		__current->wakeup_time = deadline; \
		clear_runnable(__current); \
		uk_sched_thread_blocked(__current->sched, __current); \
		ukplat_lcpu_restore_irqf(flags); \
		if ((condition) || (deadline_condition)) \
			break; \
		uk_sched_yield(); \
	} \
	flags = ukplat_lcpu_save_irqf(); \
	/* need to wake up */ \
	uk_thread_wake(__current); \
	uk_waitq_remove(wq, &__wait); \
	ukplat_lcpu_restore_irqf(flags); \
} while (0)

#define uk_waitq_wait_event(wq, condition) \
	__wq_wait_event_deadline(wq, (condition), 0, 0)

#define uk_waitq_wait_event_deadline(wq, condition, deadline) \
	__wq_wait_event_deadline(wq, (condition), \
		(deadline), \
		(deadline) && ukplat_monotonic_clock() >= (deadline))

static inline
void uk_waitq_wake_up(struct uk_waitq *wq)
{
	unsigned long flags;
	struct uk_waitq_entry *curr, *tmp;

	flags = ukplat_lcpu_save_irqf();
	UK_STAILQ_FOREACH_SAFE(curr, wq, thread_list, tmp)
		uk_thread_wake(curr->thread);
	ukplat_lcpu_restore_irqf(flags);
}

#ifdef __cplusplus
}
#endif

#endif /* __UK_SCHED_WAIT_H__ */
