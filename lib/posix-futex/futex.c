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
#include <stdbool.h>
#include <errno.h>
#include <time.h>
#include <inttypes.h>

#include <linux/futex.h>
#include <uk/syscall.h>
#include <uk/atomic.h>
#include <uk/thread.h>
#if CONFIG_LIBPOSIX_PROCESS_MULTITHREADING
#include <uk/process.h>
#endif /* CONFIG_LIBPOSIX_PROCESS_MULTITHREADING */
#include <uk/wait.h>
#include <uk/assert.h>
#include <uk/print.h>
#include <uk/spinlock.h>
#include <uk/lcpu.h>
#include <uk/plat/time.h>
#if CONFIG_LIBPOSIX_FUTEX_VAS
#include <uk/isr/string.h>
#include "futex_key.h"
#endif

/*
 * Address transactions are serialized separately from waitq membership.
 * Waitq owns enqueue/block/detach; futex code only owns the address cookie.
 * Lock order is address lock -> waitq lock -> scheduler transition.
 */
struct uk_futex {
#if CONFIG_LIBPOSIX_FUTEX_VAS
	struct futex_key key;
#else
	uint32_t *uaddr;
#endif
	uint32_t bitset;
	int woken;
};

struct futex_guard {
	unsigned long irqf;
};

static DEFINE_WAIT_QUEUE(futex_waiters);
static uk_spinlock futex_address_lock = UK_SPINLOCK_INITIALIZER();

static void futex_guard_lock(struct futex_guard *guard)
{
	guard->irqf = uk_lcpu_save_irqf();
	uk_spin_lock(&futex_address_lock);
}

static void futex_guard_unlock(struct futex_guard *guard)
{
	uk_spin_unlock(&futex_address_lock);
	uk_lcpu_restore_irqf(guard->irqf);
}

#if !CONFIG_LIBPOSIX_FUTEX_VAS
/*
 * Perform the first read before taking the address lock.  The caller must
 * keep the futex word valid for the operation.
 * Recheck under the address lock so a concurrent wake cannot pass publication.
 * The external spinlock is released by waitq only after enqueue AND block,
 * and re-acquired before detaching the ticket.
 */
static int futex_wait_bitset(uint32_t *uaddr, uint32_t val,
			     const __nsec *timeout, uint32_t bitset)
{
	struct uk_thread *current = uk_thread_current();
	struct uk_futex f = { .uaddr = uaddr, .bitset = bitset, .woken = 0 };
	struct futex_guard guard;
	__uptr previous_cookie;
	int timedout;
	int result;

	if (uk_load_n(uaddr) != val)
		return -EAGAIN;

	futex_guard_lock(&guard);
	if (uk_load_n(uaddr) != val) {
		futex_guard_unlock(&guard);
		return -EAGAIN;
	}

	previous_cookie = current->wait_ticket.cookie;
	uk_waitq_set_cookie((__uptr)&f);
	timedout = uk_waitq_wait_deadline_locked(&futex_waiters,
			timeout ? (__snsec)*timeout : 0,
			futex_guard_lock, futex_guard_unlock, &guard);

	/*
	 * Requeue changes the address cookie, not the registration lifetime.
	 * Waitq already detached this thread regardless of its current address.
	 * A committed wake wins over a deadline noticed after rescheduling.
	 */
	result = (!f.woken && timedout) ? -ETIMEDOUT : 0;
	uk_waitq_set_cookie(previous_cookie);
	futex_guard_unlock(&guard);
	return result;
}

static int futex_wait(uint32_t *uaddr, uint32_t val, const __nsec *timeout)
{
	return futex_wait_bitset(uaddr, val, timeout, FUTEX_BITSET_MATCH_ANY);
}

struct futex_wake_request {
	uint32_t *uaddr;
	uint32_t bitset;
	uint32_t remaining;
	uint32_t count;
};

/* Both the address lock and waitq lock are held by the caller. */
static int futex_select_wake(struct uk_waitq_ticket *ticket,
			     struct futex_wake_request *request)
{
	struct uk_futex *f = (struct uk_futex *)ticket->cookie;

	if (!request->remaining || f->woken || f->uaddr != request->uaddr)
		return 0;
	if (!(f->bitset & request->bitset))
		return 0;

	f->woken = 1;
	request->remaining--;
	request->count++;
	return 1;
}

