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

/* The events that put a ready fd in each set, as on Linux */
#define SELECT_READ   (UKFD_POLLIN|UKFD_POLL_ALWAYS)
#define SELECT_WRITE  (UKFD_POLLOUT|EPOLLERR)
#define SELECT_EXCEPT EPOLLPRI

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

static inline void copy_fdset(fd_set *dst, const fd_set *src)
{
	if (src)
		*dst = *src;
	else
		FD_ZERO(dst);
}

/* If hit is not zero and fd is in want, set fd in out and return 1.
 * If out is NULL, want must be empty.
 */
static inline int select_mark(int fd, fd_set *out, const fd_set *want,
			      unsigned int hit)
{
	if (!hit || !FD_ISSET(fd, want))
		return 0;
	FD_SET(fd, out);
	return 1;
}

/* Wait for events on ef. Write the time that is left back to timeout. */
static int select_wait(const struct uk_file *ef, struct epoll_event *ev,
		       int maxevents, struct timespec *timeout,
		       const struct uk_ksigset *sigset)
{
	__nsec t0;
	int ret;

	if (timeout)
		t0 = ukplat_monotonic_clock();
	if (sigset)
		ret = uk_sys_epoll_pwait2(ef, ev, maxevents, timeout,
					  sigset->ss, sigset->ss_len);
	else
		ret = uk_sys_epoll_pwait2(ef, ev, maxevents, timeout, NULL, 0);
	if (timeout) {
		__snsec waited = ukplat_monotonic_clock() - t0;
		__snsec left = uk_time_spec_to_nsec(timeout) - waited;

		*timeout = uk_time_spec_from_nsec(left > 0 ? left : 0);
	}
	return ret;
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
	fd_set want_rd, want_wr, want_ex;
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

	copy_fdset(&want_rd, readfds);
	copy_fdset(&want_wr, writefds);
	copy_fdset(&want_ex, exceptfds);

	/* Walk fds, add to epoll */
	ret = 0;
	monitored = 0;
	immediate = 0;
	FD_ZERO(&imm_rd);
	FD_ZERO(&imm_wr);
	for (int fd = 0; fd < nfds; fd++) {
		int r = FD_ISSET(fd, &want_rd) ? SELECT_READ : 0;
		int w = FD_ISSET(fd, &want_wr) ? SELECT_WRITE : 0;
		int x = FD_ISSET(fd, &want_ex) ? SELECT_EXCEPT : 0;

		if (r|w|x) {
			struct epoll_event ev = {
				.events = r|w|x,
				.data.fd = fd
			};

			/* select can ignore ERR or HUP of an fd not in the read
			 * set. Edge-triggered, these events stop one wait only.
			 */
			if (!r)
				ev.events |= EPOLLET;

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
		int nev;

		do {
			nev = select_wait(ef, ev, monitored, timeout, sigset);
			if (unlikely(nev < 0)) {
				ret = nev;
				break;
			}
			zero_fdsets(readfds, writefds, exceptfds);
			for (int i = 0; i < nev; i++) {
				int fd = ev[i].data.fd;
				unsigned int e = ev[i].events;

				ret += select_mark(fd, readfds, &want_rd,
						   e & SELECT_READ);
				ret += select_mark(fd, writefds, &want_wr,
						   e & SELECT_WRITE);
				ret += select_mark(fd, exceptfds, &want_ex,
						   e & SELECT_EXCEPT);
			}
		} while (!ret && nev);
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
