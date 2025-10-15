/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_FS_TEMPLATE_PSEUDO_H__
#define __UK_FS_TEMPLATE_PSEUDO_H__

/*
 * Unikraft filesystem template for pseudo-filesystems.
 *
 * A pseudo-filesystem has no implementation of its own, instead it relies on a
 * "backend" filesystem root, a reference initialized as part of system startup
 * and destroyed only on shutdown, which it exposes through two interfaces:
 * 1. An internal API providing a singleton reference to the backend root, used
 *    to manage the pseudofs independent of its place in the VFS (if any)
 * 2. A filesystem registration for mounting rebinds of this root in the VFS
 *
 * The driver is free to provide (1) as it wishes; this header aids in
 * implementing (2) along with any boilerplate setup & teardown code.
 */

#include <uk/file.h>
#include <uk/fs.h>
#include <uk/fs/driver.h>
#include <uk/fs/prio.h>
#include <uk/init.h>

/**
 * Define boiler plate code for, and register `drv` as a ukfs pseudo-fs.
 *
 * Boilerplate code consists of:
 * - init: Creates the backend root and initializes `rootptr`
 * - term: Releases the `rootptr` reference, cleaning up the backend
 * - vopen: Returns a rebind of the backend root, ready for mounting
 *
 * @param drv Driver name, used to register to ukfs; do not enclose in quotes
 * @param backend Driver name of the backend; string (literal)
 * @param rootptr Backend filesystem root pointer
 * @param bvol Volume argument for the initial backend
 * @param bflags Flags argument for the initial backend
 * @param bdata Data argument for the initial backend
 * @param bfmt Format argument for the initial backend
 */
#define UK_FS_TMPL_PSEUDO(drv, backend, rootptr, bvol, bflags, bdata, bfmt)\
static									\
int init_##drv##_root(struct uk_init_ctx *ictx __unused)		\
{									\
	const struct uk_fs_drv *fsdrv = uk_fs_driver((backend));	\
									\
	if (unlikely(!fsdrv))						\
		return -ENOENT;						\
									\
	(rootptr) = fsdrv->vopen((bvol), (bflags), (bdata), (bfmt));	\
	if (unlikely(PTRISERR((rootptr))))				\
		return PTR2ERR((rootptr));				\
	return 0;							\
}									\
\
static									\
void term_##drv##_root(struct uk_term_ctx *tctx __unused)		\
{									\
	uk_file_release((rootptr));					\
}									\
\
uk_rootfs_initcall_prio(init_##drv##_root, term_##drv##_root,		\
			UK_FS_PRIO_SECONDARY);				\
\
static									\
const struct uk_file *drv##_VOPEN(union uk_fs_vopen_vol vol __unused,	\
				  unsigned long flags,			\
				  union uk_fs_vopen_data data __unused,	\
				  size_t fmt __unused)			\
{									\
	return uk_fs_rebind((rootptr), flags, NULL);			\
}									\
\
UK_FS_DRIVER_REGISTER(drv, drv##_VOPEN,					\
		      UK_FS_VOPEN_VOL_IGNORE | UK_FS_VOPEN_DATA_IGNORE)

#endif /* __UK_FS_TEMPLATE_PSEUDO_H__ */
