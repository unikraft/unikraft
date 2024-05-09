/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Convenience uk_file stub operations */

#ifndef __UKFILE_FILE_NOPS_H__
#define __UKFILE_FILE_NOPS_H__

#include <uk/file.h>

extern const struct uk_file_ops uk_file_nops;

ssize_t uk_file_nop_read(const struct uk_file *f,
			 const struct iovec *iov, int iovcnt,
			 off_t off, long flags);

ssize_t uk_file_nop_write(const struct uk_file *f,
			  const struct iovec *iov, int iovcnt,
			  off_t off, long flags);

int uk_file_nop_getstat(const struct uk_file *f,
			unsigned int mask, struct uk_statx *arg);

int uk_file_nop_setstat(const struct uk_file *f,
			unsigned int mask, const struct uk_statx *arg);

int uk_file_nop_ctl(const struct uk_file *f, int fam, int req,
		    uintptr_t arg1, uintptr_t arg2, uintptr_t arg3);

#endif /* __UKFILE_FILE_NOPS_H__ */
