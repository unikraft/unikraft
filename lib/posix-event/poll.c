/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *
 * Copyright (c) 2021, Karlsruhe Insitute of Technology (KIT)
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

#define _GNU_SOURCE
#include <vfscore/file.h>
#include <vfscore/vnode.h>
#include <vfscore/eventpoll.h>
#include <uk/print.h>
#include <uk/syscall.h>
#include <uk/config.h>

#include <poll.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <limits.h>

static int do_ppoll(struct pollfd *fds, nfds_t nfds, const __nsec *timeout,
		    const sigset_t *sigmask, size_t sigsetsize __unused)
{
	struct epoll_event e;
	struct epoll_event *events = NULL;
	struct eventpoll ep;
	struct eventpoll_fd *efd;
	struct vfscore_file *fp;
	int ret, i, fd, num_fds = (int)nfds;

	if (unlikely(nfds > INT_MAX))
		return -EINVAL;

	eventpoll_init(&ep, uk_alloc_get_default());

	/* Register fds in eventpoll */
	for (i = 0; i < num_fds; i++) {
		fd = fds[i].fd;

		/* A negative fd means we should ignore it */
		if (fd < 0)
			continue;

		fp = vfscore_get_file(fd);
		if (unlikely(!fp)) {
			ret = -EBADF;
			goto EXIT;
		}

		efd = uk_malloc(uk_alloc_get_default(), sizeof(*efd));
		if (unlikely(!efd)) {
			vfscore_put_file(fp);
			ret = -ENOMEM;
			goto EXIT;
		}

		/* We can use the unsafe method of adding the fd to the
		 * eventpoll because we know that nobody except us may access
		 * the eventpoll.
		 */
		e.data.ptr = &fds[i].revents;
		e.events = fds[i].events;
		eventpoll_fd_init(efd, fp, fd, &e);
		eventpoll_add_unsafe(&ep, efd);

		/* We must add the fd to triggered list so it is
		 * checked in eventpoll_wait(). This is ok because the
		 * method will ignore the fd if it has no events
		 * pending.
		 */
		uk_list_add_tail(&efd->tr_link, &ep.tr_list);

		vfscore_put_file(fp);
	}

	/* We may call poll with no fds to just do a precise wait */
	if (num_fds > 0) {
		events = uk_calloc(uk_alloc_get_default(), num_fds,
				   sizeof(struct epoll_event));
		if (unlikely(!events)) {
			ret = -ENOMEM;
			goto EXIT;
		}
	}

	/* TODO: Implement atomic masking of signals */
	if (sigmask)
		uk_pr_warn_once("%s: signal masking not implemented.",
				__func__);

	ret = eventpoll_wait(&ep, events, num_fds, timeout);
	if (unlikely(ret < 0))
		goto ERR_FREE_EVENTS;

	for (i = 0; i < num_fds; i++)
		fds[i].revents = 0;

	UK_ASSERT(ret <= num_fds);

	for (i = 0; i < ret; i++) {
		UK_ASSERT(events[i].events);
		UK_ASSERT(events[i].data.ptr);

		/* We have a pointer to revents of the corresponding pollfd */
		*((short *)events[i].data.ptr) = events[i].events;
	}

ERR_FREE_EVENTS:
	if (events)
		uk_free(uk_alloc_get_default(), events);
EXIT:
	eventpoll_fini(&ep);
	return ret;
}

UK_SYSCALL_R_DEFINE(int, poll, struct pollfd *, fds, nfds_t, nfds,
		    int, timeout)
{
	__nsec timeout_ns;

	if (timeout >= 0)
		timeout_ns = ukarch_time_msec_to_nsec(timeout);

	return do_ppoll(fds, nfds, (timeout >= 0) ? &timeout_ns : NULL,
			NULL, 0);
}

UK_LLSYSCALL_R_DEFINE(int, ppoll, struct pollfd *, fds, nfds_t,  nfds,
		      const struct timespec *, timeout,
		      const sigset_t *, sigmask, size_t, sigsetsize)
{
	__nsec timeout_ns;

	if (timeout)
		timeout_ns = ukarch_time_sec_to_nsec(timeout->tv_sec) +
			timeout->tv_nsec;

	return do_ppoll(fds, nfds, (timeout) ? &timeout_ns : NULL,
			sigmask, sigsetsize);
}

#if UK_LIBC_SYSCALLS
/* The actual system call implemented in Linux uses a different signature! We
 * thus provide the libc call here directly.
 */
int ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout,
	  const sigset_t *sigmask)
{
	__nsec timeout_ns;
	int ret;

	if (timeout)
		timeout_ns = ukarch_time_sec_to_nsec(timeout->tv_sec) +
			timeout->tv_nsec;

	ret = do_ppoll(fds, nfds, (timeout) ? &timeout_ns : NULL,
		       sigmask, sizeof(sigset_t));
	if (unlikely(ret < 0)) {
		errno = -ret;
		ret = -1;
	}

	return ret;
}
#endif /* UK_LIBC_SYSCALLS */
