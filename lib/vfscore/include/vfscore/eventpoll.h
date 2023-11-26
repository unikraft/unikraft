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

#ifndef _VFSCORE_EVENTPOLL_H_
#define _VFSCORE_EVENTPOLL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <uk/essentials.h>
#include <uk/arch/time.h>
#include <uk/arch/spinlock.h>
#include <uk/list.h>
#include <uk/mutex.h>
#include <uk/alloc.h>
#include <uk/assert.h>

#include <sys/epoll.h>
#include <stdint.h>

struct vfscore_file;

#ifndef EPOLLNVAL
#define EPOLLNVAL	0x00000020	/* Invalid request: fd not open */
#endif

/**
 * Eventpoll context block
 * Representation of an eventpoll file description towards VFS drivers
 */
struct eventpoll_cb {
	/* If a driver internally registers an eventpoll file description to be
	 * signaled on events, it must supply an unregister callback that
	 * safely removes the eventpoll file description from the driver's
	 * signal list (e.g., when the eventpoll is canceled).
	 *
	 * The driver must add the eventpoll file description, which this
	 * context block belongs to, to its internal signal only on the first
	 * call to VOP_POLL(). The driver can check the unregister callback
	 * field for NULL or use the additional data field for this purpose.
	 *
	 * After a call to the unregister callback the driver must not access
	 * this data structure anymore!
	 *
	 * The driver must serialize calls to eventpoll_signal() with the
	 * unregister operation to avoid races where an eventpoll is destroyed
	 * while the driver is signaling an event.
	 */
	void (*unregister)(struct eventpoll_cb *ecb);

	/* Optional context for driver */
	void *data;

	/* Optional link for the driver to add this cb into a list */
	struct uk_list_head cb_link;
};

/**
 * Eventpoll file description
 * Each VFS file description being watched by an eventpoll is represented with
 * this structure.
 */
struct eventpoll_fd {
	/* Context block for driver to implement asynchronous signaling */
	struct eventpoll_cb cb;

	/* Parent eventpoll */
	struct eventpoll *ep;

	/* Reference to the monitored file */
	struct vfscore_file *vfs_file;

	/* Due to dup() we can have multiple fds point to the same file. We
	 * thus have to store the fd that has been used to register the file
	 * for eventpoll. We use it in epoll_ctl() to match the requested fd.
	 */
	int fd;

	/* User-supplied requested events and data */
	struct epoll_event event;

	/* Used to link into monitored fd list */
	struct uk_list_head fd_link;

	/* Used to link into triggered list which is scanned by
	 * eventpoll_wait() to find pending events. Being in the list, does not
	 * guarantee that events are still pending.
	 */
	struct uk_list_head tr_link;

	/* Used to link into the file's epoll list so we are informed when
	 * the file is closed.
	 */
	struct uk_list_head f_link;
};

static inline void eventpoll_fd_init(struct eventpoll_fd *efd,
				     struct vfscore_file *fp, int fd,
				     const struct epoll_event *event)
{
	UK_ASSERT(efd);
	UK_ASSERT(fp);
	UK_ASSERT(event);

	/* NOTE: We must not use fp->fd as a VFS file might be referenced by
	 * multiple file descriptors, for example, after a dup call. We thus
	 * need to explicitly receive the fd to add.
	 */
	efd->vfs_file = fp;
	efd->fd = fd;

	efd->ep = NULL;
	efd->event = *event;

	efd->cb.unregister = NULL;
	efd->cb.data = NULL;
	UK_INIT_LIST_HEAD(&efd->cb.cb_link);

	UK_INIT_LIST_HEAD(&efd->fd_link);
	UK_INIT_LIST_HEAD(&efd->tr_link);
	UK_INIT_LIST_HEAD(&efd->f_link);
}

/** Eventpoll main structure */
struct eventpoll {
	/* Lock to serialize operations on the fd and tr lists.
	 * TODO: The single lock will be a bottleneck at some point if
	 * we have many fds watched, signaled, and waited for. Add a separate
	 * lock for the triggered list. We might also get into trouble if
	 * a thread is blocked in eventpoll_signal() and eventpoll_signal()
	 * is called from an IRQ handler. So the tr list should be protected
	 * with a reader-writer spinlock.
	 */
	struct uk_mutex fd_lock;

	/* List of monitored fds */
	struct uk_list_head fd_list;

	/* List of triggered fds */
	struct uk_list_head tr_list;

	/* Queue of threads waiting on this eventpoll */
	struct uk_waitq wq;

	/* Optional allocator to use for add/del operations */
	struct uk_alloc *a;
};

static inline void eventpoll_init(struct eventpoll *ep, struct uk_alloc *a)
{
	UK_ASSERT(ep);

	/* The allocator is optional (can be NULL) if only add/del_unsafe()
	 * are used and external references to all future eventpoll fds exist
	 */
	ep->a = a;

	uk_mutex_init(&ep->fd_lock);
	UK_INIT_LIST_HEAD(&ep->fd_list);
	UK_INIT_LIST_HEAD(&ep->tr_list);
	uk_waitq_init(&ep->wq);
}

