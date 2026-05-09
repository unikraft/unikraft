/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors. */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <uk/init.h>
#include <uk/print.h>

#include <vfscore/vnode.h>
#include <vfscore/mount.h>
#include <vfscore/dentry.h>
#include <vfscore/fs.h>

#include <hyperlight-x86/setup.h>

#include "hostfs.h"

extern struct vnops hostfs_vnops;

static int hostfs_mount(struct mount *mp, const char *dev, int flags,
			const void *data);
static int hostfs_unmount(struct mount *mp, int flags);

#define hostfs_sync   ((vfsop_sync_t)vfscore_nullop)
#define hostfs_vget   ((vfsop_vget_t)vfscore_nullop)
#define hostfs_statfs ((vfsop_statfs_t)vfscore_nullop)

struct vfsops hostfs_vfsops = {
	hostfs_mount,
	hostfs_unmount,
	hostfs_sync,
	hostfs_vget,
	hostfs_statfs,
	&hostfs_vnops,
};

static struct vfscore_fs_type fs_hostfs = {
	.vs_name = "hostfs",
	.vs_init = NULL,
	.vs_op = &hostfs_vfsops,
};

UK_FS_REGISTER(fs_hostfs);

static int
hostfs_mount(struct mount *mp, const char *dev __unused,
	     int flags __unused, const void *data __unused)
{
	struct hostfs_node *np;

	np = malloc(sizeof(*np));
	if (!np)
		return ENOMEM;
	/* Root has empty path (mount root). */
	np->path[0] = '\0';
	mp->m_root->d_vnode->v_data = np;
	mp->m_root->d_vnode->v_type = VDIR;
	return 0;
}

static int
hostfs_unmount(struct mount *mp, int flags __unused)
{
	vfscore_release_mp_dentries(mp);
	return 0;
}

#ifdef CONFIG_LIBHOSTFS_AUTOMOUNT
static int hostfs_automount(struct uk_init_ctx *ictx __unused)
{
	/* Prefer the runtime mountpoint advertised by the Hyperlight host
	 * via the HLHSMNT TLV (set by `hyperlight-unikraft --mount H:/G`).
	 * If absent, fall back to the compile-time kconfig default.
	 */
	const char *runtime = hyperlight_hostfs_mountpoint_from_host();
	const char *mountpoint = runtime ? runtime : CONFIG_LIBHOSTFS_MOUNTPOINT;
	int ret;

	uk_pr_info("Mount hostfs to %s%s...\n", mountpoint,
		   runtime ? " (from host --mount)" : "");

	ret = mkdir(mountpoint, S_IRWXU);
	if (ret != 0 && errno != EEXIST) {
		uk_pr_err("Failed to create %s: %d\n", mountpoint, errno);
		return -1;
	}

	ret = mount("", mountpoint, "hostfs", 0, NULL);
	if (ret != 0) {
		uk_pr_err("Failed to mount hostfs on %s: %d\n",
			  mountpoint, errno);
		return -1;
	}

	return 0;
}

/* After vfscore mounts '/' (priority 4) and devfs (priority 5). */
uk_rootfs_initcall_prio(hostfs_automount, 0x0, 6);
#endif
