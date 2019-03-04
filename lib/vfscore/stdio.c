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

/* One function for stderr and stdout */
static ssize_t stdio_write(struct vnode *vp __unused,
			   struct uio *uio,
			   int ioflag __unused)
{
	UK_ASSERT(!uio->uio_offset);
	UK_ASSERT(uio->uio_iovcnt == 1);
	return ukplat_coutk(uio->uio_iov->iov_base, uio->uio_iov->iov_len);
}

static ssize_t stdio_read(struct vnode *vp __unused,
		      struct vfscore_file *file __unused,
		      struct uio *uio,
		      int ioflag __unused)
{
	int bytes_read;
	size_t bytes_total = 0, count;
	char *buf;

	UK_ASSERT(!uio->uio_offset);
	UK_ASSERT(uio->uio_iovcnt == 1);

	buf = uio->uio_iov->iov_base;
	count = uio->uio_iov->iov_len;

	do {
		while ((bytes_read = ukplat_cink(buf,
			count - bytes_total)) <= 0)
			;

		buf = buf + bytes_read;
		*(buf - 1) = *(buf - 1) == '\r' ?
					'\n' : *(buf - 1);

		/* Echo the input */
		ukplat_coutk(buf - bytes_read, bytes_read);
		bytes_total += bytes_read;

	} while (bytes_total < count && *(buf - 1) != '\n'
			&& *(buf - 1) != VEOF);

	return bytes_total;
}

static int
stdio_getattr(struct vnode *vnode __unused, struct vattr *attr __unused)
{
	return 0;
}

static struct vnops stdio_vnops = {
	.vop_write = stdio_write,
	.vop_read = stdio_read,
	.vop_getattr = stdio_getattr,
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
	/* reference count is 2 because close(0) is a valid
	 * operation. However it is not properly handled in the
	 * current implementation. */
	.f_count = 2,
};

void init_stdio(void)
{
	vfscore_install_fd(0, &stdio_file);
	if (dup2(0, 1) != 1)
		uk_pr_err("failed to dup to stdin\n");
	if (dup2(0, 2) != 2)
		uk_pr_err("failed to dup to stderr\n");
}
