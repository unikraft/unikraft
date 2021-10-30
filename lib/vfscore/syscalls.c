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
 * vfs_syscalls.c - everything in this file is a routine implementing
 *                  a VFS system call.
 */

#define _BSD_SOURCE
#define _GNU_SOURCE
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <dirent.h>
#include <vfscore/prex.h>
#include <vfscore/vnode.h>
#include <vfscore/file.h>

#include "vfs.h"
#include <vfscore/fs.h>

extern struct task *main_task;

static int
open_no_follow_chk(char *path)
{
	int           error;
	struct dentry *ddp;
	char          *name;
	struct dentry *dp;
	struct vnode  *vp;

	ddp = NULL;
	dp  = NULL;
	vp  = NULL;

	error = lookup(path, &ddp, &name);
	if (error) {
		return (error);
	}

	error = namei_last_nofollow(path, ddp, &dp);
	if (error) {
		goto out;
	}

	vp = dp->d_vnode;
	vn_lock(vp);
	if (vp->v_type == VLNK) {
		error = ELOOP;
		goto out;
	}

	error = 0;
out:
	if (vp != NULL) {
		vn_unlock(vp);
	}

	if (dp != NULL) {
		drele(dp);
	}

	if (ddp != NULL) {
		drele(ddp);
	}

	return (error);
}

int
sys_open(char *path, int flags, mode_t mode, struct vfscore_file **fpp)
{
	struct vfscore_file *fp;
	struct dentry *dp, *ddp;
	struct vnode *vp;
	char *filename;
	int error;

	DPRINTF(VFSDB_SYSCALL, ("sys_open: path=%s flags=%x mode=%x\n",
				path, flags, mode));

	flags = vfscore_fflags(flags);
	if (flags & O_CREAT) {
		error = namei(path, &dp);
		if (error == ENOENT) {
			/* Create new struct vfscore_file. */
			if ((error = lookup(path, &ddp, &filename)) != 0)
				return error;

			vn_lock(ddp->d_vnode);
			if ((error = vn_access(ddp->d_vnode, VWRITE)) != 0) {
				vn_unlock(ddp->d_vnode);
				drele(ddp);
				return error;
			}
			mode &= ~S_IFMT;
			mode |= S_IFREG;
			error = VOP_CREATE(ddp->d_vnode, filename, mode);
			vn_unlock(ddp->d_vnode);
			drele(ddp);

			if (error)
				return error;
			if ((error = namei(path, &dp)) != 0)
				return error;

			vp = dp->d_vnode;
			flags &= ~O_TRUNC;
		} else if (error) {
			return error;
		} else {
			/* File already exits */
			if (flags & O_EXCL) {
				error = EEXIST;
				goto out_drele;
			}
		}

		vp = dp->d_vnode;
		flags &= ~O_CREAT;
	} else {
		/* Open */
		if (flags & O_NOFOLLOW) {
			error = open_no_follow_chk(path);
			if (error != 0) {
				return (error);
			}
		}
		error = namei(path, &dp);
		if (error)
			return error;

		vp = dp->d_vnode;

		if (flags & UK_FWRITE || flags & O_TRUNC) {
			error = vn_access(vp, VWRITE);
			if (error)
				goto out_drele;

			error = EISDIR;
			if (vp->v_type == VDIR)
				goto out_drele;
		}
		if (flags & O_DIRECTORY) {
		    if (vp->v_type != VDIR) {
		        error = ENOTDIR;
		        goto out_drele;
		    }
		}
	}

	vn_lock(vp);
	/* Process truncate request */
	if (flags & O_TRUNC) {
		error = EINVAL;
		if (!(flags & UK_FWRITE) || vp->v_type == VDIR)
			goto out_vn_unlock;

		error = VOP_TRUNCATE(vp, 0);
		if (error)
			goto out_vn_unlock;
	}

	fp = calloc(sizeof(struct vfscore_file), 1);
	if (!fp) {
	    error = ENOMEM;
	    goto out_vn_unlock;
	}
	fhold(fp);
	fp->f_flags = flags;

	/*
	 * Don't need to increase refcount here, we already hold a reference
	 * to dp from namei().
	 */
	// change to std::move once dp is a dentry_ref
	fp->f_dentry = dp;
	dp = NULL;

	uk_mutex_init(&fp->f_lock);

	error = VOP_OPEN(vp, fp);
	if (error) {
		vn_unlock(vp);
		// Note direct delete of fp instead of fdrop(fp). fp was never
		// returned so cannot be in use, and because it wasn't opened
		// it cannot be close()ed.
		drele(fp->f_dentry);
		free(fp);
		return error;
	}
	vn_unlock(vp);

	*fpp = fp;
	return 0;

out_vn_unlock:
	vn_unlock(vp);
out_drele:
	if (dp) {
		drele(dp);
	}
	return error;
}

