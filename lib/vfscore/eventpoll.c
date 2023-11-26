/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *
 * Copyright (c) 2021, Karlsruhe Institute of Technology (KIT)
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

#include <vfscore/eventpoll.h>
#include <vfscore/file.h>
#include <vfscore/dentry.h>
#include <vfscore/vnode.h>
#include <uk/essentials.h>
#include <uk/plat/time.h>
#include <uk/print.h>
#include <uk/list.h>
#include <errno.h>

/* #define UK_DEBUG_TRACE */
#include <uk/trace.h>

#define EPERR_SET (EPOLLERR | EPOLLHUP | EPOLLNVAL)

UK_TRACEPOINT(trace_ep_wait, "%p %p", void *, void *);
UK_TRACEPOINT(trace_ep_wakeup, "%p %p %u", void *, void *, unsigned int);

UK_TRACEPOINT(trace_efd_notify_close, "%p %u", void *, unsigned int);
UK_TRACEPOINT(trace_efd_signal, "%p %u %u %u", void *, unsigned int,
	      unsigned int, unsigned int);
UK_TRACEPOINT(trace_efd_poll, "%p %u %u %u %d", void *, unsigned int,
	      unsigned int, unsigned int, int);
UK_TRACEPOINT(trace_efd_add, "%p %u %d", void *, unsigned int, int);
UK_TRACEPOINT(trace_efd_del, "%p %u", void *, unsigned int);

void eventpoll_fini(struct eventpoll *ep)
{
	struct eventpoll_fd *efd;
	struct uk_list_head *itr, *tmp;

	UK_ASSERT(ep);
	UK_ASSERT(uk_waitq_empty(&ep->wq));

	/* At this point there must be no external references to the eventpoll
	 * except the ones from drivers that want to signal new events and
	 * from the files so that they can inform us if they are closed.
	 *
	 * N.B. Concurrent calls to eventpoll_signal() must be serialized by
	 * the driver against the driver's unregister()-callback.
	 */
	uk_list_for_each_safe(itr, tmp, &ep->fd_list) {
		efd = uk_list_entry(itr, struct eventpoll_fd, fd_link);

		eventpoll_del_unsafe(efd);

		if (ep->a)
			uk_free(ep->a, efd);
	}

	UK_ASSERT(uk_list_empty(&ep->fd_list));
}

static struct eventpoll_fd *efd_find(struct eventpoll *ep, int fd)
{
	struct eventpoll_fd *efd;
	struct uk_list_head *itr;

	UK_ASSERT(ep);

	uk_list_for_each(itr, &ep->fd_list) {
		efd = uk_list_entry(itr, struct eventpoll_fd, fd_link);

		if (efd->fd == fd)
			return efd;
	}

	return NULL;
}

static int efd_poll(struct eventpoll_fd *efd, unsigned int *revents)
{
	struct vnode *vnode;
	unsigned int filter;
	int ret;

	UK_ASSERT(efd);
	UK_ASSERT(revents);

	UK_ASSERT(efd->vfs_file);
	vnode = efd->vfs_file->f_dentry->d_vnode;

	filter = (unsigned int)efd->event.events | EPERR_SET;

	UK_ASSERT(vnode->v_op->vop_poll);
	ret = VOP_POLL(vnode, revents, &efd->cb);
	trace_efd_poll(efd->ep, efd->fd, *revents, *revents & filter, ret);
	if (unlikely(ret))
		return ret;

	/* Overlap the actual event bitmap with the requested event mask
	 * to get the events the consumer is interested in.
	 */
	*revents &= filter;

	return 0;
}

void eventpoll_add_unsafe(struct eventpoll *ep, struct eventpoll_fd *efd)
{
	UK_ASSERT(ep);
	UK_ASSERT(efd);
	UK_ASSERT(uk_list_empty(&efd->fd_link));
	UK_ASSERT(!efd_find(ep, efd->fd));

	efd->ep = ep;

	UK_ASSERT(efd->vfs_file);
	UK_ASSERT(efd->vfs_file->f_dentry);
	UK_ASSERT(efd->vfs_file->f_dentry->d_vnode);

	uk_list_add_tail(&efd->f_link, &efd->vfs_file->f_ep);
	uk_list_add_tail(&efd->fd_link, &ep->fd_list);

	trace_efd_add(ep, efd->fd, efd->vfs_file->f_dentry->d_vnode->v_type);
}

