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

#include <uk/socket_driver.h>
#include <uk/socket_vnops.h>
#include <uk/socket.h>
#include <vfscore/vnode.h>
#include <vfscore/mount.h>
#include <vfscore/fs.h>
#include <uk/errptr.h>
#include <inttypes.h>
#include <errno.h>

#define to_sock(fp) \
	__containerof((fp), struct posix_socket_file, fd_file)

static struct fdops posix_socket_fdops;

struct posix_socket_file *
posix_socket_file_get(int sock_fd)
{
	struct fdtab_file *fos;

	fos = fdtab_get_file(fdtab_get_active(), sock_fd);
	if (unlikely(!fos))
		return ERR2PTR(-ENOENT);

	if (unlikely(fos->f_op != &posix_socket_fdops)) {
		fdtab_put_file(fos);
		return ERR2PTR(-EINVAL);
	}

	return to_sock(fos);
}

int
posix_socket_alloc_fd(struct posix_socket_driver *d, int type, void *sock_data)
{
	int vfs_fd, ret;
	struct posix_socket_file *sock;
	struct fdtab_file *fd_file;
	struct fdtab_table *fdtab = fdtab_get_active();

	/* Reserve file descriptor number */
	vfs_fd = fdtab_alloc_fd(fdtab);
	if (unlikely(vfs_fd < 0)) {
		ret = -ENFILE;
		goto ERR_EXIT;
	}

	/* Allocate file, vfs_file, and vnode */
	sock = uk_calloc(d->allocator, 1, sizeof(*sock));
	if (unlikely(!sock)) {
		ret = -ENOMEM;
		goto ERR_ALLOC_FD;
	}

	fd_file = &sock->fd_file;

	/* Put things together, and fill out necessary fields */
	fdtab_file_init(fd_file);
	fd_file->fd = vfs_fd;
	fd_file->f_flags = UK_FWRITE | UK_FREAD;
	fd_file->f_op = &posix_socket_fdops;

	sock->sock_data = sock_data;
	sock->driver = d;
	sock->type = type;

	/* Store within the vfs structure */
	ret = fdtab_install_fd(fdtab, vfs_fd, fd_file);
	if (unlikely(ret))
		goto ERR_MALLOC_SOCK;

	uk_pr_debug("Allocated socket %d for %s (%p)\n",
		    vfs_fd, d->libname, sock_data);

	/* Return file descriptor of our socket */
	return vfs_fd;

ERR_MALLOC_SOCK:
	uk_free(d->allocator, sock);
ERR_ALLOC_FD:
	fdtab_put_fd(fdtab, vfs_fd);
ERR_EXIT:
	UK_ASSERT(ret < 0);
	return ret;
}

static int
posix_socket_vfscore_close(struct fdtab_file *fp)
{
	struct posix_socket_file *sock = to_sock(fp);
	int ret;

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
posix_socket_vfscore_write(struct fdtab_file *fp,
			   struct uio *buf, int ioflag __unused)
{
	struct posix_socket_file *sock = to_sock(fp);
	ssize_t ret;

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
	FD_LOCK(fp);

	ret = posix_socket_write(sock, buf->uio_iov, buf->uio_iovcnt);

	FD_UNLOCK(fp);

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
posix_socket_vfscore_read(struct fdtab_file *fp,
			  struct uio *buf, int ioflag __unused)
{
	struct posix_socket_file *sock = to_sock(fp);
	ssize_t ret;

	/* TODO: This is not safe. See write() */
	FD_LOCK(fp);

	ret = posix_socket_read(sock, buf->uio_iov, buf->uio_iovcnt);

	FD_UNLOCK(fp);

	if (unlikely(ret < 0)) {
		if (ret != -EAGAIN) /* Don't spam log for legitimate timeouts */
			PSOCKET_ERR("read on socket %d failed: %d\n",
				    fp->fd, (int)ret);
		return (int)-ret;
	}

	buf->uio_resid -= ret;

	return 0;
}

static int
posix_socket_vfscore_ioctl(struct fdtab_file *fp,
			   unsigned long request, void *buf)
{
	struct posix_socket_file *sock = to_sock(fp);
	int ret;

	ret = posix_socket_ioctl(sock, request, buf);
	if (unlikely(ret < 0)) {
		PSOCKET_ERR("ioctl on socket %d failed: %d\n", fp->fd,
			    (int)ret);
		return -ret;
	}

	return ret;
}

static int posix_socket_vfscore_poll(struct fdtab_file *fp,
				     unsigned int *revents,
				     struct eventpoll_cb *ecb)
{
	struct posix_socket_file *sock = to_sock(fp);
	int ret;

	ret = posix_socket_poll(sock, revents, ecb);
	if (unlikely(ret < 0)) {
		PSOCKET_ERR("poll on socket %d failed: %d\n", fp->fd, (int)ret);
		return -ret;
	}

	return ret;
}

/* vnode operations */
static struct fdops posix_socket_fdops = {
	.fdop_free = posix_socket_vfscore_close,
	.fdop_write = posix_socket_vfscore_write,
	.fdop_read = posix_socket_vfscore_read,
	.fdop_ioctl = posix_socket_vfscore_ioctl,
	.fdop_poll = posix_socket_vfscore_poll,
};
