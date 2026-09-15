/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors. */

/*
 * hostfs — VFS operations for host filesystem pass-through.
 *
 * Mounts are backed by Hyperlight host functions (fs_stat, fs_read_bytes,
 * fs_write_bytes, fs_mkdir, fs_unlink, fs_truncate, fs_list).  Each VFS
 * operation makes a synchronous host call via hl_hcall_*.
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <uk/alloc.h>
#include <uk/essentials.h>
#include <vfscore/vnode.h>
#include <vfscore/mount.h>

#include <hyperlight-x86/hcall.h>

#include "hostfs.h"

extern struct vnops hostfs_vnops;

struct hostfs_io hostfs_io;

int hostfs_io_init(void)
{
	__u64 max = hl_hcall_max_payload();
	__u64 chunk, preferred;
	__u8 *blk;

	if (hostfs_io.rbuf)
		return 0;
	if (unlikely(max <= HOSTFS_STATUS_LEN))
		return EIO;

	/* As much as a call carries, unless the host prefers less. */
	chunk = max - HOSTFS_STATUS_LEN;
	if (hl_hcall_ulong("GetHostFsChunkSize", NULL, 0, &preferred) == 0 &&
	    preferred > 0 && preferred < chunk)
		chunk = preferred;

	blk = uk_malloc(uk_alloc_get_default(), max + chunk);
	if (unlikely(!blk))
		return ENOMEM;
	hostfs_io.rbuf = blk;
	hostfs_io.wbuf = blk + max;
	hostfs_io.result_max = max;
	hostfs_io.chunk = chunk;
	return 0;
}

static int hostfs_mount(struct mount *mp, const char *dev,
			int flags __unused, const void *data __unused)
{
	struct hostfs_node *root;
	int mount_idx = 0;
	int err;

	if (dev && dev[0] != '\0') {
		char *endp;
		long val = strtol(dev, &endp, 10);
		if (endp != dev && *endp == '\0')
			mount_idx = (int)val;
	}

	err = hostfs_io_init();
	if (err)
		return err;

	root = malloc(sizeof(*root));
	if (!root)
		return ENOMEM;

	memset(root, 0, sizeof(*root));
	root->hf_path[0] = '\0'; /* root is "" (relative to host mount) */
	root->hf_type = VDIR;
	root->hf_mount_idx = mount_idx;

	mp->m_root->d_vnode->v_data = root;
	mp->m_root->d_vnode->v_type = VDIR;
	mp->m_root->d_vnode->v_mode = S_IFDIR | 0755;

	return 0;
}

static int hostfs_unmount(struct mount *mp __unused, int flags __unused)
{
	return 0;
}

#define hostfs_sync    ((vfsop_sync_t)vfscore_nullop)
#define hostfs_vget    ((vfsop_vget_t)vfscore_nullop)
#define hostfs_statfs  ((vfsop_statfs_t)vfscore_nullop)

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
