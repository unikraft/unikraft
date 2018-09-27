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
#include <stdlib.h>
#include <uk/plat/config.h>
#include <uk/plat/time.h>
#include <uk/thread.h>
#include <uk/print.h>
#include <uk/assert.h>


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

	/* Must ensure that (%rsp + 8) is 16-byte aligned
	 * at the start of thread_starter.
	 */
	stack_push(sp, 0);

	stack_push(sp, (unsigned long) function);
	stack_push(sp, (unsigned long) data);
}

int uk_thread_init(struct uk_thread *thread,
		struct ukplat_ctx_callbacks *cbs, struct uk_alloc *allocator,
		const char *name, void *stack,
		void (*function)(void *), void *arg)
{
	unsigned long sp;

	UK_ASSERT(thread != NULL);
	UK_ASSERT(stack != NULL);

	/* Save pointer to the thread on the stack to get current thread */
	*((unsigned long *) stack) = (unsigned long) thread;

	init_sp(&sp, stack, function, arg);

	/* Call platform specific setup. */
	thread->ctx = ukplat_thread_ctx_create(cbs, allocator, sp);
	if (thread->ctx == NULL)
		return -1;

	thread->name = name;
	thread->stack = stack;

	/* Not runnable, not exited, not sleeping */
	thread->flags = 0;
	thread->wakeup_time = 0LL;

#ifdef CONFIG_HAVE_LIBC
	//TODO _REENT_INIT_PTR(&thread->reent);
#endif

	uk_pr_info("Thread \"%s\": pointer: %p, stack: %p\n",
		   name, thread, thread->stack);

	return 0;
}

void uk_thread_fini(struct uk_thread *thread, struct uk_alloc *allocator)
{
	UK_ASSERT(thread != NULL);
	ukplat_thread_ctx_destroy(allocator, thread->ctx);
}

static void uk_thread_block_until(struct uk_thread *thread, __snsec until)
{
	thread->wakeup_time = until;
	clear_runnable(thread);
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
	if (is_runnable(thread))
		return;

	thread->wakeup_time = 0LL;
	set_runnable(thread);
}
