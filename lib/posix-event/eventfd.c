/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *
 * Copyright (c) 2022, Karlsruhe Institute of Technology (KIT)
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
#include <vfscore/fs.h>
#include <vfscore/file.h>
#include <vfscore/dentry.h>
#include <vfscore/vnode.h>
#include <vfscore/mount.h>
#include <uk/syscall.h>
#include <uk/essentials.h>
#include <uk/print.h>
#include <uk/wait.h>
#include <uk/mutex.h>
#include <uk/list.h>
#include <uk/config.h>
#include <uk/init.h>

#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

struct eventfd {
	/** Current value of this eventfd */
	uint64_t val;
	/** Indicates if this fd is a semaphore */
	unsigned int is_semaphore;
	/** Lock to synchronize access */
	struct uk_mutex lock;
	/** Wait queue for blocked writers (i.e., value would overflow) */
	struct uk_waitq w_wq;
	/** Wait queue for blocked readers (i.e., value is zero) */
	struct uk_waitq r_wq;
	/** List of registered eventpolls. */
	struct uk_list_head ep_list;
};

#define eventfd_no_overflow(cur, val)				\
	((cur) < UINT64_MAX - (val))

static uint64_t e_inode;
static struct uk_mutex eventfd_global_lock =
	UK_MUTEX_INITIALIZER(eventfd_global_lock);

static void eventfd_init(struct eventfd *efd, unsigned int initval, int flags)
{
	efd->val = initval;
	efd->is_semaphore = (flags & EFD_SEMAPHORE);
	uk_waitq_init(&efd->w_wq);
	uk_waitq_init(&efd->r_wq);
	uk_mutex_init(&efd->lock);
	UK_INIT_LIST_HEAD(&efd->ep_list);
}

static unsigned int eventfd_events(struct eventfd *efd)
{
	unsigned int events = 0;

	if (efd->val > 0)
		events |= EPOLLIN;
	if (efd->val < UINT64_MAX)
		events |= EPOLLOUT;
	if (efd->val == UINT64_MAX)
		events |= EPOLLERR;

	return events;
}

static void eventfd_signal_eventpoll(struct eventfd *efd, unsigned int events)
{
	struct eventpoll_cb *ecb;
	struct uk_list_head *itr;

	uk_list_for_each(itr, &efd->ep_list) {
		ecb = uk_list_entry(itr, struct eventpoll_cb, cb_link);

		UK_ASSERT(ecb->unregister);

		eventpoll_signal(ecb, events);
	}
}

static int eventfd_vfscore_close(struct vnode *vnode,
				 struct vfscore_file *fp __unused)
{
	struct eventfd *efd;

	UK_ASSERT(vnode->v_data);
	UK_ASSERT(vnode->v_type == VEVENT);

	efd = (struct eventfd *)vnode->v_data;

	uk_free(uk_alloc_get_default(), efd);

	vnode->v_data = NULL;
	return 0;
}

static int eventfd_vfscore_read(struct vnode *vnode,
				struct vfscore_file *fp,
				struct uio *buf, int ioflag __unused)
{
	struct eventfd *efd = (struct eventfd *)vnode->v_data;
	uint64_t *val;

	UK_ASSERT(vnode->v_data);
	UK_ASSERT(vnode->v_type == VEVENT);

	if (unlikely(buf->uio_offset != 0))
		return EINVAL;

	if (unlikely(buf->uio_iovcnt != 1))
		return EINVAL;

	if (unlikely(!buf->uio_iov[0].iov_base))
		return EINVAL;

	if (unlikely(buf->uio_iov[0].iov_len < sizeof(uint64_t)))
		return EINVAL;

	val = (uint64_t *)buf->uio_iov[0].iov_base;

	uk_mutex_lock(&efd->lock);

	/* Check if the value is 0. In this case, block if the file descriptor
	 * is not configured to non-blocking operation.
	 */
	if ((efd->val == 0) && (fp->f_flags & O_NONBLOCK)) {
		uk_mutex_unlock(&efd->lock);
		return EAGAIN;
	}

	uk_waitq_wait_event_mutex(&efd->r_wq, efd->val > 0, &efd->lock);

	UK_ASSERT(efd->val > 0);

	*val = (efd->is_semaphore) ? 1 : efd->val;
	efd->val -= *val;

	eventfd_signal_eventpoll(efd, EPOLLOUT);

	uk_waitq_wake_up(&efd->w_wq);

	uk_mutex_unlock(&efd->lock);

	buf->uio_resid = 0;
	buf->uio_offset = sizeof(uint64_t);

	return 0;
}

