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
#include "uk/bitops.h"
#include <sys/time.h>

#include <errno.h>
#include <time.h>
#include <signal.h>
#include <stdio.h>

#include <uk/essentials.h>
#include <uk/print.h>
#include <uk/sched.h>
#include <uk/timeutil.h>
#include <uk/syscall.h>
#include <uk/plat/time.h>
#include <uk/assert.h>

#define SIGEV_NONE 0
#define SIGEV_SIGNAL 1
#define SIGEV_THREAD 2

struct timer_alloc {
	clockid_t clockid;
	struct uk_alloc *alloc;
	struct sigevent *sigev;
	struct itimerspec *spec;
};

/* Linked list */

struct timer_list {
	struct uk_alloc *alloc;
	struct timer_list *next;
	struct timer_alloc *t;
};

static struct timer_list *timer_list_head = NULL;

static int __alloc_list_elem(struct timer_alloc *t,
			     struct timer_list **new_elem)
{
	struct uk_alloc *a = uk_alloc_get_default();
	*new_elem = uk_malloc(a, sizeof(struct timer_list *));

	if (unlikely(!new_elem))
		return -ENOMEM;

	(*new_elem)->alloc = a;
	(*new_elem)->next = NULL;
	(*new_elem)->t = t;

	return 0;
}

static int __push_timer(struct timer_list *head, struct timer_alloc *t)
{
	if (head == NULL) {
		if (__alloc_list_elem(t, &head))
			return -ENOMEM;

	} else {
		if (head->next == NULL) {
			struct timer_list *new_elem;

			if (__alloc_list_elem(t, &new_elem))
				return -ENOMEM;
			
			uk_printk(KLVL_CRIT, "push timer\n");

			head->next = new_elem;
		} else {
			return __push_timer(head->next, t);
		}
	}

	return 0;
}

/* Removes timer from timer data structure and frees t_alloc */
static int __delete_timer(struct timer_list *elem, struct timer_list *parent,
			  timer_t timerid)
{
	if (elem == NULL) {
		return -EINVAL;

	} else {
		if (elem->t == timerid) {
			if (parent == NULL) {
				timer_list_head = elem->next;
			} else {
				parent->next = elem->next;
			}

			uk_free(elem->alloc, elem);
			uk_free(elem->t->alloc, elem->t);
			return 0;

		} else {
			return __delete_timer(elem->next, elem, timerid);
		}
	}

	return 0;
}

static __noreturn void __timer_updatefn(void *timerid)
{
	struct timer_alloc *t_alloc = (struct timer_alloc *)timerid;
	uk_printk(KLVL_CRIT, "timer updated\n");
}

static void __timer_thread_destroyedfn()
{
	uk_printk(KLVL_CRIT, "timer destroyed\n");
}

UK_SYSCALL_R_DEFINE(int, timer_create, clockid_t, clockid,
		    struct sigevent *__restrict, sevp, timer_t *__restrict,
		    timerid)
{

	/* Check Signal event valid */

	switch (sevp->sigev_notify) {
	case SIGEV_NONE:
	case SIGEV_SIGNAL: {
		struct timer_alloc *t_alloc;

		struct uk_alloc *a = uk_alloc_get_default();
		t_alloc = uk_malloc(a, sizeof(struct timer_alloc *));

		if (unlikely(!t_alloc))
			return -ENOMEM;

		t_alloc->alloc = a;
		t_alloc->sigev = sevp;
		t_alloc->clockid = clockid;
		t_alloc->spec = NULL;

		if (unlikely(__push_timer(timer_list_head, t_alloc))) {
			return -ENOMEM;
		}


		*timerid = t_alloc;
		return 0;
	}

	/* Thread events not implemented yet */
	default: {
		return -ENOTSUP;
	}
	}
}

UK_SYSCALL_R_DEFINE(int, timer_delete, timer_t, timerid)
{
	UK_WARN_STUBBED();
	return -ENOTSUP;
}

UK_SYSCALL_R_DEFINE(int, timer_settime, timer_t, timerid, int, flags,
		    const struct itimerspec *__restrict, new_value,
		    struct itimerspec *__restrict, old_value)
{
	struct timer_alloc *t_alloc = (struct timer_alloc *)timerid;
	uk_printk(KLVL_CRIT, "set time of timer\n");

	if (!new_value) {
		// TODO: disarm timer
		uk_printk(KLVL_CRIT, "disarm timer\n");
		return 0;
	}

	if (new_value->it_value.tv_sec < 0 || new_value->it_value.tv_nsec < 0
	    || new_value->it_value.tv_nsec > 999999999) {
		return -EINVAL;
	}

	if (old_value) {
		old_value->it_value = t_alloc->spec->it_value;
		old_value->it_interval = t_alloc->spec->it_interval;
	}

	uk_printk(KLVL_CRIT, "start thread\n");


	struct uk_thread *ut = uk_sched_thread_create_fn1(
	    uk_sched_current(), __timer_updatefn, &t_alloc, 0x0, 0x0, false,
	    false, "timer_update_thread", NULL, __timer_thread_destroyedfn);

	if (unlikely(!ut)) {
		__delete_timer(timer_list_head, NULL, t_alloc);
		return -ENODEV;
	}

	uk_sched_dumpk_threads(KLVL_CRIT, uk_sched_current());

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
	UK_WARN_STUBBED();
	return -ENOTSUP;
}
