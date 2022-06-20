/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *          Hugo Lefeuvre <hugo.lefeuvre@neclab.eu>
 *
 * Copyright (c) 2021, Karlsruhe Insitute of Technology (KIT)
 * Copyright (c) 2020, NEC Laboratories Europe GmbH, NEC Corporation,
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

#define _GNU_SOURCE
#include <vfscore/file.h>
#include <vfscore/vnode.h>
#include <vfscore/eventpoll.h>
#include <uk/print.h>
#include <uk/syscall.h>
#include <uk/config.h>

#include <sys/select.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

/* Mapping between POLLIN, POLLOUT, and POLLPRI to EPOLL masks */
#define POLLIN_SET  (EPOLLRDNORM | EPOLLRDBAND | EPOLLIN  | EPOLLHUP | EPOLLERR)
#define POLLOUT_SET (EPOLLWRNORM | EPOLLWRBAND | EPOLLOUT | EPOLLERR)
#define POLLEX_SET  (EPOLLPRI)

static int do_pselect(int nfds, fd_set *readfds, fd_set *writefds,
		      fd_set *exceptfds, const __nsec *timeout,
		      const sigset_t *sigmask, size_t sigsetsize __unused)
{
	struct epoll_event e = {0};
	struct epoll_event *events = NULL;
	struct eventpoll ep;
	struct eventpoll_fd *efd;
	struct vfscore_file *fp;
	int num_fds = 0;
	int ret, i;

	if (unlikely(nfds < 0))
		return -EINVAL;

	eventpoll_init(&ep, uk_alloc_get_default());

	/* Register fds in eventpoll */
	for (i = 0; i < nfds; i++) {
		if (readfds && FD_ISSET(i, readfds))
			e.events |= POLLIN_SET;

		if (writefds && FD_ISSET(i, writefds))
			e.events |= POLLOUT_SET;

		if (exceptfds && FD_ISSET(i, exceptfds))
			e.events |= POLLEX_SET;

		if (e.events) {
			fp = vfscore_get_file(i);
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
			 * eventpoll because we know that nobody except us
			 * may access the eventpoll.
			 */
			e.data.fd = i;
			eventpoll_fd_init(efd, fp, i, &e);
			eventpoll_add_unsafe(&ep, efd);

			/* We must add the fd to triggered list so it is
			 * checked in eventpoll_wait(). This is ok because the
			 * method will ignore the fd if it has no events
			 * pending.
			 */
			uk_list_add_tail(&efd->tr_link, &ep.tr_list);

			vfscore_put_file(fp);

			num_fds++;
			e.events = 0;
		}
	}

	/* We may call select with no fds to just do a precise wait */
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
	if (ret < 0)
		goto ERR_FREE_EVENTS;

	if (readfds)
		FD_ZERO(readfds);
	if (writefds)
		FD_ZERO(writefds);
	if (exceptfds)
		FD_ZERO(exceptfds);

	/* Timeout */
	if (ret == 0)
		goto ERR_FREE_EVENTS;

	UK_ASSERT(ret <= num_fds);
	num_fds = ret;

	ret = 0;
	for (i = 0; i < num_fds; i++) {
		UK_ASSERT(events[i].events);
		UK_ASSERT(events[i].data.fd < nfds);

		if (readfds && (events[i].events & POLLIN_SET)) {
			FD_SET(events[i].data.fd, readfds);
			ret++;
		}
		if (writefds && (events[i].events & POLLOUT_SET)) {
			FD_SET(events[i].data.fd, writefds);
			ret++;
		}
		if (exceptfds && (events[i].events & POLLEX_SET)) {
			FD_SET(events[i].data.fd, exceptfds);
			ret++;
		}
	}

ERR_FREE_EVENTS:
	if (events)
		uk_free(uk_alloc_get_default(), events);
EXIT:
	eventpoll_fini(&ep);
	return ret;
}

UK_SYSCALL_R_DEFINE(int, select, int, nfds, fd_set *, readfds,
		    fd_set *, writefds, fd_set *, exceptfds,
		    struct timeval *, timeout)
{
	__nsec timeout_ns;

	if (timeout)
		timeout_ns = ukarch_time_sec_to_nsec(timeout->tv_sec) +
			ukarch_time_usec_to_nsec(timeout->tv_usec);

	return do_pselect(nfds, readfds, writefds, exceptfds,
			  (timeout) ? &timeout_ns : NULL, NULL, 0);
}

struct linux_sigset_t {
	const sigset_t *sigmask;
	size_t sigsetsize;
};

UK_LLSYSCALL_R_DEFINE(int, pselect6, int, nfds, fd_set *, readfds,
		      fd_set *, writefds, fd_set *, exceptfds,
		      const struct timespec *, timeout,
		      const struct linux_sigset_t *, sig)
{
	__nsec timeout_ns;

	if (timeout)
		timeout_ns = ukarch_time_sec_to_nsec(timeout->tv_sec) +
			timeout->tv_nsec;

	return do_pselect(nfds, readfds, writefds, exceptfds,
			  (timeout) ? &timeout_ns : NULL,
			  (sig) ? sig->sigmask : NULL,
			  (sig) ? sig->sigsetsize : 0);
}

#if UK_LIBC_SYSCALLS
/* The actual system call implemented in Linux uses a different signature! We
 * thus provide the libc call here directly.
 */
int pselect(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
	    const struct timespec *timeout, const sigset_t *sigmask)
{
	__nsec timeout_ns;
	int ret;

	if (timeout)
		timeout_ns = ukarch_time_sec_to_nsec(timeout->tv_sec) +
			timeout->tv_nsec;

	ret = do_pselect(nfds, readfds, writefds, exceptfds,
			 (timeout) ? &timeout_ns : NULL,
			 sigmask, sizeof(sigset_t));
	if (unlikely(ret < 0)) {
		errno = -ret;
		ret = -1;
	}

	return ret;
}
#endif /* UK_LIBC_SYSCALLS */
