/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Sharan Santhanam <sharan.santhanam@neclab.eu>
 *          Alexander Jung <alexander.jung@neclab.eu>
 *
 * Copyright (c) 2020, NEC Laboratories Europe GmbH, NEC Corporation.
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

#include <uk/socket_driver.h>
#include <vfscore/vnode.h>
#include <vfscore/mount.h>
#include <vfscore/fs.h>
#include <uk/socket_vnops.h>
#include <uk/errptr.h>
#include <inttypes.h>

static uint64_t s_inode = 0;

#define posix_socket_getattr ((vnop_getattr_t) vfscore_vop_einval)
#define posix_socket_inactive ((vnop_inactive_t) vfscore_vop_nullop)

/* vnode operations */
struct vnops posix_socket_vnops = {
	.vop_close = posix_socket_vfscore_close,
	.vop_write = posix_socket_vfscore_write,
	.vop_read = posix_socket_vfscore_read,
	.vop_ioctl = posix_socket_vfscore_ioctl,
	.vop_getattr = posix_socket_getattr,
	.vop_inactive = posix_socket_inactive,
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

struct posix_socket_file *
posix_socket_file_get(int sock_fd)
{
	struct posix_socket_file *file = NULL;
	struct vfscore_file *fos;

	fos = vfscore_get_file(sock_fd);

	if (!fos) {
		uk_pr_err("Failed with invalid descriptor\n");
		file = ERR2PTR(EINVAL);
		goto EXIT;
	}

	if (fos->f_dentry->d_vnode->v_type != VSOCK) {
		uk_pr_err("File descriptor is not a socket\n");
		file = ERR2PTR(EINVAL);
		goto EXIT;
	}

	file = fos->f_data;

EXIT:
	return file;
}

int
socket_alloc_fd(struct posix_socket_driver *d, int type, void *sock_data)
{
	int ret = 0;
	int vfs_fd;
	struct posix_socket_file *file = NULL;
	struct vfscore_file *vfs_file = NULL;
	struct dentry *s_dentry;
	struct vnode *s_vnode;

	/* Reserve file descriptor number */
	vfs_fd = vfscore_alloc_fd();
	if (vfs_fd < 0) {
		ret = -ENFILE;
		uk_pr_err("failed to allocate file descriptor number\n");
		goto ERR_EXIT;
	}

	/* Allocate file, dentry, and vnode */
	file = uk_calloc(d->allocator, 1, sizeof(*file));
	if (!file) {
		ret = -ENOMEM;
		uk_pr_err("failed to allocate socket file\n");
		goto ERR_MALLOC_FILE;
	}

	vfs_file = uk_calloc(d->allocator, 1, sizeof(*vfs_file));
	if (!vfs_file) {
		ret = -ENOMEM;
		uk_pr_err("failed to allocate socket vfs_file\n");
		goto ERR_MALLOC_VFS_FILE;
	}

	ret = vfscore_vget(&posix_socket_mount, s_inode++, &s_vnode);
	UK_ASSERT(ret == 0); /* we should not find it in cache */

	if (!s_vnode) {
		ret = -ENOMEM;
		uk_pr_err("failed to allocate socket vnode\n");
		goto ERR_ALLOC_VNODE;
	}

	uk_mutex_unlock(&s_vnode->v_lock);

	/*
	 * it doesn't matter that all the dentries have the
	 * same path since we never lookup for them
	 */
	s_dentry = dentry_alloc(NULL, s_vnode, "/");

	if (!s_dentry) {
		ret = -ENOMEM;
		uk_pr_err("failed to allocate socket dentry\n");
		goto ERR_ALLOC_DENTRY;
	}

	/* Put things together, and fill out necessary fields */
	vfs_file->fd = vfs_fd;
	vfs_file->f_flags = UK_FWRITE | UK_FREAD;
	vfs_file->f_count = 1;
	vfs_file->f_data = file;
	vfs_file->f_dentry = s_dentry;
	vfs_file->f_vfs_flags = UK_VFSCORE_NOPOS;

	// s_vnode->v_data = file;
	s_vnode->v_type = VSOCK;
	s_vnode->v_ino = s_inode;

	file->vfs_file = vfs_file;
	file->sock_data = sock_data;
	file->driver = d;
	file->type = type;

	uk_pr_debug("allocated socket %d for %s (%p)\n",
			vfs_fd,
			d->libname,
			file->sock_data);

	/* Store within the vfs structure */
	ret = vfscore_install_fd(vfs_fd, file->vfs_file);
	if (ret) {
		uk_pr_err("failed to install socket fd\n");
		goto ERR_VFS_INSTALL;
	}

	/* Only the dentry should hold a reference; release ours */
	vrele(s_vnode);

	/* Return file descriptor of our socket */
	return vfs_fd;

ERR_VFS_INSTALL:
	drele(s_dentry);
ERR_ALLOC_DENTRY:
	vrele(s_vnode);
ERR_ALLOC_VNODE:
	uk_free(d->allocator, vfs_file);
ERR_MALLOC_VFS_FILE:
	uk_free(d->allocator, file);
ERR_MALLOC_FILE:
	vfscore_put_fd(vfs_fd);
ERR_EXIT:
	UK_ASSERT(ret < 0);
	return ret;
}

int
posix_socket_vfscore_close(struct vnode *s_vnode,
		struct vfscore_file *vfscore_file)
{
	int ret = 0;
	struct posix_socket_file *file = NULL;
	file = s_vnode->v_data;

