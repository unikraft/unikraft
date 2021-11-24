/* SPDX-License-Identifier: MIT */
/*
 * Authors: Rolf Neugebauer
 *          Grzegorz Milos
 *          Costin Lupu <costin.lupu@cs.pub.ro>
 *
 * Copyright (c) 2003-2005, Intel Research Cambridge
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
/*
 * Thread definitions
 * Ported from Mini-OS
 */
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <uk/plat/config.h>
#include <uk/plat/time.h>
#include <uk/thread.h>
#include <uk/sched.h>
#include <uk/wait.h>
#include <uk/print.h>
#include <uk/assert.h>
#include <uk/arch/tls.h>

/* Pushes the specified value onto the stack of the specified thread */
static void stack_push(unsigned long *sp, unsigned long value)
{
	*sp -= sizeof(unsigned long);
	*((unsigned long *) *sp) = value;
}

static void init_sp(unsigned long *sp, char *stack,
		void (*function)(void *), void *data)
{
	*sp = (unsigned long) stack + STACK_SIZE;

#if defined(__X86_64__)
	/* Must ensure that (%rsp + 8) is 16-byte aligned
	 * at the start of thread_starter.
	 */
	stack_push(sp, 0);
#endif

	stack_push(sp, (unsigned long) function);
	stack_push(sp, (unsigned long) data);
}

#ifdef CONFIG_LIBNEWLIBC
static void reent_init(struct _reent *reent)
{
	_REENT_INIT_PTR(reent);
#if 0
	/* TODO initialize basic signal handling */
	_init_signal_r(myreent);
#endif
}

struct _reent *__getreent(void)
{
	struct _reent *_reent;
	struct uk_sched *s = uk_sched_get_default();

	if (!s || !uk_sched_started(s))
		_reent = _impure_ptr;
	else
		_reent = &uk_thread_current()->reent;

	return _reent;
}
#endif /* CONFIG_LIBNEWLIBC */

extern const struct uk_thread_inittab_entry _uk_thread_inittab_start[];
extern const struct uk_thread_inittab_entry _uk_thread_inittab_end;

#define uk_thread_inittab_foreach(itr)					\
	for ((itr) = DECONST(struct uk_thread_inittab_entry*,		\
			     _uk_thread_inittab_start);			\
	     (itr) < &(_uk_thread_inittab_end);				\
	     (itr)++)

#define uk_thread_inittab_foreach_reverse2(itr, start)			\
	for ((itr) = (start);						\
	     (itr) >= _uk_thread_inittab_start;				\
	     (itr)--)

#define uk_thread_inittab_foreach_reverse(itr)				\
	uk_thread_inittab_foreach_reverse2((itr),			\
			(DECONST(struct uk_thread_inittab_entry*,	\
				 (&_uk_thread_inittab_end))) - 1)

int uk_thread_init(struct uk_thread *thread,
		struct ukplat_ctx_callbacks *cbs, struct uk_alloc *allocator,
		const char *name, void *stack, void *tls,
		void (*function)(void *), void *arg)
{
	unsigned long sp;
	void *ctx;
	int ret = 0;
	struct uk_thread_inittab_entry *itr;

	UK_ASSERT(thread != NULL);
	UK_ASSERT(stack != NULL);
	UK_ASSERT(!have_tls_area() || tls != NULL);

	/* Save pointer to the thread on the stack to get current thread */
	*((unsigned long *) stack) = (unsigned long) thread;

	/* Allocate thread context */
	ctx = uk_zalloc(allocator, ukplat_thread_ctx_size(cbs));
	if (!ctx) {
		ret = -1;
		goto err_out;
	}

	memset(thread, 0, sizeof(*thread));
	thread->ctx = ctx;
	thread->name = name;
	thread->stack = stack;
	thread->tls = tls;
	thread->entry = function;
	thread->arg = arg;

	/* Not runnable, not exited, not sleeping */
	thread->flags = 0;
	thread->wakeup_time = 0LL;
	thread->detached = false;
	uk_waitq_init(&thread->waiting_threads);
	thread->sched = NULL;
	thread->prv = NULL;

	/* TODO: Move newlibc reent initialization to newlib as
	 *       thread initialization function
	 */
#ifdef CONFIG_LIBNEWLIBC
	reent_init(&thread->reent);
#endif

	/* Iterate over registered thread initialization functions */
	uk_thread_inittab_foreach(itr) {
		if (unlikely(!itr->init))
			continue;

		uk_pr_debug("New thread %p: Call thread initialization function %p...\n",
			    thread, *itr->init);
		ret = (itr->init)(thread);
		if (ret < 0)
			goto err_fini;
	}

	/* Prepare stack and TLS
	 * NOTE: In case the function pointer was changed by a thread init
	 *       function (e.g., encapsulation), we prepare the stack here
	 *       with the final setup
	 */
	init_sp(&sp, stack, thread->entry, thread->arg);

	/* Platform specific context initialization */
	ukplat_thread_ctx_init(cbs, thread->ctx, sp,
			       (uintptr_t) ukarch_tls_pointer(tls));

	uk_pr_info("Thread \"%s\": pointer: %p, stack: %p, tls: %p\n",
		   name, thread, thread->stack, thread->tls);

	return 0;

err_fini:
	/* Run fini functions starting from one level before the failed one
	 * because we expect that the failed one cleaned up.
	 */
	uk_thread_inittab_foreach_reverse2(itr, itr - 2) {
		if (unlikely(!itr->fini))
			continue;
		(itr->fini)(thread);
	}
	uk_free(allocator, thread->ctx);
err_out:
	return ret;
}

