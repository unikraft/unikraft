/*-
 * Copyright (c) 1989, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)mount.h	8.21 (Berkeley) 5/20/95
 */

#ifndef _VFSCORE_SYS_MOUNT_H_
#define _VFSCORE_SYS_MOUNT_H_

#define _BSD_SOURCE

#include <sys/mount.h>
#include <sys/statfs.h>
#include <limits.h>
#include <uk/list.h>
#include <vfscore/vnode.h>

/*
 * Mount data
 */
struct mount {
	struct vfsops	*m_op;		/* pointer to vfs operation */
	int		m_flags;	/* mount flag */
	int		m_count;	/* reference count */
	char            m_path[PATH_MAX]; /* mounted path */
	char            m_special[PATH_MAX]; /* resource */
	struct device	*m_dev;		/* mounted device */
	struct dentry	*m_root;	/* root vnode */
	struct dentry	*m_covered;	/* vnode covered on parent fs */
	void		*m_data;	/* private data for fs */
	struct uk_list_head mnt_list;
	fsid_t 		m_fsid; 	/* id that uniquely identifies the fs */
};


/*
 * Mount flags.
 */
#ifndef MNT_RDONLY
#define	MNT_RDONLY	0x00000001	/* read only filesystem */
#endif
#ifndef	MNT_SYNCHRONOUS
#define	MNT_SYNCHRONOUS	0x00000002	/* file system written synchronously */
#endif
#ifndef	MNT_NOEXEC
#define	MNT_NOEXEC	0x00000004	/* can't exec from filesystem */
#endif
#ifndef	MNT_NOSUID
#define	MNT_NOSUID	0x00000008	/* don't honor setuid bits on fs */
#endif
#ifndef	MNT_NODEV
#define	MNT_NODEV	0x00000010	/* don't interpret special files */
#endif
#ifndef	MNT_UNION
#define	MNT_UNION	0x00000020	/* union with underlying filesystem */
#endif
#ifndef	MNT_ASYNC
#define	MNT_ASYNC	0x00000040	/* file system written asynchronously */
#endif

/*
 * Unmount flags.
 */
#ifndef MNT_FORCE
#define MNT_FORCE	0x00000001	/* forced unmount */
#endif

/*
 * exported mount flags.
 */
#ifndef	MNT_EXRDONLY
#define	MNT_EXRDONLY	0x00000080	/* exported read only */
#endif
#ifndef	MNT_EXPORTED
#define	MNT_EXPORTED	0x00000100	/* file system is exported */
#endif
#ifndef	MNT_DEFEXPORTED
#define	MNT_DEFEXPORTED	0x00000200	/* exported to the world */
#endif
#ifndef	MNT_EXPORTANON
#define	MNT_EXPORTANON	0x00000400	/* use anon uid mapping for everyone */
#endif
#ifndef	MNT_EXKERB
#define	MNT_EXKERB	0x00000800	/* exported with Kerberos uid mapping */
#endif

/*
 * Flags set by internal operations.
 */
#ifndef	MNT_LOCAL
#define	MNT_LOCAL	0x00001000	/* filesystem is stored locally */
#endif
#ifndef	MNT_QUOTA
#define	MNT_QUOTA	0x00002000	/* quotas are enabled on filesystem */
#endif
#ifndef	MNT_ROOTFS
#define	MNT_ROOTFS	0x00004000	/* identifies the root filesystem */
#endif

/*
 * Mask of flags that are visible to statfs()
 */
#ifndef	MNT_VISFLAGMASK
#define	MNT_VISFLAGMASK	0x0000ffff
#endif

/*
 * Filesystem type switch table.
 */
struct vfscore_fs_type {
	const char      *vs_name;	/* name of file system */
	int		(*vs_init)(void); /* initialize routine */
	struct vfsops	*vs_op;		/* pointer to vfs operation */
};

#define UK_FS_REGISTER(fssw) static struct vfscore_fs_type	\
	__attribute((__section__(".uk_fs_list")))		\
	*__ptr_##fssw __used = &fssw;				\

/*
 * Operations supported on virtual file system.
 */
struct vfsops {
	int (*vfs_mount)	(struct mount *, const char *, int, const void *);
	int (*vfs_unmount)	(struct mount *, int flags);
	int (*vfs_sync)		(struct mount *);
	int (*vfs_vget)		(struct mount *, struct vnode *);
	int (*vfs_statfs)	(struct mount *, struct statfs *);
	struct vnops	*vfs_vnops;
};

typedef int (*vfsop_mount_t)(struct mount *, const char *, int, const void *);
typedef int (*vfsop_umount_t)(struct mount *, int flags);
typedef int (*vfsop_sync_t)(struct mount *);
typedef int (*vfsop_vget_t)(struct mount *, struct vnode *);
typedef int (*vfsop_statfs_t)(struct mount *, struct statfs *);

/*
 * VFS interface
 */
#define VFS_MOUNT(MP, DEV, FL, DAT) ((MP)->m_op->vfs_mount)(MP, DEV, FL, DAT)
#define VFS_UNMOUNT(MP, FL)         ((MP)->m_op->vfs_unmount)(MP, FL)
#define VFS_SYNC(MP)                ((MP)->m_op->vfs_sync)(MP)
#define VFS_VGET(MP, VP)            ((MP)->m_op->vfs_vget)(MP, VP)
#define VFS_STATFS(MP, SFP)         ((MP)->m_op->vfs_statfs)(MP, SFP)

#define VFS_NULL		    ((void *)vfs_null)

int vfscore_nullop();
int vfscore_einval();

void	 vfs_busy(struct mount *mp);
void	 vfs_unbusy(struct mount *mp);

void	 vfscore_release_mp_dentries(struct mount *mp);

#endif	/* !_VFSCORE_SYS_MOUNT_H_ */