int
sys_close(struct vfscore_file *fp __unused)
{

	return 0;
}

int
sys_read(struct vfscore_file *fp, const struct iovec *iov, size_t niov,
		off_t offset, size_t *count)
{
	int error = 0;
	struct iovec *copy_iov;
	if ((fp->f_flags & UK_FREAD) == 0)
		return EBADF;

	size_t bytes = 0;
	const struct iovec *iovp = iov;

	for (unsigned i = 0; i < niov; i++) {
		if (iovp->iov_len > IOSIZE_MAX - bytes) {
			return EINVAL;
		}
		bytes += iovp->iov_len;
		iovp++;
	}

	if (bytes == 0) {
		*count = 0;
		return 0;
	}

	struct uio uio;
	/* TODO: is it necessary to copy iov within Unikraft?
	 * OSv did this, mentioning this reason:
	 *
	 * "Unfortunately, the current implementation of fp->read
	 *  zeros the iov_len fields when it reads from disk, so we
	 *  have to copy iov. "
	 */
	copy_iov = calloc(sizeof(struct iovec), niov);
	if (!copy_iov)
		return ENOMEM;
	memcpy(copy_iov, iov, sizeof(struct iovec)*niov);

	uio.uio_iov = copy_iov;
	uio.uio_iovcnt = niov;
	uio.uio_offset = offset;
	uio.uio_resid = bytes;
	uio.uio_rw = UIO_READ;
	error = vfs_read(fp, &uio, (offset == -1) ? 0 : FOF_OFFSET);
	*count = bytes - uio.uio_resid;

	free(copy_iov);
	return error;
}

int
sys_write(struct vfscore_file *fp, const struct iovec *iov, size_t niov,
		off_t offset, size_t *count)
{
	struct iovec *copy_iov;
	int error = 0;
	if ((fp->f_flags & UK_FWRITE) == 0)
		return EBADF;

	size_t bytes = 0;
	const struct iovec *iovp = iov;
	for (unsigned i = 0; i < niov; i++) {
		if (iovp->iov_len > IOSIZE_MAX - bytes) {
			return EINVAL;
		}
		bytes += iovp->iov_len;
		iovp++;
	}

	if (bytes == 0) {
		*count = 0;
		return 0;
	}

	struct uio uio;

	/* TODO: same note as in sys_read. Original comment:
	 *
	 * "Unfortunately, the current implementation of fp->write zeros the
	 *  iov_len fields when it writes to disk, so we have to copy iov.
	 */
	/* std::vector<iovec> copy_iov(iov, iov + niov); */
	copy_iov = calloc(sizeof(struct iovec), niov);
	if (!copy_iov)
		return ENOMEM;
	memcpy(copy_iov, iov, sizeof(struct iovec)*niov);

	uio.uio_iov = copy_iov;
	uio.uio_iovcnt = niov;
	uio.uio_offset = offset;
	uio.uio_resid = bytes;
	uio.uio_rw = UIO_WRITE;
	error = vfs_write(fp, &uio, (offset == -1) ? 0 : FOF_OFFSET);
	*count = bytes - uio.uio_resid;

	free(copy_iov);
	return error;
}

int
sys_lseek(struct vfscore_file *fp, off_t off, int type, off_t *origin)
{
	struct vnode *vp;

	DPRINTF(VFSDB_SYSCALL, ("sys_seek: fp=%p off=%ud type=%d\n",
				fp, (unsigned int)off, type));

	if (!fp->f_dentry) {
	    // Linux doesn't implement lseek() on pipes, sockets, or ttys.
	    // In OSV, we only implement lseek() on regular files, backed by vnode
	    return ESPIPE;
	}

	vp = fp->f_dentry->d_vnode;
	int error = EINVAL;
	vn_lock(vp);
	switch (type) {
	case SEEK_CUR:
		off = fp->f_offset + off;
		break;
	case SEEK_END:
		off = vp->v_size + off;
		break;
	}
	if (off >= 0) {
		error = VOP_SEEK(vp, fp, fp->f_offset, off);
		if (!error) {
			*origin      = off;
			fp->f_offset = off;
		}
	}
	vn_unlock(vp);
	return error;
}

int
sys_ioctl(struct vfscore_file *fp, unsigned long request, void *buf)
{
	int error = 0;

	DPRINTF(VFSDB_SYSCALL, ("sys_ioctl: fp=%p request=%lux\n", fp, request));

	if ((fp->f_flags & (UK_FREAD | UK_FWRITE)) == 0)
		return EBADF;

	switch (request) {
	case FIOCLEX:
		fp->f_flags |= O_CLOEXEC;
		break;
	case FIONCLEX:
		fp->f_flags &= ~O_CLOEXEC;
		break;
	default:
		error = vfs_ioctl(fp, request, buf);
		break;
	}

	DPRINTF(VFSDB_SYSCALL, ("sys_ioctl: comp error=%d\n", error));
	return error;
}

