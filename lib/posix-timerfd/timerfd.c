/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <sys/time.h>
#include <poll.h>

#include <uk/plat/time.h>
#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/file/nops.h>
#include <uk/posix-fd.h>
#include <uk/posix-fdtab.h>
#include <uk/posix-timerfd.h>
#include <uk/mutex.h>
#include <uk/sched.h>
#include <uk/timeutil.h>
#include <uk/syscall.h>


static const char TIMERFD_VOLID[] = "timerfd_vol";


struct timerfd_node {
	struct itimerspec set;
	__u64 val;
	clockid_t clkid;
	struct uk_thread *upthread;
};

struct timerfd_alloc {
	struct uk_alloc *alloc;
	struct uk_file f;
	uk_file_refcnt frefcnt;
	struct uk_file_state fstate;
	struct timerfd_node node;
};

struct timerfd_status {
	__u64 exp;
	__nsec next;
};

/* Internal */

static inline
struct timerfd_status _timerfd_valnext(const struct itimerspec *set,
				       const struct timespec *now)
{
	struct timerfd_status ret;
	__snsec passed = uk_time_spec_nsecdiff(&set->it_value, now);

	if (passed >= 0) {
		/* Now is after set */
		__nsec period = uk_time_spec_to_nsec(&set->it_interval);

		ret.exp = 1;
		if (period) {
			ret.exp += passed/period;
			ret.next = ret.exp * period - passed;
		} else {
			ret.next = 0;
		}
	} else {
		/* Now is before set */
		ret.exp = 0;
		ret.next = -passed;
	}
	return ret;
}

static __nsec _timerfd_update(const struct uk_file *f)
{
	__nsec deadline;
	struct timespec t;
	__nsec now;
	struct timerfd_status st;
	struct timerfd_node *d = (struct timerfd_node *)f->node;
	struct itimerspec set = d->set;

	/* Clear & sleep if timer disarmed */
	if (!set.it_value.tv_sec && !set.it_value.tv_nsec) {
		if (d->val) {
			uk_file_event_clear(f, UKFD_POLLIN);
			d->val = 0;
		}
		return 0;
	}

	/* Get time & decide value & deadline */
	uk_syscall_r_clock_gettime(d->clkid, (uintptr_t)&t);
	now = ukplat_monotonic_clock();
	st = _timerfd_valnext(&set, &t);
	deadline = now + st.next;

	/* Update val & events */
	if (st.exp != d->val) {
		d->val = st.exp;
		if (st.exp)
			uk_file_event_set(f, UKFD_POLLIN);
		else
			uk_file_event_clear(f, UKFD_POLLIN);
	}
	return deadline;
}

static void _timerfd_set(struct timerfd_node *d, const struct itimerspec *set)
{
	if (!set->it_value.tv_sec && !set->it_value.tv_nsec) {
		/* Disarm */
		if (d->set.it_value.tv_sec || d->set.it_value.tv_nsec) {
			d->set.it_value = set->it_value;
			uk_thread_wake(d->upthread);
		}
	} else {
		/* Arm */
		d->set.it_value = set->it_value;
		d->set.it_interval = set->it_interval;
		uk_thread_wake(d->upthread);
	}
}

/* Ops */

static ssize_t timerfd_read(const struct uk_file *f,
			    const struct iovec *iov, int iovcnt,
			    off_t off, long flags __unused)
{
	struct timerfd_node *d;
	__u64 v;

	if (unlikely(f->vol != TIMERFD_VOLID))
		return -EINVAL;
	if (unlikely(off != 0))
		return -EINVAL;
	if (unlikely(!iovcnt || iov[0].iov_len < sizeof(__u64)))
		return -EINVAL;
	if (unlikely(!iov[0].iov_base))
		return -EFAULT;

	d = (struct timerfd_node *)f->node;
	uk_file_event_clear(f, UKFD_POLLIN);
	v = ukarch_exchange_n(&d->val, 0);
	if (!v)
		return -EAGAIN;
	*(__u64 *)(iov[0].iov_base) = v;
	return sizeof(v);
}

static __noreturn void timerfd_updatefn(void *arg)
{
	__nsec deadline;
	const struct uk_file *f = (struct uk_file *)arg;

	UK_ASSERT(f->vol == TIMERFD_VOLID);
	for (;;) {
		uk_file_wlock(f);
		deadline = _timerfd_update(f);
		/* Unlock & wait */
		uk_thread_block_until(uk_thread_current(), deadline);
		uk_file_wunlock(f);

		uk_sched_yield();
	}
}

static void timerfd_release(const struct uk_file *f, int what)
{
	UK_ASSERT(f->vol == TIMERFD_VOLID);
	if (what & UK_FILE_RELEASE_RES) {
		struct timerfd_node *d = (struct timerfd_node *)f->node;

		/* Disarm */
		uk_file_rlock(f);
		uk_thread_terminate(d->upthread);
		uk_file_runlock(f);
		/* Collect thread */
		uk_thread_release(d->upthread);
	}
	if (what & UK_FILE_RELEASE_OBJ) {
		struct timerfd_alloc *al;

		al = __containerof(f, struct timerfd_alloc, f);
		uk_free(al->alloc, al);
	}
}

static const struct uk_file_ops timerfd_ops = {
	.read = timerfd_read,
	.write = uk_file_nop_write,
	.getstat = uk_file_nop_getstat,
	.setstat = uk_file_nop_setstat,
	.ctl = uk_file_nop_ctl
};

