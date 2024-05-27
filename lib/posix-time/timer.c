/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Costin Lupu <costin.lupu@cs.pub.ro>
 *
 * Copyright (c) 2019, University Politehnica of Bucharest. All rights reserved.
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
#include "uk/alloc.h"
#include "uk/arch/lcpu.h"
#include "uk/thread.h"
#include <sys/time.h>

#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>

#include <uk/essentials.h>
#include <uk/print.h>
#include <uk/sched.h>
#include <uk/timeutil.h>
#include <uk/syscall.h>
#include <uk/plat/time.h>
#include <uk/assert.h>

struct uk_timer {
	struct uk_list_head nodes;
	struct uk_alloc *alloc;
	__u64 exp_cnt;
	struct uk_thread *thread; /* timer thread, NULL if disarmed */
	clockid_t clockid;
	struct sigevent sigev;
	struct itimerspec spec;
};

struct uk_timer *timer_list_head = NULL;

static inline 
int __push_timer(struct uk_timer *timer)
{
	if(timer_list_head == NULL) {
		timer_list_head = timer;

	} else {
		uk_list_add(&timer->nodes, &timer_list_head->nodes);
	}

	return 0;
}

/* Removes timer from timer data structure and frees the memory */
static 
int __delete_timer(struct uk_timer *head, timer_t timerid)
{
	
	while (head != NULL) {
		if (head == timerid) {
			
			if (head->nodes.prev == NULL) {
				timer_list_head = NULL;
			
			} else {
				uk_list_del(&head->nodes);
			}

			uk_free(head->alloc, head);

			return 0;
		}

		head = (struct uk_timer *) head->nodes.next;
	}

	return -EINVAL;	
}

static
struct uk_timer* get_timer(timer_t timerid) {
	struct uk_timer *timer = (struct uk_timer* ) timerid;
	struct uk_timer *head = timer_list_head;

	/* Check if timerid points to valid list entry. Otherwise return NULL */
	while(head != timer) {
		if(head) {
			head = (struct uk_timer* ) head->nodes.next;

		} else {
			return NULL;
		}
	}

	return timer;
}

static
void timer_disarm(struct uk_thread *thread)
{
	uk_thread_terminate(thread);
	uk_printk(KLVL_CRIT, "timer disairmed\n");
}


/* Called when timer expires */
static 
void expire(struct uk_timer *timer) {
	uk_printk(KLVL_CRIT, "expired\n");
}


struct timer_status {
	__nsec next;
	__u64 exp_cnt;
	__u64 overrun_cnt;
};


/* Calculates the time to the time difference between current time and the next timer expiry. */
static inline
struct timer_status next_delay(struct uk_timer *timer,
				       const struct timespec *now)
{

	__snsec time_since_first_exp = uk_time_spec_nsecdiff(&timer->spec.it_value, now);

	struct timer_status status;
	status.overrun_cnt = 0;
	status.exp_cnt = 0;
	
	if (time_since_first_exp >= 0) {
		__nsec period = uk_time_spec_to_nsec(&timer->spec.it_interval);

		if (period) {
			status.exp_cnt = 1 + time_since_first_exp / period;

			if(status.exp_cnt - timer->exp_cnt > 1) {
				status.overrun_cnt = status.exp_cnt - timer->exp_cnt;
			}

			status.next = timer->exp_cnt * period - time_since_first_exp;

		} else {
			status.next = 0;
		}

	} else {
		status.next = (__nsec) -time_since_first_exp;
	}

	return status;
}

/* Calculates the deadline for the next timer expiry relative to CLOCK_MONOTONIC */
static __nsec _get_next_deadline(struct uk_timer *timer, struct timespec *now) 
{
	/* Clear & sleep if timer disarmed */
	if (!timer->spec.it_value.tv_sec && !timer->spec.it_value.tv_nsec) {
		if (timer->exp_cnt) {
			timer->exp_cnt = 0;
		}
		return 0;
	}

	struct timer_status status = next_delay(timer, now);
	timer->exp_cnt = status.exp_cnt;

	if(!status.next) {

		uk_printk(KLVL_CRIT, "n\n");
		return 0;
	}


	uk_printk(KLVL_CRIT, "next %lu\n", status.next);
	return ukplat_monotonic_clock() + status.next;
}

