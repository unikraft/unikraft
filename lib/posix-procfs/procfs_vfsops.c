/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2006-2007, Kohsuke Ohtani
 * Copyright (C) 2014 Cloudius Systems, Ltd.
 * Copyright (c) 2019, NEC Europe Ltd., NEC Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <errno.h>

#define _BSD_SOURCE
#include <vfscore/vnode.h>
#include <vfscore/mount.h>
#include <vfscore/dentry.h>

/* in order to mount the fs similar to devfs */
#include <uk/init.h>

#include "procfs.h"

extern struct vnops procfs_vnops;

static int procfs_mount(struct mount *mp, const char *dev, int flags,
		       const void *data);

static int procfs_unmount(struct mount *mp, int flags);

#define procfs_sync    ((vfsop_sync_t)vfscore_nullop)
#define procfs_vget    ((vfsop_vget_t)vfscore_nullop)
#define procfs_statfs    ((vfsop_statfs_t)vfscore_nullop)

/*
 * File system operations
 */
struct vfsops procfs_vfsops = {
		procfs_mount,       /* mount */
		procfs_unmount,     /* unmount */
		procfs_sync,        /* sync */
		procfs_vget,        /* vget */
		procfs_statfs,      /* statfs */
		&procfs_vnops,      /* vnops */
};

static struct vfscore_fs_type fs_procfs = {
	.vs_name = "procfs",
	.vs_init = NULL,
	.vs_op = &procfs_vfsops,
};

UK_FS_REGISTER(fs_procfs);

#ifdef CONFIG_LIBPROCFS_AUTOMOUNT
static int procfs_automount(void)
{
	int ret;

	uk_pr_info("Mount procfs to /proc...");
	uk_pr_debug("\n\nMount procfs to /proc...\n\n");

	/*
	 * Try to create target mountpoint `/proc`. If creation fails
	 * because it already exists, we are continuing.
	 */
	ret =  mkdir("/proc", S_IRWXU);
	if (ret != 0 && errno != EEXIST) {
		uk_pr_err("Failed to create /proc: %d\n", errno);
		return -1;
	}
	ret = mount("", "/proc", "procfs", 0, NULL);
	if (ret != 0) {
		uk_pr_err("Failed to mount procfs to /proc: %d\n", errno);
		return -1;
	}

	return 0;
}

/* after vfscore mounted '/' (priority 4): */
uk_rootfs_initcall_prio(procfs_automount, 5);
#endif


/* 
 * Create procfs entry helper
 */
static int 
create_entry(struct mount *mp, struct procfs_node **np,
			 const char *name, int type, int count)
{
	switch (count) {
	case 0:
		np[count] = procfs_allocate_node(name, type);
		if (np == NULL) {
			return ENOMEM;
		}
		mp->m_root->d_vnode->v_data = np[count];
		break;
	case 1:
		np[count] = procfs_allocate_node(name, type);
		np[count - 1]->rn_child = np[count];
		break;
	default:
		np[count] = procfs_allocate_node(name, type);
		np[count - 1]->rn_next = np[count];
		break;

	}

	count++;
	return count;
}

/*
 * Mount a file system.
 */
static int
procfs_mount(struct mount *mp, const char *dev __unused,
	    int flags __unused, const void *data __unused)
{

	struct procfs_node *entries[10];
	int count = 0;

	count = create_entry(mp, entries, "/", VDIR, count);
	if (count == ENOMEM) {
		return count;
	}

#ifdef CONFIG_LIBPROCFS_MEMINFO
	uk_pr_debug("%s creating meminfo\n", __func__);
	count = create_entry(mp, entries, "meminfo", VREG, count);
#endif /* CONFIG_LIBPROCFS_MEMINFO */
#ifdef CONFIG_LIBPROCFS_CMDLINE
	count = create_entry(mp, entries, "cmdline", VREG, count);
#endif /* CONFIG_LIBPROCFS_CMDLINE */
#ifdef CONFIG_LIBPROCFS_VERSION
	count = create_entry(mp, entries, "version", VREG, count);
#endif /* CONFIG_LIBPROCFS_VERSION */
#ifdef CONFIG_LIBPROCFS_FIELSYSTEMS
	count = create_entry(mp, entries, "filesystems", VREG, count);
#endif /* CONFIG_LIBPROCFS_FIELSYSTEMS */
#ifdef CONFIG_LIBPROCFS_MOUNTS
	count = create_entry(mp, entries, "mounts", VREG, count);
#endif /* CONFIG_LIBPROCFS_MOUNTS */
	
	return 0;
}

/*
 * Unmount a file system.
 *
 * NOTE: Currently, we don't support unmounting of the PROCFS. This is
 *       because we have to deallocate all nodes included in all sub
 *       directories, and it requires more work...
 */
static int
procfs_unmount(struct mount *mp, int flags __unused)
{
	vfscore_release_mp_dentries(mp);
	return 0;
}
