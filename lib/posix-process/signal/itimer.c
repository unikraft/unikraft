/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, The Unikraft Authors. All rights reserved. */

#include <errno.h>
#include <string.h>
#include <uk/config.h>
#include <uk/init.h>
#include <uk/plat/time.h>
#include <uk/process/itimer.h>
#include <uk/sched.h>
#if CONFIG_LIBUKVMEM_VAS_ACCESS && CONFIG_LIBUKVMEM_VAS_REFCOUNT && \
	CONFIG_LIBUKVMEM_THREAD_VAS
#include <uk/vmem.h>
#endif

#include "signal.h"

#define ITIMER_USEC_PER_SEC 1000000UL
#define ITIMER_NSEC_PER_USEC 1000ULL
#define ITIMER_RETRY_NSEC 1000000ULL

static struct uk_thread *itimer_service;

static __nsec bounded_add(__nsec base, __nsec amount)
{
	if (base >= (__nsec)__SNSEC_MAX)
		return __SNSEC_MAX;
	return amount > (__nsec)__SNSEC_MAX - base ?
		(__nsec)__SNSEC_MAX : base + amount;
}

static int timeval_nsec(const struct timeval *value, __nsec *out)
{
	__nsec seconds, fraction;

	if (value->tv_sec < 0 ||
	    (unsigned long)value->tv_usec >= ITIMER_USEC_PER_SEC)
		return -EINVAL;
	seconds = value->tv_sec;
	fraction = value->tv_usec * ITIMER_NSEC_PER_USEC;
	if (seconds > (__nsec)__SNSEC_MAX / UKARCH_NSEC_PER_SEC)
		*out = __SNSEC_MAX;
	else
		*out = bounded_add(seconds * UKARCH_NSEC_PER_SEC, fraction);
	return 0;
}

static void nsec_timeval(__nsec value, struct timeval *out)
{
	out->tv_sec = value / UKARCH_NSEC_PER_SEC;
	out->tv_usec = (value % UKARCH_NSEC_PER_SEC) / ITIMER_NSEC_PER_USEC;
}

static __nsec timer_snapshot_locked(struct posix_process *process,
				    struct itimerval *out, __nsec now)
{
	__nsec remaining = 0;

	if (process->real_timer.active) {
		if (process->real_timer.deadline > now)
			remaining = process->real_timer.deadline - now;
		else
			remaining = ITIMER_NSEC_PER_USEC;
	}
	nsec_timeval(remaining, &out->it_value);
	nsec_timeval(process->real_timer.interval, &out->it_interval);
	return remaining;
}

static int timer_change(struct posix_process *process,
			const struct itimerval *value, struct itimerval *old,
			__nsec *remaining)
{
	__nsec delay, interval, now, before;
	unsigned long flags;
	int result;

	result = timeval_nsec(&value->it_value, &delay);
	if (result)
		return result;
	result = timeval_nsec(&value->it_interval, &interval);
	if (result)
		return result;
	flags = pprocess_lock_acquire();
	if (process->exiting || !itimer_service) {
		pprocess_lock_release(flags);
		return -ESRCH;
	}
	now = ukplat_monotonic_clock();
	before = timer_snapshot_locked(process, old, now);
	process->real_timer.generation++;
	process->real_timer.deadline = delay ? bounded_add(now, delay) : 0;
	process->real_timer.interval = delay ? interval : 0;
	process->real_timer.active = delay != 0;
	process->real_timer.pending = false;
	uk_thread_wake(itimer_service);
	pprocess_lock_release(flags);
	if (remaining)
		*remaining = before;
	return 0;
}

static int timer_kind(int which)
{
	if (which == ITIMER_REAL)
		return 0;
	if (which == ITIMER_VIRTUAL || which == ITIMER_PROF)
		return -ENOTSUP;
	return -EINVAL;
}

static int timer_copy(struct posix_process *process, void *resident,
		      void *user, bool to_user)
{
	if (!user)
		return -EFAULT;
#if CONFIG_LIBUKVMEM_VAS_ACCESS && CONFIG_LIBUKVMEM_VAS_REFCOUNT && \
	CONFIG_LIBUKVMEM_THREAD_VAS
	if (uk_vas_is_owned(process->vas)) {
		int result = to_user ?
			uk_vas_copy_to(process->vas, (__vaddr_t)user,
				       resident,
				       sizeof(struct itimerval)) :
			uk_vas_copy_from(process->vas, resident,
					 (__vaddr_t)user,
					 sizeof(struct itimerval));

		return result ? -EFAULT : 0;
	}
#else
	(void)process;
#endif
	if (to_user)
		memcpy(user, resident, sizeof(struct itimerval));
	else
		memcpy(resident, user, sizeof(struct itimerval));
	return 0;
}

int uk_process_setitimer(int which, const struct itimerval *value,
			 struct itimerval *old)
{
	struct posix_process *process = uk_pthread_current()->process;
	struct itimerval input = { 0 }, previous;
	int result = timer_kind(which);

