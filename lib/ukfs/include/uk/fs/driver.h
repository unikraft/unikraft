/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Filesystem driver lookup & registration */

#ifndef __UKFS_FS_DRIVER_H__
#define __UKFS_FS_DRIVER_H__

#include <uk/fs.h>

struct uk_fs_vopen_memimg {
	const void *base;
	size_t len;
};

union uk_fs_vopen_vol {
	const void *raw;
	const struct uk_file *file;
	const struct uk_fs_vopen_memimg *memimg;
};

#define UK_FS_VOPEN_NULLVOL ((union uk_fs_vopen_vol){ .raw = NULL })

union uk_fs_vopen_data {
	const void *raw;
	const struct uk_file *file;
};

#define UK_FS_VOPEN_NULLDATA ((union uk_fs_vopen_data){ .raw = NULL })

/* Format bitmask for vopen is 16 bits; type is larger for API fwd compat */

/* Lower byte describes `vol` */
#define UK_FS_VOPEN_VOL_MASK	0x00ff
#define UK_FS_VOPEN_VOL_IGNORE	0x0000
#define UK_FS_VOPEN_VOL_RAW	0x0001
#define UK_FS_VOPEN_VOL_FILE	0x0002
#define UK_FS_VOPEN_VOL_MEMIMG	0x0004

/* Upper byte describes `data` */
#define UK_FS_VOPEN_DATA_MASK	0xff00
#define UK_FS_VOPEN_DATA_IGNORE	0x0000
#define UK_FS_VOPEN_DATA_RAW	0x0100
#define UK_FS_VOPEN_DATA_FILE	0x0200

/* True iff argument is ignored or explicitly supports `feat` */
static inline
bool uk_fs_vopen_supports(size_t formats, size_t mask, size_t feat)
{
	return !(formats & mask) || (formats & feat);
}

/**
 * Open volume `vol` and return its root directory ready for use.
 *
 * This function is the entry point for a filesystem driver, and the only
 * operation that is not relative to a filesystem node.
 *
 * At most one supported format flag for each of vol and data is set in `fmt`.
 *
 * @param vol Driver-specific volume specifier
 * @param flags Bitmask of flags common to all filesystems
 * @param data Driver-specific additional options
 * @param fmt Format flags
 *
 * @return
 *  !PTRISERR: Reference to the root directory of the filesystem.
 *   PTRISERR: Negative error code encoded in return
 */
typedef const struct uk_file *(*uk_fs_vopen_func)(union uk_fs_vopen_vol vol,
						  unsigned long flags,
						  union uk_fs_vopen_data data,
						  size_t fmt);

/**
 * Entry for registering a filesystem driver with ukfs.
 */
struct uk_fs_drv {
	const char *fstype; /* Filesystem name, used to look up driver */
	uk_fs_vopen_func vopen; /* Vopen operation used to create fs mounts */
	size_t formats; /* Supported formats of vopen arguments */
};

/**
 * Retrieve the filesystem registration of the driver named `fstype`.
 *
 * @return
 *  != NULL: Driver registration of desired filesystem
 *  == NULL: Filesystem name `fstype` not recognized
 */
const struct uk_fs_drv *uk_fs_driver(const char *fstype);

/**
 * Convenience macro to register a ukfs driver.
 *
 * @param fsname Filesystem name to register; do not enclose in quotes
 * @param drv_vopen vopen function of filesystem driver
 * @param drv_formats Supported formats of vopen arguments
 */
#define UK_FS_DRIVER_REGISTER(fsname, drv_vopen, drv_formats)	\
	__used __align(8)					\
		__section("." STRINGIFY(uk_fs_driver_##fsname))	\
	static const struct uk_fs_drv uk_fs_driver_##fsname = {	\
		.fstype = STRINGIFY(fsname),			\
		.vopen = (drv_vopen),				\
		.formats = (drv_formats)			\
	}

#endif /* __UKFS_FS_DRIVER_H__ */
