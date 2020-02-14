/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2005-2007, Kohsuke Ohtani
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

/*
 * vfs_mount.c - mount operations
 */

#define _BSD_SOURCE

#include <sys/param.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "vfs.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <uk/list.h>
#include <uk/mutex.h>
#include <vfscore/prex.h>
#include <vfscore/dentry.h>
#include <vfscore/vnode.h>

/*
 * List for VFS mount points.
 */

UK_LIST_HEAD(mount_list);

/*
 * Global lock to access mount point.
 */
static struct uk_mutex mount_lock = UK_MUTEX_INITIALIZER(mount_lock);

extern const struct vfscore_fs_type *uk_fslist_start;
extern const struct vfscore_fs_type *uk_fslist_end;

#define for_each_fs(iter)			\
	for (iter = &uk_fslist_start;	\
	     iter < &uk_fslist_end;		\
	     iter++)

/*
 * Lookup file system.
 */
static const struct vfscore_fs_type *
fs_getfs(const char *name)
{
	const struct vfscore_fs_type *fs = NULL, **__fs;

	UK_ASSERT(name != NULL);

	for_each_fs(__fs) {
		fs = *__fs;
		if (!fs || !fs->vs_name)
			continue;

		if (strncmp(name, fs->vs_name, FSMAXNAMES) == 0)
			return fs;
	}

	return NULL;
}

int device_open(const char *name __unused, int mode __unused,
		struct device **devp __unused)
{
	UK_CRASH("%s is not implemented (%s)\n", __func__, name);
	return 0;
}

int device_close(struct device *dev)
{
	(void) dev;
	UK_CRASH("%s not implemented", __func__);
	return 0;
}

int
mount(const char *dev, const char *dir, const char *fsname, unsigned long flags,
      const void *data)
{
	const struct vfscore_fs_type *fs;
	struct mount *mp;
	struct device *device;
	struct dentry *dp_covered = NULL;
	struct vnode *vp = NULL;
	int error;

	uk_pr_info("VFS: mounting %s at %s\n", fsname, dir);

	if (!dir || *dir == '\0')
		return ENOENT;

	/* Find a file system. */
	if (!(fs = fs_getfs(fsname)))
		return ENODEV;  /* No such file system */

	/* Open device. NULL can be specified as a device. */
	// Allow device_open() to fail, in which case dev is interpreted
	// by the file system mount routine (e.g zfs pools)
	device = 0;
	if (dev && strncmp(dev, "/dev/", 5) == 0)
		device_open(dev + 5, DO_RDWR, &device);

	/* Check if device or directory has already been mounted. */
	// We need to avoid the situation where after we already verified that
	// the mount point is free, but before we actually add it to mount_list,
	// another concurrent mount adds it. So we use a new mutex to ensure
	// that only one mount() runs at a time. We cannot reuse the existing
	// mount_lock for this purpose: If we take mount_lock and then do
	// lookups, this is lock order inversion and can result in deadlock.

	/* TODO: protect the function from reentrance, as described in
	 * the comment above */
	/* static mutex sys_mount_lock; */
	/* SCOPE_LOCK(sys_mount_lock); */

	uk_mutex_lock(&mount_lock);
	uk_list_for_each_entry(mp, &mount_list, mnt_list) {
		if (!strcmp(mp->m_path, dir) ||
		    (device && mp->m_dev == device)) {
			error = EBUSY;  /* Already mounted */
			uk_mutex_unlock(&mount_lock);
			goto err1;
		}
	}
	uk_mutex_unlock(&mount_lock);
	/*
	 * Create VFS mount entry.
	 */
	mp = malloc(sizeof(struct mount));
	if (!mp) {
		error = ENOMEM;
		goto err1;
	}
	mp->m_count = 0;
	mp->m_op = fs->vs_op;
	mp->m_flags = flags;
	mp->m_dev = device;
	mp->m_data = NULL;
	strlcpy(mp->m_path, dir, sizeof(mp->m_path));
	strlcpy(mp->m_special, dev, sizeof(mp->m_special));

	/*
	 * Get vnode to be covered in the upper file system.
	 */
	if (*dir == '/' && *(dir + 1) == '\0') {
		/* Ignore if it mounts to global root directory. */
		dp_covered = NULL;
	} else {
		if ((error = namei(dir, &dp_covered)) != 0) {

			error = ENOENT;
			goto err2;
		}
		if (dp_covered->d_vnode->v_type != VDIR) {
			error = ENOTDIR;
			goto err3;
		}
	}
	mp->m_covered = dp_covered;

	/*
	 * Create a root vnode for this file system.
	 */
	vfscore_vget(mp, 0, &vp);
	if (vp == NULL) {
		error = ENOMEM;
		goto err3;
	}
	vp->v_type = VDIR;
	vp->v_flags = VROOT;
	vp->v_mode = S_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR;

	mp->m_root = dentry_alloc(NULL, vp, "/");
	if (!mp->m_root) {
		vput(vp);
		goto err3;
	}
	vput(vp);

	/*
	 * Call a file system specific routine.
	 */
	if ((error = VFS_MOUNT(mp, dev, flags, data)) != 0)
		goto err4;

	if (mp->m_flags & MNT_RDONLY)
		vp->v_mode &=~S_IWUSR;

	/*
	 * Insert to mount list
	 */
	uk_mutex_lock(&mount_lock);
	uk_list_add_tail(&mp->mnt_list, &mount_list);
	uk_mutex_unlock(&mount_lock);

	return 0;   /* success */
 err4:
	drele(mp->m_root);
 err3:
	if (dp_covered)
		drele(dp_covered);
 err2:
	free(mp);
 err1:
	if (device)
		device_close(device);

	return error;
}