int
sys_fsync(struct vfscore_file *fp)
{
	struct vnode *vp;
	int error;

	DPRINTF(VFSDB_SYSCALL, ("sys_fsync: fp=%p\n", fp));

	if (!fp->f_dentry)
		return EINVAL;

	vp = fp->f_dentry->d_vnode;
	vn_lock(vp);
	error = VOP_FSYNC(vp, fp);
	vn_unlock(vp);
	return error;
}

int
sys_fstat(struct vfscore_file *fp, struct stat *st)
{
	int error = 0;

	DPRINTF(VFSDB_SYSCALL, ("sys_fstat: fp=%p\n", fp));

	error = vfs_stat(fp, st);

	return error;
}

/*
 * Return 0 if directory is empty
 */
static int
check_dir_empty(char *path)
{
	int error;
	struct vfscore_file *fp;
	struct dirent dir;

	DPRINTF(VFSDB_SYSCALL, ("check_dir_empty\n"));

	error = sys_open(path, O_RDONLY, 0, &fp);
	if (error)
		goto out_error;

	do {
		error = sys_readdir(fp, &dir);
		if (error != 0 && error != EACCES)
			break;
	} while (!strcmp(dir.d_name, ".") || !strcmp(dir.d_name, ".."));

	if (error == ENOENT)
		error = 0;
	else if (error == 0) {
	    // Posix specifies to return EEXIST in this case (rmdir of non-empty
	    // directory, but Linux actually returns ENOTEMPTY).
		error = ENOTEMPTY;
	}
	fdrop(fp);
out_error:
	return error;
}

int
sys_readdir(struct vfscore_file *fp, struct dirent *dir)
{
	struct vnode *dvp;
	int error;

	DPRINTF(VFSDB_SYSCALL, ("sys_readdir: fp=%p\n", fp));

	if (!fp->f_dentry)
		return ENOTDIR;

	dvp = fp->f_dentry->d_vnode;
	vn_lock(dvp);
	if (dvp->v_type != VDIR) {
		vn_unlock(dvp);
		return ENOTDIR;
	}
	error = VOP_READDIR(dvp, fp, dir);
	DPRINTF(VFSDB_SYSCALL, ("sys_readdir: error=%d path=%s\n",
				error, dir->d_name));
	vn_unlock(dvp);
	return error;
}

int
sys_rewinddir(struct vfscore_file *fp)
{
	struct vnode *dvp;

	if (!fp->f_dentry)
		return ENOTDIR;

	dvp = fp->f_dentry->d_vnode;
	vn_lock(dvp);
	if (dvp->v_type != VDIR) {
		vn_unlock(dvp);
		return EBADF;
	}
	fp->f_offset = 0;
	vn_unlock(dvp);
	return 0;
}

int
sys_seekdir(struct vfscore_file *fp, long loc)
{
	struct vnode *dvp;

	if (!fp->f_dentry)
		return ENOTDIR;

	dvp = fp->f_dentry->d_vnode;
	vn_lock(dvp);
	if (dvp->v_type != VDIR) {
		vn_unlock(dvp);
		return EBADF;
	}
	fp->f_offset = (off_t)loc;
	vn_unlock(dvp);
	return 0;
}

int
sys_telldir(struct vfscore_file *fp, long *loc)
{
	struct vnode *dvp;

	if (!fp->f_dentry)
		return ENOTDIR;

	dvp = fp->f_dentry->d_vnode;
	vn_lock(dvp);
	if (dvp->v_type != VDIR) {
		vn_unlock(dvp);
		return EBADF;
	}
	*loc = (long)fp->f_offset;
	vn_unlock(dvp);
	return 0;
}

int
sys_mkdir(char *path, mode_t mode)
{
	char *name;
	struct dentry *dp, *ddp;
	int error;

	DPRINTF(VFSDB_SYSCALL, ("sys_mkdir: path=%s mode=%d\n",	path, mode));

	error = namei(path, &dp);
	if (!error) {
		/* File already exists */
		drele(dp);
		return EEXIST;
	}

	if ((error = lookup(path, &ddp, &name)) != 0) {
		/* Directory already exists */
		return error;
	}

	vn_lock(ddp->d_vnode);
	if ((error = vn_access(ddp->d_vnode, VWRITE)) != 0)
		goto out;
	mode &= ~S_IFMT;
	mode |= S_IFDIR;

	error = VOP_MKDIR(ddp->d_vnode, name, mode);
 out:
	vn_unlock(ddp->d_vnode);
	drele(ddp);
	return error;
}

