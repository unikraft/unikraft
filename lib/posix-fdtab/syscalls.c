/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2005-2007, Kohsuke Ohtani
 * Copyright (C) 2014 Cloudius Systems, Ltd.
 * Copyright (c) 2019, NEC Europe Ltd., NEC Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#define _BSD_SOURCE
#define _GNU_SOURCE

#include <uk/fdtab/fd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

#include "syscalls.h"

int
sys_read(struct fdtab_file *fp, const struct iovec *iov, size_t niov,
		off_t offset, size_t *count)
{
	int error = 0;
	struct iovec *copy_iov;

	if ((fp->f_flags & UK_FREAD) == 0)
		return -EBADF;

	size_t bytes = 0;
	const struct iovec *iovp = iov;

	for (unsigned int i = 0; i < niov; i++) {
		if (iovp->iov_len > IOSIZE_MAX - bytes)
			return -EINVAL;
		bytes += iovp->iov_len;
		iovp++;
	}

	if (bytes == 0) {
		*count = 0;
		return 0;
	}

	struct uio uio;
	/* TODO: is it necessary to copy iov within Unikraft?
	 * OSv did this, mentioning this reason:
	 *
	 * "Unfortunately, the current implementation of fp->read
	 *  zeros the iov_len fields when it reads from disk, so we
	 *  have to copy iov. "
	 */
	copy_iov = calloc(sizeof(struct iovec), niov);
	if (!copy_iov)
		return ENOMEM;
	memcpy(copy_iov, iov, sizeof(struct iovec)*niov);

	uio.uio_iov = copy_iov;
	uio.uio_iovcnt = niov;
	// uio.uio_offset = (offset == -1) ? fp->f_offset : offset;
	uio.uio_offset = offset;
	uio.uio_resid = bytes;
	uio.uio_rw = UIO_WRITE;
	error = FDOP_READ(fp, &uio, 0);
	*count = bytes - uio.uio_resid;

	free(copy_iov);
	return error;
}

int
sys_write(struct fdtab_file *fp, const struct iovec *iov, size_t niov,
		off_t offset, size_t *count)
{
	struct iovec *copy_iov;
	int error = 0;

	if ((fp->f_flags & UK_FWRITE) == 0)
		return -EBADF;

	size_t bytes = 0;
	const struct iovec *iovp = iov;

	for (unsigned int i = 0; i < niov; i++) {
		if (iovp->iov_len > IOSIZE_MAX - bytes)
			return -EINVAL;
		bytes += iovp->iov_len;
		iovp++;
	}

	if (bytes == 0) {
		*count = 0;
		return 0;
	}

	struct uio uio;

	/* TODO: same note as in sys_read. Original comment:
	 *
	 * "Unfortunately, the current implementation of fp->write zeros the
	 *  iov_len fields when it writes to disk, so we have to copy iov.
	 */
	/* std::vector<iovec> copy_iov(iov, iov + niov); */
	copy_iov = calloc(sizeof(struct iovec), niov);
	if (!copy_iov)
		return -ENOMEM;
	memcpy(copy_iov, iov, sizeof(struct iovec)*niov);

	uio.uio_iov = copy_iov;
	uio.uio_iovcnt = niov;
	// uio.uio_offset = (offset == -1) ? fp->f_offset : offset;
	uio.uio_offset = offset;
	uio.uio_resid = bytes;
	uio.uio_rw = UIO_WRITE;
	error = FDOP_WRITE(fp, &uio, 0);
	*count = bytes - uio.uio_resid;

	free(copy_iov);
	return error;
}

int
sys_lseek(struct fdtab_file *fp, off_t off, int type, off_t *origin)
{
	if (fp->f_op->fdop_seek == NULL)
		return -ESPIPE;

	return FDOP_SEEK(fp, off, type, origin);
}

int
sys_ioctl(struct fdtab_file *fp, unsigned long request, void *buf)
{
	int error = 0;

	if ((fp->f_flags & (UK_FREAD | UK_FWRITE)) == 0)
		return -EBADF;

	switch (request) {
	case FIOCLEX:
		fp->f_flags |= O_CLOEXEC;
		break;
	case FIONCLEX:
		fp->f_flags &= ~O_CLOEXEC;
		break;
	default:
		if (fp->f_op->fdop_ioctl == NULL)
			return -ENOTTY;

		error = FDOP_IOCTL(fp, request, buf);
		break;
	}

	return error;
}

int
sys_fsync(struct fdtab_file *fp)
{
	if (fp->f_op->fdop_fsync == NULL)
		return -EINVAL;

	return FDOP_FSYNC(fp);
}

int
sys_fstat(struct fdtab_file *fp, struct stat *st)
{
	if (fp->f_op->fdop_fstat == NULL)
		return -EINVAL;

	return FDOP_FSTAT(fp, st);
}

int
sys_ftruncate(struct fdtab_file *fp, off_t length)
{
	if (fp->f_op->fdop_truncate == NULL)
		return -EINVAL;

	return FDOP_TRUNCATE(fp, length);
}

int
sys_fallocate(struct fdtab_file *fp, int mode, off_t offset, off_t len)
{
	int error;

	if (!(fp->f_flags & UK_FWRITE))
		return -EBADF;

	if (offset < 0 || len <= 0)
		return -EINVAL;

	// Strange, but that's what Linux returns.
	if ((mode & FALLOC_FL_PUNCH_HOLE) && !(mode & FALLOC_FL_KEEP_SIZE))
		return -ENOTSUP;

	if (fp->f_op->fdop_fallocate != NULL) {
		error = FDOP_FALLOCATE(fp, mode, offset, len);
	} else {
		// EOPNOTSUPP here means that the underlying file system
		// referred by fp doesn't support fallocate.
		error = -EOPNOTSUPP;
	}

	return error;
}
