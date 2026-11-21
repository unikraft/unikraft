/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __VFSCORE_BORROW_H__
#define __VFSCORE_BORROW_H__

#include <stddef.h>
#include <sys/ioctl.h>

/*
 * ioctl on an empty regular file: make @data its contents without copying
 * it.  The file system reads @data in place and copies it before the first
 * change, so @data may be read-only, and must outlive the file.  ramfs
 * supports it; other file systems fail the ioctl, and the caller writes
 * the data instead.
 */
struct vfscore_borrow {
	const void *data;
	size_t len;
};

#define VFSCORE_IOC_BORROW _IOW('v', 0x42, struct vfscore_borrow)

#endif /* __VFSCORE_BORROW_H__ */