int
sys_rmdir(char *path)
{
	struct dentry *dp, *ddp;
	struct vnode *vp;
	int error;
	char *name;

	DPRINTF(VFSDB_SYSCALL, ("sys_rmdir: path=%s\n", path));

	if ((error = check_dir_empty(path)) != 0)
		return error;
	error = namei(path, &dp);
	if (error)
		return error;

	vp = dp->d_vnode;
	vn_lock(vp);
	if ((error = vn_access(vp, VWRITE)) != 0)
		goto out;
	if (vp->v_type != VDIR) {
		error = ENOTDIR;
		goto out;
	}
	if (vp->v_flags & VROOT || vp->v_refcnt >= 2) {
		error = EBUSY;
		goto out;
	}
	if ((error = lookup(path, &ddp, &name)) != 0)
		goto out;

	vn_lock(ddp->d_vnode);
	error = VOP_RMDIR(ddp->d_vnode, vp, name);
	vn_unlock(ddp->d_vnode);

	vn_unlock(vp);
	dentry_remove(dp);
	drele(ddp);
	drele(dp);
	return error;

 out:
	vn_unlock(vp);
	drele(dp);
	return error;
}

int
sys_mknod(char *path, mode_t mode)
{
	char *name;
	struct dentry *dp, *ddp;
	int error;

	DPRINTF(VFSDB_SYSCALL, ("sys_mknod: path=%s mode=%d\n",	path, mode));

	switch (mode & S_IFMT) {
	case S_IFREG:
	case S_IFDIR:
	case S_IFIFO:
	case S_IFSOCK:
		/* OK */
		break;
	default:
		return EINVAL;
	}

	error = namei(path, &dp);
	if (!error) {
		drele(dp);
		return EEXIST;
	}

	if ((error = lookup(path, &ddp, &name)) != 0)
		return error;

	vn_lock(ddp->d_vnode);
	if ((error = vn_access(ddp->d_vnode, VWRITE)) != 0)
		goto out;
	if (S_ISDIR(mode))
		error = VOP_MKDIR(ddp->d_vnode, name, mode);
	else
		error = VOP_CREATE(ddp->d_vnode, name, mode);
 out:
	vn_unlock(ddp->d_vnode);
	drele(ddp);
	return error;
}

/*
 * Returns true when @parent path could represent parent directory
 * of a file or directory represented by @child path.
 *
 * Assumes both paths do not have trailing slashes.
 */
static int
is_parent(const char *parent, const char *child)
{
	size_t p_len = strlen(parent);
	return !strncmp(parent, child, p_len) && (parent[p_len-1] == '/' || child[p_len] == '/');
}

static int
has_trailing(const char *path, char ch)
{
	size_t len = strlen(path);
	return len && path[len - 1] == ch;
}

static void
strip_trailing(char *path, char ch)
{
	size_t len = strlen(path);

	while (len && path[len - 1] == ch)
		len--;

	path[len] = '\0';
}

int
sys_rename(char *src, char *dest)
{
	struct dentry *dp1, *dp2 = 0, *ddp1, *ddp2;
	struct vnode *vp1, *vp2 = 0, *dvp1, *dvp2;
	char *sname, *dname;
	int error;
	char root[] = "/";
	int ts; /* trailing slash */

	DPRINTF(VFSDB_SYSCALL, ("sys_rename: src=%s dest=%s\n", src, dest));

	ts = 0;
	if (has_trailing(src, '/')) {
		if (strlen(src) != 1) {
			/* remove trailing slash iff path is none root */
			strip_trailing(src, '/');
			ts = 1;
		}
	}

	error = lookup(src, &ddp1, &sname);
	if (error != 0) {
		return (error);
	}

	error = namei_last_nofollow(src, ddp1, &dp1);
	if (error != 0) {
		drele(ddp1);
		return (error);
	}

	vp1 = dp1->d_vnode;
	vn_lock(vp1);

	if (vp1->v_type != VDIR && ts) {
		error = ENOTDIR;
		goto err1;
	}

	ts = 0;
	if (has_trailing(dest, '/')) {
		if (strlen(dest) != 1) {
			/* remove trailing slash iff path is none root */
			strip_trailing(dest, '/');
			ts = 1;
		}
	}

	error = lookup(dest, &ddp2, &dname);
	if (error != 0) {
		goto err1;
	}

	error = namei_last_nofollow(dest, ddp2, &dp2);
	if (error == 0) {
		/* target exists */

		vp2 = dp2->d_vnode;
		vn_lock(vp2);

		if (vp2->v_type != VDIR && vp2->v_type != VLNK) {
			if (vp1->v_type == VDIR || ts) {
				error = ENOTDIR;
				goto err2;
			}
		} else if (vp1->v_type != VDIR && vp2->v_type == VDIR) {
			error = EISDIR;
			goto err2;
		}
		if (vp2->v_type == VDIR && check_dir_empty(dest)) {
			error = EEXIST;
			goto err2;
		}
	} else if (error == ENOENT) {
		if (vp1->v_type != VDIR && ts) {
			error = ENOTDIR;
			goto err2;
		}
	} else {
		goto err2;
	}

	if (strcmp(dest, "/"))
		strip_trailing(dest, '/');

	if (strcmp(src, "/"))
		strip_trailing(src, '/');

	/* If source and dest are the same, do nothing */
	if (!strncmp(src, dest, PATH_MAX))
		goto err2;

	/* Check if target is directory of source */
	if (is_parent(src, dest)) {
		error = EINVAL;
		goto err2;
	}

	dname = strrchr(dest, '/');
	if (dname == NULL) {
		error = ENOTDIR;
		goto err2;
	}
	if (dname == dest)
		dest = root;

	*dname = 0;
	dname++;

	dvp1 = ddp1->d_vnode;
	vn_lock(dvp1);

	dvp2 = ddp2->d_vnode;
	vn_lock(dvp2);

	/* Source and destination directions should be writable) */
	if ((error = vn_access(dvp1, VWRITE)) != 0)
	    goto err3;
	if ((error = vn_access(dvp2, VWRITE)) != 0)
	    goto err3;

	/* The source and dest must be same file system */
	if (dvp1->v_mount != dvp2->v_mount) {
		error = EXDEV;
		goto err3;
	}

	error = VOP_RENAME(dvp1, vp1, sname, dvp2, vp2, dname);
	if (error)
		goto err3;

	error = dentry_move(dp1, ddp2, dname);

	if (dp2)
		dentry_remove(dp2);

 err3:
	vn_unlock(dvp2);
	vn_unlock(dvp1);
 err2:
	if (vp2) {
		vn_unlock(vp2);
		drele(dp2);
	}
	drele(ddp2);
 err1:
	vn_unlock(vp1);
	drele(dp1);
	drele(ddp1);
	return error;
}

