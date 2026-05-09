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
hostfs_mount(struct mount *mp, const char *dev,
	     int flags __unused, const void *data __unused)
{
	struct hostfs_node *np;

	np = malloc(sizeof(*np));
	if (!np)
		return ENOMEM;
	/* The root vnode's `path` is the absolute guest mount path
	 * (e.g. "/host", "/input"). Every RPC the vnops issue prepends
	 * the parent's path and passes the full guest path to the host,
	 * which picks the matching preopen by prefix.
	 */
	if (dev && *dev) {
		strlcpy(np->path, dev, sizeof(np->path));
	} else {
		/* Legacy / kconfig-default path: fall back to the compile
		 * time LIBHOSTFS_MOUNTPOINT so the host-side router still
		 * has a prefix to match against.
		 */
		strlcpy(np->path, CONFIG_LIBHOSTFS_MOUNTPOINT,
			sizeof(np->path));
	}
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
static int hostfs_mount_one(const char *mountpoint)
{
	int ret = mkdir(mountpoint, S_IRWXU);
	if (ret != 0 && errno != EEXIST) {
		uk_pr_err("Failed to create %s: %d\n", mountpoint, errno);
		return -1;
	}
	/* Pass the absolute guest mount path as the `dev` argument so the
	 * hostfs_mount callback can stash it as the per-mount prefix.
	 */
	ret = mount(mountpoint, mountpoint, "hostfs", 0, NULL);
	if (ret != 0) {
		uk_pr_err("Failed to mount hostfs on %s: %d\n",
			  mountpoint, errno);
		return -1;
	}
	uk_pr_info("Mounted hostfs at %s\n", mountpoint);
	return 0;
}

static int hostfs_automount(struct uk_init_ctx *ictx __unused)
{
	unsigned int n = hyperlight_hostfs_preopen_count();
	if (n == 0) {
		/* No host-provided preopens — use the kconfig default. */
		return hostfs_mount_one(CONFIG_LIBHOSTFS_MOUNTPOINT);
	}
	for (unsigned int i = 0; i < n; i++) {
		const char *mp = hyperlight_hostfs_preopen(i);
		if (!mp || !*mp)
			continue;
		if (hostfs_mount_one(mp) != 0)
			return -1;
	}
	return 0;
}

/* After vfscore mounts '/' (priority 4) and devfs (priority 5). */
uk_rootfs_initcall_prio(hostfs_automount, 0x0, 6);
#endif
