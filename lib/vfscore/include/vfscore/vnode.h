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

#ifndef _SYS_VNODE_H_
#define _SYS_VNODE_H_

#include <sys/stat.h>
#include <dirent.h>

#include <uk/mutex.h>
#include <uk/list.h>
#include <time.h>
#include <vfscore/uio.h>
#include <vfscore/dentry.h>

struct vfsops;
struct vnops;
struct vnode;
struct vfscore_file;

/*
 * Vnode types.
 */
enum vtype {
	VNON,	    /* no type */
	VREG,	    /* regular file  */
	VDIR,	    /* directory */
	VBLK,	    /* block device */
	VCHR,	    /* character device */
	VLNK,	    /* symbolic link */
	VSOCK,	    /* socks */
	VFIFO,	    /* FIFO */
	VBAD
};

/*
 * Reading or writing any of these items requires holding the
 * appropriate lock.
 */
struct vnode {
	uint64_t	v_ino;		/* inode number */
	struct uk_list_head v_link;	/* link for hash list */
	struct mount	*v_mount;	/* mounted vfs pointer */
	struct vnops	*v_op;		/* vnode operations */
	int		v_refcnt;	/* reference count */
	int		v_type;		/* vnode type */
	int		v_flags;	/* vnode flag */
	mode_t		v_mode;		/* file mode */
	off_t		v_size;		/* file size */
	struct uk_mutex	v_lock;		/* lock for this vnode */
	struct uk_list_head v_names;	/* directory entries pointing at this */
	void		*v_data;	/* private data for fs */
};

/* flags for vnode */
#define VROOT		0x0001		/* root of its file system */
#define VISTTY		0x0002		/* device is tty */
#define VPROTDEV	0x0004		/* protected device */

/*
 * Vnode attribute
 */
struct vattr {
	unsigned int	va_mask;
	enum vtype	va_type;	/* vnode type */
	mode_t		va_mode;	/* file access mode */
	nlink_t		va_nlink;
	uid_t		va_uid;
	gid_t		va_gid;
	dev_t           va_fsid;        /* id of the underlying filesystem */
	ino_t		va_nodeid;
	struct timespec	va_atime;
	struct timespec	va_mtime;
	struct timespec	va_ctime;
	dev_t		va_rdev;
	uint64_t	va_nblocks;
	off_t		va_size;
};

/* struct vattr is used to consolidate multiple types of file
 * attributes for passing them as function parameters. Macros bellow
 * are selectors, of what types of attributes are valid in particular
 * struct vattr*/
#define	AT_TYPE		0x00001
#define	AT_MODE		0x00002
#define	AT_UID		0x00004
#define	AT_GID		0x00008
#define	AT_FSID		0x00010
#define	AT_NODEID	0x00020
#define	AT_NLINK	0x00040
#define	AT_SIZE		0x00080
#define	AT_ATIME	0x00100
#define	AT_MTIME	0x00200
#define	AT_CTIME	0x00400
#define	AT_RDEV		0x00800
#define	AT_BLKSIZE	0x01000
#define	AT_NBLOCKS	0x02000
#define	AT_SEQ		0x08000
#define	AT_XVATTR	0x10000

/*
 *  Modes.
 */
#define VAPPEND 00010
#define	VREAD	00004		/* read, write, execute permissions */
#define	VWRITE	00002
#define	VEXEC	00001

#define IO_APPEND	0x0001
#define IO_SYNC		0x0002

/*
 * ARC actions
 */
#define ARC_ACTION_QUERY    0
#define ARC_ACTION_HOLD     1
#define ARC_ACTION_RELEASE  2

typedef	int (*vnop_open_t)	(struct vfscore_file *);
typedef	int (*vnop_close_t)	(struct vnode *, struct vfscore_file *);
typedef	int (*vnop_read_t)	(struct vnode *, struct vfscore_file *,
				 struct uio *, int);
typedef	int (*vnop_write_t)	(struct vnode *, struct uio *, int);
typedef	int (*vnop_seek_t)	(struct vnode *, struct vfscore_file *,
				 off_t, off_t);
typedef	int (*vnop_ioctl_t)	(struct vnode *, struct vfscore_file *, unsigned long, void *);
typedef	int (*vnop_fsync_t)	(struct vnode *, struct vfscore_file *);
typedef	int (*vnop_readdir_t)	(struct vnode *, struct vfscore_file *, struct dirent *);
typedef	int (*vnop_lookup_t)	(struct vnode *, char *, struct vnode **);
typedef	int (*vnop_create_t)	(struct vnode *, char *, mode_t);
typedef	int (*vnop_remove_t)	(struct vnode *, struct vnode *, char *);
typedef	int (*vnop_rename_t)	(struct vnode *, struct vnode *, char *,
				 struct vnode *, struct vnode *, char *);
typedef	int (*vnop_mkdir_t)	(struct vnode *, char *, mode_t);
typedef	int (*vnop_rmdir_t)	(struct vnode *, struct vnode *, char *);
typedef	int (*vnop_getattr_t)	(struct vnode *, struct vattr *);
typedef	int (*vnop_setattr_t)	(struct vnode *, struct vattr *);
typedef	int (*vnop_inactive_t)	(struct vnode *);
typedef	int (*vnop_truncate_t)	(struct vnode *, off_t);
typedef	int (*vnop_link_t)      (struct vnode *, struct vnode *, char *);
typedef int (*vnop_cache_t) (struct vnode *, struct vfscore_file *, struct uio *);
typedef int (*vnop_fallocate_t) (struct vnode *, int, off_t, off_t);
typedef int (*vnop_readlink_t)  (struct vnode *, struct uio *);
typedef int (*vnop_symlink_t)   (struct vnode *, char *, char *);