void
vfscore_release_mp_dentries(struct mount *mp)
{
	/* Decrement referece count of root vnode */
	if (mp->m_covered) {
		drele(mp->m_covered);
	}

	/* Release root dentry */
	drele(mp->m_root);
}

int
umount2(const char *path, int flags)
{
	struct mount *mp, *tmp;
	int error, pathlen;

	uk_pr_info("VFS: unmounting %s\n", path);

	uk_mutex_lock(&mount_lock);

	pathlen = strlen(path);
	if (pathlen >= MAXPATHLEN) {
		error = ENAMETOOLONG;
		goto out;
	}

	/* Get mount entry */
	uk_list_for_each_entry(tmp, &mount_list, mnt_list) {
		if (!strcmp(path, tmp->m_path)) {
			mp = tmp;
			goto found;
		}
	}

	error = EINVAL;
	goto out;

found:
	/*
	 * Root fs can not be unmounted.
	 */
	if (mp->m_covered == NULL && !(flags & MNT_FORCE)) {
		error = EINVAL;
		goto out;
	}

	if ((error = VFS_UNMOUNT(mp, flags)) != 0)
		goto out;
	uk_list_del_init(&mp->mnt_list);

#ifdef HAVE_BUFFERS
	/* Flush all buffers */
	binval(mp->m_dev);
#endif

	if (mp->m_dev)
		device_close(mp->m_dev);
	free(mp);
 out:
	uk_mutex_unlock(&mount_lock);
	return error;
}

int
umount(const char *path)
{
	return umount2(path, 0);
}

