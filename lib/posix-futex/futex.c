/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Daniel Dinca <dincadaniel97@gmail.com>
 *          Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *          Razvan Deaconescu <razvan.deaconescu@cs.pub.ro>
 *          Simon Kuenzer <simon.kuenzer@neclab.eu>
 *          Adina Smeu <adina.smeu@gmail.com>
 *
 * Copyright (c) 2021, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
 * Copyright (c) 2021, Karlsruhe Institute of Technology (KIT).
 *                     All rights reserved.
 * Copyright (c) 2021, University Politehnica of Bucharest.
 *                     All rights reserved.
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

#include <uk/config.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

#include <linux/futex.h>
#include <uk/syscall.h>
#include <uk/arch/atomic.h>
#include <uk/thread.h>
#include <uk/thread.h>
#include <uk/list.h>
#if CONFIG_LIBPOSIX_PROCESS_CLONE
#include <uk/process.h>
#endif
#include <uk/sched.h>
#include <uk/list.h>
#include <uk/assert.h>
#include <uk/print.h>
#include <uk/spinlock.h>
#include <uk/plat/lcpu.h>

/** @struct uk_futex
 *  @brief Futex structure.
 */
struct uk_futex {
	uint32_t *uaddr; /** The futex address. */
	struct uk_thread *thread; /** The thread waiting on the futex. */
	struct uk_list_head list_node; /** The list containing all the futexes
					 * on which the threads are waiting.
					 */
};

static UK_LIST_HEAD(futex_list);
static uk_spinlock futex_list_lock = UK_SPINLOCK_INITIALIZER();

/**
 * Prepare to wait on a futex.
 *
 * Get the futex value atomically and compare it with the expected value. Add
 * the thread to the wait list and then block it if the value is equal to the
 * expected one. If the futex was not removed from the list when the thread was
 * unblocked, then it means that it timed out.
 *
 * @param uaddr	The futex userspace address
 * @param val	The expected value
 * @param tm	Timespec structure which contains the timeout for which the
 *		thread will blocked. If it is null, the thread will wait
 *		indefinitely.
 *
 * @return
 *	0: uaddr contains val and the thread finished waiting;
 *	<1: -EAGAIN (uaddr does not contain val) or -ETIMEDOUT (the futex timed
 *       out)
 */
static int futex_wait(uint32_t *uaddr, uint32_t val, const struct timespec *tm)
{
	unsigned long irqf;
	struct uk_list_head *itr, *tmp;
	struct uk_futex *f_tmp;
	struct uk_thread *current = uk_thread_current();
	struct uk_futex f = {.uaddr = uaddr, .thread = current};

	if (ukarch_load_n(uaddr) == val) {
		irqf = ukplat_lcpu_save_irqf();
		uk_spin_lock(&futex_list_lock);

		/* Enqueue thread to wait list */
		uk_list_add_tail(&f.list_node, &futex_list);

		uk_spin_unlock(&futex_list_lock);
		ukplat_lcpu_restore_irqf(irqf);

		if (tm)
			/* Block for at most timeout nanosecs */
			uk_thread_block_timeout(current, tm->tv_nsec);
		else
			/* Block indefinitely */
			uk_thread_block(current);

		uk_sched_yield();

		irqf = ukplat_lcpu_save_irqf();
		uk_spin_lock(&futex_list_lock);

		/* If the futex is still in the wait list, then it timed out */
		uk_list_for_each_safe(itr, tmp, &futex_list) {
			f_tmp = uk_list_entry(itr, struct uk_futex, list_node);

			if (f_tmp->uaddr == uaddr && f_tmp->thread == current) {
				/* Remove the thread from the futex list */
				uk_list_del(&f_tmp->list_node);
				uk_spin_unlock(&futex_list_lock);
				ukplat_lcpu_restore_irqf(irqf);

				return -ETIMEDOUT;
			}
		}

		uk_spin_unlock(&futex_list_lock);
		ukplat_lcpu_restore_irqf(irqf);

		return 0;
	}

	/* Futex word does not contain expected val */
	return -EAGAIN;
}

/**
 * Wake up threads waiting on a futex.
 *
 * Find val threads in the wait list for the futex, remove the futexes from the
 * list and wake up the threads.
 *
 * @param uaddr	The futex userspace address
 * @param val	The number of threads waiting on the futex to be woken up
 *
 * @return
 *	0: no threads were woken up;
 *	>0: the number of threads woken up
 */
static int futex_wake(uint32_t *uaddr, uint32_t val)
{
	unsigned long irqf;
	struct uk_list_head *itr, *tmp;
	struct uk_futex *f;
	uint32_t count = 0;

	irqf = ukplat_lcpu_save_irqf();
	uk_spin_lock(&futex_list_lock);

	uk_list_for_each_safe(itr, tmp, &futex_list) {
		f = uk_list_entry(itr, struct uk_futex, list_node);

		if (f->uaddr == uaddr) {
			/* Remove the thread from the futex list */
			uk_list_del(&f->list_node);

			/* TODO: Replace with uk_thread_wakeup when the new
			 * scheduler API is ready
			 */
			uk_thread_wake(f->thread);

			/* Wake at most val threads */
			if (++count >= val)
				break;
		}
	}

	uk_spin_unlock(&futex_list_lock);
	ukplat_lcpu_restore_irqf(irqf);

	return (int) count;
}

