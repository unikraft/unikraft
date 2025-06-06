/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_POSIX_VFS_FSTAB_EINITRD_H__
#define __UK_POSIX_VFS_FSTAB_EINITRD_H__

#ifndef __UK_POSIX_VFS_FSTAB_H__
#error "Do not include this header directly"
#endif /* __UK_POSIX_VFS_FSTAB_H__ */

#if CONFIG_LIBPOSIX_VFS_FSTAB_EINITRD

/* Implemented in einitrd.S */
extern const char vfs_einitrd_start[];
extern const char vfs_einitrd_end;

#endif /* CONFIG_LIBPOSIX_VFS_FSTAB_EINITRD */

#endif /* __UK_POSIX_VFS_FSTAB_EINITRD_H__ */