static int eventfd_vfscore_write(struct vnode *vnode, struct uio *buf,
				 int ioflag __unused)
{
	struct eventfd *efd = (struct eventfd *)vnode->v_data;
	uint64_t val;

	UK_ASSERT(vnode->v_data);
	UK_ASSERT(vnode->v_type == VEVENT);

	if (unlikely(buf->uio_offset != 0))
		return EINVAL;

	if (unlikely(buf->uio_iovcnt != 1))
		return EINVAL;

	if (unlikely(!buf->uio_iov[0].iov_base))
		return EINVAL;

	if (unlikely(buf->uio_iov[0].iov_len != sizeof(uint64_t)))
		return EINVAL;

	val = *((uint64_t *)buf->uio_iov[0].iov_base);
	if (unlikely(val == (uint64_t)-1))
		return EINVAL;

	uk_mutex_lock(&efd->lock);

	/* Check if the addition would overflow. In this case, block if the
	 * file descriptor is not configured to non-blocking operation.
	 *
	 * TODO: Make fp available here
	 */
	/*
	if ((efd->val >= (uint64_t)-1 - val) && (fp->f_flags & O_NONBLOCK)) {
		uk_mutex_unlock(&efd->lock);
		return EAGAIN;
	}
	*/

	uk_waitq_wait_event_mutex(&efd->w_wq,
				  eventfd_no_overflow(efd->val, val),
				  &efd->lock);

	UK_ASSERT(eventfd_no_overflow(efd->val, val));

	/* Unblock readers if value is zero */
	if (efd->val == 0)
		uk_waitq_wake_up(&efd->r_wq);

	efd->val += val;

	eventfd_signal_eventpoll(efd, EPOLLIN);

	uk_mutex_unlock(&efd->lock);

	buf->uio_resid = 0;
	buf->uio_offset = sizeof(uint64_t);

	return 0;
}

static void eventfd_unregister_eventpoll(struct eventpoll_cb *ecb)
{
	UK_ASSERT(ecb);

	uk_mutex_lock(&eventfd_global_lock);
	UK_ASSERT(!uk_list_empty(&ecb->cb_link));
	uk_list_del(&ecb->cb_link);

	ecb->data = NULL;
	ecb->unregister = NULL;
	uk_mutex_unlock(&eventfd_global_lock);
}

static int eventfd_vfscore_poll(struct vnode *vnode, unsigned int *revents,
				struct eventpoll_cb *ecb)
{
	struct eventfd *efd = (struct eventfd *)vnode->v_data;
	unsigned int events;

	UK_ASSERT(vnode->v_data);
	UK_ASSERT(vnode->v_type == VEVENT);

	uk_mutex_lock(&efd->lock);

	events = eventfd_events(efd);

	uk_mutex_lock(&eventfd_global_lock);
	if (!ecb->unregister) {
		UK_ASSERT(uk_list_empty(&ecb->cb_link));
		UK_ASSERT(!ecb->data);

		/* This is the first time we see this cb. Add it to the
		 * eventpoll list and set the unregister callback so
		 * we remove it when the eventpoll is freed.
		 */
		uk_list_add_tail(&ecb->cb_link, &efd->ep_list);

		ecb->data = efd;
		ecb->unregister = eventfd_unregister_eventpoll;
	}
	uk_mutex_unlock(&eventfd_global_lock);

	uk_mutex_unlock(&efd->lock);

	*revents = events;

	return 0;
}

static int eventfd_vfscore_ioctl(struct vnode *vnode __unused,
				 struct vfscore_file *fp __unused,
				 unsigned long request, void *buf __unused)
{

	switch (request) {
	case FIONBIO:
		/* eventfd operations look for O_NONBLOCK flag. We only need to
		 * confirm here that we support O_NONBLOCK. Setting O_NONBLOCK
		 * is implemented with an `ioctl` call with the FIONBIO flag).
		 */
		return 0;
	default:
		break;
	}
	return EINVAL;
}

/* vnode operations */
#define eventfd_vfscore_inactive ((vnop_inactive_t) vfscore_vop_einval)

static struct vnops eventfd_vnops = {
	.vop_close = eventfd_vfscore_close,
	.vop_inactive = eventfd_vfscore_inactive,
	.vop_read = eventfd_vfscore_read,
	.vop_write = eventfd_vfscore_write,
	.vop_poll = eventfd_vfscore_poll,
	.vop_ioctl = eventfd_vfscore_ioctl
};

/* file system operations */
#define eventfd_vget ((vfsop_vget_t) vfscore_nullop)

static struct vfsops eventfd_vfsops = {
	.vfs_vget = eventfd_vget,
	.vfs_vnops = &eventfd_vnops
};

/* bogus mount point used by all eventfd fds */
static struct mount eventfd_mount = {
	.m_op = &eventfd_vfsops
};

/* Prototype of fcntl system call is needed when lib/syscall_shim is off */
long uk_syscall_r_fcntl(long, long, long);