/**
 * Requeue waiters from uaddr to uaddr2.
 *
 * Requeue waiters on uaddr to uaddr2. Wakes up a maximum of val waiters that
 * are waiting on the futex at uaddr.  If there are more than val waiters, then
 * the remaining waiters are removed from the wait queue of the source futex at
 * uaddr and added to the wait queue of the target futex at uaddr2. The val2
 * argument specifies an upper limit on the number of waiters that are requeued
 * to the futex at uaddr2.
 *
 * @param uaddr		Source futex user address
 * @param val		Number of waiters to wake
 * @param val2		Number of waiters to requeue (0-INT_MAX)
 * @param uaddr2	Target futex user address
 * @param val3		uaddr expected value
 *
 * @return
 *	>=0: on success, the number of tasks requeued or woken;
 *	<0: on error
 */
static int futex_cmp_requeue(uint32_t *uaddr, uint32_t val, uint32_t val2,
			     uint32_t *uaddr2, uint32_t val3)
{
	unsigned long irqf;
	struct uk_list_head *itr, *tmp;
	struct uk_futex *f;
	int woken_uaddr1;
	uint32_t waiters_uaddr2 = 0;

	if (!((uint32_t)val3 == ukarch_load_n(uaddr)))
		return -EAGAIN;

	/* Wake up val waiters on uaddr */
	woken_uaddr1 = futex_wake(uaddr, val);

	if (!val2)
		return woken_uaddr1;

	irqf = ukplat_lcpu_save_irqf();
	uk_spin_lock(&futex_list_lock);

	/* Requeue val2 waiters on uaddr2 */
	uk_list_for_each_safe(itr, tmp, &futex_list) {
		f = uk_list_entry(itr, struct uk_futex, list_node);

		if (f->uaddr == uaddr) {
			/* Requeue thread to uaddr2 */
			uk_list_del(&f->list_node);
			f->uaddr = uaddr2;
			uk_list_add_tail(&f->list_node, &futex_list);

			/* Requeue at most val2 threads */
			if (++waiters_uaddr2 >= val2)
				break;
		}
	}

	uk_spin_unlock(&futex_list_lock);
	ukplat_lcpu_restore_irqf(irqf);

	return woken_uaddr1 + waiters_uaddr2;
}

/**
 * According to man pages, there exists no libc wrapper for futex
 *
 * @param uaddr		Source futex user address
 * @param futex_op	Futex operation
 * @param val		Depends on the futex operation
 * @param timeout	Usually a timeout for the futex operation, but can be
 *			used as an integer value for other operations
 * @param uaddr2	Optional target futex user address
 * @param val3		Depends on the futex operation
 *
 * @return
 *	>=0: on success
 *	<0: on error
 */
UK_LLSYSCALL_R_DEFINE(int, futex, uint32_t *, uaddr, int, futex_op,
		      uint32_t, val, const struct timespec *, timeout,
		      uint32_t *, uaddr2, uint32_t, val3)
{
	switch (futex_op) {
	case FUTEX_WAIT:
	case FUTEX_WAIT_PRIVATE:
		return futex_wait(uaddr, val, timeout);

	case FUTEX_WAKE:
	case FUTEX_WAKE_PRIVATE:
		return futex_wake(uaddr, val);

	case FUTEX_FD:
	case FUTEX_REQUEUE:
		return -ENOSYS;

	case FUTEX_CMP_REQUEUE:
		return futex_cmp_requeue(uaddr, val, (unsigned long)timeout,
					 uaddr2, val3);

	default:
		return -ENOSYS;
		/* TODO: other operations? */
	}
}

#if CONFIG_LIBPOSIX_PROCESS_CLONE
/*
 * Reference to child TID that should be cleared on thread exit
 * (if not NULL):
 *  https://man7.org/linux/man-pages/man2/set_tid_address.2.html
 */
static __uk_tls pid_t *child_tid_clear_ref = NULL;

/* Store reference for clearing child TID on exit */
static int pfutex_child_cleartid(const struct clone_args *cl_args,
				 size_t cl_args_len __unused,
				 struct uk_thread *child __unused,
				 struct uk_thread *parent __unused)
{
	child_tid_clear_ref = (pid_t *) cl_args->child_tid;
	return 0;
}
UK_POSIX_CLONE_HANDLER(CLONE_CHILD_CLEARTID, true, pfutex_child_cleartid, 0x0);

/* Update the stored reference for clearing child TID on exit
 * NOTE: There is no libc wrapper
 * NOTE: The system call also returns the caller's thread ID.
 *       We call SYS_gettid effectively and in case of an error
 *       we forward the error code to the caller. In such a case,
 *       we also do not update the reference.
 */
UK_LLSYSCALL_R_DEFINE(pid_t, set_tid_address, pid_t *, tid_ref)
{
	pid_t self_tid = uk_syscall_r_gettid();

	if (self_tid >= 0) {
		/* Store new reference */
		child_tid_clear_ref = tid_ref;
	}
	return self_tid;
}

/* Thread exit handler that clears child TID at the stored reference */
static void pfutex_child_cleartid_term(struct uk_thread *child __unused)
{
	if (child_tid_clear_ref != NULL) {
		*((pid_t *) child_tid_clear_ref) = 0;
		futex_wake((uint32_t *) child_tid_clear_ref, 0);
	}
}
UK_THREAD_INIT_PRIO(0x0, pfutex_child_cleartid_term, UK_PRIO_EARLIEST);

#endif /* CONFIG_LIBPOSIX_PROCESS_CLONE */