void uk_thread_fini(struct uk_thread *thread, struct uk_alloc *allocator)
{
	struct uk_thread_inittab_entry *itr;

	UK_ASSERT(thread != NULL);

	uk_thread_inittab_foreach_reverse(itr) {
		if (unlikely(!itr->fini))
			continue;
		(itr->fini)(thread);
	}
	uk_free(allocator, thread->ctx);
	thread->ctx = NULL;
}

static void uk_thread_block_until(struct uk_thread *thread, __snsec until)
{
	unsigned long flags;

	flags = ukplat_lcpu_save_irqf();
	thread->wakeup_time = until;
	clear_runnable(thread);
	uk_sched_thread_blocked(thread->sched, thread);
	ukplat_lcpu_restore_irqf(flags);
}

void uk_thread_block_timeout(struct uk_thread *thread, __nsec nsec)
{
	__snsec until = (__snsec) ukplat_monotonic_clock() + nsec;

	uk_thread_block_until(thread, until);
}

void uk_thread_block(struct uk_thread *thread)
{
	uk_thread_block_until(thread, 0LL);
}

void uk_thread_wake(struct uk_thread *thread)
{
	unsigned long flags;

	flags = ukplat_lcpu_save_irqf();
	if (!is_runnable(thread)) {
		uk_sched_thread_woken(thread->sched, thread);
		thread->wakeup_time = 0LL;
		set_runnable(thread);
	}
	ukplat_lcpu_restore_irqf(flags);
}

void uk_thread_exit(struct uk_thread *thread)
{
	UK_ASSERT(thread);

	set_exited(thread);

	if (!thread->detached)
		uk_waitq_wake_up(&thread->waiting_threads);

	uk_pr_debug("Thread \"%s\" exited.\n", thread->name);
}

int uk_thread_wait(struct uk_thread *thread)
{
	UK_ASSERT(thread);

	/* TODO critical region */

	if (thread->detached)
		return -EINVAL;

	uk_waitq_wait_event(&thread->waiting_threads, is_exited(thread));

	thread->detached = true;

	uk_sched_thread_destroy(thread->sched, thread);

	return 0;
}

int uk_thread_detach(struct uk_thread *thread)
{
	UK_ASSERT(thread);

	thread->detached = true;

	return 0;
}

int uk_thread_set_prio(struct uk_thread *thread, prio_t prio)
{
	if (!thread)
		return -EINVAL;

	return uk_sched_thread_set_prio(thread->sched, thread, prio);
}

int uk_thread_get_prio(const struct uk_thread *thread, prio_t *prio)
{
	if (!thread)
		return -EINVAL;

	return uk_sched_thread_get_prio(thread->sched, thread, prio);
}

int uk_thread_set_timeslice(struct uk_thread *thread, int timeslice)
{
	if (!thread)
		return -EINVAL;

	return uk_sched_thread_set_timeslice(thread->sched, thread, timeslice);
}

int uk_thread_get_timeslice(const struct uk_thread *thread, int *timeslice)
{
	if (!thread)
		return -EINVAL;

	return uk_sched_thread_get_timeslice(thread->sched, thread, timeslice);
}