static int do_eventfd(struct uk_alloc *a, unsigned int initval, int flags)
{
	int vfs_fd, ret;
	struct eventfd *efd;
	struct vfscore_file *vfs_file;
	struct dentry *vfs_dentry;
	struct vnode *vfs_vnode;

	if (unlikely(flags & ~(EFD_CLOEXEC | EFD_SEMAPHORE | EFD_NONBLOCK)))
		return -EINVAL;

	/* Reserve a file descriptor number */
	vfs_fd = vfscore_alloc_fd();
	if (unlikely(vfs_fd < 0)) {
		ret = -ENFILE;
		goto ERR_EXIT;
	}

	/* Allocate file, vfs_file, and vnode */
	efd = uk_malloc(a, sizeof(struct eventfd));
	if (unlikely(!efd)) {
		ret = -ENOMEM;
		goto ERR_MALLOC_FILE;
	}

	vfs_file = malloc(sizeof(struct vfscore_file));
	if (unlikely(!vfs_file)) {
		ret = -ENOMEM;
		goto ERR_MALLOC_VFS_FILE;
	}

	/* TODO: Synchronize inode increment */
	ret = vfscore_vget(&eventfd_mount, e_inode++, &vfs_vnode);
	UK_ASSERT(ret == 0); /* we should not find it in the cache */
	if (unlikely(!vfs_vnode)) {
		ret = -ENOMEM;
		goto ERR_ALLOC_VNODE;
	}

	/* It doesn't matter that all the dentries have the same path since
	 * we never look them up.
	 */
	vfs_dentry = dentry_alloc(NULL, vfs_vnode, "/");
	if (unlikely(!vfs_dentry)) {
		ret = -ENOMEM;
		goto ERR_ALLOC_DENTRY;
	}

	/* Initialize data structures */
	vfs_file->fd = vfs_fd;
	vfs_file->f_flags = UK_FREAD | UK_FWRITE;
	vfs_file->f_count = 1;
	vfs_file->f_data = efd;
	vfs_file->f_dentry = vfs_dentry;
	vfs_file->f_vfs_flags = UK_VFSCORE_NOPOS;
	vfs_file->f_offset = 0;

	uk_mutex_init(&vfs_file->f_lock);
	UK_INIT_LIST_HEAD(&vfs_file->f_ep);

	vfs_vnode->v_data = efd;
	vfs_vnode->v_type = VEVENT;

	eventfd_init(efd, initval, flags);

	/* Store within the vfs structure */
	ret = vfscore_install_fd(vfs_fd, vfs_file);
	if (unlikely(ret))
		goto ERR_VFS_INSTALL;

	/* Only the dentry should hold a reference; release ours */
	vput(vfs_vnode);

	if (flags & EFD_NONBLOCK) {
		ret = uk_syscall_r_fcntl(vfs_fd, F_SETFL, O_NONBLOCK);
		/* Setting the O_NONBLOCK here must not fail.
		 * According to `man` pages, `F_SETFL` returns zero on success.
		 */
		UK_ASSERT(ret == 0);
	}

	return vfs_fd;

ERR_VFS_INSTALL:
	drele(vfs_dentry);
ERR_ALLOC_DENTRY:
	vput(vfs_vnode);
ERR_ALLOC_VNODE:
	free(vfs_file);
ERR_MALLOC_VFS_FILE:
	uk_free(a, efd);
ERR_MALLOC_FILE:
	vfscore_put_fd(vfs_fd);
ERR_EXIT:
	UK_ASSERT(ret < 0);
	return ret;
}

UK_SYSCALL_R_DEFINE(int, eventfd2, unsigned int, initval, int, flags)
{
	return do_eventfd(uk_alloc_get_default(), initval, flags);
}

UK_LLSYSCALL_R_DEFINE(int, eventfd, unsigned int, initval)
{
	return do_eventfd(uk_alloc_get_default(), initval, 0);
}

#if UK_LIBC_SYSCALLS
/* The actual system call implemented in Linux uses a different signature! We
 * thus provide the libc call here directly.
 */
int eventfd(unsigned int initval, int flags)
{
	int ret;

	ret = do_eventfd(uk_alloc_get_default(), initval, flags);
	if (unlikely(ret < 0)) {
		errno = -ret;
		ret = -1;
	}

	return ret;
}
#endif /* UK_LIBC_SYSCALLS */

static int eventfd_mount_init(void)
{
	int ret;

	eventfd_mount.m_path = strdup("");
	if (!eventfd_mount.m_path) {
		ret = -ENOMEM;
		goto err_out;
	}

	eventfd_mount.m_special = strdup("");
	if (!eventfd_mount.m_special) {
		ret = -ENOMEM;
		goto err_free_m_path;
	}

	return 0;

err_free_m_path:
	free(eventfd_mount.m_path);
err_out:
	return ret;
}
uk_lib_initcall(eventfd_mount_init);
