/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Yuri Volchkov <yuri.volchkov@neclab.eu>
 *
 *
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
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

#include <vfscore/file.h>
#include <vfscore/fs.h>
#include <uk/plat/console.h>
#include <uk/essentials.h>
#include <termios.h>
#include <vfscore/vnode.h>
#include <unistd.h>
#include <vfscore/uio.h>
#include <vfscore/vnode.h>
#include <vfscore/mount.h>

static int __write_fn(void *dst __unused, void *src, size_t *cnt)
{
	int ret = ukplat_coutk(src, *cnt);

	if (ret < 0)
		/* TODO: remove -1 when vfscore switches to negative
		 * error numbers
		 */
		return ret * -1;

	*cnt = (size_t) ret;
	return 0;
}

/* One function for stderr and stdout */
static int stdio_write(struct vnode *vp __unused,
			   struct uio *uio,
			   int ioflag __unused)
{
	if (uio->uio_offset)
		return ESPIPE;

	return vfscore_uioforeach(__write_fn, NULL, uio->uio_resid, uio);
}


static int __read_fn(void *dst, void *src __unused, size_t *cnt)
{
	int bytes_read;
	size_t bytes_total = 0, count;
	char *buf = dst;

	count = *cnt;

	do {
		while ((bytes_read = ukplat_cink(buf,
			count - bytes_total)) <= 0)
			;

		buf = buf + bytes_read;
		*(buf - 1) = *(buf - 1) == '\r' ?
					'\n' : *(buf - 1);

		/* Echo the input */
		if (*(buf - 1) == '\177') {
			/* DELETE control character */
			if (buf - 1 != dst) {
				/* If this is not the first byte */
				ukplat_coutk("\b \b", 3);
				buf -= 1;
				bytes_total -= 1;
			}
			buf -= 1;
		} else {
			ukplat_coutk(buf - bytes_read, bytes_read);
			bytes_total += bytes_read;
		}

	} while (bytes_total < count && *(buf - 1) != '\n'
			&& *(buf - 1) != VEOF);

	*cnt = bytes_total;

	/* The INT_MIN here is a special return code. It makes the
	 * vfscore_uioforeach to quit from the loop. But this is not
	 * an error (for example a user hit Ctrl-C). That is why this
	 * special return value is fixed up to 0 in the stdio_read.
	 */
	if (*(buf - 1) == '\n' || *(buf - 1) == VEOF)
		return INT_MIN;

	return 0;
}

static int stdio_read(struct vnode *vp __unused,
		      struct vfscore_file *file __unused,
		      struct uio *uio,
		      int ioflag __unused)
{
	int ret;

	if (uio->uio_offset)
		return ESPIPE;

	ret = vfscore_uioforeach(__read_fn, NULL, uio->uio_resid, uio);
	ret = (ret == INT_MIN) ? 0 : ret;

	return ret;
}

static int
stdio_getattr(struct vnode *vnode __unused, struct vattr *attr __unused)
{
	return 0;
}

#define stdio_open	((vnop_open_t)vfscore_nullop)
#define stdio_close	((vnop_close_t)vfscore_nullop)
#define stdio_seek	((vnop_seek_t)vfscore_vop_nullop)
#define stdio_ioctl	((vnop_ioctl_t)vfscore_nullop)
#define stdio_fsync	((vnop_fsync_t)vfscore_vop_nullop)
#define stdio_readdir	((vnop_readdir_t)vfscore_vop_einval)
#define stdio_lookup	((vnop_lookup_t)vfscore_nullop)
#define stdio_create	((vnop_create_t)vfscore_vop_einval)
#define stdio_remove	((vnop_remove_t)vfscore_vop_einval)
#define stdio_rename	((vnop_rename_t)vfscore_vop_einval)
#define stdio_mkdir	((vnop_mkdir_t)vfscore_vop_einval)
#define stdio_rmdir	((vnop_rmdir_t)vfscore_vop_einval)
#define stdio_setattr	((vnop_setattr_t)vfscore_vop_eperm)
#define stdio_inactive	((vnop_inactive_t)vfscore_vop_nullop)
#define stdio_truncate	((vnop_truncate_t)vfscore_vop_nullop)
#define stdio_link	((vnop_link_t)vfscore_vop_eperm)
#define stdio_fallocate	((vnop_fallocate_t)vfscore_vop_nullop)
#define stdio_readlink	((vnop_readlink_t)vfscore_vop_nullop)
#define stdio_symlink	((vnop_symlink_t)vfscore_vop_nullop)

static struct vnops stdio_vnops = {
	stdio_open,		/* open */
	stdio_close,		/* close */
	stdio_read,		/* read */
	stdio_write,		/* write */
	stdio_seek,		/* seek */
	stdio_ioctl,		/* ioctl */
	stdio_fsync,		/* fsync */
	stdio_readdir,		/* readdir */
	stdio_lookup,		/* lookup */
	stdio_create,		/* create */
	stdio_remove,		/* remove */
	stdio_rename,		/* remame */
	stdio_mkdir,		/* mkdir */
	stdio_rmdir,		/* rmdir */
	stdio_getattr,		/* getattr */
	stdio_setattr,		/* setattr */
	stdio_inactive,		/* inactive */
	stdio_truncate,		/* truncate */
	stdio_link,		/* link */
	(vnop_cache_t) NULL, /* arc */
	stdio_fallocate,	/* fallocate */
	stdio_readlink,		/* read link */
	stdio_symlink,		/* symbolic link */
};

static struct vnode stdio_vnode = {
	.v_ino = 1,
	.v_op = &stdio_vnops,
	.v_lock = UK_MUTEX_INITIALIZER(stdio_vnode.v_lock),
	.v_refcnt = 1,
	.v_type = VCHR,
};

static struct dentry stdio_dentry = {
	.d_vnode = &stdio_vnode,
};

static struct vfscore_file  stdio_file = {
	.fd = 1,
	.f_flags = UK_FWRITE | UK_FREAD,
	.f_dentry = &stdio_dentry,
	.f_vfs_flags = UK_VFSCORE_NOPOS,
	/* reference count is 2 because close(0) is a valid
	 * operation. However it is not properly handled in the
	 * current implementation. */
	.f_count = 2,
};

void init_stdio(void)
{
	int fd;

	fd = vfscore_alloc_fd();
	UK_ASSERT(fd == 0);
	vfscore_install_fd(0, &stdio_file);
	if (dup2(0, 1) != 1)
		uk_pr_err("failed to dup to stdin\n");
	if (dup2(0, 2) != 2)
		uk_pr_err("failed to dup to stderr\n");
}
