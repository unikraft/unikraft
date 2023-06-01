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
/* Taken and modified from Mini-OS (include/semphore.h) */
#ifndef __UK_SEMAPHORE_H__
#define __UK_SEMAPHORE_H__

#include <uk/config.h>

#if CONFIG_LIBUKLOCK_SEMAPHORE
#include <uk/spinlock.h>
#include <uk/print.h>
#include <uk/assert.h>
#include <uk/plat/lcpu.h>
#include <uk/thread.h>
#include <uk/wait.h>
#include <uk/wait_types.h>
#include <uk/plat/time.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Semaphore that relies on a scheduler
 * uses wait queues for threads
 */
struct uk_semaphore {
	struct uk_spinlock sl;
	long count;
	struct uk_waitq wait;
};

void uk_semaphore_init(struct uk_semaphore *s, long count);

static inline void uk_semaphore_down(struct uk_semaphore *s)
{
	unsigned long irqf;

	UK_ASSERT(s);

	for (;;) {
		uk_waitq_wait_event(&s->wait, s->count > 0);
		uk_spin_lock_irqsave(&(s->sl), irqf);
		if (s->count > 0)
			break;
		uk_spin_unlock_irqrestore(&(s->sl), irqf);
	}
	--s->count;
#ifdef UK_SEMAPHORE_DEBUG
	uk_pr_debug("Decreased semaphore %p to %ld\n", s, s->count);
#endif
	uk_spin_unlock_irqrestore(&(s->sl), irqf);
}

static inline int uk_semaphore_down_try(struct uk_semaphore *s)
{
	unsigned long irqf;
	int ret = 0;

	UK_ASSERT(s);

	uk_spin_lock_irqsave(&(s->sl), irqf);
	if (s->count > 0) {
		ret = 1;
		--s->count;
#ifdef UK_SEMAPHORE_DEBUG
		uk_pr_debug("Decreased semaphore %p to %ld\n",
			    s, s->count);
#endif
	}
	uk_spin_unlock_irqrestore(&(s->sl), irqf);
	return ret;
}

/* Returns __NSEC_MAX on timeout, expired time when down was successful */
static inline __nsec uk_semaphore_down_to(struct uk_semaphore *s,
					  __nsec timeout)
{
	unsigned long irqf;
	__nsec then = ukplat_monotonic_clock();
	__nsec deadline;

	UK_ASSERT(s);

	deadline = then + timeout;

	for (;;) {
		uk_waitq_wait_event_deadline(&s->wait, s->count > 0, deadline);
		uk_spin_lock_irqsave(&(s->sl), irqf);
		if (s->count > 0 || (deadline &&
				     ukplat_monotonic_clock() >= deadline))
			break;
		uk_spin_unlock_irqrestore(&(s->sl), irqf);
	}
	if (s->count > 0) {
		s->count--;
#ifdef UK_SEMAPHORE_DEBUG
		uk_pr_debug("Decreased semaphore %p to %ld\n",
			    s, s->count);
#endif
		uk_spin_unlock_irqrestore(&(s->sl), irqf);
		return ukplat_monotonic_clock() - then;
	}

	uk_spin_unlock_irqrestore(&(s->sl), irqf);
#ifdef UK_SEMAPHORE_DEBUG
	uk_pr_debug("Timed out while waiting for semaphore %p\n", s);
#endif
	return __NSEC_MAX;
}

static inline void uk_semaphore_up(struct uk_semaphore *s)
{
	unsigned long irqf;

	UK_ASSERT(s);

	uk_spin_lock_irqsave(&(s->sl), irqf);
	++s->count;
#ifdef UK_SEMAPHORE_DEBUG
	uk_pr_debug("Increased semaphore %p to %ld\n",
		    s, s->count);
#endif
	uk_waitq_wake_up(&s->wait);
	uk_spin_unlock_irqrestore(&(s->sl), irqf);
}

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_LIBUKLOCK_SEMAPHORE */

#endif /* __UK_SEMAPHORE_H__ */