/*
 * vnode operations
 */
struct vnops {
	vnop_open_t		vop_open;
	vnop_close_t		vop_close;
	vnop_read_t		vop_read;
	vnop_write_t		vop_write;
	vnop_seek_t		vop_seek;
	vnop_ioctl_t		vop_ioctl;
	vnop_fsync_t		vop_fsync;
	vnop_readdir_t		vop_readdir;
	vnop_lookup_t		vop_lookup;
	vnop_create_t		vop_create;
	vnop_remove_t		vop_remove;
	vnop_rename_t		vop_rename;
	vnop_mkdir_t		vop_mkdir;
	vnop_rmdir_t		vop_rmdir;
	vnop_getattr_t		vop_getattr;
	vnop_setattr_t		vop_setattr;
	vnop_inactive_t		vop_inactive;
	vnop_truncate_t		vop_truncate;
	vnop_link_t		vop_link;
	vnop_cache_t		vop_cache;
	vnop_fallocate_t	vop_fallocate;
	vnop_readlink_t		vop_readlink;
	vnop_symlink_t		vop_symlink;
};

/*
 * vnode interface
 */
#define VOP_OPEN(VP, FP)	   ((VP)->v_op->vop_open)(FP)
#define VOP_CLOSE(VP, FP)	   ((VP)->v_op->vop_close)(VP, FP)
#define VOP_READ(VP, FP, U, F)	   ((VP)->v_op->vop_read)(VP, FP, U, F)
#define VOP_CACHE(VP, FP, U)	   ((VP)->v_op->vop_cache)(VP, FP, U)
#define VOP_WRITE(VP, U, F)	   ((VP)->v_op->vop_write)(VP, U, F)
#define VOP_SEEK(VP, FP, OLD, NEW) ((VP)->v_op->vop_seek)(VP, FP, OLD, NEW)
#define VOP_IOCTL(VP, FP, C, A)	   ((VP)->v_op->vop_ioctl)(VP, FP, C, A)
#define VOP_FSYNC(VP, FP)	   ((VP)->v_op->vop_fsync)(VP, FP)
#define VOP_READDIR(VP, FP, DIR)   ((VP)->v_op->vop_readdir)(VP, FP, DIR)
#define VOP_LOOKUP(DVP, N, VP)	   ((DVP)->v_op->vop_lookup)(DVP, N, VP)
#define VOP_CREATE(DVP, N, M)	   ((DVP)->v_op->vop_create)(DVP, N, M)
#define VOP_REMOVE(DVP, VP, N)	   ((DVP)->v_op->vop_remove)(DVP, VP, N)
#define VOP_RENAME(DVP1, VP1, N1, DVP2, VP2, N2) \
			   ((DVP1)->v_op->vop_rename)(DVP1, VP1, N1, DVP2, VP2, N2)
#define VOP_MKDIR(DVP, N, M)	   ((DVP)->v_op->vop_mkdir)(DVP, N, M)
#define VOP_RMDIR(DVP, VP, N)	   ((DVP)->v_op->vop_rmdir)(DVP, VP, N)
#define VOP_GETATTR(VP, VAP)	   ((VP)->v_op->vop_getattr)(VP, VAP)
#define VOP_SETATTR(VP, VAP)	   ((VP)->v_op->vop_setattr)(VP, VAP)
#define VOP_INACTIVE(VP)	   ((VP)->v_op->vop_inactive)(VP)
#define VOP_TRUNCATE(VP, N)	   ((VP)->v_op->vop_truncate)(VP, N)
#define VOP_LINK(DVP, SVP, N) 	   ((DVP)->v_op->vop_link)(DVP, SVP, N)
#define VOP_FALLOCATE(VP, M, OFF, LEN) ((VP)->v_op->vop_fallocate)(VP, M, OFF, LEN)
#define VOP_READLINK(VP, U)        ((VP)->v_op->vop_readlink)(VP, U)
#define VOP_SYMLINK(DVP, OP, NP)   ((DVP)->v_op->vop_symlink)(DVP, OP, NP)

int vfscore_vop_nullop();
int vfscore_vop_einval();
int vfscore_vop_eperm();
int vfscore_vop_erofs();
struct vnode *vn_lookup(struct mount *, uint64_t);
void	 vn_lock(struct vnode *);
void	 vn_unlock(struct vnode *);
int	 vn_stat(struct vnode *, struct stat *);
int	 vn_settimes(struct vnode *, struct timespec[2]);
int	 vn_setmode(struct vnode *, mode_t mode);
int	 vn_access(struct vnode *, int);
int	 vfscore_vget(struct mount *, uint64_t ino, struct vnode **vpp);
void	 vput(struct vnode *);
void	 vref(struct vnode *);
void	 vrele(struct vnode *);
void	 vflush(struct mount *);
void vn_add_name(struct vnode *, struct dentry *);
void vn_del_name(struct vnode *, struct dentry *);

extern enum vtype iftovt_tab[];
extern int vttoif_tab[];
#define IFTOVT(mode)    (iftovt_tab[((mode) & S_IFMT) >> 12])
#define VTTOIF(indx)	(vttoif_tab[(int)(indx)])
#define MAKEIMODE(indx, mode)   (int)(VTTOIF(indx) | (mode))

#define VATTR_NULL(vp) (*(vp) = (vattr_t){})

static inline void vnode_pager_setsize(struct vnode *vp, off_t size)
{
	vp->v_size = size;
}


#endif /* !_SYS_VNODE_H_ */
