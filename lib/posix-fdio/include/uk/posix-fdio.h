/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* File I/O and control */

#ifndef __UK_POSIX_FDIO_H__
#define __UK_POSIX_FDIO_H__

#include <sys/stat.h>

#include <uk/posix-fd.h>

/* I/O */

ssize_t uk_sys_preadv(struct uk_ofile *of, const struct iovec *iov, int iovcnt,
		      off_t offset);

ssize_t uk_sys_pread(struct uk_ofile *of, void *buf, size_t count,
		     off_t offset);

ssize_t uk_sys_readv(struct uk_ofile *of, const struct iovec *iov, int iovcnt);

ssize_t uk_sys_read(struct uk_ofile *of, void *buf, size_t count);

ssize_t uk_sys_preadv2(struct uk_ofile *of, const struct iovec *iov, int iovcnt,
		       off_t offset, int flags);

ssize_t uk_sys_pwritev(struct uk_ofile *of, const struct iovec *iov, int iovcnt,
		       off_t offset);

ssize_t uk_sys_pwrite(struct uk_ofile *of, const void *buf, size_t count,
		  off_t offset);

ssize_t uk_sys_writev(struct uk_ofile *of, const struct iovec *iov, int iovcnt);

ssize_t uk_sys_write(struct uk_ofile *of, const void *buf, size_t count);

ssize_t uk_sys_pwritev2(struct uk_ofile *of, const struct iovec *iov,
			int iovcnt, off_t offset, int flags);


off_t uk_sys_lseek(struct uk_ofile *of, off_t offset, int whence);

/* Metadata */

int uk_sys_fstat(struct uk_ofile *of, struct stat *statbuf);

int uk_sys_fstatx(struct uk_ofile *of, unsigned int mask,
		  struct uk_statx *statxbuf);

int uk_sys_fchmod(struct uk_ofile *of, mode_t mode);

int uk_sys_fchown(struct uk_ofile *of, uid_t owner, gid_t group);

#endif /* __UK_POSIX_FDIO_H__ */
