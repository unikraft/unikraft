/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2019, University Politehnica of Bucharest.
 * Copyright (c) 2025, Florin Postolache.
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

/*
 * procfs - proc file system.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <string.h>

#include <vfscore/vnode.h>
#include <vfscore/fs.h>

#include <uk/init.h>

#include <vfscore/file.h>
#include <procfs/proc.h>

static uint64_t inode_count = 1; /* inode 0 is reserved to root */
struct vnops procfs_vnops;
extern struct uk_mutex procfs_lock;


static int
procfs_read(struct vnode *vp, struct vfscore_file *fp __unused,
			struct uio *uio, int ioflags)
{
	return proc_file_read((struct procfs_entry *)vp->v_data, uio, ioflags);
}

static int
procfs_write(struct vnode *vp, struct uio *uio, int ioflags)
{
	return 0;
}


static int
procfs_lookup(struct vnode *dvp, const char *name, struct vnode **vpp)
{
	struct procfs_entry *np, *dnp;
	struct vnode *vp;
	size_t len;
	int found;

	*vpp = NULL;

	if (*name == '\0'){
		return ENOENT;
	}
	uk_mutex_lock(&procfs_lock);

	len = strlen(name);
	dnp = dvp->v_data;
	found = 0;
	for (np = dnp->children; np != NULL; np = np->sibling) {
		if (np->name_len == len &&
			memcmp(name, np->name, len) == 0) {
			found = 1;
			break;
		}
	}
	if (found == 0) {
		uk_mutex_unlock(&procfs_lock);
		uk_pr_debug("Not found\n");
		return ENOENT;
	}
	if (vfscore_vget(dvp->v_mount, inode_count++, &vp)) {
		*vpp = vp;
		uk_mutex_unlock(&procfs_lock);
		return 0;
	}
	if (!vp) {
		uk_mutex_unlock(&procfs_lock);
		return ENOMEM;
	}
	vp->v_data = np;
	vp->v_mode = UK_ALLPERMS;
	vp->v_type = (np->type == PROCFS_NODE_DIR) ? VDIR : (np->type == PROCFS_NODE_FILE) ? VREG : VLNK;
	vp->v_size = np->rn_size;

	uk_mutex_unlock(&procfs_lock);

	*vpp = vp;
    return 0;
}

/*
 * @vp: vnode of the directory.
 */
extern void
set_times_to_now(struct timespec *time1, struct timespec *time2,
		 struct timespec *time3);
static int
procfs_readdir(struct vnode *vp __unused, struct vfscore_file *fp,
	      struct dirent64 *dir)
{
	return proc_file_readdir((struct procfs_entry *)vp->v_data, fp, dir);
}

/*
 * Mount a file system.
 */
static int
procfs_mount(struct mount *mp, const char *dev __unused,
	    int flags __unused, const void *data __unused)
{

	int count = 0;

	count = create_root(mp);
	if (count == ENOMEM)
		return count;

#ifdef CONFIG_LIBPROCFS_VERSION
	count = procfs_register_version();
#endif /* CONFIG_LIBPROCFS_VERSION */

#ifdef CONFIG_LIBPROCFS_CMDLINE
	count = procfs_register_cmdline();
#endif /* CONFIG_LIBPROCFS_CMDLINE */

#ifdef CONFIG_LIBPROCFS_MOUNTS
	count = procfs_register_mounts();
#endif /* CONFIG_LIBPROCFS_MOUNTS */

#ifdef CONFIG_LIBPROCFS_FILESYSTEMS
	count = procfs_register_filesystems();
#endif /* CONFIG_LIBPROCFS_FILESYSTEMS */

#ifdef CONFIG_LIBPROCFS_SELF_LINK
	count = procfs_register_self_link();
#endif /* CONFIG_LIBPROCFS_SELF_LINK */

#ifdef CONFIG_LIBPROCFS_SYS_FOLDER
	count = procfs_register_sys_folder();
#endif /* CONFIG_LIBPROCFS_SYS_FOLDER */

	return count;
}

