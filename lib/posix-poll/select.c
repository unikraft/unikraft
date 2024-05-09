/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/essentials.h>
#include <uk/posix-fd.h>
#include <uk/posix-poll.h>
#include <uk/timeutil.h>
#include <uk/syscall.h>

/* HACK: Because we need to support both vfscore and uk_file, we need to defer
 * to epoll(), which can handle both. Once vfscore shim is no longer needed, we
 * can switch to a more efficient bespoke implementation.
 */

#define SELECT_READ   UKFD_POLLIN
#define SELECT_WRITE  UKFD_POLLOUT
#define SELECT_EXCEPT (EPOLLPRI|UKFD_POLL_ALWAYS)

/* Internal syscalls */

static inline void zero_fdsets(fd_set *restrict readfds,
			       fd_set *restrict writefds,
			       fd_set *restrict exceptfds)
{
	if (readfds)
		FD_ZERO(readfds);
	if (writefds)
		FD_ZERO(writefds);
	if (exceptfds)
		FD_ZERO(exceptfds);
}

int uk_sys_pselect(int nfds, fd_set *restrict readfds,
		   fd_set *restrict writefds,
		   fd_set *restrict exceptfds,
		   struct timespec *restrict timeout,
		   const struct uk_ksigset *sigset)
{
	const struct uk_file *ef;
	int ret;
	int monitored;
	int immediate;
	fd_set imm_rd;
	fd_set imm_wr;

	if (unlikely(nfds < 0))
		return -EINVAL;
	if (unlikely(timeout && uk_time_spec_to_nsec(timeout) < 0))
		return -EINVAL;
	if (unlikely(nfds > FD_SETSIZE)) {
		uk_pr_warn("STUB: select fd_set too small (req %d)\n", nfds);
		return -EINVAL;
	}
	if (unlikely(sigset && sigset->ss)) {
		uk_pr_warn_once("STUB: pselect no sigmask support\n");
		return -ENOSYS;
	}

	ef = uk_epollfile_create();
	if (unlikely(!ef))
		return -ENOMEM;

	/* Walk fds, add to epoll */
	ret = 0;
	monitored = 0;
	immediate = 0;
	FD_ZERO(&imm_rd);
	FD_ZERO(&imm_wr);
	for (int fd = 0; fd < nfds; fd++) {
		int r = 0, w = 0, x = 0;

		if (readfds)
			r = FD_ISSET(fd, readfds) ? SELECT_READ : 0;
		if (writefds)
			w = FD_ISSET(fd, writefds) ? SELECT_WRITE : 0;
		if (exceptfds)
			x = FD_ISSET(fd, exceptfds) ? SELECT_EXCEPT : 0;

		if (r|w|x) {
			struct epoll_event ev = {
				.events = r|w|x,
				.data.fd = fd
			};

			ret = uk_sys_epoll_ctl(ef, EPOLL_CTL_ADD, fd, &ev);
			if (ret == -EPERM) {
				/* Files without epoll support return rd|wr */
				if (r|w)
					immediate = fd + 1;
				if (r)
					FD_SET(fd, &imm_rd);
				if (w)
					FD_SET(fd, &imm_wr);
				ret = 0;
			} else if (unlikely(ret)) {
				break;
			}
			monitored++;
		}
	}
	if (!monitored)
		monitored = 1;

	if (immediate) {
		ret = 0;
		zero_fdsets(readfds, writefds, exceptfds);

		for (int i = 0; i < immediate; i++) {
			if (FD_ISSET(i, &imm_rd)) {
				FD_SET(i, readfds);
				ret++;
			}
			if (FD_ISSET(i, &imm_wr)) {
				FD_SET(i, writefds);
				ret++;
			}
		}
	}

	if (!ret) {
		struct epoll_event ev[monitored];
		__nsec t0;

		if (timeout)
			t0 = ukplat_monotonic_clock();
		/* Wait */
		if (sigset)
			ret = uk_sys_epoll_pwait2(ef, ev, monitored, timeout,
						  sigset->ss, sigset->ss_len);
		else
			ret = uk_sys_epoll_pwait2(ef, ev, monitored, timeout,
						  NULL, 0);
		/* Writeout */
		if (timeout) {
			__snsec waited = ukplat_monotonic_clock() - t0;
			__snsec left = uk_time_spec_to_nsec(timeout) - waited;

			*timeout = uk_time_spec_from_nsec(left > 0 ? left : 0);
		}
		zero_fdsets(readfds, writefds, exceptfds);
		for (int i = 0; i < ret; i++) {
			int fd = ev[i].data.fd;

			if (readfds && ev[i].events & SELECT_READ)
				FD_SET(fd, readfds);
			if (writefds && ev[i].events & SELECT_WRITE)
				FD_SET(fd, writefds);
			if (exceptfds && ev[i].events & SELECT_EXCEPT)
				FD_SET(fd, exceptfds);
		}
	}

	uk_file_release(ef);
	return ret;
}

/* Userspace syscalls */

UK_SYSCALL_R_DEFINE(int, pselect6, int, nfds,
		    fd_set *restrict, readfds,
		    fd_set *restrict, writefds,
		    fd_set *restrict, exceptfds,
		    struct timespec *restrict, timeout,
		    const struct uk_ksigset *, sigset)
{
	return uk_sys_pselect(nfds, readfds, writefds, exceptfds,
			      timeout, sigset);
}

UK_SYSCALL_R_DEFINE(int, select, int, nfds,
		    fd_set *restrict, readfds,
		    fd_set *restrict, writefds,
		    fd_set *restrict, exceptfds,
		    struct timeval *restrict, timeout)
{
	if (timeout) {
		struct timespec ts = uk_time_spec_from_val(timeout);
		int r = uk_sys_pselect(nfds, readfds, writefds, exceptfds, &ts,
				       NULL);

		*timeout = uk_time_val_from_spec(&ts);
		return r;
	} else {
		return uk_sys_pselect(nfds, readfds, writefds, exceptfds, NULL,
				      NULL);
	}
}
