/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2005-2008, Kohsuke Ohtani
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
 * vfs_vnode.c - vnode service
 */
#define _BSD_SOURCE

#include <limits.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>

#include <vfscore/prex.h>
#include <vfscore/dentry.h>
#include <vfscore/vnode.h>
#include "vfs.h"

#define __UK_S_BLKSIZE 512

enum vtype iftovt_tab[16] = {
	VNON, VFIFO, VCHR, VNON, VDIR, VNON, VBLK, VNON,
	VREG, VNON, VLNK, VNON, VSOCK, VNON, VNON, VBAD,
};
int vttoif_tab[10] = {
	0, S_IFREG, S_IFDIR, S_IFBLK, S_IFCHR, S_IFLNK,
	S_IFSOCK, S_IFIFO, S_IFMT, S_IFMT
};

/*
 * Memo:
 *
 * Function   Ref count Lock
 * ---------- --------- ----------
 * vn_lock     *        Lock
 * vn_unlock   *        Unlock
 * vfscore_vget        1        Lock
 * vput       -1        Unlock
 * vref       +1        *
 * vrele      -1        *
 */

#define VNODE_BUCKETS 32		/* size of vnode hash table */

/*
 * vnode table.
 * All active (opened) vnodes are stored on this hash table.
 * They can be accessed by its path name.
 */
static struct uk_list_head vnode_table[VNODE_BUCKETS];

/*
 * Global lock to access all vnodes and vnode table.
 * If a vnode is already locked, there is no need to
 * lock this global lock to access internal data.
 */
static struct uk_mutex vnode_lock = UK_MUTEX_INITIALIZER(vnode_lock);
#define VNODE_LOCK()	uk_mutex_lock(&vnode_lock)
#define VNODE_UNLOCK()	uk_mutex_unlock(&vnode_lock)

/* TODO: implement mutex_owned */
#define VNODE_OWNED()	(1)
/* #define VNODE_OWNED()	mutex_owned(&vnode_lock) */


/*
 * Get the hash value from the mount point and path name.
 * XXX(hch): replace with a better hash for 64-bit pointers.
 */
static unsigned int vn_hash(struct mount *mp, uint64_t ino)
{
	return (ino ^ (unsigned long)mp) & (VNODE_BUCKETS - 1);
}

/*
 * Returns locked vnode for specified mount point and path.
 * vn_lock() will increment the reference count of vnode.
 *
 * Locking: VNODE_LOCK must be held.
 */
struct vnode *
vn_lookup(struct mount *mp, uint64_t ino)
{
	struct vnode *vp;

	UK_ASSERT(VNODE_OWNED());
	uk_list_for_each_entry(vp, &vnode_table[vn_hash(mp, ino)], v_link) {
		if (vp->v_mount == mp && vp->v_ino == ino) {
			vp->v_refcnt++;
			uk_mutex_lock(&vp->v_lock);
			return vp;
		}
	}
	return NULL;		/* not found */
}

#ifdef DEBUG_VFS
static const char *
vn_path(struct vnode *vp)
{
	struct dentry *dp;

	if (uk_list_empty(&vp->v_names) == 1) {
		return (" ");
	}
	dp = uk_list_first_entry(&vp->v_names, struct dentry, d_names_link);
	return (dp->d_path);
}
#endif

/*
 * Lock vnode
 */
void
vn_lock(struct vnode *vp)
{
	UK_ASSERT(vp);
	UK_ASSERT(vp->v_refcnt > 0);

	uk_mutex_lock(&vp->v_lock);
	DPRINTF(VFSDB_VNODE, ("vn_lock:   %s\n", vn_path(vp)));
}

/*
 * Unlock vnode
 */
void
vn_unlock(struct vnode *vp)
{
	UK_ASSERT(vp);
	UK_ASSERT(vp->v_refcnt >= 0);

	uk_mutex_unlock(&vp->v_lock);
	DPRINTF(VFSDB_VNODE, ("vn_lock:   %s\n", vn_path(vp)));
}

/*
 * Allocate new vnode for specified path.
 * Increment its reference count and lock it.
 * Returns 1 if vnode was found in cache; otherwise returns 0.
 */
