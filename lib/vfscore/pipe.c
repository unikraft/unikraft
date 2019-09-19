/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Bogdan-George Lascu <lascu.bogdan96@gmail.com>
 *          Costin Lupu <costin.lupu@cs.pub.ro>
 *
 * Copyright (c) 2019, University Politehnica of Bucharest. All rights reserved.
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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#include <uk/config.h>
#include <stdio.h>
#include <string.h>
#include <vfscore/file.h>
#include <vfscore/fs.h>
#include <vfscore/mount.h>
#include <vfscore/vnode.h>
#include <uk/wait.h>
#include <sys/ioctl.h>

/* We use the default size in Linux kernel */
#define PIPE_MAX_SIZE	(1 << CONFIG_LIBVFSCORE_PIPE_SIZE_ORDER)

struct pipe_buf {
	/* The buffer */
	char *data;
	/* The buffer capacity, always a power of 2 */
	unsigned long capacity;
	/* Producer index */
	unsigned long prod;
	/* Consumer index */
	unsigned long cons;

	/* Read lock */
	struct uk_mutex rdlock;
	/* Write lock */
	struct uk_mutex wrlock;

	/* Readers queue */
	struct uk_waitq rdwq;
	/* Writers queue */
	struct uk_waitq wrwq;
};

#define PIPE_BUF_IDX(buf, n)    ((n) & ((buf)->capacity - 1))
#define PIPE_BUF_PROD_IDX(buf)  PIPE_BUF_IDX((buf), (buf)->prod)
#define PIPE_BUF_CONS_IDX(buf)  PIPE_BUF_IDX((buf), (buf)->cons)

struct pipe_file {
	/* Pipe buffer */
	struct pipe_buf *buf;
	/* Write reference count */
	int w_refcount;
	/* Read reference count */
	int r_refcount;
	/* Flags */
	int flags;
};


static struct pipe_buf *pipe_buf_alloc(int capacity)
{
	struct pipe_buf *pipe_buf;

	UK_ASSERT(POWER_OF_2(capacity));

	pipe_buf = malloc(sizeof(*pipe_buf));
	if (!pipe_buf)
		return NULL;

	pipe_buf->data = malloc(capacity);
	if (!pipe_buf->data) {
		free(pipe_buf);
		return NULL;
	}

	pipe_buf->capacity = capacity;
	pipe_buf->cons = 0;
	pipe_buf->prod = 0;
	uk_mutex_init(&pipe_buf->rdlock);
	uk_mutex_init(&pipe_buf->wrlock);
	uk_waitq_init(&pipe_buf->rdwq);
	uk_waitq_init(&pipe_buf->wrwq);

	return pipe_buf;
}

void pipe_buf_free(struct pipe_buf *pipe_buf)
{
	free(pipe_buf->data);
	free(pipe_buf);
}

static unsigned long pipe_buf_get_available(const struct pipe_buf *pipe_buf)
{
	return pipe_buf->prod - pipe_buf->cons;
}

static unsigned long pipe_buf_get_free_space(struct pipe_buf *pipe_buf)
{
	return pipe_buf->capacity - pipe_buf_get_available(pipe_buf);
}

static int pipe_buf_can_write(struct pipe_buf *pipe_buf)
{
	return pipe_buf_get_free_space(pipe_buf) > 0;
}

static int pipe_buf_can_read(struct pipe_buf *pipe_buf)
{
	return pipe_buf_get_available(pipe_buf) > 0;
}

static unsigned long pipe_buf_write(struct pipe_buf *pipe_buf,
		struct iovec *iovec, size_t iovec_off)
{
	unsigned long prod_idx, to_write;
	void *iovec_data = iovec->iov_base + iovec_off;
	size_t iov_len = iovec->iov_len - iovec_off;

	prod_idx = PIPE_BUF_PROD_IDX(pipe_buf);
	to_write = MIN(pipe_buf_get_free_space(pipe_buf), iov_len);
	if (to_write == 0)
		goto out;

	/* Copy in one piece */
	if (prod_idx + to_write <= pipe_buf->capacity)
		memcpy(pipe_buf->data + prod_idx, iovec_data, to_write);

	else {
		int first_copy_bytes, second_copy_bytes;

		/* Copy the first part */
		first_copy_bytes = pipe_buf->capacity - prod_idx;
		memcpy(pipe_buf->data + prod_idx,
				iovec_data,
				first_copy_bytes);

		/* Copy the second part */
		second_copy_bytes = prod_idx + to_write - pipe_buf->capacity;
		memcpy(pipe_buf->data,
				iovec_data + first_copy_bytes,
				second_copy_bytes);
	}