int
sys_symlink(const char *oldpath, const char *newpath)
{
	struct task	*t = main_task;
	int		error;
	char		op[PATH_MAX];
	char		np[PATH_MAX];
	struct dentry	*newdp;
	struct dentry	*newdirdp;
	char		*name;

	if (oldpath == NULL || newpath == NULL) {
		return (EFAULT);
	}

	DPRINTF(VFSDB_SYSCALL, ("sys_link: oldpath=%s newpath=%s\n",
				oldpath, newpath));

	newdp		= NULL;
	newdirdp	= NULL;

	error = task_conv(t, newpath, VWRITE, np);
	if (error != 0) {
		return (error);
	}

	/* parent directory for new path must exist */
	if ((error = lookup(np, &newdirdp, &name)) != 0) {
		error = ENOENT;
		goto out;
	}
	vn_lock(newdirdp->d_vnode);

	/* newpath should not already exist */
	if (namei_last_nofollow(np, newdirdp, &newdp) == 0) {
		drele(newdp);
		error = EEXIST;
		goto out;
	}

	/* check for write access at newpath */
	if ((error = vn_access(newdirdp->d_vnode, VWRITE)) != 0) {
		goto out;
	}

	/* oldpath may not be const char * to VOP_SYMLINK - need to copy */
	size_t tocopy;
	tocopy = strlcpy(op, oldpath, PATH_MAX);
	if (tocopy >= PATH_MAX - 1) {
		error = ENAMETOOLONG;
		goto out;
	}
	error = VOP_SYMLINK(newdirdp->d_vnode, name, op);

out:
	if (newdirdp != NULL) {
		vn_unlock(newdirdp->d_vnode);
		drele(newdirdp);
	}

	return (error);
}

int
sys_link(char *oldpath, char *newpath)
{
	struct dentry *olddp, *newdp, *newdirdp;
	struct vnode *vp;
	char *name;
	int error;

	DPRINTF(VFSDB_SYSCALL, ("sys_link: oldpath=%s newpath=%s\n",
				oldpath, newpath));

	/* File from oldpath must exist */
	if ((error = namei(oldpath, &olddp)) != 0)
		return error;

	vp = olddp->d_vnode;
	vn_lock(vp);

	/* If newpath exists, it shouldn't be overwritten */
	if (!namei(newpath, &newdp)) {
		error = EEXIST;
		goto out;
	}

	if (vp->v_type == VDIR) {
		error = EPERM;
		goto out;
	}

	/* Get pointer to the parent dentry of newpath */
	if ((error = lookup(newpath, &newdirdp, &name)) != 0)
		goto out;

	vn_lock(newdirdp->d_vnode);

	/* Both files must reside on the same mounted file system */
	if (olddp->d_mount != newdirdp->d_mount) {
		error = EXDEV;
		goto out1;
	}

	/* Write access to the dir containing newpath is required */
	if ((error = vn_access(newdirdp->d_vnode, VWRITE)) != 0)
		goto out1;

	/* Map newpath into dentry hash with the same vnode as oldpath */
	if (!(newdp = dentry_alloc(newdirdp, vp, newpath))) {
		error = ENOMEM;
		goto out1;
	}

	error = VOP_LINK(newdirdp->d_vnode, vp, name);
 out1:
	vn_unlock(newdirdp->d_vnode);
	drele(newdirdp);
 out:
	vn_unlock(vp);
	drele(olddp);
	drele(newdp);
	return error;
}

