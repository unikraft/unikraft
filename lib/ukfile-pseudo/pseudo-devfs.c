/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/assert.h>
#include <uk/devfs.h>
#include <uk/init.h>
#include <uk/file-pseudo.h>
#include <uk/fs.h>
#include <uk/fs/prio.h>

static int init_ukfile_pseudo_devfs(struct uk_init_ctx *ictx __unused)
{
	const union uk_fs_create_target null_target = { .file = &uk_file_null };
	const union uk_fs_create_target zero_target = { .file = &uk_file_zero };
	const void *r;

	/* We borrow the singleton static reference, no refcounting needed */
	UK_ASSERT(uk_fs_devfs_root);

	/* We do not clean up created files on error, as they will be dropped
	 * when the devfs root is released on system shutdown.
	 */
	r = uk_fs_createat(uk_fs_devfs_root, "null", 4,
			   0666, O_EXCL, null_target);
	if (unlikely(PTRISERR(r))) {
		uk_pr_err("Failed to create /dev/null: %d\n", PTR2ERR(r));
		return PTR2ERR(r);
	}
	r = uk_fs_createat(uk_fs_devfs_root, "zero", 4,
			   0666, O_EXCL, zero_target);
	if (unlikely(PTRISERR(r))) {
		uk_pr_err("Failed to create /dev/zero: %d\n", PTR2ERR(r));
		return PTR2ERR(r);
	}
	return 0;
}

uk_rootfs_initcall_prio(init_ukfile_pseudo_devfs, 0, UK_FS_PRIO_FSAVAIL);