	/* Update producer */
	pipe_buf->prod += to_write;

out:
	return to_write;
}

static unsigned long pipe_buf_read(struct pipe_buf *pipe_buf,
		struct iovec *iovec, size_t iovec_off)
{
	unsigned long cons_idx, to_read;
	void *iovec_data = iovec->iov_base + iovec_off;
	size_t iov_len = iovec->iov_len - iovec_off;

	cons_idx = PIPE_BUF_CONS_IDX(pipe_buf);
	to_read = MIN(pipe_buf_get_available(pipe_buf), iov_len);
	if (to_read == 0)
		goto out;

	/* Copy in one piece */
	if (cons_idx + to_read <= pipe_buf->capacity)
		memcpy(iovec_data, pipe_buf->data + cons_idx, to_read);

	else {
		int first_copy_bytes;
		int second_copy_bytes;

		/* Copy the first part */
		first_copy_bytes = pipe_buf->capacity - pipe_buf->cons;
		memcpy(iovec_data,
				pipe_buf->data + cons_idx,
				first_copy_bytes);

		/* Copy the second part */
		second_copy_bytes = cons_idx + to_read - pipe_buf->capacity;
		memcpy(iovec_data + first_copy_bytes,
				pipe_buf->data,
				second_copy_bytes);
	}

	/* Update consumer */
	pipe_buf->cons += to_read;

out:
	return to_read;
}

struct pipe_file *pipe_file_alloc(int capacity, int flags)
{
	struct pipe_file *pipe_file;

	pipe_file = malloc(sizeof(*pipe_file));
	if (!pipe_file)
		return NULL;

	pipe_file->buf = pipe_buf_alloc(capacity);
	if (!pipe_file->buf) {
		free(pipe_file);
		return NULL;
	}

	pipe_file->w_refcount = 1;
	pipe_file->r_refcount = 1;
	pipe_file->flags = flags;

	return pipe_file;
}

void pipe_file_free(struct pipe_file *pipe_file)
{
	pipe_buf_free(pipe_file->buf);
	free(pipe_file);
}

static int pipe_write(struct vnode *vnode,
		struct uio *buf, int ioflag __unused)
{
	struct pipe_file *pipe_file = vnode->v_data;
	struct pipe_buf *pipe_buf = pipe_file->buf;
	bool nonblocking = false; /* TODO handle nonblocking */
	bool data_available = true;
	int uio_idx = 0;

	if (!pipe_file->r_refcount) {
		/* TODO before returning the error, send a SIGPIPE signal */
		return -EPIPE;
	}

	uk_mutex_lock(&pipe_buf->wrlock);
	while (data_available && uio_idx < buf->uio_iovcnt) {
		struct iovec *iovec = &buf->uio_iov[uio_idx];
		unsigned long off = 0;

		while (off < iovec->iov_len) {
			unsigned long written_bytes;

			written_bytes = pipe_buf_write(pipe_buf, iovec, off);
			if (written_bytes == 0) {
				/* No data */
				if (nonblocking) {
					data_available = false;
					break;

				} else {
					/* Wait until data available */
					while (!pipe_buf_can_write(pipe_buf)) {
						uk_mutex_unlock(&pipe_buf->wrlock);
						uk_waitq_wait_event(&pipe_buf->wrwq,
							pipe_buf_can_write(pipe_buf));
						uk_mutex_lock(&pipe_buf->wrlock);
					}
				}

			} else {
				/* Update bytes written_bytes. */
				buf->uio_resid -= written_bytes;

				off += written_bytes;

				/* wake some readers */
				uk_waitq_wake_up(&pipe_buf->rdwq);
			}
		}

		uio_idx++;
	}
	uk_mutex_unlock(&pipe_buf->wrlock);

	return 0;
}

static int pipe_read(struct vnode *vnode,
		struct vfscore_file *vfscore_file,
		struct uio *buf, int ioflag __unused)
{
	struct pipe_file *pipe_file = vnode->v_data;
	struct pipe_buf *pipe_buf = pipe_file->buf;
	bool nonblocking = (vfscore_file->f_flags & O_NONBLOCK);
	bool data_available = true;
	int uio_idx = 0;

	uk_mutex_lock(&pipe_buf->rdlock);
	if (nonblocking && !pipe_buf_can_read(pipe_buf)) {
		uk_mutex_unlock(&pipe_buf->rdlock);
		return EAGAIN;
	}