#if 0
int
sys_pivot_root(const char *new_root, const char *put_old)
{
	struct mount *newmp = NULL, *oldmp = NULL;
	int error;

	WITH_LOCK(mount_lock) {
		for (auto&& mp : mount_list) {
			if (!strcmp(mp->m_path, new_root)) {
				newmp = mp;
			}
			if (!strcmp(mp->m_path, put_old)) {
				oldmp = mp;
			}
		}
		if (!newmp || !oldmp || newmp == oldmp) {
			return EINVAL;
		}
		for (auto&& mp : mount_list) {
			if (mp == newmp || mp == oldmp) {
				continue;
			}
			if (!strncmp(mp->m_path, put_old, strlen(put_old))) {
				return EBUSY;
			}
		}
		if ((error = VFS_UNMOUNT(oldmp, 0)) != 0) {
			return error;
		}
		mount_list.remove(oldmp);

		newmp->m_root->d_vnode->v_mount = newmp;

		if (newmp->m_covered) {
			drele(newmp->m_covered);
		}
		newmp->m_covered = NULL;

		if (newmp->m_root->d_parent) {
			drele(newmp->m_root->d_parent);
		}
		newmp->m_root->d_parent = NULL;

		strlcpy(newmp->m_path, "/", sizeof(newmp->m_path));
	}
	return 0;
}
#endif

void sync(void)
{
	struct mount *mp;
	uk_mutex_lock(&mount_lock);

	/* Call each mounted file system. */
	uk_list_for_each_entry(mp, &mount_list, mnt_list) {
		VFS_SYNC(mp);
	}
#ifdef HAVE_BUFFERS
	bio_sync();
#endif
	uk_mutex_unlock(&mount_lock);
}

/*
 * Compare two path strings. Return matched length.
 * @path: target path.
 * @root: vfs root path as mount point.
 */
static size_t
count_match(const char *path, char *mount_root)
{
	size_t len = 0;

	while (*path && *mount_root) {
		if (*path != *mount_root)
			break;

		path++;
		mount_root++;
		len++;
	}
	if (*mount_root != '\0')
		return 0;

	if (len == 1 && *(path - 1) == '/')
		return 1;

	if (*path == '\0' || *path == '/')
		return len;
	return 0;
}

/*
 * Get the root directory and mount point for specified path.
 * @path: full path.
 * @mp: mount point to return.
 * @root: pointer to root directory in path.
 */
int
vfs_findroot(const char *path, struct mount **mp, char **root)
{
	struct mount *m = NULL, *tmp;
	size_t len, max_len = 0;

	if (!path)
		return -1;

	/* Find mount point from nearest path */
	uk_mutex_lock(&mount_lock);
	uk_list_for_each_entry(tmp, &mount_list, mnt_list) {
		len = count_match(path, tmp->m_path);
		if (len > max_len) {
			max_len = len;
			m = tmp;
		}
	}
	uk_mutex_unlock(&mount_lock);
	if (m == NULL)
		return -1;
	*root = (char *)(path + max_len);
	if (**root == '/')
		(*root)++;
	*mp = m;
	return 0;
}

/*
 * Mark a mount point as busy.
 */
void
vfs_busy(struct mount *mp)
{
	/* The m_count is not really checked anywhere
	 * currently. Atomic is enough. But it could be that obtaining
	 * mount_lock will be needed in the future */
	ukarch_inc(&mp->m_count);
}


/*
 * Mark a mount point as busy.
 */
void
vfs_unbusy(struct mount *mp)
{
	/* The m_count is not really checked anywhere
	 * currently. Atomic is enough. But it could be that obtaining
	 * mount_lock will be needed in the future */
	ukarch_dec(&mp->m_count);
}

int vfscore_nullop(void)
{
	return 0;
}

int
vfs_einval(void)
{
	return EINVAL;
}

#ifdef DEBUG_VFS
void
vfscore_mount_dump(void)
{
	struct mount *mp;
	uk_mutex_lock(&mount_lock);

	uk_pr_debug("vfscore_mount_dump\n");
	uk_pr_debug("dev      count root\n");
	uk_pr_debug("-------- ----- --------\n");

	uk_list_for_each_entry(mp, &mount_list, mnt_list) {
		uk_pr_debug("%8p %5d %s\n", mp->m_dev, mp->m_count, mp->m_path);
	}
	uk_mutex_unlock(&mount_lock);
}
#endif