int
sys_unlink(char *path)
{
	char *name;
	struct dentry *dp, *ddp;
	struct vnode *vp;
	int error;

	DPRINTF(VFSDB_SYSCALL, ("sys_unlink: path=%s\n", path));

	ddp   = NULL;
	dp    = NULL;
	vp    = NULL;

	error = lookup(path, &ddp, &name);
	if (error != 0) {
		return (error);
	}

	error = namei_last_nofollow(path, ddp, &dp);
	if (error != 0) {
		goto out;
	}

	vp = dp->d_vnode;
	vn_lock(vp);
	if (vp->v_type == VDIR) {
	    // Posix specifies that we should return EPERM here, but Linux
	    // actually returns EISDIR.
		error = EISDIR;
		goto out;
	}
	if (vp->v_flags & VROOT) {
		error = EBUSY;
		goto out;
	}

	vn_lock(ddp->d_vnode);
	if ((error = vn_access(ddp->d_vnode, VWRITE)) != 0) {
	    vn_unlock(ddp->d_vnode);
	    goto out;
	}
	error = VOP_REMOVE(ddp->d_vnode, vp, name);
	vn_unlock(ddp->d_vnode);

	vn_unlock(vp);
	dentry_remove(dp);
	drele(ddp);
	drele(dp);
	return error;
 out:
	if (vp != NULL) {
		vn_unlock(vp);
	}

	if (dp != NULL) {
		drele(dp);
	}

	if (ddp != NULL) {
		drele(ddp);
	}
	return error;
}

int
sys_access(char *path, int mode)
{
	struct dentry *dp;
	int error, flags;

	DPRINTF(VFSDB_SYSCALL, ("sys_access: path=%s mode=%x\n", path, mode));

	/* If F_OK is set, we return here if file is not found. */
	error = namei(path, &dp);
	if (error)
		return error;

	flags = 0;
	if (mode & R_OK)
		flags |= VREAD;
	if (mode & W_OK)
		flags |= VWRITE;
	if (mode & X_OK)
		flags |= VEXEC;

	error = vn_access(dp->d_vnode, flags);

	drele(dp);
	return error;
}

int
sys_stat(char *path, struct stat *st)
{
	struct dentry *dp;
	int error;

	DPRINTF(VFSDB_SYSCALL, ("sys_stat: path=%s\n", path));

	error = namei(path, &dp);
	if (error)
		return error;
	error = vn_stat(dp->d_vnode, st);
	drele(dp);
	return error;
}

int sys_lstat(char *path, struct stat *st)
{
	int           error;
	struct dentry *ddp;
	char          *name;
	struct dentry *dp;

	DPRINTF(VFSDB_SYSCALL, ("sys_lstat: path=%s\n", path));

	error = lookup(path, &ddp, &name);
	if (error) {
		return (error);
	}

	error = namei_last_nofollow(path, ddp, &dp);
	if (error) {
		drele(ddp);
		return error;
	}

	error = vn_stat(dp->d_vnode, st);
	drele(dp);
	drele(ddp);
	return error;
}

int
sys_statfs(char *path, struct statfs *buf)
{
	struct dentry *dp;
	int error;

	memset(buf, 0, sizeof(*buf));

	error = namei(path, &dp);
	if (error)
		return error;

	error = VFS_STATFS(dp->d_mount, buf);
	drele(dp);

	return error;
}

int
sys_fstatfs(struct vfscore_file *fp, struct statfs *buf)
{
	struct vnode *vp;
	int error = 0;

	if (!fp->f_dentry)
		return EBADF;

	vp = fp->f_dentry->d_vnode;
	memset(buf, 0, sizeof(*buf));

	vn_lock(vp);
	error = VFS_STATFS(vp->v_mount, buf);
	vn_unlock(vp);

	return error;
}

int
sys_truncate(char *path, off_t length)
{
	struct dentry *dp;
	int error;

	error = namei(path, &dp);
	if (error)
		return error;

	vn_lock(dp->d_vnode);
	error = VOP_TRUNCATE(dp->d_vnode, length);
	vn_unlock(dp->d_vnode);

	drele(dp);
	return error;
}

int
sys_ftruncate(struct vfscore_file *fp, off_t length)
{
	struct vnode *vp;
	int error;

	if (!fp->f_dentry)
		return EBADF;

	vp = fp->f_dentry->d_vnode;
	vn_lock(vp);
	error = VOP_TRUNCATE(vp, length);
	vn_unlock(vp);

	return error;
}

