/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Convenience filesystem operations */

#ifndef __UKFS_FS_COMMON_OPS_H__
#define __UKFS_FS_COMMON_OPS_H__

#include <uk/fs.h>

/* No-op filesystem sync; does nothing successfully */
int uk_fs_common_nop_sync(const struct uk_file *f);

/* Unsupported operations; return -ENOSYS */

const struct uk_file *uk_fs_common_nop_rebind(const struct uk_file *f,
					      unsigned long flags,
					      const void *data);

int uk_fs_common_nop_graft(const struct uk_file *f, const struct uk_file *ref);

/* Operations for read-only filesystems, return -EROFS */

const struct uk_file *uk_fs_common_rofs_create(const struct uk_file *f,
					       const char *name, size_t len,
					       unsigned int mode, int flags,
					       union uk_fs_create_target targ);

int uk_fs_common_rofs_unlink(const struct uk_file *f, const char *name,
			     size_t len, unsigned int flags);

int uk_fs_common_rofs_rename(const struct uk_file *f,
			     const char *name, size_t nlen,
			     const struct uk_file *dest,
			     const char *dname, size_t dlen,
			     unsigned int flags);

#endif /* __UKFS_FS_COMMON_OPS_H__ */
