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
/*
 * Thread definitions
 * Ported from Mini-OS
 */
#ifndef __UK_THREAD_H__
#define __UK_THREAD_H__

#include <stdint.h>
#ifdef HAVE_LIBC
#include <sys/reent.h>
#endif
#include <uk/arch/thread.h>
#include <uk/arch/time.h>
#include <uk/plat/thread.h>
#include <uk/list.h>
#include <uk/essentials.h>

struct uk_sched;

struct uk_thread {
	char *name;
	char *stack;
	struct ukplat_thread_ctx plat_ctx;
	UK_TAILQ_ENTRY(struct uk_thread) thread_list;
	uint32_t flags;
	__snsec wakeup_time;
	struct uk_sched *sched;
#ifdef HAVE_LIBC
	struct _reent reent;
#endif
};

UK_TAILQ_HEAD(uk_thread_list, struct uk_thread);

#define uk_thread_create(name, function, data) \
	uk_sched_thread_create(uk_sched_get_default(), name, function, data)
#define uk_thread_destroy(thread) \
	uk_sched_thread_destroy(thread->sched, thread)
#define uk_thread_start(thread) \
	uk_sched_thread_start(thread->sched, thread)
#define uk_thread_stop(thread) \
	uk_sched_thread_stop(thread->sched, thread)

static inline
struct uk_thread *uk_thread_current(void)
{
	struct ukplat_thread_ctx *ctx = ukplat_thread_ctx_current();

	return __containerof(ctx, struct uk_thread, plat_ctx);
}

static inline
void uk_thread_switch(struct uk_thread *prev, struct uk_thread *next)
{
	ukplat_thread_ctx_switch(&prev->plat_ctx, &next->plat_ctx);
}

#define RUNNABLE_FLAG   0x00000001

#define is_runnable(_thread)    ((_thread)->flags &   RUNNABLE_FLAG)
#define set_runnable(_thread)   ((_thread)->flags |=  RUNNABLE_FLAG)
#define clear_runnable(_thread) ((_thread)->flags &= ~RUNNABLE_FLAG)

void uk_thread_block_millis(struct uk_thread *thread, uint32_t millis);
void uk_thread_block(struct uk_thread *thread);
void uk_thread_wake(struct uk_thread *thread);

#endif /* __UK_THREAD_H__ */
