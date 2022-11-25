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
 */

#include <uk/config.h>
#include <uk/fdtab/fd.h>
#include <string.h>
#include <uk/essentials.h>
#include <uk/bitmap.h>
#include <uk/assert.h>
#include <vfscore/file.h>
#include <uk/plat/lcpu.h>
#include <errno.h>
#include <uk/init.h>
#if CONFIG_LIBPOSIX_PROCESS_CLONE
#include <uk/process.h>
#endif /* CONFIG_LIBPOSIX_PROCESS_CLONE */

int init_stdio(void);

int vfscore_alloc_fd(void)
{
	return fdtab_alloc_fd(fdtab_get_active());
}

int vfscore_put_fd(int fd)
{
	return fdtab_put_fd(fdtab_get_active(), fd);
}

int vfscore_install_fd(int fd, struct vfscore_file *file)
{
	return fdtab_install_fd(fdtab_get_active(), fd, &file->f_file);
}

struct vfscore_file *vfscore_get_file(int fd)
{
	struct fdtab_file *fp;

	fp = fdtab_get_file(fdtab_get_active(), fd);
	if (fp->f_op != &vfscore_fdops) {
		fdtab_put_file(fp);
		return NULL;
	}

	return __containerof(fp, struct vfscore_file, f_file);
}

void vfscore_put_file(struct vfscore_file *file)
{
	fdtab_fdrop(&file->f_file);
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

static int fdtable_init(void)
{
	memset(&fdtable, 0, sizeof(fdtable));

	return init_stdio();
}

uk_early_initcall_prio(fdtable_init, UK_PRIO_EARLIEST);

#if CONFIG_LIBPOSIX_PROCESS_CLONE
static int uk_posix_clone_files(const struct clone_args *cl_args,
				size_t cl_args_len __unused,
				struct uk_thread *child __unused,
				struct uk_thread *parent __unused)
{
	if (unlikely(!(cl_args->flags & CLONE_FILES))) {
		uk_pr_warn("CLONE_FILES not set");
		return -ENOTSUP;
	}

	/* CLONE_FILES says that file descriptor table is shared
	 * with the child, this is what we have implemented at the moment
	 */
	return 0;
}
UK_POSIX_CLONE_HANDLER(CLONE_FILES, false, uk_posix_clone_files, 0x0);
#endif /* CONFIG_LIBPOSIX_PROCESS_CLONE */
