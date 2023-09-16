/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Alexander Jung <alexander.jung@neclab.eu>
 *          Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *
 * Copyright (c) 2020, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
 * Copyright (c) 2022, Karlsruhe Institute of Technology (KIT).
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

#include <uk/socket_driver.h>
#include <uk/socket_vnops.h>
#include <uk/socket.h>
#include <vfscore/vnode.h>
#include <vfscore/mount.h>
#include <vfscore/fs.h>
#include <uk/errptr.h>
#include <uk/init.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

static struct mount posix_socket_mount;

static uint64_t s_inode;

struct posix_socket_file *
posix_socket_file_get(int sock_fd)
{
	struct vfscore_file *fos;

	fos = vfscore_get_file(sock_fd);
	if (unlikely(!fos))
		return ERR2PTR(-ENOENT);

	UK_ASSERT(fos->f_dentry);
	UK_ASSERT(fos->f_dentry->d_vnode);
	if (unlikely(fos->f_dentry->d_vnode->v_type != VSOCK)) {
		vfscore_put_file(fos);
		return ERR2PTR(-EINVAL);
	}

	UK_ASSERT(fos->f_data);
	return (struct posix_socket_file *)fos->f_data;
}

int
posix_socket_alloc_fd(struct posix_socket_driver *d, int type, void *sock_data)
{
	int vfs_fd, ret;
	struct posix_socket_file *sock;
	struct vfscore_file *vfs_file;
	struct dentry *vfs_dentry;
	struct vnode *vfs_vnode;

	/* Reserve file descriptor number */
	vfs_fd = vfscore_alloc_fd();
	if (unlikely(vfs_fd < 0)) {
		ret = -ENFILE;
		goto ERR_EXIT;
	}

	/* Allocate file, vfs_file, and vnode */
	sock = uk_calloc(d->allocator, 1, sizeof(*sock));
	if (unlikely(!sock)) {
		ret = -ENOMEM;
		goto ERR_MALLOC_FILE;
	}

	vfs_file = calloc(1, sizeof(*vfs_file));
	if (unlikely(!vfs_file)) {
		ret = -ENOMEM;
		goto ERR_MALLOC_VFS_FILE;
	}

	/* TODO: Synchronize inode increment */
	ret = vfscore_vget(&posix_socket_mount, s_inode++, &vfs_vnode);
	UK_ASSERT(ret == 0); /* we should not find it in cache */
	if (unlikely(!vfs_vnode)) {
		ret = -ENOMEM;
		goto ERR_ALLOC_VNODE;
	}

	/* Create an unnamed dentry */
	vfs_dentry = dentry_alloc(NULL, vfs_vnode, "/");
	if (unlikely(!vfs_dentry)) {
		ret = -ENOMEM;
		goto ERR_ALLOC_DENTRY;
	}

	/* Put things together, and fill out necessary fields */
	vfs_file->fd = vfs_fd;
	vfs_file->f_flags = UK_FWRITE | UK_FREAD;
	vfs_file->f_count = 1;
	vfs_file->f_data = sock;
	vfs_file->f_dentry = vfs_dentry;
	vfs_file->f_vfs_flags = UK_VFSCORE_NOPOS;

	uk_mutex_init(&vfs_file->f_lock);
	UK_INIT_LIST_HEAD(&vfs_file->f_ep);

	vfs_vnode->v_data = sock;
	vfs_vnode->v_type = VSOCK;

	sock->vfs_file = vfs_file;
	sock->sock_data = sock_data;
	sock->driver = d;
	sock->type = type;

	/* Store within the vfs structure */
	ret = vfscore_install_fd(vfs_fd, vfs_file);
	if (unlikely(ret))
		goto ERR_VFS_INSTALL;

	/* Only the dentry should hold a reference; release ours */
	vput(vfs_vnode);

	uk_pr_debug("Allocated socket %d for %s (%p)\n",
		    vfs_fd, d->libname, sock_data);

	/* Return file descriptor of our socket */
	return vfs_fd;

ERR_VFS_INSTALL:
	drele(vfs_dentry);
ERR_ALLOC_DENTRY:
	vput(vfs_vnode);
ERR_ALLOC_VNODE:
	free(vfs_file);
ERR_MALLOC_VFS_FILE:
	uk_free(d->allocator, sock);
ERR_MALLOC_FILE:
	vfscore_put_fd(vfs_fd);
ERR_EXIT:
	UK_ASSERT(ret < 0);
	return ret;
}

static int
posix_socket_vfscore_close(struct vnode *vnode,
			   struct vfscore_file *fp __maybe_unused)
{
	struct posix_socket_file *sock;
	int ret;

	UK_ASSERT(vnode->v_data);
	UK_ASSERT(vnode->v_type == VSOCK);

	sock = (struct posix_socket_file *)vnode->v_data;

	/* Close and release the network socket */
	ret = posix_socket_close(sock);

	uk_free(sock->driver->allocator, sock);

	if (unlikely(ret < 0)) {
		PSOCKET_ERR("close on socket %d failed: %d\n", fp->fd, ret);
		return -ret;
	}

	return 0;
}

static int
posix_socket_vfscore_write(struct vnode *vnode,
			   struct uio *buf, int ioflag __unused)
{
	struct posix_socket_file *sock;
	ssize_t ret;

	UK_ASSERT(vnode->v_data);
	UK_ASSERT(vnode->v_type == VSOCK);

	sock = (struct posix_socket_file *)vnode->v_data;

