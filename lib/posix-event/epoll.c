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
#include <vfscore/fs.h>
#include <vfscore/file.h>
#include <vfscore/dentry.h>
#include <vfscore/vnode.h>
#include <vfscore/mount.h>
#include <uk/alloc.h>
#include <uk/syscall.h>
#include <uk/config.h>
#include <uk/init.h>

#include <inttypes.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

static uint64_t e_inode;

#define epoll_vfscore_inactive ((vnop_inactive_t) vfscore_vop_einval)

static int epoll_vfscore_close(struct vnode *vnode,
			       struct vfscore_file *fp __unused)
{
	struct eventpoll *ep;

	UK_ASSERT(vnode->v_data);
	UK_ASSERT(vnode->v_type == VEPOLL);

	ep = (struct eventpoll *)vnode->v_data;

	/* Clean up interest lists and release fds */
	eventpoll_fini(ep);

	uk_free(uk_alloc_get_default(), ep);

	vnode->v_data = NULL;
	return 0;
}

/* vnode operations */
static struct vnops epoll_vnops = {
	.vop_close = epoll_vfscore_close,
	.vop_inactive = epoll_vfscore_inactive
};

#define epoll_vget ((vfsop_vget_t) vfscore_nullop)

/* file system operations */
static struct vfsops epoll_vfsops = {
	.vfs_vget = epoll_vget,
	.vfs_vnops = &epoll_vnops
};

/* bogus mount point used by all epoll objects */
static struct mount epoll_mount = {
	.m_op = &epoll_vfsops
};

static inline int is_epoll_file(struct vfscore_file *fp)
{
	struct vnode *np = fp->f_dentry->d_vnode;

	return (np->v_op == &epoll_vnops);
}

static inline int has_poll(struct vfscore_file *fp)
{
	struct vnode *np = fp->f_dentry->d_vnode;

	return (np->v_op->vop_poll != NULL);
}

static int do_epoll_create(struct uk_alloc *a, int flags)
{
	int vfs_fd, ret;
	struct eventpoll *ep;
	struct vfscore_file *vfs_file;
	struct dentry *vfs_dentry;
	struct vnode *vfs_vnode;

	if (unlikely(flags & ~EPOLL_CLOEXEC))
		return -EINVAL;

	/* Reserve a file descriptor number */
	vfs_fd = vfscore_alloc_fd();
	if (unlikely(vfs_fd < 0)) {
		ret = -ENFILE;
		goto ERR_EXIT;
	}

	/* Allocate file, vfs_file, and vnode */
	ep = uk_malloc(a, sizeof(struct eventpoll));
	if (unlikely(!ep)) {
		ret = -ENOMEM;
		goto ERR_MALLOC_FILE;
	}

	vfs_file = malloc(sizeof(struct vfscore_file));
	if (unlikely(!vfs_file)) {
		ret = -ENOMEM;
		goto ERR_MALLOC_VFS_FILE;
	}

	/* TODO: Synchronize inode increment */
	ret = vfscore_vget(&epoll_mount, e_inode++, &vfs_vnode);
	UK_ASSERT(ret == 0); /* we should not find it in the cache */
	if (unlikely(!vfs_vnode)) {
		ret = -ENOMEM;
		goto ERR_ALLOC_VNODE;
	}

	/*
	 * It doesn't matter that all the dentries have the same path since
	 * we never look them up.
	 */
	vfs_dentry = dentry_alloc(NULL, vfs_vnode, "/");
	if (unlikely(!vfs_dentry)) {
		ret = -ENOMEM;
		goto ERR_ALLOC_DENTRY;
	}

	/* Initialize data structures */
	vfs_file->fd = vfs_fd;
	vfs_file->f_flags = 0;
	vfs_file->f_count = 1;
	vfs_file->f_data = ep;
	vfs_file->f_dentry = vfs_dentry;
	vfs_file->f_vfs_flags = UK_VFSCORE_NOPOS;

	uk_mutex_init(&vfs_file->f_lock);
	UK_INIT_LIST_HEAD(&vfs_file->f_ep);

	vfs_vnode->v_data = ep;
	vfs_vnode->v_type = VEPOLL;

	eventpoll_init(ep, a);

	/* Store within the vfs structure */
	ret = vfscore_install_fd(vfs_fd, vfs_file);
	if (unlikely(ret))
		goto ERR_VFS_INSTALL;

	/* Only the dentry should hold a reference; release ours */
	vput(vfs_vnode);

	return vfs_fd;

ERR_VFS_INSTALL:
	drele(vfs_dentry);
ERR_ALLOC_DENTRY:
	vput(vfs_vnode);
ERR_ALLOC_VNODE:
	free(vfs_file);
ERR_MALLOC_VFS_FILE:
	uk_free(a, ep);
ERR_MALLOC_FILE:
	vfscore_put_fd(vfs_fd);
ERR_EXIT:
	UK_ASSERT(ret < 0);
	return ret;
}

UK_SYSCALL_R_DEFINE(int, epoll_create1, int, flags)
{
	return do_epoll_create(uk_alloc_get_default(), flags);
}

UK_SYSCALL_R_DEFINE(int, epoll_create, int, size)
{
	/*
	 * size was used as hint for how much memory needs to be allocated. Not
	 * used nowadays but must still be greater than zero for compatibility
	 * reasons.
	 */
	if (unlikely(size <= 0))
		return -EINVAL;

	return do_epoll_create(uk_alloc_get_default(), 0);
}