/* The address lock remains held across compare/wake/requeue transactions. */
static int futex_wake_bitset_locked(uint32_t *uaddr, uint32_t val,
				    uint32_t bitset)
{
	struct uk_waitq_ticket *ticket;
	struct futex_wake_request request = {
		.uaddr = uaddr,
		.bitset = bitset,
		.remaining = val,
		.count = 0,
	};

	if (!val)
		return 0;

	uk_waitq_wake_up_if(&futex_waiters, ticket,
			   futex_select_wake(ticket, &request));
	return (int)request.count;
}

static int futex_wake_locked(uint32_t *uaddr, uint32_t val)
{
	return futex_wake_bitset_locked(uaddr, val, FUTEX_BITSET_MATCH_ANY);
}

static int futex_wake_bitset(uint32_t *uaddr, uint32_t val, uint32_t bitset)
{
	struct futex_guard guard;
	int count;

	futex_guard_lock(&guard);
	count = futex_wake_bitset_locked(uaddr, val, bitset);
	futex_guard_unlock(&guard);
	return count;
}

static int futex_wake(uint32_t *uaddr, uint32_t val)
{
	return futex_wake_bitset(uaddr, val, FUTEX_BITSET_MATCH_ANY);
}
#else
/* Remains reachable after a wake detaches the waitq ticket, until return. */
static __uk_tls struct uk_futex *futex_active_wait;

static struct uk_vas *futex_current_vas(void)
{
	struct uk_vas *vas = uk_thread_current()->vas;

	return vas ? vas : uk_vas_get_kernel();
}

static int futex_vas_wait(__u32 *addr, __u32 val,
			  const __nsec *timeout, __u32 bitset, bool private)
{
	struct uk_thread *current = uk_thread_current();
	struct uk_vas *vas = futex_current_vas();
	struct uk_futex f = { .bitset = bitset };
	struct futex_guard guard;
	__uptr previous_cookie;
	__snsec deadline = timeout ? (__snsec)(*timeout) : 0;
	__u32 value;
	int rc, timedout;

	if (uk_lcpu_irqs_disabled())
		return -EWOULDBLOCK;
	for (;;) {
		futex_guard_lock(&guard);
		rc = futex_key_read(vas, addr, private, &value, &f.key);
		if (rc != -EAGAIN)
			break;
		futex_guard_unlock(&guard);
		/* Fault in pages after releasing the guard. */
		rc = uk_vas_fault(vas, (__vaddr_t)addr, UK_VMA_FAULT_READ);
		if (rc)
			return rc;
	}
	if (rc || value != val) {
		futex_guard_unlock(&guard);
		futex_key_put(&f.key);
		return rc ? rc : -EAGAIN;
	}
	UK_ASSERT(!uk_thread_uktls_var(current, futex_active_wait));
	uk_thread_uktls_var(current, futex_active_wait) = &f;
	previous_cookie = current->wait_ticket.cookie;
	uk_waitq_set_cookie((__uptr)&f);
	timedout = uk_waitq_wait_deadline_locked(&futex_waiters, deadline,
						 futex_guard_lock,
						 futex_guard_unlock, &guard);
	rc = (!f.woken && timedout) ? -ETIMEDOUT : 0;
	uk_waitq_set_cookie(previous_cookie);
	uk_thread_uktls_var(current, futex_active_wait) = NULL;
	futex_guard_unlock(&guard);
	futex_key_put(&f.key);
	return rc;
}

struct futex_vas_wake_request {
	const struct futex_key *key;
	__u32 bitset;
	__u32 remaining;
	__u32 count;
};

static int futex_vas_select(struct uk_waitq_ticket *ticket,
			    struct futex_vas_wake_request *request)
{
	struct uk_futex *f = (struct uk_futex *)ticket->cookie;

	if (!request->remaining || f->woken ||
	    !futex_key_equal(&f->key, request->key) ||
	    !(f->bitset & request->bitset))
		return 0;
	f->woken = 1;
	request->remaining--;
	request->count++;
	return 1;
}

static int futex_vas_wake_locked(const struct futex_key *key, __u32 val,
				 __u32 bitset)
{
	struct uk_waitq_ticket *ticket;
	struct futex_vas_wake_request request = {
		.key = key, .remaining = val, .bitset = bitset
	};

	if (val)
		uk_waitq_wake_up_if(&futex_waiters, ticket,
				    futex_vas_select(ticket, &request));
	return (int)request.count;
}

