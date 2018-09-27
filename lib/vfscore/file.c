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

#include <unistd.h>
#include <errno.h>
#include <uk/print.h>
#include <vfscore/file.h>
#include <uk/assert.h>


int close(int fd)
{
	struct vfscore_file *file = vfscore_get_file(fd);

	if (!file) {
		uk_pr_warn("no such file descriptor: %d\n", fd);
		errno = EBADF;
		return -1;
	}

	if (!file->fops->close) {
		errno = EIO;
		return -1;
	}

	return file->fops->close(file);
}

ssize_t write(int fd, const void *buf, size_t count)
{
	struct vfscore_file *file = vfscore_get_file(fd);

	if (!file) {
		uk_pr_warn("no such file descriptor: %d\n", fd);
		errno = EBADF;
		return -1;
	}

	if (!file->fops->write) {
		uk_pr_warn("file does not have write op: %d\n", fd);
		errno = EINVAL;
		return -1;
	}

	return file->fops->write(file, buf, count);
}

ssize_t read(int fd, void *buf, size_t count)
{
	struct vfscore_file *file = vfscore_get_file(fd);

	if (!file) {
		uk_pr_warn("no such file descriptor: %d\n", fd);
		errno = EBADF;
		return -1;
	}

	if (!file->fops->read) {
		uk_pr_warn("file does not have read op: %d\n", fd);
		errno = EINVAL;
		return -1;
	}

	return file->fops->read(file, buf, count);
}
