/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <string.h>

#include <uk/fs/driver.h>

/* Filesystem driver table; constructed via linker magic. See driver-list.ld. */
extern const struct uk_fs_drv uk_fs_driver_list_start[];
extern const struct uk_fs_drv uk_fs_driver_list_end[];

#define UKFS_DRIVER_COUNT \
	(uk_fs_driver_list_end - uk_fs_driver_list_start)

uk_fs_vopen_func uk_fs_driver(const char *fstype)
{
	const struct uk_fs_drv *drv = uk_fs_driver_list_start;
	size_t len = UKFS_DRIVER_COUNT;

	/* Driver list is sorted by fsname at linktime, can do binary search */
	while (len) {
		size_t mid = len / 2;
		int r = strcmp(fstype, drv[mid].fstype);

		if (!r)
			return drv[mid].vopen;
		if (r < 0) {
			len = mid;
		} else {
			drv = &drv[mid + 1];
			len -= mid + 1;
		}
	}
	return NULL;
}
