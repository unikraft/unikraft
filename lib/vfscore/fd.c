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
#include <string.h>
#include <uk/essentials.h>
#include <uk/bitops/bitmap.h>
#include <uk/assert.h>
#include <vfscore/file.h>
#include <uk/plat/lcpu.h>
#include <errno.h>
#if CONFIG_LIBPOSIX_PROCESS_MULTITHREADING
#include <uk/process.h>
#endif /* CONFIG_LIBPOSIX_PROCESS_MULTITHREADING */

#include <uk/posix-fdtab-legacy.h>

struct vfscore_file *vfscore_get_file(int fd)
{
	return uk_fdtab_legacy_get(fd);
}

void vfscore_put_file(struct vfscore_file *file)
{
	fdrop(file);
}

int fget(int fd, struct vfscore_file **out_fp)
{
	struct vfscore_file *fp = vfscore_get_file(fd);
	if (!fp)
		return EBADF;
	*out_fp = fp;
	return 0;
}

int fdalloc(struct vfscore_file *fp, int *newfd)
{
	int r = uk_fdtab_legacy_open(fp);
	if (r < 0)
		return r;
	*newfd = r;
	return 0;
}

#if CONFIG_LIBPOSIX_PROCESS_MULTITHREADING
static int uk_posix_clone_files(void *arg)
{
	struct posix_process_clone_event_data *event_data;
	const struct clone_args *cl_args;

	event_data = (struct posix_process_clone_event_data *)arg;
	UK_ASSERT(event_data);

	cl_args = event_data->cl_args;
	UK_ASSERT(event_data->cl_args);

	if (unlikely(!(cl_args->flags & CLONE_FILES) &&
		     !(cl_args->flags & CLONE_VM))) {
		uk_pr_warn("CLONE_FILES not set");
		return -ENOTSUP;
	}

	/* CLONE_FILES says that file descriptor table is shared
	 * with the child, this is what we have implemented at the moment
	 */
	return UK_EVENT_HANDLED_CONT;
}

POSIX_PROCESS_CLONE_HANDLER(CLONE_FILES, uk_posix_clone_files);
#endif /* CONFIG_LIBPOSIX_PROCESS_MULTITHREADING */