static int
procfs_unmount(struct mount *mp, int flags __unused)
{
	vfscore_release_mp_dentries(mp);
	return 0;
}


static int
procfs_getattr(struct vnode *vnode, struct vattr *attr)
{
	attr->va_nodeid = vnode->v_ino;
	attr->va_size = vnode->v_size;
	return 0;
}

static int procfs_readlink(struct vnode *vp, struct uio *uio)
{
	return proc_link_read((struct procfs_entry *)vp->v_data, uio);
}


#define procfs_open      ((vnop_open_t)vfscore_vop_nullop)
#define procfs_close     ((vnop_close_t)vfscore_vop_nullop)


#define procfs_seek			((vnop_seek_t)vfscore_vop_nullop)
#define procfs_ioctl		((vnop_ioctl_t)vfscore_vop_einval)
#define procfs_fsync		((vnop_fsync_t)vfscore_vop_nullop)
#define procfs_inactive		((vnop_inactive_t)vfscore_vop_nullop)
#define procfs_link			((vnop_link_t)vfscore_vop_eperm)
#define procfs_fallocate	((vnop_fallocate_t)vfscore_vop_nullop)
#define procfs_truncate		((vnop_truncate_t)vfscore_vop_nullop)
#define procfs_rename		((vnop_rename_t)vfscore_vop_einval)
#define procfs_setattr		((vnop_setattr_t)vfscore_vop_nullop)
#define procfs_symlink		((vnop_symlink_t)vfscore_vop_eperm)
#define procfs_poll		((vnop_poll_t)vfscore_vop_nullop)
#define procfs_create		((vnop_create_t)vfscore_vop_einval)
#define procfs_remove		((vnop_remove_t)vfscore_vop_einval)
#define procfs_mkdir		((vnop_mkdir_t)vfscore_vop_einval)
#define procfs_rmdir		((vnop_rmdir_t)vfscore_vop_einval)

#define procfs_sync	((vfsop_sync_t)vfscore_nullop)
#define procfs_vget	((vfsop_vget_t)vfscore_nullop)
#define procfs_statfs	((vfsop_statfs_t)vfscore_nullop)

/*
 * vnode operations
 */
struct vnops procfs_vnops = {
	procfs_open,		/* open */
	procfs_close,		/* close */
	procfs_read,		/* read */
	procfs_write,		/* write */
	procfs_seek,		/* seek */
	procfs_ioctl,		/* ioctl */
	procfs_fsync,		/* fsync */
	procfs_readdir,		/* readdir */
	procfs_lookup,		/* lookup */
	procfs_create,		/* create */
	procfs_remove,		/* remove */
	procfs_rename,		/* remame */
	procfs_mkdir,		/* mkdir */
	procfs_rmdir,		/* rmdir */
	procfs_getattr,		/* getattr */
	procfs_setattr,		/* setattr */
	procfs_inactive,		/* inactive */
	procfs_truncate,		/* truncate */
	procfs_link,		/* link */
	(vnop_cache_t) NULL, /* arc */
	procfs_fallocate,	/* fallocate */
	procfs_readlink,		/* read link */
	procfs_symlink,		/* symbolic link */
	procfs_poll,		/* poll */
};

/*
 * File system operations
 */
struct vfsops procfs_vfsops = {
	procfs_mount,		/* mount */
	procfs_unmount,		/* unmount */
	procfs_sync,		/* sync */
	procfs_vget,		/* vget */
	procfs_statfs,		/* statfs */
	&procfs_vnops,		/* vnops */
};

/*
 * Registration structure
 */
static struct vfscore_fs_type fs_procfs = {
    .vs_name = "procfs",
    .vs_init = NULL,
    .vs_op = &procfs_vfsops,
};

UK_FS_REGISTER(fs_procfs);

#ifdef CONFIG_LIBPROCFS_AUTOMOUNT
static int procfs_automount(struct uk_init_ctx *ictx __unused)
{
	int ret;

	uk_pr_info("Mount procfs to /proc...");

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
uk_rootfs_initcall_prio(procfs_automount, 0x0, 5);
#endif