int
vfscore_vget(struct mount *mp, uint64_t ino, struct vnode **vpp)
{
	struct vnode *vp;
	int error;

	*vpp = NULL;

	DPRINTF(VFSDB_VNODE, ("vfscore_vget %llu\n", (unsigned long long) ino));

	VNODE_LOCK();

	vp = vn_lookup(mp, ino);
	if (vp) {
		VNODE_UNLOCK();
		*vpp = vp;
		return 1;
	}

	vp = calloc(1, sizeof(*vp));
	if (!vp) {
		VNODE_UNLOCK();
		return 0;
	}

	UK_INIT_LIST_HEAD(&vp->v_names);
	vp->v_ino = ino;
	vp->v_mount = mp;
	vp->v_refcnt = 1;
	vp->v_op = mp->m_op->vfs_vnops;
	uk_mutex_init(&vp->v_lock);
	/*
	 * Request to allocate fs specific data for vnode.
	 */
	if ((error = VFS_VGET(mp, vp)) != 0) {
		VNODE_UNLOCK();
		free(vp);
		return 0;
	}
	vfs_busy(vp->v_mount);
	uk_mutex_lock(&vp->v_lock);

	uk_list_add(&vp->v_link, &vnode_table[vn_hash(mp, ino)]);
	VNODE_UNLOCK();

	*vpp = vp;

	return 0;
}

/*
 * Unlock vnode and decrement its reference count.
 */
void
vput(struct vnode *vp)
{
	UK_ASSERT(vp);
	UK_ASSERT(vp->v_refcnt > 0);
	DPRINTF(VFSDB_VNODE, ("vput: ref=%d %s\n", vp->v_refcnt, vn_path(vp)));

	VNODE_LOCK();
	vp->v_refcnt--;
	if (vp->v_refcnt > 0) {
	    VNODE_UNLOCK();
		vn_unlock(vp);
		return;
	}
	uk_list_del(&vp->v_link);
	VNODE_UNLOCK();

	/*
	 * Deallocate fs specific vnode data
	 */
	if (vp->v_op->vop_inactive)
		VOP_INACTIVE(vp);
	vfs_unbusy(vp->v_mount);
	uk_mutex_unlock(&vp->v_lock);
	free(vp);
}

/*
 * Increment the reference count on an active vnode.
 */
void
vref(struct vnode *vp)
{
	UK_ASSERT(vp);
	UK_ASSERT(vp->v_refcnt > 0);	/* Need vfscore_vget */

	VNODE_LOCK();
	DPRINTF(VFSDB_VNODE, ("vref: ref=%d\n", vp->v_refcnt));
	vp->v_refcnt++;
	VNODE_UNLOCK();
}

/*
 * Decrement the reference count of the vnode.
 * Any code in the system which is using vnode should call vrele()
 * when it is finished with the vnode.
 * If count drops to zero, call inactive routine and return to freelist.
 */
void
vrele(struct vnode *vp)
{
	UK_ASSERT(vp);
	UK_ASSERT(vp->v_refcnt > 0);

	VNODE_LOCK();
	DPRINTF(VFSDB_VNODE, ("vrele: ref=%d\n", vp->v_refcnt));
	vp->v_refcnt--;
	if (vp->v_refcnt > 0) {
		VNODE_UNLOCK();
		return;
	}
	uk_list_del(&vp->v_link);
	VNODE_UNLOCK();

	/*
	 * Deallocate fs specific vnode data
	 */
	VOP_INACTIVE(vp);
	vfs_unbusy(vp->v_mount);
	free(vp);
}

/*
 * Remove all vnode in the vnode table for unmount.
 */
void
vflush(struct mount *mp __unused)
{
}

int
vn_stat(struct vnode *vp, struct stat *st)
{
	struct vattr vattr;
	struct vattr *vap;
	mode_t mode;
	int error;

	vap = &vattr;

	memset(st, 0, sizeof(struct stat));

	memset(vap, 0, sizeof(struct vattr));

	error = VOP_GETATTR(vp, vap);
	if (error)
		return error;

	st->st_ino = (ino_t)vap->va_nodeid;
	st->st_size = vap->va_size;
	mode = vap->va_mode;
	switch (vp->v_type) {
	case VREG:
		mode |= S_IFREG;
		break;
	case VDIR:
		mode |= S_IFDIR;
		break;
	case VBLK:
		mode |= S_IFBLK;
		break;
	case VCHR:
		mode |= S_IFCHR;
		break;
	case VLNK:
		mode |= S_IFLNK;
		break;
	case VSOCK:
		mode |= S_IFSOCK;
		break;
	case VFIFO:
		mode |= S_IFIFO;
		break;
	default:
		return EBADF;
	};
	st->st_mode = mode;
	st->st_nlink = vap->va_nlink;
	st->st_blksize = BSIZE;
	st->st_blocks = vap->va_size / __UK_S_BLKSIZE;
	st->st_uid = vap->va_uid;
	st->st_gid = vap->va_gid;
	st->st_dev = vap->va_fsid;
	if (vp->v_type == VCHR || vp->v_type == VBLK)
		st->st_rdev = vap->va_rdev;

	st->st_atim = vap->va_atime;
	st->st_mtim = vap->va_mtime;
	st->st_ctim = vap->va_ctime;

	return 0;
}