/* File creation */

struct uk_file *uk_timerfile_create(clockid_t id)
{
	struct uk_alloc *a;
	struct timerfd_alloc *al;
	struct uk_thread *ut;

	/* Check clock id */
	if (unlikely(uk_syscall_r_clock_getres(id, (uintptr_t)NULL)))
		return ERR2PTR(-EINVAL);

	/* Alloc stuff */
	a = uk_alloc_get_default();
	al = uk_malloc(a, sizeof(*al));
	if (unlikely(!al))
		return ERR2PTR(-ENOMEM);

	/* Fill in fields */
	al->alloc = a;
	al->node = (struct timerfd_node){
		.set = {
			.it_interval = {0, 0},
			.it_value = {0, 0},
		},
		.val = 0,
		.clkid = id,
		.upthread = NULL
	};
	al->fstate = UK_FILE_STATE_INITIALIZER(al->fstate);
	al->frefcnt = UK_FILE_REFCNT_INITIALIZER;
	al->f = (struct uk_file){
		.vol = TIMERFD_VOLID,
		.node = &al->node,
		.refcnt = &al->frefcnt,
		.state = &al->fstate,
		.ops = &timerfd_ops,
		._release = timerfd_release
	};

	/* Create update thread */
	ut = uk_sched_thread_create(
		uk_sched_current(),
		timerfd_updatefn, &al->f,
		"timerfd_update_thread"
	);
	if (unlikely(!ut)) {
		uk_free(a, al);
		return ERR2PTR(-ENODEV);
	}
	al->node.upthread = ut;

	return &al->f;
}

/* Internal API */

int uk_sys_timerfd_create(clockid_t id, int flags)
{
	int ret;
	struct uk_file *timerf;
	unsigned int mode = O_RDONLY|UKFD_O_NOSEEK;

	/* Get file */
	timerf = uk_timerfile_create(id);
	if (unlikely(PTRISERR(timerf)))
		return PTR2ERR(timerf);

	/* Register fd */
	if (flags & TFD_NONBLOCK)
		mode |= O_NONBLOCK;
	if (flags & TFD_CLOEXEC)
		mode |= O_CLOEXEC;
	ret = uk_fdtab_open(timerf, mode);
	uk_file_release(timerf);
	return ret;
}


int uk_sys_timerfd_settime(const struct uk_file *f, int flags,
			   const struct itimerspec *new_value,
			   struct itimerspec *old_value)
{
	struct timerfd_node *d;
	const struct itimerspec *set;
	struct itimerspec absset;
	const int disarm = !new_value->it_value.tv_sec &&
			   !new_value->it_value.tv_nsec;

	if (unlikely(flags & ~TFD_TIMER_ABSTIME))
		return -EINVAL;
	if (unlikely(f->vol != TIMERFD_VOLID))
		return -EINVAL;

	d = f->node;
	uk_file_wlock(f);
	if (disarm || flags & TFD_TIMER_ABSTIME) {
		set = new_value;
	} else {
		struct timespec t;

		uk_syscall_r_clock_gettime(d->clkid, (uintptr_t)&t);
		absset.it_interval = new_value->it_interval;
		absset.it_value = uk_time_spec_sum(&new_value->it_value, &t);
		set = &absset;
	}
	if (old_value)
		*old_value = d->set;
	_timerfd_set(d, set);
	(void)_timerfd_update(f);
	uk_file_wunlock(f);
	return 0;
}

int uk_sys_timerfd_gettime(const struct uk_file *f,
			   struct itimerspec *curr_value)
{
	struct timerfd_node *d;
	struct timespec t;
	struct timerfd_status st;
	struct itimerspec set;

	if (unlikely(f->vol != TIMERFD_VOLID))
		return -EINVAL;

	d = f->node;
	uk_file_rlock(f);
	uk_syscall_r_clock_gettime(d->clkid, (uintptr_t)&t);
	set = d->set;
	st = _timerfd_valnext(&set, &t);
	uk_file_runlock(f);

	curr_value->it_interval = set.it_interval;
	curr_value->it_value = uk_time_spec_from_nsec(st.next);
	return 0;
}

/* Syscalls */

UK_SYSCALL_R_DEFINE(int, timerfd_create, int, id, int, flags)
{
	return uk_sys_timerfd_create(id, flags);
}

UK_SYSCALL_R_DEFINE(int, timerfd_settime, int, fd, int, flags,
		    const struct itimerspec *, new_value,
		    struct itimerspec *, old_value)
{
	int r;
	struct uk_ofile *of;

	if (unlikely(!new_value))
		return -EFAULT;

	of = uk_fdtab_get(fd);
	if (unlikely(!of))
		return -EBADF;
	r = uk_sys_timerfd_settime(of->file, flags, new_value, old_value);
	uk_fdtab_ret(of);
	return r;
}

UK_SYSCALL_R_DEFINE(int, timerfd_gettime, int, fd,
		    struct itimerspec *, curr_value)
{
	int r;
	struct uk_ofile *of;

	if (unlikely(!curr_value))
		return -EFAULT;

	of = uk_fdtab_get(fd);
	if (unlikely(!of))
		return -EBADF;
	r = uk_sys_timerfd_gettime(of->file, curr_value);
	uk_fdtab_ret(of);
	return r;
}