int
sys_fchdir(struct vfscore_file *fp, char *cwd)
{
	struct vnode *dvp;

	if (!fp->f_dentry)
		return EBADF;

	dvp = fp->f_dentry->d_vnode;
	vn_lock(dvp);
	if (dvp->v_type != VDIR) {
		vn_unlock(dvp);
		return EBADF;
	}
	strlcpy(cwd, fp->f_dentry->d_path, PATH_MAX);
	vn_unlock(dvp);
	return 0;
}

int
sys_readlink(char *path, char *buf, size_t bufsize, ssize_t *size)
{
	int		error;
	struct dentry	*ddp;
	char		*name;
	struct dentry	*dp;
	struct vnode	*vp;
	struct iovec	vec;
	struct uio	uio;

	*size = 0;
	error = lookup(path, &ddp, &name);
	if (error) {
		return (error);
	}

	error = namei_last_nofollow(path, ddp, &dp);
	if (error) {
		drele(ddp);
		return (error);
	}

	if (dp->d_vnode->v_type != VLNK) {
		drele(dp);
		drele(ddp);
		return (EINVAL);
	}
	vec.iov_base	= buf;
	vec.iov_len	= bufsize;

	uio.uio_iov	= &vec;
	uio.uio_iovcnt	= 1;
	uio.uio_offset	= 0;
	uio.uio_resid	= bufsize;
	uio.uio_rw	= UIO_READ;

	vp = dp->d_vnode;
	vn_lock(vp);
	error = VOP_READLINK(vp, &uio);
	vn_unlock(vp);

	drele(dp);
	drele(ddp);

	if (error) {
		return (error);
	}

	*size = bufsize - uio.uio_resid;
	return (0);
}

/*
 * Check the validity of the members of a struct timeval.
 */
static int is_timeval_valid(const struct timeval *time)
{
	return (time->tv_sec >= 0) &&
		   (time->tv_usec >= 0 && time->tv_usec < 1000000);
}

/*
 * Convert a timeval struct to a timespec one.
 */
static void convert_timeval(struct timespec *to, const struct timeval *from)
{
	if (from) {
		to->tv_sec = from->tv_sec;
		to->tv_nsec = from->tv_usec * 1000; // Convert microseconds to nanoseconds
	} else {
		clock_gettime(CLOCK_REALTIME, to);
	}
}

int
sys_utimes(char *path, const struct timeval *times, int flags)
{
	int error;
	struct dentry *dp;
	struct timespec timespec_times[2];

	DPRINTF(VFSDB_SYSCALL, ("sys_utimes: path=%s\n", path));

	if (times && (!is_timeval_valid(&times[0]) || !is_timeval_valid(&times[1])))
		return EINVAL;

	// Convert each element of timeval array to the timespec type
	convert_timeval(&timespec_times[0], times ? times + 0 : NULL);
	convert_timeval(&timespec_times[1], times ? times + 1 : NULL);

	if (flags & AT_SYMLINK_NOFOLLOW) {
		struct dentry *ddp;
		error = lookup(path, &ddp, NULL);
		if (error) {
			return error;
		}

		error = namei_last_nofollow(path, ddp, &dp);
		if (ddp != NULL) {
			drele(ddp);
		}
		if (error) {
			return error;
		}
	} else {
		error = namei(path, &dp);
		if (error)
			return error;
	}

	if (dp->d_mount->m_flags & MNT_RDONLY) {
		error = EROFS;
	} else {
		error = vn_settimes(dp->d_vnode, timespec_times);
	}

	drele(dp);
	return error;
}

/*
 * Check the validity of members of a struct timespec
 */
static int is_timespec_valid(const struct timespec *time)
{
	return (time->tv_sec >= 0) &&
	   ((time->tv_nsec >= 0 && time->tv_nsec <= 999999999) ||
	    time->tv_nsec == UTIME_NOW ||
	    time->tv_nsec == UTIME_OMIT);
}

void init_timespec(struct timespec *_times, const struct timespec *times)
{
	if (times == NULL || times->tv_nsec == UTIME_NOW) {
		clock_gettime(CLOCK_REALTIME, _times);
	} else {
		_times->tv_sec = times->tv_sec;
		_times->tv_nsec = times->tv_nsec;
	}
	return;
}