	while (data_available && uio_idx < buf->uio_iovcnt) {
		struct iovec *iovec = &buf->uio_iov[uio_idx];
		unsigned long off = 0;

		while (off < iovec->iov_len) {
			unsigned long read_bytes;

			read_bytes = pipe_buf_read(pipe_buf, iovec, off);
			if (read_bytes == 0) {
				/* No data */
				if (nonblocking) {
					data_available = false;
					break;

				} else {
					/* Wait until data available */
					while (!pipe_buf_can_read(pipe_buf)) {
						uk_mutex_unlock(&pipe_buf->rdlock);
						uk_waitq_wait_event(&pipe_buf->rdwq,
							pipe_buf_can_read(pipe_buf));
						uk_mutex_lock(&pipe_buf->rdlock);
					}
				}

			} else {
				/* Update bytes read */
				buf->uio_resid -= read_bytes;

				off += read_bytes;

				/* wake some writers */
				uk_waitq_wake_up(&pipe_buf->wrwq);
			}
		}

		uio_idx++;
	}
	uk_mutex_unlock(&pipe_buf->rdlock);

	return 0;
}

static int pipe_close(struct vnode *vnode,
		struct vfscore_file *vfscore_file)
{
	struct pipe_file *pipe_file = vnode->v_data;

	UK_ASSERT(vfscore_file->f_dentry->d_vnode == vnode);
	UK_ASSERT(vnode->v_refcnt == 1);

	if (vfscore_file->f_flags & UK_FREAD)
		pipe_file->r_refcount--;

	if (vfscore_file->f_flags & UK_FWRITE)
		pipe_file->w_refcount--;

	if (!pipe_file->r_refcount && !pipe_file->w_refcount)
		pipe_file_free(pipe_file);

	return 0;
}

static int pipe_seek(struct vnode *vnode __unused,
			struct vfscore_file *vfscore_file __unused,
			off_t off1 __unused, off_t off2 __unused)
{
	errno = ESPIPE;
	return -1;
}

static int pipe_ioctl(struct vnode *vnode,
		struct vfscore_file *vfscore_file __unused,
		unsigned long com, void *data)
{
	struct pipe_file *pipe_file = vnode->v_data;
	struct pipe_buf *pipe_buf = pipe_file->buf;

	switch (com) {
	case FIONREAD:
		uk_mutex_lock(&pipe_buf->rdlock);
		*((int *) data) = pipe_buf_get_available(pipe_buf);
		uk_mutex_unlock(&pipe_buf->rdlock);
		return 0;
	default:
		return -EINVAL;
	}
}

#define pipe_open        ((vnop_open_t) vfscore_vop_einval)
#define pipe_fsync       ((vnop_fsync_t) vfscore_vop_nullop)
#define pipe_readdir     ((vnop_readdir_t) vfscore_vop_einval)
#define pipe_lookup      ((vnop_lookup_t) vfscore_vop_einval)
#define pipe_create      ((vnop_create_t) vfscore_vop_einval)
#define pipe_remove      ((vnop_remove_t) vfscore_vop_einval)
#define pipe_rename      ((vnop_rename_t) vfscore_vop_einval)
#define pipe_mkdir       ((vnop_mkdir_t) vfscore_vop_einval)
#define pipe_rmdir       ((vnop_rmdir_t) vfscore_vop_einval)
#define pipe_getattr     ((vnop_getattr_t) vfscore_vop_einval)
#define pipe_setattr     ((vnop_setattr_t) vfscore_vop_nullop)
#define pipe_inactive    ((vnop_inactive_t) vfscore_vop_einval)
#define pipe_truncate    ((vnop_truncate_t) vfscore_vop_nullop)
#define pipe_link        ((vnop_link_t) vfscore_vop_eperm)
#define pipe_cache       ((vnop_cache_t) NULL)
#define pipe_readlink    ((vnop_readlink_t) vfscore_vop_einval)
#define pipe_symlink     ((vnop_symlink_t) vfscore_vop_eperm)
#define pipe_fallocate   ((vnop_fallocate_t) vfscore_vop_nullop)