static int futex_vas_wake(struct uk_vas *vas, __u32 *addr, __u32 val,
			  __u32 bitset, bool private)
{
	struct futex_key key = { 0 };
	struct futex_guard guard;
	int rc;

	if (uk_lcpu_irqs_disabled())
		return -EWOULDBLOCK;
	for (;;) {
		futex_guard_lock(&guard);
		rc = futex_key_lookup(vas, addr, private, &key);
		if (rc != -EAGAIN)
			break;
		futex_guard_unlock(&guard);
		uk_arch_spinwait();
	}
	if (!rc)
		rc = futex_vas_wake_locked(&key, val, bitset);
	futex_guard_unlock(&guard);
	futex_key_put(&key);
	return rc;
}

static void futex_vas_retire_wait(struct uk_thread *thread)
{
	struct futex_key key = { 0 };
	struct futex_guard guard;
	struct uk_futex *f;

	futex_guard_lock(&guard);
	f = uk_thread_uktls_var(thread, futex_active_wait);
	if (f) {
		key = f->key;
		memset_isr(&f->key, 0, sizeof(f->key));
		uk_thread_uktls_var(thread, futex_active_wait) = NULL;
	}
	if (thread->wait_ticket.wq == &futex_waiters)
		uk_waitq_cancel(thread);
	futex_guard_unlock(&guard);
	futex_key_put(&key);
}

UK_THREAD_INIT_PRIO_FLAGS(0, futex_vas_retire_wait, UK_PRIO_EARLIEST,
			  UK_THREAD_INITF_UKTLS);
#endif

static int futex_timeout_deadline(const struct timespec *timeout,
				  bool abs_time,
				  bool realtime_clock,
				  __nsec *deadline_out)
{
	__nsec max_deadline = (__nsec)__SNSEC_MAX;
	__nsec max_sec = max_deadline / (__nsec)UKARCH_NSEC_PER_SEC;
	__nsec ts_ns, deadline;

	if (timeout->tv_sec < 0 || timeout->tv_nsec < 0 ||
	    timeout->tv_nsec >= UKARCH_NSEC_PER_SEC)
		return -EINVAL;

	if ((__nsec)timeout->tv_sec >= max_sec) {
		ts_ns = max_deadline;
	} else {
		__nsec sec_ns = (__nsec)timeout->tv_sec *
				(__nsec)UKARCH_NSEC_PER_SEC;
		__nsec sub_ns = (__nsec)timeout->tv_nsec;

		ts_ns = (sub_ns > max_deadline - sec_ns) ?
			max_deadline : (sec_ns + sub_ns);
	}

	if (!abs_time) {
		__nsec now_mono = ukplat_monotonic_clock();

		deadline = (ts_ns > max_deadline - now_mono) ?
			   max_deadline : (now_mono + ts_ns);
	} else if (realtime_clock) {
		__nsec now_real = ukplat_wall_clock();
		__nsec now_mono = ukplat_monotonic_clock();

		if (ts_ns <= now_real) {
			deadline = 1;
		} else {
			__nsec delta = ts_ns - now_real;

			deadline = (delta > max_deadline - now_mono) ?
				   max_deadline : (now_mono + delta);
		}
	} else {
		deadline = ts_ns;
	}

	if (deadline == 0)
		deadline = 1;
	*deadline_out = deadline;
	return 0;
}