	uk_pr_debug("%s fd:%d driver:%s: sock_data:%p\n",
			__func__,
			file->vfs_file->fd,
			file->driver->libname,
			file->sock_data);

	/* Close and release the socket */
	ret = posix_socket_close(file);

	/*
	 * Free socket file
	 * The rest of the resources will be freed by vfs
	 *
	 * TODO: vfs ignores close errors right now, so free our file
	 */
	uk_free(file->driver->allocator, file);

	if (ret < 0)
		return errno;

	return ret;
}

int
posix_socket_vfscore_write(struct vnode *s_vnode,
		struct uio *buf, int ioflag __unused)
{
	int ret = 0;
	struct posix_socket_file *file = NULL;

	file = s_vnode->v_data;
	uk_pr_debug("%s fd:%d driver:%s sock_data:%p\n",
		__func__,
		file->vfs_file->fd,
		file->driver->libname,
		file->sock_data);

	/* Write to the socket */
	ret = posix_socket_write(file, buf->uio_iov->iov_base, buf->uio_iov->iov_len);

	/*
	 * Some socket implementations, such as LwIP, may set the errno and return to
	 * -1 as an error, but vfs expects us to return a positive errno.
	 */
	if (ret < 0)
		return errno;

	buf->uio_resid -= ret;
	return 0;
}

int
posix_socket_vfscore_read(struct vnode *s_vnode,
		struct vfscore_file *vfscore_file __unused,
		struct uio *buf, int ioflag __unused)
{
	int ret = 0;
	struct posix_socket_file *file = NULL;

	file = s_vnode->v_data;
	if (PTRISERR(file)) {
		ret = -1;
		SOCKET_ERR(PTR2ERR(file), "failed to identify socket descriptor");
		goto EXIT;
	}

	uk_pr_debug("%s fd:%d driver:%s sock_data:%p\n",
			__func__,
			file->vfs_file->fd,
			file->driver->libname,
			file->sock_data);

	ret = posix_socket_read(file, buf->uio_iov->iov_base, buf->uio_iov->iov_len);
	if (ret < 0) {
		ret = errno;
		goto EXIT;
	}

	buf->uio_resid -= ret;

EXIT:
	return ret;
}

int
posix_socket_vfscore_ioctl(struct vnode *s_vnode,
		struct vfscore_file *vfscore_file __unused,
		unsigned long request,
		void *buf)
{
	int ret = 0;
	struct posix_socket_file *file = NULL;

	file = s_vnode->v_data;
	if (PTRISERR(file)) {
		ret = -1;
		SOCKET_ERR(PTR2ERR(file), "failed to identify socket descriptor");
		goto EXIT;
	}

	uk_pr_debug("%s fd:%d driver:%s sock_data:%p\n",
			__func__,
			file->vfs_file->fd,
			file->driver->libname,
			file->sock_data);

	ret = posix_socket_ioctl(file, request, buf);
	if (ret < 0)
		ret = errno;

EXIT:
	return ret;
}
