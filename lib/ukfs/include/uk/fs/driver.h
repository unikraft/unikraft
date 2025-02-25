/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Filesystem driver lookup & registration */

#ifndef __UKFS_FS_DRIVER_H__
#define __UKFS_FS_DRIVER_H__

#include <uk/fs.h>

/**
 * Open volume `vol` and return its root directory ready for use.
 *
 * This function is the entry point for a filesystem driver, and the only
 * operation that is not relative to a filesystem node.
 *
 * @param vol Driver-specific volume specifier
 * @param flags Bitmask of flags common to all filesystems
 * @param data Driver-specific additional options
 *
 * @return
 *  !PTRISERR: Reference to the root directory of the filesystem.
 *   PTRISERR: Negative error code encoded in return
 */
typedef const struct uk_file *(*uk_fs_vopen_func)(const void *vol,
						  unsigned long flags,
						  const void *data);

/**
 * Retrieve the vopen function of the filesystem driver registered for the
 * filesystem named `fstype`.
 *
 * @return
 *  != NULL: vopen function of desired filesystem
 *  == NULL: Filesystem name `fstype` not recognized
 */
uk_fs_vopen_func uk_fs_driver(const char *fstype);

/**
 * Entry for registering a filesystem driver with ukfs.
 */
struct uk_fs_drv {
	const char *fstype; /* Filesystem name, used to look up driver */
	uk_fs_vopen_func vopen; /* Vopen operation used to create fs mounts */
};

/**
 * Convenience macro to register a ukfs driver.
 *
 * @param fsname Filesystem name to register; do not enclose in quotes
 * @param drv_vopen vopen function of filesystem driver
 */
#define UK_FS_DRIVER_REGISTER(fsname, drv_vopen)		\
	__used __align(8)					\
		__section("." STRINGIFY(uk_fs_driver_##fsname))	\
	static const struct uk_fs_drv uk_fs_driver_##fsname = {	\
		.fstype = STRINGIFY(fsname),			\
		.vopen = (drv_vopen)				\
	}

#endif /* __UKFS_FS_DRIVER_H__ */