int eventpoll_add(struct eventpoll *ep, int fd, struct vfscore_file *fp,
		  const struct epoll_event *event)
{
	struct eventpoll_fd *efd;
	unsigned int revents = 0;
	int ret;

	UK_ASSERT(ep);
	UK_ASSERT(fp);

	if (unlikely(!event))
		return -EINVAL;

	/* Allocate and initialize a new eventpoll fd
	 *
	 * TODO:
	 * We must not increment the reference count of fp here because this
	 * would prevent fp from being closed. However, it would be good to
	 * have the memory reference counted separately so that the fd is
	 * removed from the interest list as soon as all true references
	 * (i.e., all fds to the vfs file fp) have been closed, but the memory
	 * is only freed when the fp is also removed from the interest list
	 * (and not pointed to anywhere else).
	 */
	UK_ASSERT(ep->a);
	efd = uk_malloc(ep->a, sizeof(*efd));
	if (unlikely(!efd))
		return -ENOMEM;

	eventpoll_fd_init(efd, fp, fd, event);

	uk_mutex_lock(&ep->fd_lock);

	/* fds must not be added multiple times */
	if (unlikely(efd_find(ep, fd))) {
		ret = -EEXIST;
		goto ERR_EXIT_FREE;
	}

	efd->ep = ep;

	/* We need to poll the fd here so if there is currently a wait
	 * pending, the driver is informed that it should signal events for
	 * this fd. Note from this point on, we might be getting signaled by
	 * the driver.
	 */
	ret = efd_poll(efd, &revents);
	if (unlikely(ret))
		goto ERR_EXIT_FREE;

	/* Add fd to event poll and the underlying file */
	eventpoll_add_unsafe(ep, efd);

	/* We also have to wake up any waiters if the fd is already ready */
	if (revents & efd->event.events) {
		trace_efd_signal(ep, efd->fd, revents,
				 revents & efd->event.events);

		if (uk_list_empty(&efd->tr_link))
			uk_list_add_tail(&efd->tr_link, &ep->tr_list);

		uk_waitq_wake_up(&ep->wq);
	}

EXIT:
	uk_mutex_unlock(&ep->fd_lock);
	return ret;
ERR_EXIT_FREE:
	uk_free(ep->a, efd);
	goto EXIT;
}

int eventpoll_mod(struct eventpoll *ep, int fd, const struct epoll_event *event)
{
	struct eventpoll_fd *efd;
	unsigned int revents = 0;
	int ret;

	UK_ASSERT(ep);

	if (unlikely(!event))
		return -EINVAL;

	uk_mutex_lock(&ep->fd_lock);

	efd = efd_find(ep, fd);
	if (unlikely(!efd)) {
		ret = -ENOENT;
		goto EXIT;
	}

	efd->event = *event;

	/* We need to poll the fd here to check if the new configuration
	 * influences the triggered state of the fd. In that case, we wake up
	 * any potential waiters.
	 */
	ret = efd_poll(efd, &revents);
	if (unlikely(ret))
		goto EXIT;

	if (revents & efd->event.events) {
		trace_efd_signal(ep, efd->fd, revents,
				 revents & efd->event.events);

		if (uk_list_empty(&efd->tr_link))
			uk_list_add_tail(&efd->tr_link, &ep->tr_list);

		uk_waitq_wake_up(&ep->wq);
	}

EXIT:
	uk_mutex_unlock(&ep->fd_lock);
	return ret;
}

void eventpoll_del_unsafe(struct eventpoll_fd *efd)
{
	UK_ASSERT(efd);
	UK_ASSERT(efd->ep);
	UK_ASSERT(!uk_list_empty(&efd->fd_link));
	UK_ASSERT(efd_find(efd->ep, efd->fd));

	if (efd->cb.unregister)
		efd->cb.unregister(&efd->cb);

	uk_list_del(&efd->f_link);
	uk_list_del(&efd->tr_link);
	uk_list_del(&efd->fd_link);

	trace_efd_del(efd->ep, efd->fd);

	efd->ep = NULL;
}

int eventpoll_del(struct eventpoll *ep, int fd)
{
	struct eventpoll_fd *efd;
	int ret = 0;

	UK_ASSERT(ep);

	uk_mutex_lock(&ep->fd_lock);

	efd = efd_find(ep, fd);
	if (unlikely(!efd)) {
		ret = -ENOENT;
		goto EXIT;
	}

	eventpoll_del_unsafe(efd);

	/* TODO: If we count memory references to vfs_files, we have to
	 * put fp here
	 */

	UK_ASSERT(ep->a);
	uk_free(ep->a, efd);
EXIT:
	uk_mutex_unlock(&ep->fd_lock);
	return ret;
}

