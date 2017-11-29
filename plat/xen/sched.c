/* SPDX-License-Identifier: MIT */
/*
 ****************************************************************************
 * (C) 2005 - Grzegorz Milos - Intel Research Cambridge
 ****************************************************************************
 *
 *        File: sched.c
 *      Author: Grzegorz Milos
 *     Changes: Robert Kaiser
 *
 *        Date: Aug 2005
 *
 * Environment: Xen Minimal OS
 * Description: simple scheduler for Mini-Os
 *              Ported from Mini-OS
 *
 * The scheduler is non-preemptive (cooperative), and schedules according
 * to Round Robin algorithm.
 *
 ****************************************************************************
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

#include <stdlib.h>
#include <stdint.h>
#include <common/hypervisor.h>
#include <common/sched.h>
#include <xen-x86/irq.h>
#include <uk/sched.h>
#include <uk/list.h>
#include <uk/assert.h>


#ifdef SCHED_DEBUG
#define DEBUG(_f, _a...) \
	uk_printk("MINI_OS(file=sched.c, line=%d) " _f "\n", __LINE__, ## _a)
#else
#define DEBUG(_f, _a...)	((void)0)
#endif

#if 0//TODO revisit
#ifdef HAVE_LIBC
static struct _reent callback_reent;
struct _reent *__getreent(void)
{
	struct _reent *_reent;

	if (!threads_started)
		_reent = _impure_ptr;
	else if (in_callback)
		_reent = &callback_reent;
	else
		_reent = &get_current_ctx()->reent;

#ifndef NDEBUG
#if defined(__x86_64__) || defined(__x86__)
	{
#ifdef __x86_64__
		register unsigned long sp asm ("rsp");
#else
		register unsigned long sp asm ("esp");
#endif
		if ((sp & (STACK_SIZE-1)) < STACK_SIZE / 16) {
			static int overflowing;

			if (!overflowing) {
				overflowing = 1;
				uk_printk("stack overflow\n");
				UC_BUG();
			}
		}
	}
#endif
#else
#error Not implemented yet
#endif
	return _reent;
}
#endif
#endif

void exit_thread(void)
{
	struct uk_thread *thread = uk_thread_current();

	uk_printk("Thread \"%s\" exited.\n", thread->name);

	uk_thread_stop(thread);
	UK_CRASH("Error stopping thread.");
}