#if !CONFIG_LIBPOSIX_FUTEX_VAS
static int futex_cmp_requeue(uint32_t *uaddr, uint32_t val, uint32_t val2,
			     uint32_t *uaddr2, uint32_t val3)
{
	struct futex_guard guard;
	struct uk_waitq_ticket *ticket;
	uint32_t requeued = 0;
	int woken;

	if (uk_load_n(uaddr) != val3)
		return -EAGAIN;

	futex_guard_lock(&guard);
	if (uk_load_n(uaddr) != val3) {
		futex_guard_unlock(&guard);
		return -EAGAIN;
	}

	woken = futex_wake_locked(uaddr, val);
	if (val2) {
		uk_waitq_visit(&futex_waiters, ticket, {
			struct uk_futex *f = (struct uk_futex *)ticket->cookie;

			if (!f->woken && f->uaddr == uaddr) {
				f->uaddr = uaddr2;
				if (++requeued >= val2)
					break;
			}
		});
	}
	futex_guard_unlock(&guard);
	return woken + (int)requeued;
}
#else
static int futex_vas_requeue(__u32 *addr, __u32 val, __u32 val2,
			     __u32 *addr2, __u32 val3, bool private)
{
	struct uk_vas *vas = futex_current_vas();
	struct futex_key source = { 0 }, target = { 0 };
	struct futex_guard guard;
	struct uk_waitq_ticket *ticket;
	__u32 value, requeued = 0;
	int rc;

	if (uk_lcpu_irqs_disabled())
		return -EWOULDBLOCK;
	for (;;) {
		futex_guard_lock(&guard);
		rc = futex_key_read(vas, addr, private, &value, &source);
		if (rc == -EAGAIN) {
			futex_guard_unlock(&guard);
			rc = uk_vas_fault(vas, (__vaddr_t)addr,
					  UK_VMA_FAULT_READ);
			if (rc)
				return rc;
			continue;
		}
		if (rc)
			break;
		if (value != val3) {
			rc = -EAGAIN;
			break;
		}
		rc = futex_key_lookup(vas, addr2, private, &target);
		if (rc != -EAGAIN)
			break;
		futex_guard_unlock(&guard);
		futex_key_put(&source);
		uk_arch_spinwait();
	}
	if (rc)
		goto unlock;
	rc = futex_vas_wake_locked(&source, val, FUTEX_BITSET_MATCH_ANY);
	if (val2) {
		uk_waitq_visit(&futex_waiters, ticket, {
			struct uk_futex *f = (struct uk_futex *)ticket->cookie;

			if (!f->woken && futex_key_equal(&f->key, &source)) {
				/* Retire source pins after unlocking. */
				futex_key_copy(&f->key, &target);
				if (++requeued >= val2)
					break;
			}
		});
	}
	rc += (int)requeued;
unlock:
	futex_guard_unlock(&guard);
	while (requeued) {
		struct futex_key old = source;

		futex_key_put(&old);
		requeued--;
	}
	futex_key_put(&source);
	futex_key_put(&target);
	return rc;
}
#endif

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
	__nsec timeout_ns;
 	int cmd = futex_op & FUTEX_CMD_MASK;

	/* Reject invalid combinations of the realtime clock flag */
	if (futex_op & FUTEX_CLOCK_REALTIME && !(
		cmd == FUTEX_WAIT ||
		cmd == FUTEX_WAIT_BITSET ||
		cmd == FUTEX_LOCK_PI2 ||
		cmd == FUTEX_WAIT_REQUEUE_PI))
		return -ENOSYS;

	/*
	 * N.B. CLOCK_(MONOTONIC|REALTIME|...) are at the moment all the same in
	 * Unikraft. Therefore, we can just use CLOCK_MONOTONIC for timeouts in
	 * the following code.
	 */
	switch (cmd) {
	case FUTEX_WAIT:
		if (timeout) {
			int rc = futex_timeout_deadline(
				timeout, false,
				!!(futex_op & FUTEX_CLOCK_REALTIME),
				&timeout_ns);
			if (rc < 0)
				return rc;
		}
#if CONFIG_LIBPOSIX_FUTEX_VAS
		return futex_vas_wait(uaddr, val, timeout ? &timeout_ns : NULL,
				       FUTEX_BITSET_MATCH_ANY,
				       !!(futex_op & FUTEX_PRIVATE_FLAG));
#else
		return futex_wait(uaddr, val, timeout ? &timeout_ns : NULL);
#endif

	case FUTEX_WAIT_BITSET:
		if (val3 == 0)
			return -EINVAL;

		if (timeout) {
			int rc = futex_timeout_deadline(
				timeout, true,
				!!(futex_op & FUTEX_CLOCK_REALTIME),
				&timeout_ns);
			if (rc < 0)
				return rc;
		}
#if CONFIG_LIBPOSIX_FUTEX_VAS
		return futex_vas_wait(uaddr, val, timeout ? &timeout_ns : NULL,
				       val3, !!(futex_op & FUTEX_PRIVATE_FLAG));
#else
		return futex_wait_bitset(uaddr, val,
					 timeout ? &timeout_ns : NULL, val3);
#endif

	case FUTEX_WAKE:
#if CONFIG_LIBPOSIX_FUTEX_VAS
		return futex_vas_wake(futex_current_vas(), uaddr, val,
				       FUTEX_BITSET_MATCH_ANY,
				       !!(futex_op & FUTEX_PRIVATE_FLAG));
#else
		return futex_wake(uaddr, val);
#endif

	case FUTEX_WAKE_BITSET:
		if (val3 == 0)
			return -EINVAL;
#if CONFIG_LIBPOSIX_FUTEX_VAS
		return futex_vas_wake(futex_current_vas(), uaddr, val, val3,
				       !!(futex_op & FUTEX_PRIVATE_FLAG));
#else
		return futex_wake_bitset(uaddr, val, val3);
#endif

	case FUTEX_FD:
	case FUTEX_REQUEUE:
		return -ENOSYS;

	case FUTEX_CMP_REQUEUE:
#if CONFIG_LIBPOSIX_FUTEX_VAS
		return futex_vas_requeue(uaddr, val, (unsigned long)timeout,
					  uaddr2, val3,
					  !!(futex_op & FUTEX_PRIVATE_FLAG));
#else
		return futex_cmp_requeue(uaddr, val, (unsigned long)timeout,
					 uaddr2, val3);
#endif

	default:
		return -ENOSYS;
		/* TODO: other operations? */
	}
}