int
sys_utimensat(int dirfd, const char *pathname, const struct timespec times[2], int flags)
{
	int error;
	char *ap;
	struct timespec timespec_times[2];
	extern struct task *main_task;
	struct dentry *dp;

	/* utimensat should return ENOENT when pathname is empty */
	if(pathname && pathname[0] == 0)
		return ENOENT;

	if (flags && !(flags & AT_SYMLINK_NOFOLLOW))
		return EINVAL;

	if (times && (!is_timespec_valid(&times[0]) || !is_timespec_valid(&times[1])))
		return EINVAL;

	init_timespec(&timespec_times[0], times ? times + 0 : NULL);
	init_timespec(&timespec_times[1], times ? times + 1 : NULL);

	if (pathname && pathname[0] == '/') {
		ap = strdup(pathname);
		if (!ap)
			return ENOMEM;

	} else if (dirfd == AT_FDCWD) {
		if (!pathname)
			return EFAULT;
		error = asprintf(&ap, "%s/%s", main_task->t_cwd, pathname);
		if (error || !ap)
			return ENOMEM;

	} else {
		struct vfscore_file *fp;

		fp = vfscore_get_file(dirfd);
		if (!fp)
			return EBADF;

		if (!fp->f_dentry)
			return EBADF;

		if (!(fp->f_dentry->d_vnode->v_type & VDIR))
			return ENOTDIR;

		if (pathname)
			error = asprintf(&ap, "%s/%s/%s", fp->f_dentry->d_mount->m_path,
					fp->f_dentry->d_path, pathname);
		else
			error = asprintf(&ap, "%s/%s", fp->f_dentry->d_mount->m_path,
					fp->f_dentry->d_path);
		if (error || !ap)
			return ENOMEM;
	}

	/* FIXME: Add support for AT_SYMLINK_NOFOLLOW */

	error = namei(ap, &dp);
	free(ap);

	if (error)
		return error;

	if (dp->d_mount->m_flags & MNT_RDONLY) {
		error = EROFS;
	} else {
		if (vn_access(dp->d_vnode, VWRITE)) {
			return EACCES;
		}
		if (times &&
			(times[0].tv_nsec != UTIME_NOW || times[1].tv_nsec != UTIME_NOW) &&
			(times[0].tv_nsec != UTIME_OMIT || times[1].tv_nsec != UTIME_OMIT) &&
			(!(dp->d_vnode->v_mode & ~VAPPEND)))
			return EPERM;
		error = vn_settimes(dp->d_vnode, timespec_times);
	}

	drele(dp);
	return error;
}

int
sys_futimens(int fd, const struct timespec times[2])
{
	int error;
	struct vfscore_file *fp;
	char *pathname;

	fp = vfscore_get_file(fd);
	if (!fp)
		return EBADF;

	if (!fp->f_dentry)
		return EBADF;

	pathname = fp->f_dentry->d_path;
	error = sys_utimensat(AT_FDCWD, pathname, times, 0);
	return error;
}

int
sys_fallocate(struct vfscore_file *fp, int mode, off_t offset, off_t len)
{
	int error;
	struct vnode *vp;

	DPRINTF(VFSDB_SYSCALL, ("sys_fallocate: fp=%p", fp));

	if (!fp->f_dentry || !(fp->f_flags & UK_FWRITE))
		return EBADF;

	if (offset < 0 || len <= 0) {
		return EINVAL;
	}

	// Strange, but that's what Linux returns.
	if ((mode & FALLOC_FL_PUNCH_HOLE) && !(mode & FALLOC_FL_KEEP_SIZE)) {
		return ENOTSUP;
	}

	vp = fp->f_dentry->d_vnode;
	vn_lock(vp);

	// NOTE: It's not detected here whether or not the device underlying
	// the fs is a block device. It's up to the fs itself tell us whether
	// or not fallocate is supported. See below:
	if (vp->v_type != VREG && vp->v_type != VDIR) {
		error = ENODEV;
		goto ret;
	}

	// EOPNOTSUPP here means that the underlying file system
	// referred by vp doesn't support fallocate.
	if (!vp->v_op->vop_fallocate) {
		error = EOPNOTSUPP;
		goto ret;
	}

	error = VOP_FALLOCATE(vp, mode, offset, len);
ret:
	vn_unlock(vp);
	return error;
}

int
sys_chmod(const char *path, mode_t mode)
{
	int error;
	struct dentry *dp;
	DPRINTF(VFSDB_SYSCALL, ("sys_chmod: path=%s\n", path));
	error = namei(path, &dp);
	if (error)
		return error;
	if (dp->d_mount->m_flags & MNT_RDONLY) {
		error = EROFS;
	} else {
		error = vn_setmode(dp->d_vnode, mode);
	}
	drele(dp);
	return error;
}

int
sys_fchmod(int fd, mode_t mode)
{
	struct vfscore_file *f = vfscore_get_file(fd);
	if (!f)
		return EBADF;
	// Posix is ambivalent on what fchmod() should do on an fd that does not
	// refer to a real file. It suggests an implementation may (but not must)
	// fail EINVAL on a pipe, can behave in an "unspecified" manner on a
	// socket, and for a STREAM, it must succeed and do nothing. Linux seems
	// to just do the last thing (do nothing and succeed).
	if (!f->f_dentry) {
		return 0;
	}
	if (f->f_dentry->d_mount->m_flags & MNT_RDONLY) {
		return EROFS;
	} else {
		return vn_setmode(f->f_dentry->d_vnode, mode);
	}
}
