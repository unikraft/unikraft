/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* vfscore_file Unikraft-internal syscall API */

#ifndef __VFSCORE_SYSCALLS_H__
#define __VFSCORE_SYSCALLS_H__

#include <vfscore/file.h>

ssize_t vfscore_read(struct vfscore_file *fp, void *buf, size_t count);
ssize_t vfscore_readv(struct vfscore_file *fp,
		      const struct iovec *iov, int iovcnt);
ssize_t vfscore_pread64(struct vfscore_file *fp,
			void *buf, size_t count, off_t offset);
ssize_t vfscore_preadv(struct vfscore_file *fp, const struct iovec *iov,
		       int iovcnt, off_t offset);

ssize_t vfscore_pwritev(struct vfscore_file *fp, const struct iovec *iov,
			int iovcnt, off_t offset);
ssize_t vfscore_pwrite64(struct vfscore_file *fp, const void *buf,
			 size_t count, off_t offset);
ssize_t vfscore_writev(struct vfscore_file *fp,
		       const struct iovec *vec, int vlen);
ssize_t vfscore_write(struct vfscore_file *fp, const void *buf, size_t count);
int vfscore_lseek(struct vfscore_file *fp, off_t off, int type, off_t *origin);

int vfscore_fstat(struct vfscore_file *fp, struct stat *st);

int vfscore_fcntl(struct vfscore_file *fp, unsigned int cmd, int arg);
int vfscore_ioctl(struct vfscore_file *fp, unsigned long request, void *buf);

#endif /* __VFSCORE_SYSCALLS_H__ */