#if CONFIG_LIBPOSIX_PROCESS_MULTITHREADING
/*
 * Reference to child TID that should be cleared on thread exit
 * (if not NULL):
 *  https://man7.org/linux/man-pages/man2/set_tid_address.2.html
 */
static __uk_tls pid_t *child_tid_clear_ref = NULL;

/* Store reference for clearing child TID on exit */
static int pfutex_child_cleartid(void *arg)
{
	struct posix_process_clone_event_data *event_data;
	const struct clone_args *cl_args;
	struct uk_thread *child;
	pid_t *child_tid;

	event_data = (struct posix_process_clone_event_data *)arg;
	UK_ASSERT(event_data);

	cl_args = event_data->cl_args;
	UK_ASSERT(cl_args);

	if (!(cl_args->flags & CLONE_CHILD_CLEARTID))
		return UK_EVENT_NOT_HANDLED;

	child_tid = (pid_t *)cl_args->child_tid;
	child = event_data->child;

	uk_thread_uktls_var(child, child_tid_clear_ref) = child_tid;

	return UK_EVENT_HANDLED_CONT;
}

POSIX_PROCESS_CLONE_HANDLER(CLONE_CHILD_CLEARTID, pfutex_child_cleartid);

/* Update the stored reference for clearing child TID on exit
 * NOTE: There is no libc wrapper
 * NOTE: The system call also returns the caller's thread ID.
 *       We call SYS_gettid effectively and in case of an error
 *       we forward the error code to the caller. In such a case,
 *       we also do not update the reference.
 */
UK_LLSYSCALL_R_DEFINE(pid_t, set_tid_address, pid_t *, tid_ref)
{
	pid_t self_tid = uk_sys_gettid();

	if (self_tid >= 0) {
		/* Store new reference */
		child_tid_clear_ref = tid_ref;
	}
	return self_tid;
}

static int pthread_exit_handler(void *arg)
{
	struct posix_process_ptexit_event_data *event_data;
	struct uk_thread *child;
	pid_t *clear_ref;
#if !CONFIG_LIBPOSIX_FUTEX_VAS
	struct futex_guard guard;
#endif

	UK_ASSERT(arg);

	event_data = (struct posix_process_ptexit_event_data *)arg;
	child = event_data->thread;
	clear_ref = (pid_t *)uk_thread_uktls_var(child, child_tid_clear_ref);
	uk_thread_uktls_var(child, child_tid_clear_ref) = NULL;

#if CONFIG_LIBPOSIX_FUTEX_VAS
	futex_vas_retire_wait(child);
	if (clear_ref) {
		struct uk_vas *vas = uk_vas_get(child->vas ? child->vas :
					       uk_vas_get_kernel());

		/* The child may use a different VAS from the exiting caller. */
		if (!uk_vas_store_u32(vas, (__vaddr_t)clear_ref, 0))
			(void)futex_vas_wake(vas, (__u32 *)clear_ref, 1,
					    FUTEX_BITSET_MATCH_ANY, false);
		uk_vas_put(vas);
	}
#else
	/* Retire the old registration before publishing the exit notification. */
	futex_guard_lock(&guard);
	if (child->wait_ticket.wq == &futex_waiters)
		uk_waitq_cancel(child);

	/* Linux clear-child-tid publishes zero and wakes one join waiter. */
	if (clear_ref != NULL) {
		uk_store_n(clear_ref, 0);
		futex_wake_locked((uint32_t *)clear_ref, 1);
	}
	futex_guard_unlock(&guard);
#endif
	return 0;
}

POSIX_PROCESS_PTEXIT_HANDLER_PRIO(pthread_exit_handler, UK_PRIO_EARLIEST);

#endif /* CONFIG_LIBPOSIX_PROCESS_MULTITHREADING */
