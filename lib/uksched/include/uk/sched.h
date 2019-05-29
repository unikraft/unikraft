/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Costin Lupu <costin.lupu@cs.pub.ro>
 *
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#ifndef __UK_SCHED_H__
#define __UK_SCHED_H__

#include <uk/alloc.h>
#include <uk/thread.h>
#include <uk/assert.h>
#include <uk/arch/types.h>
#include <uk/essentials.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

struct uk_sched;

struct uk_sched *uk_sched_default_init(struct uk_alloc *a);

extern char _tls_start[], _etdata[], _tls_end[];
#define have_tls_area() (_tls_end - _tls_start)

extern struct uk_sched *uk_sched_head;
int uk_sched_register(struct uk_sched *s);
struct uk_sched *uk_sched_get_default(void);
int uk_sched_set_default(struct uk_sched *s);


typedef void  (*uk_sched_yield_func_t)
		(struct uk_sched *s);

typedef int   (*uk_sched_thread_add_func_t)
		(struct uk_sched *s, struct uk_thread *t,
			const uk_thread_attr_t *attr);
typedef void  (*uk_sched_thread_remove_func_t)
		(struct uk_sched *s, struct uk_thread *t);
typedef void  (*uk_sched_thread_blocked_func_t)
		(struct uk_sched *s, struct uk_thread *t);
typedef void  (*uk_sched_thread_woken_func_t)
		(struct uk_sched *s, struct uk_thread *t);

typedef int   (*uk_sched_thread_set_prio_func_t)
		(struct uk_sched *s, struct uk_thread *t, prio_t prio);
typedef int   (*uk_sched_thread_get_prio_func_t)
		(struct uk_sched *s, const struct uk_thread *t, prio_t *prio);
typedef int   (*uk_sched_thread_set_tslice_func_t)
		(struct uk_sched *s, struct uk_thread *t, int tslice);
typedef int   (*uk_sched_thread_get_tslice_func_t)
		(struct uk_sched *s, const struct uk_thread *t, int *tslice);

struct uk_sched {
	uk_sched_yield_func_t yield;

	uk_sched_thread_add_func_t      thread_add;
	uk_sched_thread_remove_func_t   thread_remove;
	uk_sched_thread_blocked_func_t  thread_blocked;
	uk_sched_thread_woken_func_t    thread_woken;

	uk_sched_thread_set_prio_func_t   thread_set_prio;
	uk_sched_thread_get_prio_func_t   thread_get_prio;
	uk_sched_thread_set_tslice_func_t thread_set_tslice;
	uk_sched_thread_get_tslice_func_t thread_get_tslice;

	/* internal */
	bool threads_started;
	struct uk_thread idle;
	struct uk_thread_list exited_threads;
	struct ukplat_ctx_callbacks plat_ctx_cbs;
	struct uk_alloc *allocator;
	struct uk_sched *next;
	void *prv;
};

/* wrapper functions over scheduler callbacks */
static inline void uk_sched_yield(void)
{
	struct uk_sched *s;
	struct uk_thread *current = uk_thread_current();

	UK_ASSERT(current);

	s = current->sched;
	UK_ASSERT(s);
	s->yield(s);
}

static inline int uk_sched_thread_add(struct uk_sched *s,
		struct uk_thread *t, const uk_thread_attr_t *attr)
{
	int rc;

	UK_ASSERT(s);
	UK_ASSERT(t);
	if (attr)
		t->detached = attr->detached;
	rc = s->thread_add(s, t, attr);
	if (rc == 0)
		t->sched = s;
	return rc;
}

static inline int uk_sched_thread_remove(struct uk_sched *s,
		struct uk_thread *t)
{
	UK_ASSERT(s);
	UK_ASSERT(t);
	UK_ASSERT(t->sched == s);
	s->thread_remove(s, t);
	return 0;
}

static inline void uk_sched_thread_blocked(struct uk_sched *s,
		struct uk_thread *t)
{
	UK_ASSERT(s);
	s->thread_blocked(s, t);
}

static inline void uk_sched_thread_woken(struct uk_sched *s,
		struct uk_thread *t)
{
	UK_ASSERT(s);
	s->thread_woken(s, t);
}

static inline int uk_sched_thread_set_prio(struct uk_sched *s,
		struct uk_thread *t, prio_t prio)
{
	UK_ASSERT(s);

	if (!s->thread_set_prio)
		return -EINVAL;

	return s->thread_set_prio(s, t, prio);
}

static inline int uk_sched_thread_get_prio(struct uk_sched *s,
		const struct uk_thread *t, prio_t *prio)
{
	UK_ASSERT(s);

	if (!s->thread_get_prio)
		return -EINVAL;

	return s->thread_get_prio(s, t, prio);
}

static inline int uk_sched_thread_set_timeslice(struct uk_sched *s,
		struct uk_thread *t, int tslice)
{
	UK_ASSERT(s);

	if (!s->thread_set_tslice)
		return -EINVAL;

	return s->thread_set_tslice(s, t, tslice);
}

static inline int uk_sched_thread_get_timeslice(struct uk_sched *s,
		const struct uk_thread *t, int *tslice)
{
	UK_ASSERT(s);

	if (!s->thread_get_tslice)
		return -EINVAL;

	return s->thread_get_tslice(s, t, tslice);
}

/*
 * Internal scheduler functions
 */

struct uk_sched *uk_sched_create(struct uk_alloc *a, size_t prv_size);

void uk_sched_idle_init(struct uk_sched *sched,
		void *stack, void (*function)(void *));

static inline struct uk_thread *uk_sched_get_idle(struct uk_sched *s)
{
	UK_ASSERT(s);
	return &s->idle;
}

#define uk_sched_init(s, yield_func, \
		thread_add_func, thread_remove_func, \
		thread_blocked_func, thread_woken_func, \
		thread_set_prio_func, thread_get_prio_func, \
		thread_set_tslice_func, thread_get_tslice_func) \
	do { \
		(s)->yield           = yield_func; \
		(s)->thread_add      = thread_add_func; \
		(s)->thread_remove   = thread_remove_func; \
		(s)->thread_blocked  = thread_blocked_func; \
		(s)->thread_woken    = thread_woken_func; \
		(s)->thread_set_prio    = thread_set_prio_func; \
		(s)->thread_get_prio    = thread_get_prio_func; \
		(s)->thread_set_tslice  = thread_set_tslice_func; \
		(s)->thread_get_tslice  = thread_get_tslice_func; \
		uk_sched_register((s)); \
	} while (0)

/*
 * Public scheduler functions
 */

void uk_sched_start(struct uk_sched *sched) __noreturn;

static inline bool uk_sched_started(struct uk_sched *sched)
{
	return sched->threads_started;
}


/*
 * Internal thread scheduling functions
 */

struct uk_thread *uk_sched_thread_create(struct uk_sched *sched,
		const char *name, const uk_thread_attr_t *attr,
		void (*function)(void *), void *arg);
void uk_sched_thread_destroy(struct uk_sched *sched,
		struct uk_thread *thread);
void uk_sched_thread_kill(struct uk_sched *sched,
		struct uk_thread *thread);

static inline
void uk_sched_thread_switch(struct uk_sched *sched,
		struct uk_thread *prev, struct uk_thread *next)
{
	ukplat_thread_ctx_switch(&sched->plat_ctx_cbs, prev->ctx, next->ctx);
}

/*
 * Public thread scheduling functions
 */

void uk_sched_thread_sleep(__nsec nsec);
void uk_sched_thread_exit(void) __noreturn;

#ifdef __cplusplus
}
#endif

#endif /* __UK_SCHED_H__ */