void eventpoll_notify_close(struct vfscore_file *fp)
{
	struct eventpoll_fd *efd;
	struct uk_list_head *itr;
	struct uk_list_head *tmp;
	struct eventpoll *ep;

	UK_ASSERT(fp);

	/* At this point we know there are no open references to the file
	 * except from eventpolls
	 */
	uk_list_for_each_safe(itr, tmp, &fp->f_ep) {
		efd = uk_list_entry(itr, struct eventpoll_fd, f_link);

		UK_ASSERT(efd->ep);
		ep = efd->ep;

		trace_efd_notify_close(ep, efd->fd);

		uk_mutex_lock(&ep->fd_lock);

		eventpoll_del_unsafe(efd);

		/* If this was the last fd in the eventpoll, wake up any
		 * waiters so that they are not stuck forever
		 */
		if (uk_list_empty(&ep->fd_list))
			uk_waitq_wake_up(&ep->wq);

		uk_mutex_unlock(&ep->fd_lock);

		if (ep->a)
			uk_free(ep->a, efd);
	}
}

int eventpoll_wait(struct eventpoll *ep, struct epoll_event *events,
		   int maxevents, const __nsec *timeout)
{
	struct eventpoll_fd *efd;
	struct uk_list_head *itr, *tmp;
	unsigned int revents = 0;
	__nsec deadline;
	int timedout;
	int ret, n = 0;

	UK_ASSERT(ep);

	deadline = (timeout) ? ukplat_monotonic_clock() + *timeout : 0;

	uk_mutex_lock(&ep->fd_lock);

	for (;;) {
		UK_ASSERT(n == 0);

		/* Find triggered fds and poll their state. We have to redo
		 * the poll although the fd is already in the triggered list
		 * because the condition could have changed in the meantime.
		 * For instance, if the fd is level-triggered, we keep it
		 * in the triggered list because we don't know if the caller
		 * will actually perform the available operations (e.g., read
		 * _all_ pending data). So we poll the fd again to determine if
		 * the state has changed. If the fd is not actually ready, we
		 * remove it from the list. It is then added to the list again
		 * in two cases:
		 *   1) the fd is ready during eventpoll_mod
		 *   2) the driver signals an event
		 */
		uk_list_for_each_safe(itr, tmp, &ep->tr_list) {
			efd = uk_list_entry(itr, struct eventpoll_fd, tr_link);

			if (n == maxevents)
				break;

			ret = efd_poll(efd, &revents);
			if (unlikely(ret))
				revents = EPOLLERR;

			if (revents) {
				UK_ASSERT(events);

				events[n].events = (uint32_t)revents;
				events[n].data = efd->event.data;
				n++;

				/* If the fd is edge-triggered we remove the fd
				 * from the triggered list
				 */
				if (efd->event.events & EPOLLET)
					uk_list_del_init(&efd->tr_link);
			} else {
				/* The fd is not actually ready so remove it
				 * from the triggered list
				 */
				uk_list_del_init(&efd->tr_link);
			}
		}

		if (n > 0)
			break;

		trace_ep_wait(ep, uk_thread_current());

		timedout = uk_waitq_wait_event_deadline_mutex(&ep->wq,
				!uk_list_empty(&ep->fd_list) &&
				!uk_list_empty(&ep->tr_list), deadline,
				&ep->fd_lock);

		trace_ep_wakeup(ep, uk_thread_current(), (timedout) ? 1 : 0);

		if (timedout || uk_list_empty(&ep->fd_list))
			break;
	}

	uk_mutex_unlock(&ep->fd_lock);

	return n;
}

void eventpoll_signal(struct eventpoll_cb *ecb, unsigned int revents)
{
	struct eventpoll *ep;
	struct eventpoll_fd *efd;
	unsigned int filtered;

	UK_ASSERT(ecb);

	efd = __containerof(ecb, struct eventpoll_fd, cb);
	UK_ASSERT(efd->ep);
	ep = efd->ep;

	filtered = revents & ((unsigned int)efd->event.events | EPERR_SET);

	uk_mutex_lock(&ep->fd_lock);

	trace_efd_signal(ep, efd->fd, revents, filtered);

	if (filtered) {
		if (uk_list_empty(&efd->tr_link))
			uk_list_add_tail(&efd->tr_link, &ep->tr_list);

		uk_waitq_wake_up(&ep->wq);
	}

	uk_mutex_unlock(&ep->fd_lock);
}
