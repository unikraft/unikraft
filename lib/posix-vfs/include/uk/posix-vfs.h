/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Unikraft POSIX-compatible virtual file system interface */

#ifndef __UK_POSIX_VFS_H__
#define __UK_POSIX_VFS_H__

#include <fcntl.h>
#include <time.h>
#include <utime.h>
#include <sys/stat.h>

#include <uk/fs.h>
#include <uk/posix-fd.h>

/*
 * Internal syscalls
 */

/* FS ops */
void uk_sys_sync(void);
int uk_sys_statfs(const char *path, struct statfs *buf);

/* VFS context */
int uk_sys_fchdir(struct uk_ofile *of);
int uk_sys_chdir(const char *path);
ssize_t uk_sys_getcwd(char *buf, size_t len);
int uk_sys_chroot(const char *path);
mode_t uk_sys_umask(mode_t mask);

/* Read node metadata */
int uk_sys_faccessat(const struct uk_file *f, const char *path,
		     int mode, int flags);
int uk_sys_statx(const struct uk_file *f, const char *restrict path, int flags,
		 unsigned int mask, struct uk_statx *restrict statxbuf);
int uk_sys_fstatat(const struct uk_file *f, const char *restrict path,
		   struct stat *restrict statbuf, int flags);

/* Change node metadata */
int uk_sys_fchownat(const struct uk_file *f, const char *path,
		    uid_t owner, gid_t group, int flags);
int uk_sys_fchmodat(const struct uk_file *f, const char *path,
		    mode_t mode, int flags);
int uk_sys_futimesat(const struct uk_file *f, const char *path,
		     const struct timeval *times);
int uk_sys_utime(const char *path, const struct utimbuf *times);
int uk_sys_utimensat(const struct uk_file *f, const char *path,
		     const struct timespec *times, int flags);

/* Read node contents */
ssize_t uk_sys_getdents64(const struct uk_file *f, size_t *curp,
			  void *dirp, size_t count);
ssize_t uk_sys_readlinkat(const struct uk_file *f, const char *restrict path,
			  char *restrict buf, size_t bufsz);

/* Change node contents */
int uk_sys_truncate(const char *path, off_t len);

/* Create node w/o opening */
int uk_sys_mkdirat(const struct uk_file *f, const char *path, mode_t mode);
int uk_sys_linkat(const struct uk_file *olddir, const char *oldpath,
		  const struct uk_file *newdir, const char *newpath, int flags);
int uk_sys_symlinkat(const char *target, const struct uk_file *f,
		     const char *linkpath);
int uk_sys_mknodat(const struct uk_file *f, const char *path,
		   mode_t mode, dev_t dev);

/* Remove/rename node */
int uk_sys_unlinkat(const struct uk_file *f, const char *path, int flags);
int uk_sys_renameat(const struct uk_file *olddir, const char *oldpath,
		    const struct uk_file *newdir, const char *newpath,
		    int flags);

/* Mounting */
int uk_sys_mount(const char *source, const char *target, const char *fstype,
		 unsigned long flags, void *data);
int uk_sys_umount(const char *target, int flags);

/* Open */
struct uk_ofile *uk_sys_openat(struct uk_ofile *of, const char *path,
			       int flags, mode_t mode);

/*
 * Inlines implemented as calls to above
 */

/* FS ops */
static inline
int uk_sys_syncfs(const struct uk_file *f)
{
	return uk_fs_isfs(f) ? uk_fs_sync(f) : 0;
}

static inline
int uk_sys_fstatfs(const struct uk_file *f, struct statfs *buf)
{
	return uk_fs_isfs(f) ? uk_fs_stat(f, buf) : -EBADF;
}

/* Read node metadata */
static inline
int uk_sys_access(const char *path, int mode)
{
	return uk_sys_faccessat(NULL, path, mode, 0);
}

static inline
int uk_sys_stat(const char *restrict path, struct stat *restrict statbuf)
{
	return uk_sys_fstatat(NULL, path, statbuf, 0);
}

static inline
int uk_sys_lstat(const char *restrict path, struct stat *restrict statbuf)
{
	return uk_sys_fstatat(NULL, path, statbuf, AT_SYMLINK_NOFOLLOW);
}

/* Change node metadata */
static inline
int uk_sys_chown(const char *path, uid_t owner, gid_t group)
{
	return uk_sys_fchownat(NULL, path, owner, group, 0);
}

static inline
int uk_sys_lchown(const char *path, uid_t owner, gid_t group)
{
	return uk_sys_fchownat(NULL, path, owner, group, AT_SYMLINK_NOFOLLOW);
}

static inline
int uk_sys_chmod(const char *path, mode_t mode)
{
	return uk_sys_fchmodat(NULL, path, mode, 0);
}

static inline
int uk_sys_utimes(const char *path, const struct timeval *times)
{
	return uk_sys_futimesat(NULL, path, times);
}

/* Read node contents */
static inline
ssize_t uk_sys_readlink(const char *restrict path,
			char *restrict buf, size_t bufsz)
{
	return uk_sys_readlinkat(NULL, path, buf, bufsz);
}

/* Create node w/o opening */
static inline
int uk_sys_mkdir(const char *path, mode_t mode)
{
	return uk_sys_mkdirat(NULL, path, mode);
}

static inline
int uk_sys_link(const char *oldpath, const char *newpath)
{
	return uk_sys_linkat(NULL, oldpath, NULL, newpath, 0);
}

static inline
int uk_sys_symlink(const char *target, const char *linkpath)
{
	return uk_sys_symlinkat(target, NULL, linkpath);
}

static inline
int uk_sys_mknod(const char *path, mode_t mode, dev_t dev)
{
	return uk_sys_mknodat(NULL, path, mode, dev);
}

/* Remove/rename node */
static inline
int uk_sys_unlink(const char *path)
{
	return uk_sys_unlinkat(NULL, path, 0);
}

static inline
int uk_sys_rmdir(const char *path)
{
	return uk_sys_unlinkat(NULL, path, AT_REMOVEDIR);
}

static inline
int uk_sys_rename(const char *oldpath, const char *newpath)
{
	return uk_sys_renameat(NULL, oldpath, NULL, newpath, 0);
}

/* Open */
static inline
struct uk_ofile *uk_sys_open(const char *path, int flags, mode_t mode)
{
	return uk_sys_openat(NULL, path, flags, mode);
}

static inline
struct uk_ofile *uk_sys_creat(const char *path, mode_t mode)
{
	return uk_sys_open(path, O_CREAT | O_WRONLY | O_TRUNC, mode);
}

#endif /* __UK_POSIX_VFS_H__ */
