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

#ifndef _VFS_H
#define _VFS_H

#define _GNU_SOURCE
#include <vfscore/mount.h>

#include <limits.h>
#include <fcntl.h>
#include <sys/statfs.h>
#include <sys/time.h>

/*
 * Tunable parameters
 */
#define FSMAXNAMES	16		/* max length of 'file system' name */

#ifdef DEBUG_VFS

extern int vfs_debug;

#define	VFSDB_CORE	0x00000001
#define	VFSDB_SYSCALL	0x00000002
#define	VFSDB_VNODE	0x00000004
#define	VFSDB_BIO	0x00000008
#define	VFSDB_CAP	0x00000010

#define VFSDB_FLAGS	0x00000013

#define	DPRINTF(_m,X)	if (vfs_debug & (_m)) uk_pr_debug X
#else
#define	DPRINTF(_m, X)
#endif

#define ASSERT(e)	assert(e)

/*
 * per task data
 */
struct task {
	char 	    t_cwd[PATH_MAX];	/* current working directory */
	struct vfscore_file *t_cwdfp;		/* directory for cwd */
};

int	 sys_open(char *path, int flags, mode_t mode, struct vfscore_file **fp);
int	 sys_read(struct vfscore_file *fp, const struct iovec *iov, size_t niov,
		off_t offset, size_t *count);
int	 sys_write(struct vfscore_file *fp, const struct iovec *iov, size_t niov,
		off_t offset, size_t *count);
int	 sys_lseek(struct vfscore_file *fp, off_t off, int type, off_t * cur_off);
int	 sys_ioctl(struct vfscore_file *fp, unsigned long request, void *buf);
int	 sys_fstat(struct vfscore_file *fp, struct stat *st);
int	 sys_fstatfs(struct vfscore_file *fp, struct statfs *buf);
int	 sys_fsync(struct vfscore_file *fp);
int	 sys_ftruncate(struct vfscore_file *fp, off_t length);

int	 sys_readdir(struct vfscore_file *fp, struct dirent *dirent);
int	 sys_rewinddir(struct vfscore_file *fp);
int	 sys_seekdir(struct vfscore_file *fp, long loc);
int	 sys_telldir(struct vfscore_file *fp, long *loc);
int	 sys_fchdir(struct vfscore_file *fp, char *path);

int	 sys_mkdir(char *path, mode_t mode);
int	 sys_rmdir(char *path);
int	 sys_mknod(char *path, mode_t mode);
int	 sys_rename(char *src, char *dest);
int	 sys_link(char *oldpath, char *newpath);
int	 sys_unlink(char *path);
int	 sys_symlink(const char *oldpath, const char *newpath);
int	 sys_access(char *path, int mode);
int	 sys_stat(char *path, struct stat *st);
int	 sys_lstat(char *path, struct stat *st);
int	 sys_statfs(char *path, struct statfs *buf);
int	 sys_truncate(char *path, off_t length);
int	 sys_readlink(char *path, char *buf, size_t bufsize, ssize_t *size);
int  sys_utimes(char *path, const struct timeval times[2], int flags);
int  sys_utimensat(int dirfd, const char *pathname,
				   const struct timespec times[2], int flags);
int  sys_futimens(int fd, const struct timespec times[2]);
int  sys_fallocate(struct vfscore_file *fp, int mode, loff_t offset, loff_t len);

int	 sys_pivot_root(const char *new_root, const char *old_put);
void	 sync(void);
int	 sys_chmod(const char *path, mode_t mode);
int	 sys_fchmod(int fd, mode_t mode);


int	 task_alloc(struct task **pt);
int	 task_conv(struct task *t, const char *path, int mode, char *full);
int	 path_conv(char *wd, const char *cpath, char *full);

//int	 sec_file_permission(task_t task, char *path, int mode);
int	 sec_vnode_permission(char *path);

int     namei(const char *path, struct dentry **dpp);
int	 namei_last_nofollow(char *path, struct dentry *ddp, struct dentry **dp);
int	 lookup(char *path, struct dentry **dpp, char **name);
void	 vnode_init(void);
void	 lookup_init(void);

int     vfs_findroot(const char *path, struct mount **mp, char **root);
int	 vfs_dname_copy(char *dest, const char *src, size_t size);

int	 fs_noop(void);

void dentry_init(void);

int vfs_close(struct vfscore_file *fp);
int vfs_read(struct vfscore_file *fp, struct uio *uio, int flags);
int vfs_write(struct vfscore_file *fp, struct uio *uio, int flags);
int vfs_ioctl(struct vfscore_file *fp, unsigned long com, void *data);
int vfs_stat(struct vfscore_file *fp, struct stat *st);

int fget(int fd, struct vfscore_file **out_fp);
int fdalloc(struct vfscore_file *fp, int *newfd);

#ifdef DEBUG_VFS
void	 vnode_dump(void);
void	 vfscore_mount_dump(void);
#endif





#endif /* !_VFS_H */
