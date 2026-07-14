/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <uk/print.h>
#include <uk/essentials.h>
#include <uk/init.h>
#include <uk/list.h>
#include <uk/file-console.h>
#include <uk/fs.h>
#include <uk/fs/prio.h>

#include <uk/devfs.h>

#include "virtio_console_priv.h"

/*
 * Cached reference to the /dev/virtio-ports directory.
 * NULL until the initcall has run.
 */
static const struct uk_file *vtcons_vports_dir;

/*
 * Buffer size for a "vportXportY\0" node name.
 */
#define VTCONS_VPORT_NODE_SIZE		32

/*
 * Buffer size for a "/dev/vportXportY\0" absolute symlink target.
 */
#define VTCONS_VPORT_PATH_SIZE		(5 + VTCONS_VPORT_NODE_SIZE)

void vtcons_devfs_mknode(struct vtcons_port *port)
{
	/* We can do both output and input */
	const unsigned int mode = 0222 | 0444;
	char name[VTCONS_VPORT_NODE_SIZE];
	const struct uk_file *f;
	const void *r;
	int namelen;

	namelen = snprintf(name, sizeof(name), "vport%uport%u",
			   port->dev->id, port->id);
	f = uk_file_console_create(&port->con_drv.cons);
	if (unlikely(PTRISERR(f))) {
		uk_pr_err_isr("Failed to create console file for %s\n", name);
		return;
	}

	/* We do not clean up created files on error, as they will be
	 * dropped when the devfs root is released on system shutdown.
	 */
	r = uk_fs_createat(uk_fs_devfs_root,
			   name, namelen, mode, O_EXCL,
			   (union uk_fs_create_target){
				.file = f,
			   });
	uk_file_release(f);
	if (unlikely(PTRISERR(r)))
		uk_pr_err_isr("Failed to create /dev/%s: %d\n",
			      name, PTR2ERR(r));
}

void vtcons_devfs_mksymlink(struct vtcons_port *port)
{
	char target[VTCONS_VPORT_PATH_SIZE];
	const struct uk_file *sym;

	if (!port->name[0])
		return;

	UK_ASSERT(vtcons_vports_dir);

	/* Create /dev/virtio-ports/<name> -> ../vportXportY. */
	snprintf(target, sizeof(target), "../vport%uport%u",
		 port->dev->id, port->id);

	sym = uk_fs_createat(vtcons_vports_dir,
			     port->name, strlen(port->name),
			     S_IFLNK | 0777, O_EXCL,
			     (union uk_fs_create_target){ .path = target });
	if (unlikely(PTRISERR(sym)))
		uk_pr_err_isr("Failed to create symlink %s -> %s: %d\n",
			      port->name, target, PTR2ERR(sym));
	else
		uk_file_release(sym);
}

void vtcons_devfs_rmnode(struct vtcons_port *port)
{
	char name[VTCONS_VPORT_NODE_SIZE];
	int namelen, rc;

	namelen = snprintf(name, sizeof(name), "vport%uport%u",
			   port->dev->id, port->id);
	rc = uk_fs_unlinkat(uk_fs_devfs_root, name, namelen, 0);
	if (unlikely(rc))
		uk_pr_err_isr("Failed to unlink virtio-ports/%s: %d\n",
			      name, rc);

	if (!port->name[0])
		return;

	rc = uk_fs_unlinkat(vtcons_vports_dir,
			    port->name, strlen(port->name), 0);
	if (unlikely(rc))
		uk_pr_err_isr("Failed to unlink virtio-ports/%s: %d\n",
			      port->name, rc);
}

/*
 * devfs initcall: runs once the root filesystem is mounted and /dev is
 * writable.  Creates /dev/virtio-ports/ and caches a reference to it,
 * then catches up on all vportXportY nodes and named symlinks that were
 * deferred during probe because devfs was not yet available.
 *
 * After each device is processed, devfs_ready is set so that subsequent
 * PORT_ADD and PORT_NAME control events create nodes immediately.
 */
static int vtcons_devfs_init(struct uk_init_ctx *ictx __unused)
{
	const struct uk_file *newdir;
	struct vtcons_port *port;
	struct vtcons_dev *dev;
	__u32 i;

	UK_ASSERT(uk_fs_devfs_root);

	newdir = uk_fs_createat(uk_fs_devfs_root,
				"virtio-ports",
				sizeof("virtio-ports") - 1,
				S_IFDIR | 0755, O_EXCL,
				UKFS_NOTARGET);
	if (unlikely(PTRISERR(newdir))) {
		uk_pr_err_isr("Failed to create /dev/virtio-ports: %d\n",
			      PTR2ERR(newdir));
		return PTR2ERR(newdir);
	}

	vtcons_vports_dir = newdir;

	uk_list_for_each_entry(dev, &vtcons_dev_list, list) {
		for (i = 0; i < dev->max_nr_ports; i++) {
			port = &dev->ports[i];

			/* Skip ports not yet announced by the host and
			 * console ports, which use an HVC node instead of
			 * a vportXportY entry.
			 */
			if (!port->is_registered || port->is_console)
				continue;

			vtcons_devfs_mknode(port);

			if (port->name[0] != '\0')
				vtcons_devfs_mksymlink(port);
		}

		/* All pending nodes for this device are now created.
		 * Subsequent PORT_ADD and PORT_NAME events will call the
		 * helpers directly now that devfs_ready is set.
		 */
		dev->devfs_ready = 1;
	}

	return 0;
}

uk_rootfs_initcall_prio(vtcons_devfs_init, 0x0, UK_FS_PRIO_FSAVAIL);