	if (result)
		return result;
	if (value) {
		result = timer_copy(process, &input, (void *)value, false);
		if (result)
			return result;
	}
	result = timer_change(process, &input, &previous, NULL);
	if (!result && old)
		result = timer_copy(process, &previous, old, true);
	return result;
}

int uk_process_getitimer(int which, struct itimerval *value)
{
	struct posix_process *process = uk_pthread_current()->process;
	struct itimerval current;
	unsigned long flags;
	int result = timer_kind(which);

	if (result)
		return result;
	flags = pprocess_lock_acquire();
	timer_snapshot_locked(process, &current, ukplat_monotonic_clock());
	pprocess_lock_release(flags);
	return timer_copy(process, &current, value, true);
}

unsigned int uk_process_alarm(unsigned int seconds)
{
	struct posix_process *process = uk_pthread_current()->process;
	struct itimerval value = { .it_value.tv_sec = seconds }, old;
	__nsec remaining, fraction;
	unsigned int previous;

	if (timer_change(process, &value, &old, &remaining))
		return 0;
	previous = remaining / UKARCH_NSEC_PER_SEC;
	fraction = remaining % UKARCH_NSEC_PER_SEC;
	if ((!previous && fraction) || fraction >= UKARCH_NSEC_PER_SEC / 2)
		previous++;
	return previous;
}

void pprocess_itimer_dequeued_locked(struct posix_process *process)
{
	__nsec now, interval, steps;

	if (process->exiting || process->real_timer.active ||
	    process->real_timer.pending || !process->real_timer.interval)
		return;
	now = ukplat_monotonic_clock();
	interval = process->real_timer.interval;
	steps = process->real_timer.deadline > now ? 0 :
		(now - process->real_timer.deadline) / interval + 1;
	if (steps > ((__nsec)__SNSEC_MAX - process->real_timer.deadline) /
	    interval)
		return;
	process->real_timer.deadline += steps * interval;
	process->real_timer.active = true;
	uk_thread_wake(itimer_service);
}

/* The caller retains the selected process across allocation and enqueue. */
static void timer_dispatch(struct posix_process *process, __u64 generation)
{
	struct uk_signal *signal = uk_signal_alloc(process->_a);
	unsigned long flags;
	bool queued = false;
	int result = -EAGAIN;

	if (signal) {
		signal->siginfo.si_signo = SIGALRM;
		signal->siginfo.si_code = SI_KERNEL;
	}
	flags = pprocess_lock_acquire();
	if (!process->exiting && process->state == POSIX_PROCESS_RUNNING &&
	    process->real_timer.generation == generation &&
	    process->real_timer.pending) {
		if (pprocess_signal_should_drop(process, SIGALRM)) {
			result = 0;
		} else if (signal) {
			result = pprocess_signal_queue_locked(process, NULL,
							      signal, &queued);
		}
		if (result)
			process->real_timer.retry =
				bounded_add(ukplat_monotonic_clock(),
					    ITIMER_RETRY_NSEC);
		else
			process->real_timer.pending = false;
	}
	pprocess_lock_release(flags);
	if (signal && !queued)
		uk_signal_free(signal->alloc, signal);
}

static __noreturn void timer_service_loop(void *unused __unused)
{
	struct uk_thread *self = uk_thread_current();
	struct posix_process *process, *selected;
	unsigned long flags;
	__nsec now, next, deadline;
	__u64 generation;

	for (;;) {
		flags = pprocess_lock_acquire();
		now = ukplat_monotonic_clock();
		next = __NSEC_MAX;
		selected = NULL;
		uk_pprocess_foreach(process) {
			if (process->exiting ||
			    process->state != POSIX_PROCESS_RUNNING)
				continue;
			if (process->real_timer.active &&
			    process->real_timer.deadline <= now) {
				process->real_timer.active = false;
				process->real_timer.pending = true;
				process->real_timer.retry = now;
			}
			if (process->real_timer.pending)
				deadline = process->real_timer.retry;
			else if (process->real_timer.active)
				deadline = process->real_timer.deadline;
			else
				continue;
			if (deadline < next) {
				selected = process;
				next = deadline;
			}
		}
		if (!selected || next > now) {
			uk_thread_block_until(self, selected ? next : 0);
			pprocess_lock_release(flags);
			uk_sched_yield();
			continue;
		}
		pprocess_get(selected);
		generation = selected->real_timer.generation;
		pprocess_lock_release(flags);
		timer_dispatch(selected, generation);
		pprocess_put(selected);
	}
}

static int timer_service_init(struct uk_init_ctx *context __unused)
{
	struct uk_thread *thread;
	unsigned long flags;

	thread = uk_sched_thread_create_fn1(uk_sched_current(),
					    timer_service_loop, NULL, 0, 0,
					    false, false, "process-itimer",
					    NULL, NULL);
	if (!thread)
		return -ENOMEM;
	flags = pprocess_lock_acquire();
	itimer_service = thread;
	pprocess_lock_release(flags);
	return 0;
}

uk_late_initcall(timer_service_init, 0x1);