	/* TODO: This is a dirty hack to allow parallel reads and writes on
	 * the socket. The locking is a problem, for example, when the socket
	 * read blocks and another thread wants to send data over the same
	 * socket. The write is then blocked until the read finishes. Depending
	 * on the application, however, the socket may only receive data after
	 * the network partner has received the data, which we cannot send.
	 *
	 * Since we do not access the vnode in the following code, we can
	 * unlock it here.
	 *
	 * BUT: This allows a different thread to close the vnode in between,
	 * which would then release it (refcnt = 0). When this thread returns,
	 * we have a classic use-after-free. Is does not help to increment the
	 * reference count here before unlocking and then dereference it after
	 * locking it again. While this would prevent a use-after-free in this
	 * function the vnode is released as soon as we dereference it, leading
	 * to a use-after-free in the caller.
	 *
	 * A safe solution is to add a vref in all VFS code (i.e., in the
	 * outmost call frame that works with the vnode), so that it is safe
	 * to close the vnode in between as the VFS code holds an own reference
	 * to the vnode as long as it is working with it.
	 */
	vn_unlock(vnode);

	ret = posix_socket_write(sock, buf->uio_iov, buf->uio_iovcnt);

	vn_lock(vnode);

	if (unlikely(ret < 0)) {
		/* TODO: Add fp when write provides it */
		if (ret != -EAGAIN) /* Don't spam log for legitimate timeouts */
			PSOCKET_ERR("write on socket failed: %d\n", (int)ret);
		return (int)-ret;
	}

	buf->uio_resid -= ret;

	return 0;
}

static int
posix_socket_vfscore_read(struct vnode *vnode,
			  struct vfscore_file *fp __maybe_unused,
			  struct uio *buf, int ioflag __unused)
{
	struct posix_socket_file *sock;
	ssize_t ret;

	UK_ASSERT(vnode->v_data);
	UK_ASSERT(vnode->v_type == VSOCK);

	sock = (struct posix_socket_file *)vnode->v_data;

	/* TODO: This is not safe. See write() */
	vn_unlock(vnode);

	ret = posix_socket_read(sock, buf->uio_iov, buf->uio_iovcnt);

	vn_lock(vnode);

	if (unlikely(ret < 0)) {
		if (ret != -EAGAIN) /* Don't spam log for legitimate timeouts */
			PSOCKET_ERR("read on socket %d failed: %d\n", fp->fd,
				    (int)ret);
		return (int)-ret;
	}

	buf->uio_resid -= ret;

	return 0;
}

static int
posix_socket_vfscore_ioctl(struct vnode *vnode,
			   struct vfscore_file *fp __maybe_unused,
			   unsigned long request, void *buf)
{
	struct posix_socket_file *sock;
	int ret;

	UK_ASSERT(vnode->v_data);
	UK_ASSERT(vnode->v_type == VSOCK);

	sock = (struct posix_socket_file *)vnode->v_data;

	ret = posix_socket_ioctl(sock, request, buf);
	if (unlikely(ret < 0)) {
		PSOCKET_ERR("ioctl on socket %d failed: %d\n", fp->fd,
			    (int)ret);
		return -ret;
	}

	return 0;
}

static int posix_socket_vfscore_poll(struct vnode *vnode, unsigned int *revents,
				     struct eventpoll_cb *ecb)
{
	struct posix_socket_file *sock;
	int ret;

	UK_ASSERT(vnode->v_data);
	UK_ASSERT(vnode->v_type == VSOCK);

	sock = (struct posix_socket_file *)vnode->v_data;

	ret = posix_socket_poll(sock, revents, ecb);
	if (unlikely(ret < 0)) {
		PSOCKET_ERR("poll on socket %d failed: %d\n",
			    sock->vfs_file->fd, (int)ret);
		return -ret;
	}

	return 0;
}

#define posix_socket_vfscore_getattr ((vnop_getattr_t) vfscore_vop_einval)
#define posix_socket_vfscore_inactive ((vnop_inactive_t) vfscore_vop_nullop)

/* vnode operations */
struct vnops posix_socket_vnops = {
	.vop_close = posix_socket_vfscore_close,
	.vop_write = posix_socket_vfscore_write,
	.vop_read = posix_socket_vfscore_read,
	.vop_ioctl = posix_socket_vfscore_ioctl,
	.vop_getattr = posix_socket_vfscore_getattr,
	.vop_inactive = posix_socket_vfscore_inactive,
	.vop_poll = posix_socket_vfscore_poll,
};

#define posix_socket_vget ((vfsop_vget_t) vfscore_nullop)

/* file system operations */
static struct vfsops posix_socket_vfsops = {
	.vfs_vget = posix_socket_vget,
	.vfs_vnops = &posix_socket_vnops,
};

/* bogus mount point used by all sockets */
static struct mount posix_socket_mount = {
	.m_op = &posix_socket_vfsops
};

static int posix_socket_mount_init(void)
{
	int ret;

	posix_socket_mount.m_path = strdup("");
	if (!posix_socket_mount.m_path) {
		ret = -ENOMEM;
		goto err_out;
	}

	posix_socket_mount.m_special = strdup("");
	if (!posix_socket_mount.m_special) {
		ret = -ENOMEM;
		goto err_free_m_path;
	}

	return 0;

err_free_m_path:
	free(posix_socket_mount.m_path);
err_out:
	return ret;
}
uk_lib_initcall(posix_socket_mount_init);