/**
 * Finalize an eventpoll. This will remove all eventpoll file descriptions and
 * release them if the eventpoll makes explicit use of an allocator. When
 * calling this function, there must be no external references to the eventpoll
 * (e.g., by waiting threads) except the ones from drivers and from the linked
 * files. After calling this function it is safe to release the memory of the
 * eventpoll object.
 *
 * @param ep the eventpoll to finalize
 */
void eventpoll_fini(struct eventpoll *ep);

/**
 * Add a file description to the specified eventpoll. This will allocate and
 * initialize a new eventpoll file description and register it in the VFS
 * file's eventpoll list so it is closed when the VFS file is closed. The
 * function also performs an initial poll to the file's VFS driver to inform it
 * that we are interested in events for this fd.
 *
 * @param ep the eventpoll to add the file description to
 * @param fd the file descriptor to add
 * @param fp the VFS file object that the file descriptor references. Note that
 *    we cannot infer fd from fp as there can be multiple fds pointing to fp
 *    after a dup call
 * @param event the epoll event that describes the events of interest
 *
 * @return 0 on success, a negative errno value otherwise
 */
int eventpoll_add(struct eventpoll *ep, int fd, struct vfscore_file *fp,
		  const struct epoll_event *event);

/**
 * Update the epoll event for a watched file description. This will trigger a
 * poll on the underlying VFS file to check if there are pending events based
 * on the updated epoll event object.
 *
 * @param ep the eventpoll to which the watched file descriptor belongs
 * @param fd the file descriptor for which to update the epoll event object
 * @param event the updated epoll event object
 *
 * @return 0 on success, a negative errno value otherwise
 */
int eventpoll_mod(struct eventpoll *ep, int fd,
		  const struct epoll_event *event);

/**
 * Remove a file description from an eventpoll. This will call the unregister
 * function supplied by the driver and also remove the eventpoll from the
 * respective VFS file.
 *
 * @param ep the eventpoll from which to the remove the file description from
 * @param fd the file descriptor to remove
 *
 * @return 0 on success, a negative errno value otherwise
 */
int eventpoll_del(struct eventpoll *ep, int fd);

/**
 * Add a prepared eventpoll file description to the given eventpoll. This will
 * only ensure proper linking into the eventpoll and VFS file objects without
 * taking any locks. The caller must ensure correct synchronization.
 *
 * @param ep the eventpoll to add the file description to
 * @param efd the eventpoll file description to add. It must be prepared with
 *    eventpoll_fd_init(). If the eventpoll has been supplied an allocator,
 *    the file description must have been allocated with the same allocator
 */
void eventpoll_add_unsafe(struct eventpoll *ep, struct eventpoll_fd *efd);

/**
 * Remove an eventpoll file descriptor from its eventpoll. This will also
 * unregister the file description from the driver's signal list and unlink the
 * eventpoll file description from the referenced VFS file. It is safe to
 * release the memory for the eventpoll file description after this call.
 * The caller must ensure correct synchronization.
 *
 * @param efd the eventpoll file description to remove
 */
void eventpoll_del_unsafe(struct eventpoll_fd *efd);

/**
 * Wait for events on the specified eventpoll. The function immediately returns
 * if no file descriptions have been added to the eventpoll or if there are
 * pending events that are of interest. Otherwise the current thread is blocked.
 *
 * @param ep the eventpoll to wait on
 * @param events pointer to a pre-allocated array of epoll event objects that
 *    is used to report pending events. The function will copy-in the epoll
 *    event objects currently configured for the signaled file descriptions
 * @param maxevents the length of the pre-allocated epoll event object array
 * @param timeout optional timeout for the wait in ns. If timeout is NULL the
 *    thread blocks indefinitly until an event is signaled
 *
 * @return the number of events written into the supplied events array
 */
int eventpoll_wait(struct eventpoll *ep, struct epoll_event *events,
		   int maxevents, const __nsec *timeout);

/**
 * Signal events for an eventpoll file description. This function is called by
 * VFS drivers to inform the eventpoll about changes in the respective file's
 * state (e.g., new data to read)
 *
 * NOTE: The function must not be called from within an IRQ context due to the
 *   way we perform locking internally at the moment.
 *
 * @param ecb the eventpoll context block supplied in the poll call
 * @param revents a bitmask specifying all the active epoll events for this file
 */
void eventpoll_signal(struct eventpoll_cb *ecb, unsigned int revents);

/**
 * @internal Called by VFS to inform the eventpoll API that a file description
 * is closed and the respective file should be removed from all eventpolls
 * watching it. If this was the last file in an eventpoll all waiters will be
 * woken up.
 *
 * @param fp the VFS file object which is closed by the VFS
 */
void eventpoll_notify_close(struct vfscore_file *fp);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _VFSCORE_EVENTPOLL_H_ */