static __noreturn void __timer_update_thread(void *timerid)
{
	__nsec deadline;

	struct uk_timer *timer = get_timer(timerid);

	if(!timer) {
		timer_disarm(uk_thread_current());
	}

	while(true) {		
		
		struct timespec now;
		int err = uk_syscall_r_clock_gettime(timer->clockid, (uintptr_t)&now);

		if(unlikely(err)) {
			/* terminate on error */
			timer_disarm(uk_thread_current());
		}
		
		deadline = _get_next_deadline(timer, &now);
		uk_printk(KLVL_CRIT, "deadline: %lu\n", deadline);

		if (deadline) {
			uk_thread_block_until(uk_thread_current(), deadline);
			expire(timer);
		
		} else {
			/* disarm */
			timer_disarm(uk_thread_current());
		}

		uk_sched_yield();
	}
}

UK_SYSCALL_R_DEFINE(int, timer_create, clockid_t, clockid,
		    struct sigevent *__restrict, sevp, timer_t *__restrict,
		    timerid)
{

	/* Check clock id */
	if (unlikely(uk_syscall_r_clock_getres(clockid, (uintptr_t)NULL)))
		return -EINVAL;

	/* Check Signal event valid */

	switch (sevp->sigev_notify) {
	case SIGEV_NONE:
	case SIGEV_SIGNAL: {
		struct uk_timer *timer;

		struct uk_alloc *a = uk_alloc_get_default();
		timer = uk_malloc(a, sizeof(struct uk_timer *));

		if (unlikely(!timer))
			return -ENOMEM;

		timer->alloc = a;
		timer->sigev = *sevp;
		timer->clockid = clockid;
		timer->exp_cnt = 0;
		timer->thread = NULL;

		if (unlikely(__push_timer(timer))) {
			return -ENOMEM;
		}

		*timerid = (timer_t *) timer;
		return 0;
	}

	/* Thread events not implemented yet */
	default: 
		return -ENOTSUP;
	
	}
}


UK_SYSCALL_R_DEFINE(int, timer_delete, timer_t, timerid)
{
	return -ENOTSUP;
}

UK_SYSCALL_R_DEFINE(int, timer_settime, timer_t, timerid, int, flags,
		    const struct itimerspec *__restrict, new_value,
		    struct itimerspec *__restrict, old_value)
{
	struct uk_timer *timer = get_timer(timerid);
	if (!timer) {
		return -EINVAL;
	}

	const int disarm = !new_value->it_value.tv_sec &&
			   !new_value->it_value.tv_nsec;
	
	/* Disarm timer if armed */
	if (disarm && timer->thread != NULL) {
		timer_disarm(timer->thread);
		return 0;
	}

	/* Return EINVAL if timespec values out of range */
	if (new_value->it_value.tv_sec < 0 || new_value->it_value.tv_nsec < 0
	    |	new_value->it_value.tv_nsec > 999999999) {
		return -EINVAL;
	}

	if (old_value) {
		old_value->it_value = timer->spec.it_value;
		old_value->it_interval = timer->spec.it_interval;
	}

	if (!(flags & TIMER_ABSTIME)) {
		struct timespec now;

		int err = uk_syscall_r_clock_gettime(timer->clockid, (uintptr_t)&now);
		if(unlikely(err)) {
			return err;
		}

		timer->spec.it_value = uk_time_spec_sum(&new_value->it_value, &now);
		timer->spec.it_interval = new_value->it_interval;

	} else {
		timer->spec.it_value = new_value->it_value;
		timer->spec.it_interval = new_value->it_interval;
	}

	struct uk_thread *ut = uk_sched_thread_create_fn1(
	    uk_sched_current(), __timer_update_thread, timer, 0x0, 0x0, false,
	    false, "timer_update_thread", NULL, NULL);

	if (unlikely(!ut)) {
		return -ENODEV;
	}

	return 0;
}

UK_SYSCALL_R_DEFINE(int, timer_gettime, timer_t, timerid, struct itimerspec *,
		    curr_value)
{
	UK_WARN_STUBBED();
	return -ENOTSUP;
}

UK_SYSCALL_R_DEFINE(int, timer_getoverrun, timer_t, timerid)
{	
	struct uk_timer *timer = get_timer(timerid);
	if (!timer) {
		return -EINVAL;
	}

	struct timespec now;

	int err = uk_syscall_r_clock_gettime(timer->clockid, (uintptr_t)&now);
	if(unlikely(err)) {
		return err;
	}

	struct timer_status status = next_delay(timer, &now);

	return status.overrun_cnt;
}
