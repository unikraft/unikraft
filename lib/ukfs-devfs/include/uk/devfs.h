/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_FS_DEVFS_H__
#define __UK_FS_DEVFS_H__

#include <uk/file.h>

/* Singleton static reference initialized at boot time */
extern const struct uk_file *uk_fs_devfs_root;

#endif /* __UK_FS_DEVFS_H__ */
