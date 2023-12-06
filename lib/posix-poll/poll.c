/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <poll.h>

#include <uk/essentials.h>
#include <uk/posix-poll.h>
#include <uk/timeutil.h>
#include <uk/syscall.h>

/* For performance we copy between epoll and poll events with no conversion. */
/* This assumes them to be equal, which we ensure with these asserts */
UK_CTASSERT(EPOLLIN == POLLIN);
UK_CTASSERT(EPOLLOUT == POLLOUT);
UK_CTASSERT(EPOLLPRI == POLLPRI);
UK_CTASSERT(EPOLLERR == POLLERR);
UK_CTASSERT(EPOLLHUP == POLLHUP);
UK_CTASSERT(EPOLLRDHUP == POLLRDHUP);
UK_CTASSERT(EPOLLRDNORM == POLLRDNORM);
UK_CTASSERT(EPOLLRDBAND == POLLRDBAND);
UK_CTASSERT(EPOLLWRNORM == POLLWRNORM);
UK_CTASSERT(EPOLLWRBAND == POLLWRBAND);

/* HACK: Because we need to support both vfscore and uk_file, we need to defer
 * to epoll(), which can handle both. This has the limitation of not supporting
 * multiple pollfd entries with the same fd, in which case we fail early with
 * -ENOSYS.
 * Once vfscore shim is no longer needed, we can switch to a more efficient
 * and correct bespoke implementation.
 */

/* Internal syscalls */

int uk_sys_ppoll(struct pollfd *fds, nfds_t nfds,
		 const struct timespec *timeout,
		 const sigset_t *sigmask, size_t sigsetsize)
{
	const struct uk_file *ef;
	int ret;
	int monitored;

	if (unlikely(!fds))
		return -EFAULT;
	if (timeout && uk_time_spec_to_nsec(timeout) < 0)
		return -EINVAL;
	if (unlikely(sigmask)) {
		uk_pr_warn_once("STUB: ppoll no sigmask support\n");
		return -ENOSYS;
	}

	ef = uk_epollfile_create();
	if (unlikely(!ef))
		return -ENOMEM;

	/* Register fds & prepare output */
	ret = 0;
	monitored = 0;
	for (nfds_t i = 0; i < nfds; i++) {
		struct pollfd *p = &fds[i];
		int r = 0;

		if (p->fd > 0) {
			struct epoll_event ev = {
				.events = p->events,
				.data.fd = p->fd
			};

			r = uk_sys_epoll_ctl(ef, EPOLL_CTL_ADD, p->fd, &ev);
			monitored++;
		}
		if (unlikely(r))
			switch (r) {
			case -EBADF:
				p->revents = POLLNVAL;
				ret++;
				break;
			case -EEXIST:
				uk_pr_warn("Duplicate fd in poll: %d\n", p->fd);
				ret = -ENOSYS;
				goto out;
			default:
				ret = r;
				goto out;
			}
		else
			p->revents = 0;
	}
	if (!monitored)
		monitored = 1; /* Need at least 1 return entry */

	/* Wait */
	if (!ret) {
		struct epoll_event ev[monitored];
		nfds_t pi;

		ret = uk_sys_epoll_pwait2(ef, ev, monitored, timeout,
					  sigmask, sigsetsize);
		/* Process epoll() output */
		/*
		 * HACK: this relies on epoll returning entries in the order
		 * of registration; currently this is the case, but beware.
		 */
		pi = 0;
		for (int ei = 0; ei < ret; ei++) {
			while (pi < nfds && fds[pi].fd != ev[ei].data.fd)
				pi++;
			if (unlikely(pi == nfds)) {
				uk_pr_err("Bad fd returned from epoll: %d\n",
					  ev[ei].data.fd);
				ret = ei;
				break;
			}
			fds[pi].revents = ev[ei].events;
		}
	}
out:
	uk_file_release(ef);
	return ret;
}

/* Userspace syscalls */

UK_SYSCALL_R_DEFINE(int, ppoll, struct pollfd *, fds, nfds_t, nfds,
		    const struct timespec *, timeout,
		    const sigset_t *, sigmask, size_t, sigsetsize)
{
	return uk_sys_ppoll(fds, nfds, timeout, sigmask, sigsetsize);
}

UK_SYSCALL_R_DEFINE(int, poll, struct pollfd *, fds, nfds_t, nfds, int, timeout)
{
	return uk_sys_ppoll(
		fds, nfds,
		(timeout < 0) ? NULL : &uk_time_spec_from_msec(timeout),
		NULL, 0
	);
}
