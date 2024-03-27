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

#ifdef __cplusplus
extern "C" {
#endif

extern struct uk_sched *uk_sched_head;

int uk_sched_register(struct uk_sched *s);

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

	prev = ukplat_per_lcpu_current(__uk_sched_thread_current);

	UK_ASSERT(prev);

	ukplat_per_lcpu_current(__uk_sched_thread_current) = next;

	prev->tlsp = ukplat_tlsp_get();
	if (prev->ectx)
		ukarch_ectx_store(prev->ectx);

	/* Load next TLS and extended registers before context switch.
	 * This avoids requiring special initialization code for newly
	 * created threads to do the loading.
	 */
	ukplat_tlsp_set(next->tlsp);
	if (next->ectx)
		ukarch_ectx_load(next->ectx);

	ukplat_lcpu_set_auxsp(next->auxsp);

	ukarch_ctx_switch(&prev->ctx, &next->ctx);
}

#ifdef __cplusplus
}
#endif

#endif /* __UK_SCHED_IMPL_H__ */