static int do_epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
{
	struct vfscore_file *epf, *fp;
	struct eventpoll *ep;
	int ret;

	epf = vfscore_get_file(epfd);
	if (unlikely(!epf)) {
		ret = -EBADF;
		goto EXIT;
	}

	fp = vfscore_get_file(fd);
	if (unlikely(!fp)) {
		ret = -EBADF;
		goto ERR_PUT_EPF;
	}

	if (unlikely(!has_poll(fp))) {
		ret = -EPERM;
		goto ERR_PUT_F;
	}

	if (unlikely((epfd == fd) || (!is_epoll_file(epf)))) {
		ret = -EINVAL;
		goto ERR_PUT_F;
	}

	/* TODO: The current epoll implementation does not support adding
	 * an eventpoll fd to another eventpoll.
	 */
	if (unlikely(is_epoll_file(fp))) {
		uk_pr_err_once("No nested epoll support.\n");
		return -EBADF;
	}

	UK_ASSERT(epf->f_data);

	ep = (struct eventpoll *)epf->f_data;

	switch (op) {
	case EPOLL_CTL_ADD:
		ret = eventpoll_add(ep, fd, fp, event);
		break;
	case EPOLL_CTL_MOD:
		ret = eventpoll_mod(ep, fd, event);
		break;
	case EPOLL_CTL_DEL:
		ret = eventpoll_del(ep, fd);
		break;
	default:
		ret = -EINVAL;
		break;
	}

ERR_PUT_F:
	vfscore_put_file(fp);
ERR_PUT_EPF:
	vfscore_put_file(epf);
EXIT:
	return ret;
}

UK_SYSCALL_R_DEFINE(int, epoll_ctl, int, epfd, int, op, int, fd,
		    struct epoll_event *, event)
{
	return do_epoll_ctl(epfd, op, fd, event);
}

static int do_epoll_pwait(int epfd, struct epoll_event *events, int maxevents,
			  const __nsec *timeout, const sigset_t *sigmask,
			  size_t sigsegsize __unused)
{
	struct vfscore_file *epf;
	struct eventpoll *ep;
	int ret;

	epf = vfscore_get_file(epfd);
	if (unlikely(!epf)) {
		ret = -EBADF;
		goto EXIT;
	}

	if (unlikely((maxevents <= 0) || !is_epoll_file(epf))) {
		ret = -EINVAL;
		goto ERR_PUT_EPF;
	}

	if (unlikely(!events)) {
		ret = -EFAULT;
		goto ERR_PUT_EPF;
	}

	UK_ASSERT(epf->f_data);

	ep = (struct eventpoll *)epf->f_data;

	/* TODO: Implement atomic masking of signals */
	if (sigmask)
		uk_pr_warn_once("%s: signal masking not implemented.",
				__func__);

	ret = eventpoll_wait(ep, events, maxevents, timeout);

ERR_PUT_EPF:
	vfscore_put_file(epf);
EXIT:
	return ret;
}

UK_SYSCALL_R_DEFINE(int, epoll_wait, int, epfd, struct epoll_event *, events,
		    int, maxevents, int, timeout)
{
	__nsec timeout_ns;

	if (timeout >= 0)
		timeout_ns = ukarch_time_msec_to_nsec(timeout);

	return do_epoll_pwait(epfd, events, maxevents,
			      (timeout >= 0) ? &timeout_ns : NULL,
			      NULL, 0);
}

UK_LLSYSCALL_R_DEFINE(int, epoll_pwait, int, epfd, struct epoll_event *, events,
		      int, maxevents, int, timeout, const sigset_t *, sigmask,
		      size_t, sigsetsize)
{
	__nsec timeout_ns;

	if (timeout >= 0)
		timeout_ns = ukarch_time_msec_to_nsec(timeout);

	return do_epoll_pwait(epfd, events, maxevents,
			      (timeout >= 0) ? &timeout_ns : NULL,
			      sigmask, sigsetsize);
}

#if UK_LIBC_SYSCALLS
/* The actual system call implemented in Linux uses a different signature! We
 * thus provide the libc call here directly.
 */
int epoll_pwait(int epfd, struct epoll_event *events, int maxevents,
		int timeout, const sigset_t *sigmask)
{
	__nsec timeout_ns;
	int ret;

	if (timeout >= 0)
		timeout_ns = ukarch_time_msec_to_nsec(timeout);

	ret = do_epoll_pwait(epfd, events, maxevents,
			     (timeout >= 0) ? &timeout_ns : NULL,
			     sigmask, sizeof(sigset_t));
	if (unlikely(ret < 0)) {
		errno = -ret;
		ret = -1;
	}

	return ret;
}
#endif /* UK_LIBC_SYSCALLS */

static int epoll_mount_init(void)
{
	int ret;

	epoll_mount.m_path = strdup("");
	if (!epoll_mount.m_path) {
		ret = -ENOMEM;
		goto err_out;
	}

	epoll_mount.m_special = strdup("");
	if (!epoll_mount.m_special) {
		ret = -ENOMEM;
		goto err_free_m_path;
	}

	return 0;

err_free_m_path:
	free(epoll_mount.m_path);
err_out:
	return ret;
}
uk_lib_initcall(epoll_mount_init);
