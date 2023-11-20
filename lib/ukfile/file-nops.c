/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>

#include <uk/file/nops.h>


ssize_t uk_file_nop_read(const struct uk_file *f __unused,
			 const struct iovec *iov __unused, int iovcnt __unused,
			 off_t off __unused, long flags __unused)
{
	return -ENOSYS;
}

ssize_t uk_file_nop_write(const struct uk_file *f __unused,
			  const struct iovec *iov __unused, int iovcnt __unused,
			  off_t off __unused, long flags __unused)
{
	return -ENOSYS;
}

int uk_file_nop_getstat(const struct uk_file *f __unused,
			unsigned mask __unused, struct uk_statx *arg __unused)
{
	return -ENOSYS;
}

int uk_file_nop_setstat(const struct uk_file *f __unused,
			unsigned mask __unused,
			const struct uk_statx *arg __unused)
{
	return -ENOSYS;
}

int uk_file_nop_ctl(const struct uk_file *f __unused, int fam __unused,
		    int req __unused, uintptr_t arg1 __unused,
		    uintptr_t arg2 __unused, uintptr_t arg3 __unused)
{
	return -ENOSYS;
}

const struct uk_file_ops uk_file_nops = {
	.read = uk_file_nop_read,
	.write = uk_file_nop_write,
	.getstat = uk_file_nop_getstat,
	.setstat = uk_file_nop_setstat,
	.ctl = uk_file_nop_ctl
};
