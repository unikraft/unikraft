/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* File event polling */

#ifndef __UK_POSIX_POLL_H__
#define __UK_POSIX_POLL_H__

#include <poll.h>
#include <sys/select.h>
#include <sys/epoll.h>

#include <uk/file.h>
#include <uk/posix-fdtab.h>
#include <uk/timeutil.h>


/* select */

/* Taken from pselect(2) manpage */
struct uk_ksigset {
	const sigset_t *ss;
	size_t ss_len;
};

int uk_sys_pselect(int nfds, fd_set *restrict readfds,
		   fd_set *restrict writefds,
		   fd_set *restrict exceptfds,
		   struct timespec *restrict timeout,
		   const struct uk_ksigset *sigset);


/* poll */

int uk_sys_ppoll(struct pollfd *fds, nfds_t nfds,
		 const struct timespec *timeout,
		 const sigset_t *sigmask, size_t sigsetsize);


/* epoll */

struct uk_file *uk_epollfile_create(void);

int uk_sys_epoll_create(int flags);

int uk_sys_epoll_ctl(const struct uk_file *epf, int op, int fd,
		     const struct epoll_event *event);

int uk_sys_epoll_pwait2(const struct uk_file *epf, struct epoll_event *events,
			int maxevents, const struct timespec *timeout,
			const sigset_t *sigmask, size_t sigsetsize);

static inline
int uk_sys_epoll_pwait(const struct uk_file *epf, struct epoll_event *events,
		       int maxevents, int timeout,
		       const sigset_t *sigmask, size_t sigsetsize)
{
	return uk_sys_epoll_pwait2(
		epf, events, maxevents,
		(timeout < 0) ? NULL : &uk_time_spec_from_msec(timeout),
		sigmask, sigsetsize
	);
}

#endif /* __UK_POSIX_POLL_H__ */
