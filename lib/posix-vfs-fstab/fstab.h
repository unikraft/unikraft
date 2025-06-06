/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* File system table sources; internal use only in fstab.c */

#ifndef __UK_POSIX_VFS_FSTAB_H__
#define __UK_POSIX_VFS_FSTAB_H__

#include <uk/config.h>

struct vfs_fstab_entry {
	const char *dev;
	const char *path;
	const char *fstype;
	unsigned long flags;
	char *opts;
	unsigned int ukopts;
};

#define FSTAB_UKOPT_MKMP  1
#define FSTAB_OPTSTR_MKMP "mkmp"

#define FSTAB_UKOPT_IFINITRD0  2
#define FSTAB_OPTSTR_IFINITRD0 "ifinitrd0"

#define FSTAB_UKOPT_IFNOINITRD0  4
#define FSTAB_OPTSTR_IFNOINITRD0 "ifnoinitrd0"

/* Built-in fs table: `fstab_builtin[]` */
#include "fstab-builtin.h"

/* User-supplied fs table + fallback: fstab_user[] + fstab_fallback[] */
#include "fstab-user.h"

/* Embedded initrd */
#include "einitrd.h"

#endif /* __UK_POSIX_VFS_FSTAB_H__ */
