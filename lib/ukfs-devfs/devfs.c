/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/devfs.h>
#include <uk/fs/template/pseudo.h>

const struct uk_file *uk_fs_devfs_root;

UK_FS_TMPL_PSEUDO(devfs, "ramfs", uk_fs_devfs_root,
		  UK_FS_VOPEN_NULLVOL, 0, UK_FS_VOPEN_NULLDATA,
		  UK_FS_VOPEN_VOL_IGNORE | UK_FS_VOPEN_DATA_IGNORE);
