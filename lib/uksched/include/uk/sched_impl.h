/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Costin Lupu <costin.lupu@cs.pub.ro>
 *          Simon Kuenzer <simon@unikraft.io>
 *
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
 * Copyright (c) 2022, NEC Laboratories GmbH, NEC Corrporation,
 *                     All rights reserved.
 * Copyright (c) 2022, Unikraft GmbH. All rights reserved.
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

/*
 * NOTE: This header should only be used by actual scheduler implementations.
 *       These functions are not part of the public uksched API.
 */

#ifndef __UK_SCHED_IMPL_H__
#define __UK_SCHED_IMPL_H__

#include <uk/sched.h>

#if CONFIG_LIBUKSCHED_STATS
#include <string.h>
#endif /* CONFIG_LIBUKSCHED_STATS */

#ifdef __cplusplus
extern "C" {
#endif

extern struct uk_sched *uk_sched_head;

int uk_sched_register(struct uk_sched *s);

#if CONFIG_LIBUKSCHED_STATS
#define uk_sched_stats_reset(_s)			\
	memset(&(_s)->stats, 0, sizeof((_s)->stats))
#else
#define uk_sched_stats_reset(_s) do {} while (0)
#endif /* CONFIG_LIBUKSCHED_STATS */

#define uk_sched_init(s, start_func, yield_func, \
		thread_add_func, thread_remove_func, \
		thread_blocked_func, thread_woken_func, \
		thread_woken_isr_func, idle_thread_func, \
		sched_a, sched_a_stack, sched_a_auxstack, sched_a_uktls) \
	do { \
		(s)->sched_start      = start_func; \
		(s)->yield            = yield_func; \
		(s)->thread_add       = thread_add_func; \
		(s)->thread_remove    = thread_remove_func; \
		(s)->thread_blocked   = thread_blocked_func; \
		(s)->thread_woken     = thread_woken_func; \
		(s)->thread_woken_isr = thread_woken_isr_func; \
		(s)->idle_thread      = idle_thread_func; \
		uk_sched_register((s)); \
		\
		(s)->a = (sched_a); \
		(s)->a_stack = (sched_a_stack); \
		(s)->a_auxstack = (sched_a_auxstack); \
		(s)->a_uktls = (sched_a_uktls); \
		UK_TAILQ_INIT(&(s)->thread_list); \
		UK_TAILQ_INIT(&(s)->exited_threads); \
		uk_sched_stats_reset(s); \
	} while (0)

/**
 * Releases self-exited threads (garbage collection)
 *
 * @return
 *   - (0): No work was done
 *   - (>0): Number of threads that were cleaned up
 */
unsigned int uk_sched_thread_gc(struct uk_sched *sched);

static inline
void uk_sched_thread_switch(struct uk_thread *next)
{
	struct uk_thread *prev;

	prev = uk_per_lcpu_current(__uk_sched_thread_current);

	UK_ASSERT(prev);

	uk_per_lcpu_current(__uk_sched_thread_current) = next;

	prev->tlsp = uk_lcpu_tlsp_get();

	/* Load next TLS and extended registers before context switch.
	 * This avoids requiring special initialization code for newly
	 * created threads to do the loading.
	 */
	uk_lcpu_tlsp_set(next->tlsp);

	uk_lcpu_set_auxsp(next->auxsp);

	/**
	 * NOTE: There is also prev->ectx/next->ectx! Since we only have
	 * cooperative scheduling at the moment, saving/restoring ECTX
	 * when thread switching is not necessary, since the ABI treats these
	 * registers as scratch registers and thread switching essentially
	 * happens through actual explicit function calls.
	 * On the other hand, this is obviously not the case with preemptive
	 * scheduling since thread switching can happen out of the blue,
	 * typically on a timer IRQ, so then ECTX saving/restoring is definetely
	 * necessary.
	 */
	ukarch_ctx_switch(&prev->ctx, &next->ctx);
}

/* These should be used to instrument a scheduler implementation for
 * statistics. For more info on metrics see include/uk/sched/store.h
 */
#if CONFIG_LIBUKSCHED_STATS
static inline void uk_sched_stats_idle_count_incr(struct uk_sched *s)
{
	s->stats.idle_count++;
}

static inline void uk_sched_stats_next_count_incr(struct uk_sched *s)
{
	s->stats.next_count++;
}

static inline void uk_sched_stats_sched_count_incr(struct uk_sched *s)
{
	s->stats.sched_count++;
}

static inline void uk_sched_stats_yield_count_incr(struct uk_sched *s)
{
	s->stats.yield_count++;
}
#else /* !CONFIG_LIBUKSCHED_STATS */
#define uk_sched_stats_idle_count_incr(_s)  do {} while (0)
#define uk_sched_stats_next_count_incr(_s)  do {} while (0)
#define uk_sched_stats_sched_count_incr(_s) do {} while (0)
#define uk_sched_stats_yield_count_incr(_s) do {} while (0)
#endif /* !CONFIG_LIBUKSCHED_STATS */

#ifdef __cplusplus
}
#endif

#endif /* __UK_SCHED_IMPL_H__ */