/*
 * Set access and modification times of the vnode
 */
int
vn_settimes(struct vnode *vp, struct timespec times[2])
{
	struct vattr vattr;
	struct vattr *vap;
	int error;

	vap = &vattr;
	memset(vap, 0, sizeof(struct vattr));

	vap->va_atime = times[0];
	vap->va_mtime = times[1];
	vap->va_mask = ((times[0].tv_nsec == UTIME_OMIT) ? 0 : AT_ATIME)
					| ((times[1].tv_nsec == UTIME_OMIT) ? 0 : AT_MTIME);
	vn_lock(vp);
	error = VOP_SETATTR(vp, vap);
	vn_unlock(vp);

	return error;
}

/*
 * Set chmod permissions on the vnode.
 */
int
vn_setmode(struct vnode *vp, mode_t new_mode)
{
	struct vattr vattr;
	memset(&vattr, 0, sizeof(vattr));
	vattr.va_mode = new_mode;
	vattr.va_mask = AT_MODE;
	vn_lock(vp);
	vp->v_mode = new_mode;
	int error = VOP_SETATTR(vp, &vattr);
	vn_unlock(vp);
	return error;
}

/*
 * Check permission on vnode pointer.
 */
int
vn_access(struct vnode *vp, int flags)
{
	int error = 0;

	if ((flags & VEXEC) && (vp->v_mode & 0111) == 0) {
		error = EACCES;
		goto out;
	}
	if ((flags & VREAD) && (vp->v_mode & 0444) == 0) {
		error = EACCES;
		goto out;
	}
	if (flags & VWRITE) {
		if (vp->v_mount->m_flags & MNT_RDONLY) {
			error = EROFS;
			goto out;
		}
		if ((vp->v_mode & 0222) == 0) {
			error = EACCES;
			goto out;
		}
	}
 out:
	return error;
}

#ifdef DEBUG_VFS
/*
 * Dump all all vnode.
 */
void
vnode_dump(void)
{
	int i;
	struct vnode *vp;
	struct mount *mp;
	char type[][6] = { "VNON ", "VREG ", "VDIR ", "VBLK ", "VCHR ",
			   "VLNK ", "VSOCK", "VFIFO" };

	VNODE_LOCK();

	uk_pr_debug("Dump vnode\n");
	uk_pr_debug(" vnode            mount            type  refcnt path\n");
	uk_pr_debug(" ---------------- ---------------- ----- ------ ------------------------------\n");

	for (i = 0; i < VNODE_BUCKETS; i++) {
		uk_list_for_each_entry(vp, &vnode_table[i], v_link) {
			mp = vp->v_mount;


			uk_pr_debug(" %016lx %016lx %s %6d %s%s\n",
				    (unsigned long) vp,
				    (unsigned long) mp, type[vp->v_type],
				    vp->v_refcnt,
				    (strlen(mp->m_path) == 1) ? "\0" : mp->m_path,
				    vn_path(vp));
		}
	}
	uk_pr_debug("\n");
	VNODE_UNLOCK();
}
#endif

int
vfscore_vop_nullop(void)
{
	return 0;
}

int
vfscore_vop_einval(void)
{
	return EINVAL;
}

int
vfscore_vop_eperm(void)
{
	return EPERM;
}

int
vfscore_vop_erofs(void)
{
	return EROFS;
}

/*
 * vnode_init() is called once (from vfs_init)
 * in initialization.
 */
void
vnode_init(void)
{
	int i;

	for (i = 0; i < VNODE_BUCKETS; i++)
		UK_INIT_LIST_HEAD(&vnode_table[i]);
}

void vn_add_name(struct vnode *vp __unused, struct dentry *dp)
{
	/* TODO: Re-enable this check when preemption and/or smp is
	 * here */
	/* UK_ASSERT(uk_mutex_is_locked(&vp->v_lock)); */
	uk_list_add(&dp->d_names_link, &vp->v_names);
}

void vn_del_name(struct vnode *vp __unused, struct dentry *dp)
{
	/* TODO: Re-enable this check when preemption and/or smp is
	 * here */
	/* UK_ASSERT(uk_mutex_is_locked(&vp->v_lock)); */
	uk_list_del(&dp->d_names_link);
}