static struct vnops pipe_vnops = {
	.vop_open      = pipe_open,
	.vop_close     = pipe_close,
	.vop_read      = pipe_read,
	.vop_write     = pipe_write,
	.vop_seek      = pipe_seek,
	.vop_ioctl     = pipe_ioctl,
	.vop_fsync     = pipe_fsync,
	.vop_readdir   = pipe_readdir,
	.vop_lookup    = pipe_lookup,
	.vop_create    = pipe_create,
	.vop_remove    = pipe_remove,
	.vop_rename    = pipe_rename,
	.vop_mkdir     = pipe_mkdir,
	.vop_rmdir     = pipe_rmdir,
	.vop_getattr   = pipe_getattr,
	.vop_setattr   = pipe_setattr,
	.vop_inactive  = pipe_inactive,
	.vop_truncate  = pipe_truncate,
	.vop_link      = pipe_link,
	.vop_cache     = pipe_cache,
	.vop_fallocate = pipe_fallocate,
	.vop_readlink  = pipe_readlink,
	.vop_symlink   = pipe_symlink
};

#define pipe_vget  ((vfsop_vget_t) vfscore_vop_nullop)

static struct vfsops pipe_vfsops = {
	.vfs_vget = pipe_vget,
	.vfs_vnops = &pipe_vnops
};

static uint64_t p_inode;

/*
 * Bogus mount point used by all sockets
 */
static struct mount p_mount = {
	.m_op = &pipe_vfsops
};

static int pipe_fd_alloc(struct pipe_file *pipe_file, int flags)
{
	int ret = 0;
	int vfs_fd;
	struct vfscore_file *vfs_file = NULL;
	struct dentry *p_dentry;
	struct vnode *p_vnode;

	/* Reserve file descriptor number */
	vfs_fd = vfscore_alloc_fd();
	if (vfs_fd < 0) {
		ret = -ENFILE;
		goto ERR_EXIT;
	}

	/* Allocate file, dentry, and vnode */
	vfs_file = calloc(1, sizeof(*vfs_file));
	if (!vfs_file) {
		ret = -ENOMEM;
		goto ERR_MALLOC_VFS_FILE;
	}

	ret = vfscore_vget(&p_mount, p_inode++, &p_vnode);
	UK_ASSERT(ret == 0); /* we should not find it in cache */

	if (!p_vnode) {
		ret = -ENOMEM;
		goto ERR_ALLOC_VNODE;
	}

	uk_mutex_unlock(&p_vnode->v_lock);

	p_dentry = dentry_alloc(NULL, p_vnode, "/");
	if (!p_dentry) {
		ret = -ENOMEM;
		goto ERR_ALLOC_DENTRY;
	}

	/* Fill out necessary fields. */
	vfs_file->fd = vfs_fd;
	vfs_file->f_flags = flags;
	vfs_file->f_count = 1;
	vfs_file->f_data = pipe_file;
	vfs_file->f_dentry = p_dentry;
	vfs_file->f_vfs_flags = UK_VFSCORE_NOPOS;

	p_vnode->v_data = pipe_file;
	p_vnode->v_type = VFIFO;

	/* Assign the file descriptors to the corresponding vfs_file. */
	ret = vfscore_install_fd(vfs_fd, vfs_file);
	if (ret)
		goto ERR_VFS_INSTALL;

	/* Only the dentry should hold a reference; release ours */
	vrele(p_vnode);

	return vfs_fd;

ERR_VFS_INSTALL:
	drele(p_dentry);
ERR_ALLOC_DENTRY:
	vrele(p_vnode);
ERR_ALLOC_VNODE:
	free(vfs_file);
ERR_MALLOC_VFS_FILE:
	vfscore_put_fd(vfs_fd);
ERR_EXIT:
	UK_ASSERT(ret < 0);
	return ret;
}

int pipe(int pipefd[2])
{
	int ret = 0;
	int r_fd, w_fd;
	struct pipe_file *pipe_file;

	/* Allocate pipe internal structure. */
	pipe_file = pipe_file_alloc(PIPE_MAX_SIZE, 0);
	if (!pipe_file) {
		ret = -ENOMEM;
		goto ERR_EXIT;
	}

	r_fd = pipe_fd_alloc(pipe_file, UK_FREAD);
	if (r_fd < 0)
		goto ERR_VFS_INSTALL;

	w_fd = pipe_fd_alloc(pipe_file, UK_FWRITE);
	if (w_fd < 0)
		goto ERR_W_FD;

	/* Fill pipefd fields. */
	pipefd[0] = r_fd;
	pipefd[1] = w_fd;

	return ret;

ERR_W_FD:
	vfscore_put_fd(r_fd);
ERR_VFS_INSTALL:
	pipe_file_free(pipe_file);
ERR_EXIT:
	UK_ASSERT(ret < 0);
	return ret;
}
