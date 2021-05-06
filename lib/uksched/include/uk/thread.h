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
#include <stdbool.h>
#ifdef CONFIG_LIBNEWLIBC
#include <sys/reent.h>
#endif
#include <uk/alloc.h>
#include <uk/arch/lcpu.h>
#include <uk/arch/time.h>
#include <uk/plat/thread.h>
#if CONFIG_LIBUKSIGNAL
#include <uk/uk_signal.h>
#endif
#include <uk/thread_attr.h>
#include <uk/wait_types.h>
#include <uk/list.h>
#include <uk/prio.h>
#include <uk/essentials.h>

#ifdef __cplusplus
extern "C" {
#endif

struct uk_sched;

struct uk_thread {
	const char *name;
	void *stack;
	void *tls;
	void *ctx;
	UK_TAILQ_ENTRY(struct uk_thread) thread_list;
	uint32_t flags;
	__snsec wakeup_time;
	bool detached;
	struct uk_waitq waiting_threads;
	struct uk_sched *sched;
	void (*entry)(void *);
	void *arg;
	void *prv;
#ifdef CONFIG_LIBNEWLIBC
	struct _reent reent;
#endif
#if CONFIG_LIBUKSIGNAL
	/* TODO: Move to `TLS` and define within uksignal */
	struct uk_thread_sig signals_container;
#endif
};

UK_TAILQ_HEAD(uk_thread_list, struct uk_thread);

#define uk_thread_create_attr(name, attr, function, data) \
	uk_sched_thread_create(uk_sched_get_default(), \
			name, attr, function, data)
#define uk_thread_create(name, function, data) \
	uk_thread_create_attr(name, NULL, function, data)
#define uk_thread_kill(thread) \
	uk_sched_thread_kill(thread->sched, thread)
void uk_thread_exit(struct uk_thread *thread);

int uk_thread_wait(struct uk_thread *thread);
int uk_thread_detach(struct uk_thread *thread);

int uk_thread_set_prio(struct uk_thread *thread, prio_t prio);
int uk_thread_get_prio(const struct uk_thread *thread, prio_t *prio);

int uk_thread_set_timeslice(struct uk_thread *thread, int timeslice);
int uk_thread_get_timeslice(const struct uk_thread *thread, int *timeslice);

static inline
struct uk_thread *uk_thread_current(void)
{
	struct uk_thread **current;
	unsigned long sp = ukarch_read_sp();

	current = (struct uk_thread **) (sp & STACK_MASK_TOP);

	return *current;
}

#define RUNNABLE_FLAG   0x00000001
#define EXITED_FLAG     0x00000002
#define QUEUEABLE_FLAG  0x00000004

#define is_runnable(_thread)    ((_thread)->flags &   RUNNABLE_FLAG)
#define set_runnable(_thread)   ((_thread)->flags |=  RUNNABLE_FLAG)
#define clear_runnable(_thread) ((_thread)->flags &= ~RUNNABLE_FLAG)

#define is_exited(_thread)      ((_thread)->flags &   EXITED_FLAG)
#define set_exited(_thread)     ((_thread)->flags |=  EXITED_FLAG)

#define is_queueable(_thread)    ((_thread)->flags &   QUEUEABLE_FLAG)
#define set_queueable(_thread)   ((_thread)->flags |=  QUEUEABLE_FLAG)
#define clear_queueable(_thread) ((_thread)->flags &= ~QUEUEABLE_FLAG)

int uk_thread_init(struct uk_thread *thread,
		struct ukplat_ctx_callbacks *cbs, struct uk_alloc *allocator,
		const char *name, void *stack, void *tls,
		void (*function)(void *), void *arg);
void uk_thread_fini(struct uk_thread *thread,
		struct uk_alloc *allocator);
void uk_thread_block_timeout(struct uk_thread *thread, __nsec nsec);
void uk_thread_block(struct uk_thread *thread);
void uk_thread_wake(struct uk_thread *thread);

/**
 * Registers a thread initialization function that is
 * called during thread creation
 *
 * @param fn
 *   initialization function to be called (uk_thread_init_func_t)
 * @param prio
 *   Priority level (0 (earliest) to 9 (latest))
 *   Use the UK_PRIO_AFTER() helper macro for computing priority dependencies.
 *   Note: Any other value for level will be ignored
 */
typedef int  (*uk_thread_init_func_t)(struct uk_thread *thread);
typedef void (*uk_thread_fini_func_t)(struct uk_thread *thread);
struct uk_thread_inittab_entry {
	uk_thread_init_func_t init;
	uk_thread_fini_func_t fini;
};

#define __UK_THREAD_INITTAB_ENTRY(init_fn, fini_fn, prio)		\
	static const struct uk_thread_inittab_entry			\
	__used __section(".uk_thread_inittab" # prio) __align(8)	\
		__uk_thread_inittab ## prio ## _ ## init_fn ## _ ## fini_fn = {\
		.init = (init_fn),					\
		.fini = (fini_fn)					\
	}

#define _UK_THREAD_INITTAB_ENTRY(init_fn, fini_fn, prio)		\
	__UK_THREAD_INITTAB_ENTRY(init_fn, fini_fn, prio)

#define UK_THREAD_INIT_PRIO(init_fn, fini_fn, prio)			\
	_UK_THREAD_INITTAB_ENTRY(init_fn, fini_fn, prio)

#define UK_THREAD_INIT(init_fn, fini_fn)				\
	_UK_THREAD_INITTAB_ENTRY(init_fn, fini_fn, UK_PRIO_LATEST)

#ifdef __cplusplus
}
#endif

#endif /* __UK_THREAD_H__ */
