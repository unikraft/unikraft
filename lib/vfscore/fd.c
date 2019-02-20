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

#include <string.h>
#include <uk/essentials.h>
#include <uk/bitops.h>
#include <uk/assert.h>
#include <vfscore/file.h>
#include <uk/plat/lcpu.h>
#include <errno.h>

#define FDTABLE_MAX_FILES (sizeof(uint64_t) * 8)

void init_stdio(void);

struct fdtable {
	uint64_t bitmap;
	uint32_t fd_start;
	struct vfscore_file *files[FDTABLE_MAX_FILES];
};
struct fdtable fdtable;

int vfscore_alloc_fd(void)
{
	unsigned long flags;
	int ret;

	flags = ukplat_lcpu_save_irqf();
	ret = ukarch_ffsl(~fdtable.bitmap);

	if (!ret) {
		ret = -ENFILE;
		goto exit;
	}

	fdtable.bitmap |= (uint64_t) 1 << ret;

exit:
	ukplat_lcpu_restore_irqf(flags);
	return ret;
}

void vfscore_put_fd(int fd)
{
	struct vfscore_file *fp;
	unsigned long flags;

	UK_ASSERT(fd < (int) FDTABLE_MAX_FILES);
	/* Currently it is not allowed to free std(in|out|err) */
	UK_ASSERT(fd > 2);

	flags = ukplat_lcpu_save_irqf();
	__uk_clear_bit(fd, &fdtable.bitmap);\
	fp = fdtable.files[fd];
	fdtable.files[fd] = NULL;
	ukplat_lcpu_restore_irqf(flags);

	fdrop(fp);
}

int vfscore_install_fd(int fd, struct vfscore_file *file)
{
	unsigned long flags;
	struct vfscore_file *orig;

	if ((fd >= (int) FDTABLE_MAX_FILES) || (!file))
		return -EBADF;

	fhold(file);

	file->fd = fd;

	flags = ukplat_lcpu_save_irqf();
	orig = fdtable.files[fd];
	fdtable.files[fd] = file;
	ukplat_lcpu_restore_irqf(flags);

	fdrop(file);

	if (orig)
		fdrop(orig);

	return 0;
}

struct vfscore_file *vfscore_get_file(int fd)
{
	unsigned long flags;
	struct vfscore_file *ret = NULL;

	UK_ASSERT(fd < (int) FDTABLE_MAX_FILES);

	flags = ukplat_lcpu_save_irqf();
	if (!(fdtable.bitmap & ((uint64_t) 1 << fd)))
		goto exit;
	ret = fdtable.files[fd];
	fhold(ret);

exit:
	ukplat_lcpu_restore_irqf(flags);
	return ret;
}

void vfscore_put_file(struct vfscore_file *file)
{
	fdrop(file);
}

int fget(int fd, struct vfscore_file **out_fp)
{
	int ret = 0;
	struct vfscore_file *fp = vfscore_get_file(fd);

	if (!fp)
		ret = EBADF;
	else
		*out_fp = fp;

	return ret;
}

int fdalloc(struct vfscore_file *fp, int *newfd)
{
	int fd, ret = 0;

	fhold(fp);

	fd = vfscore_alloc_fd();
	if (fd < 0) {
		ret = fd;
		goto exit;
	}

	ret = vfscore_install_fd(fd, fp);
	if (ret)
		fdrop(fp);
	else
		*newfd = fd;

exit:
	return ret;
}


/* TODO: move this constructor to main.c */
__constructor static void fdtable_init(void)
{
	memset(&fdtable, 0, sizeof(fdtable));

	/* reserve stdin, stdout and stderr */
	fdtable.bitmap = 7;
	init_stdio();
}
