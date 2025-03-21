/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Linux ABI-compatible VFS syscalls */

#define _GNU_SOURCE

#include <fcntl.h>
#include <limits.h>
#include <string.h>
#if UK_LIBC_SYSCALLS
#include <stdarg.h>
#endif /* UK_LIBC_SYSCALLS */

#include <uk/errptr.h>
#include <uk/posix-fdio.h>
#include <uk/posix-fdtab.h>
#include <uk/posix-vfs.h>
#include <uk/syscall.h>

/**
 * INTERNAL. Process the *at arguments `dfd` & `path`, opening the former if
 * required and returning the open file description in `ofp`.
 *
 * @return
 *  == 0: Success; `*ofp` is:
 *       - `NULL` for absolute paths & CWD-relative lookups
 *       - open file description otherwise
 *   < 0: Negative errno
 */
static inline int _vfs_atfd(int dfd, const char *path, struct uk_ofile **ofp)
{
	if (unlikely(!path))
		return -EFAULT;
	if (dfd == AT_FDCWD || path[0] == '/') {
		*ofp = NULL;
	} else {
		struct uk_ofile *of = uk_fdtab_get(dfd);

		if (unlikely(!of))
			return -EBADF;
		*ofp = of;
	}
	return 0;
}

/**
 * INTERNAL. Variant of `_vfs_atfd` that allows for NULL paths, in which case
 * `dfd` will always be openend and returned.
 */
static inline
int _vfs_atfd_nullpath(int dfd, const char *path, struct uk_ofile **ofp)
{
	if (path && (dfd == AT_FDCWD || path[0] == '/')) {
		*ofp = NULL;
	} else {
		struct uk_ofile *of = uk_fdtab_get(dfd);

		if (unlikely(!of))
			return -EBADF;
		*ofp = of;
	}
	return 0;
}

/* True if path is expected to be empty and is indeed so */
static inline int _vfs_emptypath(const char *path, int flags)
{
	return (flags & AT_EMPTY_PATH) && !*path;
}

/* FS ops */

UK_SYSCALL_R_DEFINE(int, sync)
{
	uk_sys_sync();
	return 0;
}

UK_SYSCALL_R_DEFINE(int, syncfs, int, fd)
{
	int ret;
	struct uk_ofile *of = uk_fdtab_get(fd);

	if (unlikely(!of))
		return -EBADF;
	ret = uk_sys_syncfs(of->file);
	uk_ofile_release(of);
	return ret;
}

UK_SYSCALL_R_DEFINE(int, statfs, const char *, path, struct statfs *, buf)
{
	return uk_sys_statfs(path, buf);
}

#if UK_LIBC_SYSCALLS
#ifdef statfs64
#undef statfs64
#endif

__alias(statfs, statfs64);
#endif /* UK_LIBC_SYSCALLS */

UK_SYSCALL_R_DEFINE(int, fstatfs, int, fd, struct statfs *, buf)
{
	int ret;
	struct uk_ofile *of = uk_fdtab_get(fd);

	if (unlikely(!of))
		return -EBADF;
	ret = uk_sys_fstatfs(of->file, buf);
	uk_ofile_release(of);
	return ret;
}

#if UK_LIBC_SYSCALLS
#ifdef fstatfs64
#undef fstatfs64
#endif

__alias(fstatfs, fstatfs64);
#endif /* UK_LIBC_SYSCALLS */

/* VFS context */

UK_SYSCALL_R_DEFINE(int, fchdir, int, fd)
{
	int ret;
	struct uk_ofile *of = uk_fdtab_get(fd);

	if (unlikely(!of))
		return -EBADF;
	ret = uk_sys_fchdir(of);
	uk_ofile_release(of);
	return ret;
}

UK_SYSCALL_R_DEFINE(int, chdir, const char *, path)
{
	return uk_sys_chdir(path);
}

UK_LLSYSCALL_R_DEFINE(ssize_t, getcwd, char *, path, size_t, len)
{
	return uk_sys_getcwd(path, len);
}

UK_SYSCALL_R_DEFINE(int, chroot, const char *, path)
{
	return uk_sys_chroot(path);
}

UK_SYSCALL_R_DEFINE(mode_t, umask, mode_t, mode)
{
	return uk_sys_umask(mode);
}

/* Read node metadata */

/**
 * INTERNAL. Common fd-consuming implementation of faccessat*.
 */
static int _vfs_faccessat(int dfd, const char *path, int mode, int flags)
{
	struct uk_ofile *of;
	int ret = _vfs_atfd(dfd, path, &of);

	if (unlikely(ret))
		return ret;
	ret = uk_sys_faccessat(of ? of->file : NULL, path, mode, flags);
	if (of)
		uk_ofile_release(of);
	return ret;
}

UK_SYSCALL_R_DEFINE(int, faccessat2, int, dfd, const char *, path,
		    int, mode, int, flags)
{
	return _vfs_faccessat(dfd, path, mode, flags);
}

UK_SYSCALL_R_DEFINE(int, faccessat, int, dfd, const char *, path, int, mode)
{
	return _vfs_faccessat(dfd, path, mode, 0);
}

UK_SYSCALL_R_DEFINE(int, access, const char *, path, int, mode)
{
	return uk_sys_access(path, mode);
}

UK_SYSCALL_R_DEFINE(int, statx, int, dfd, const char *, path,
		    unsigned int, flags, unsigned int, mask,
		    void *, buf)
{
	struct uk_ofile *of;
	int ret = _vfs_atfd(dfd, path, &of);

	if (unlikely(ret))
		return ret;
	if (of && _vfs_emptypath(path, flags))
		/* Shortcut to fstatx */
		ret = uk_sys_fstatx(of, mask, (struct uk_statx *)buf);
	else
		ret = uk_sys_statx(of ? of->file : NULL, path, flags, mask,
				   (struct uk_statx *)buf);
	if (of)
		uk_ofile_release(of);
	return ret;
}

UK_SYSCALL_R_DEFINE(int, newfstatat, int, dfd, const char *, path,
		    struct stat *, st, int, flags)
{
	struct uk_ofile *of;
	int ret = _vfs_atfd(dfd, path, &of);

	if (unlikely(ret))
		return ret;
	if (of && _vfs_emptypath(path, flags))
		/* Shortcut to fstat */
		ret = uk_sys_fstat(of, st);
	else
		ret = uk_sys_fstatat(of ? of->file : NULL, path, st, flags);
	if (of)
		uk_ofile_release(of);
	return ret;
}

#if UK_LIBC_SYSCALLS
#ifdef fstatat64
#undef fstatat64
#endif
#ifdef fstatat
#undef fstatat
#endif

__alias(newfstatat, fstatat);
__alias(newfstatat, fstatat64);
#endif /* UK_LIBC_SYSCALLS */

UK_SYSCALL_R_DEFINE(int, stat, const char *, path, struct stat *, st)
{
	return uk_sys_stat(path, st);
}

#if UK_LIBC_SYSCALLS
#ifdef stat64
#undef stat64
#endif

__alias(stat, stat64);
#endif /* UK_LIBC_SYSCALLS */

UK_SYSCALL_R_DEFINE(int, lstat, const char *, path, struct stat *, st)
{
	return uk_sys_lstat(path, st);
}

#if UK_LIBC_SYSCALLS
#ifdef lstat64
#undef lstat64
#endif

__alias(lstat, lstat64);
#endif /* UK_LIBC_SYSCALLS */

/* Change node metadata */

UK_SYSCALL_R_DEFINE(int, fchownat, int, dfd, const char *, path, uid_t, owner,
		    gid_t, group, int, flags)
{
	struct uk_ofile *of;
	int ret = _vfs_atfd(dfd, path, &of);

	if (unlikely(ret))
		return ret;
	if (of && _vfs_emptypath(path, flags))
		/* Shortcut to fchown */
		ret = uk_sys_fchown(of, owner, group);
	else
		ret = uk_sys_fchownat(of ? of->file : NULL, path,
				      owner, group, flags);
	if (of)
		uk_ofile_release(of);
	return ret;
}

UK_SYSCALL_R_DEFINE(int, chown, const char *, path, uid_t, owner, gid_t, group)
{
	return uk_sys_chown(path, owner, group);
}

UK_SYSCALL_R_DEFINE(int, lchown, const char *, path, uid_t, owner, gid_t, group)
{
	return uk_sys_lchown(path, owner, group);
}

/**
 * INTERNAL. Common fd-consuming implementation of fchmodat*.
 */
static int _vfs_fchmodat(int dfd, const char *path, mode_t mode, int flags)
{
	struct uk_ofile *of;
	int ret = _vfs_atfd(dfd, path, &of);

	if (unlikely(ret))
		return ret;
	if (of && _vfs_emptypath(path, flags))
		/* Shortcut to fchmod */
		ret = uk_sys_fchmod(of, mode);
	else
		ret = uk_sys_fchmodat(of ? of->file : NULL, path, mode, flags);
	if (of)
		uk_ofile_release(of);
	return ret;
}

UK_SYSCALL_R_DEFINE(int, fchmodat2, int, dfd, const char *, path, mode_t, mode,
		    int, flags)
{
	return _vfs_fchmodat(dfd, path, mode, flags);
}

#if UK_LIBC_SYSCALLS
__alias(fchmodat2, fchmodat);
#endif /* UK_LIBC_SYSCALLS */

UK_LLSYSCALL_R_DEFINE(int, fchmodat, int, dfd, const char *, path, mode_t, mode)
{
	return _vfs_fchmodat(dfd, path, mode, 0);
}

UK_SYSCALL_R_DEFINE(int, chmod, const char *, path, mode_t, mode)
{
	return uk_sys_chmod(path, mode);
}

UK_SYSCALL_R_DEFINE(int, futimesat, int, dfd, const char *, path,
		    const struct timeval *, times)
{
	struct uk_ofile *of;
	int ret = _vfs_atfd(dfd, path, &of);

	if (unlikely(ret))
		return ret;
	ret = uk_sys_futimesat(of ? of->file : NULL, path, times);
	if (of)
		uk_ofile_release(of);
	return ret;
}

UK_SYSCALL_R_DEFINE(int, utimes, const char *, path,
		    const struct timeval *, times)
{
	return uk_sys_utimes(path, times);
}

UK_SYSCALL_R_DEFINE(int, utime, const char *, path,
		    const struct utimbuf *, times)
{
	return uk_sys_utime(path, times);
}

UK_SYSCALL_R_DEFINE(int, utimensat, int, dfd, const char *, path,
		    const struct timespec *, times, int, flags)
{
	int ret;
	struct uk_ofile *of;

	/* utimensat uniquely supports null paths as empty */
	ret = _vfs_atfd_nullpath(dfd, path, &of);
	if (unlikely(ret))
		return ret;
	if (of && (!path || _vfs_emptypath(path, flags)))
		/* Shortcut to futimens */
		ret = uk_sys_futimens(of, times);
	else
		ret = uk_sys_utimensat(of ? of->file : NULL, path,
				       times, flags);
	if (of)
		uk_ofile_release(of);
	return ret;
}

/* Read node contents */

UK_SYSCALL_R_DEFINE(ssize_t, readlinkat, int, dfd, const char *, path,
		    char *, buf, size_t, bufsz)
{
	struct uk_ofile *of;
	int ret = _vfs_atfd(dfd, path, &of);

	if (unlikely(ret))
		return ret;
	ret = uk_sys_readlinkat(of ? of->file : NULL, path, buf, bufsz);
	if (of)
		uk_ofile_release(of);
	return ret;
}

UK_SYSCALL_R_DEFINE(ssize_t, readlink, const char *, path, char *, buf,
		    size_t, bufsz)
{
	return uk_sys_readlink(path, buf, bufsz);
}

UK_SYSCALL_R_DEFINE(ssize_t, getdents64, int, fd, void *, dirp, size_t, count)
{
	int ret;
	struct uk_ofile *of = uk_fdtab_get(fd);

	if (unlikely(!of))
		return -EBADF;
	if (unlikely(!uk_fs_isdir(of->file)))
		return -ENOTDIR;
	/* Directories may not have the NOSEEK flag set */
	UK_ASSERT(!(of->mode & UKFD_O_NOSEEK));

	uk_mutex_lock(&of->lock);
	ret = uk_sys_getdents64(of->file, &of->pos, dirp, count);
	uk_mutex_unlock(&of->lock);
	uk_ofile_release(of);
	return ret;
}

UK_SYSCALL_R_DEFINE(long, getdents, int, fd, void *, dirp, size_t, count)
{
	/* TODO: call getdents64 & convert output */
	return -ENOSYS;
}

/* Change node contents */

UK_SYSCALL_R_DEFINE(int, truncate, const char *, path, off_t, len)
{
	return uk_sys_truncate(path, len);
}

#if UK_LIBC_SYSCALLS
#ifdef truncate64
#undef truncate64
#endif

__alias(truncate, truncate64);
#endif /* UK_LIBC_SYSCALLS */

/* Create node w/o opening */

UK_SYSCALL_R_DEFINE(int, mkdirat, int, dfd, const char *, path, mode_t, mode)
{
	struct uk_ofile *of;
	int ret = _vfs_atfd(dfd, path, &of);

	if (unlikely(ret))
		return ret;
	ret = uk_sys_mkdirat(of ? of->file : NULL, path, mode);
	if (of)
		uk_ofile_release(of);
	return ret;
}

UK_SYSCALL_R_DEFINE(int, mkdir, const char *, path, mode_t, mode)
{
	return uk_sys_mkdir(path, mode);
}

UK_SYSCALL_R_DEFINE(int, linkat, int, olddfd, const char *, oldname,
		    int, newdfd, const char *, newname, int, flags)
{
	int ret;
	struct uk_ofile *oldof;
	struct uk_ofile *newof;

	if (unlikely((ret = _vfs_atfd(olddfd, oldname, &oldof))))
		return ret;
	if (unlikely((ret = _vfs_atfd(newdfd, newname, &newof))))
		goto out_oldof;
	ret = uk_sys_linkat(oldof ? oldof->file : NULL, oldname,
			    newof ? newof->file : NULL, newname, flags);
	if (newof)
		uk_ofile_release(newof);
out_oldof:
	if (oldof)
		uk_ofile_release(oldof);
	return ret;
}

UK_SYSCALL_R_DEFINE(int, link, const char *, oldname, const char *, newname)
{
	return uk_sys_link(oldname, newname);
}

UK_SYSCALL_R_DEFINE(int, symlinkat, const char *, path, int, dfd,
		    const char *, linkname)
{
	struct uk_ofile *of;
	int ret = _vfs_atfd(dfd, path, &of);

	if (unlikely(ret))
		return ret;
	ret = uk_sys_symlinkat(path, of ? of->file : NULL, linkname);
	if (of)
		uk_ofile_release(of);
	return ret;
}

UK_SYSCALL_R_DEFINE(int, symlink, const char *, path, const char *, linkpath)
{
	return uk_sys_symlink(path, linkpath);
}

UK_SYSCALL_R_DEFINE(int, mknodat, int, dfd, const char *, path, mode_t, mode,
		    dev_t, dev)
{
	struct uk_ofile *of;
	int ret = _vfs_atfd(dfd, path, &of);

	if (unlikely(ret))
		return ret;
	ret = uk_sys_mknodat(of ? of->file : NULL, path, mode, dev);
	if (of)
		uk_ofile_release(of);
	return ret;
}

UK_SYSCALL_R_DEFINE(int, mknod, const char *, path, mode_t, mode,
		    dev_t, dev)
{
	return uk_sys_mknod(path, mode, dev);
}

/* Remove/rename node */

UK_SYSCALL_R_DEFINE(int, unlinkat, int, dfd, const char *, path, int, flags)
{
	struct uk_ofile *of;
	int ret = _vfs_atfd(dfd, path, &of);

	if (unlikely(ret))
		return ret;
	ret = uk_sys_unlinkat(of ? of->file : NULL, path, flags);
	if (of)
		uk_ofile_release(of);
	return ret;
}

UK_SYSCALL_R_DEFINE(int, unlink, const char *, path)
{
	return uk_sys_unlink(path);
}

UK_SYSCALL_R_DEFINE(int, rmdir, const char *, path)
{
	return uk_sys_rmdir(path);
}

/**
 * INTERNAL. Common fd-consuming implementation of renameat*.
 */
static int _vfs_renameat(int olddfd, const char *oldname,
			 int newdfd, const char *newname, unsigned int flags)
{
	int ret;
	struct uk_ofile *oldof;
	struct uk_ofile *newof;

	if (unlikely((ret = _vfs_atfd(olddfd, oldname, &oldof))))
		return ret;
	if (unlikely((ret = _vfs_atfd(newdfd, newname, &newof))))
		goto out_oldof;
	ret = uk_sys_renameat(oldof ? oldof->file : NULL, oldname,
			      newof ? newof->file : NULL, newname, flags);
	if (newof)
		uk_ofile_release(newof);
out_oldof:
	if (oldof)
		uk_ofile_release(oldof);
	return ret;
}

UK_SYSCALL_R_DEFINE(int, renameat2, int, olddfd, const char *, oldname,
		    int, newdfd, const char *, newname, unsigned int, flags)
{
	return _vfs_renameat(olddfd, oldname, newdfd, newname, flags);
}

UK_SYSCALL_R_DEFINE(int, renameat, int, olddfd, const char *, oldname,
		    int, newdfd, const char *, newname)
{
	return _vfs_renameat(olddfd, oldname, newdfd, newname, 0);
}

UK_SYSCALL_R_DEFINE(int, rename, const char *, oldpath, const char *, newpath)
{
	return uk_sys_rename(oldpath, newpath);
}

/* Mounting */

UK_SYSCALL_R_DEFINE(int, mount, const char *, devname, const char *, dirname,
		    const char *, type, unsigned long, flags, void *, data)
{
	return uk_sys_mount(devname, dirname, type, flags, data);
}

UK_SYSCALL_R_DEFINE(int, umount2, const char *, path, int, flags)
{
	return uk_sys_umount(path, flags);
}

#if UK_LIBC_SYSCALLS
int umount(const char *path)
{
	return umount2(path, 0);
}
#endif /* UK_LIBC_SYSCALLS */

/* Open */

UK_LLSYSCALL_R_DEFINE(int, openat, int, dfd, const char *, path,
		      int, flags, mode_t, mode)
{
	struct uk_ofile *newf;
	struct uk_ofile *of;
	int ret;

	ret = _vfs_atfd(dfd, path, &of);
	if (unlikely(ret))
		return ret;

	newf = uk_sys_openat(of, path, flags, mode);
	if (unlikely(PTRISERR(newf))) {
		ret = PTR2ERR(newf);
	} else {
		ret = uk_fdtab_open_desc(newf, flags);
		if (unlikely(ret < 0))
			uk_ofile_release(newf);
	}
	if (of)
		uk_ofile_release(of);
	return ret;
}

#if UK_LIBC_SYSCALLS
int openat(int dirfd, const char *pathname, int flags, ...)
{
	mode_t mode = 0;

	if (flags & O_CREAT) {
		va_list ap;

		va_start(ap, flags);
		mode = va_arg(ap, mode_t);
		va_end(ap);
	}

	return uk_syscall_e_openat(dirfd, (long)pathname, flags, mode);
}

#ifdef openat64
#undef openat64
#endif

__alias(openat, openat64);
#endif /* UK_LIBC_SYSCALLS */

#include <string.h>

UK_LLSYSCALL_R_DEFINE(int, open, const char *, path, int, flags, mode_t, mode)
{
	struct uk_ofile *newf = uk_sys_open(path, flags, mode);
	int fd;

	if (unlikely(PTRISERR(newf)))
		return PTR2ERR(newf);
	fd = uk_fdtab_open_desc(newf, flags);
	if (unlikely(fd < 0))
		uk_ofile_release(newf);
	return fd;
}

#if UK_LIBC_SYSCALLS
int open(const char *pathname, int flags, ...)
{
	mode_t mode = 0;

	if (flags & O_CREAT) {
		va_list ap;

		va_start(ap, flags);
		mode = va_arg(ap, mode_t);
		va_end(ap);
	}

	return uk_syscall_e_open((long)pathname, flags, mode);
}

#ifdef open64
#undef open64
#endif

__alias(open, open64);
#endif /* UK_LIBC_SYSCALLS */

UK_SYSCALL_R_DEFINE(int, creat, const char *, path, mode_t, mode)
{
	struct uk_ofile *newf = uk_sys_creat(path, mode);
	int fd;

	if (unlikely(PTRISERR(newf)))
		return PTR2ERR(newf);
	fd = uk_fdtab_open_desc(newf, O_CREAT | O_WRONLY | O_TRUNC);
	if (unlikely(fd < 0))
		uk_ofile_release(newf);
	return fd;
}

#if UK_LIBC_SYSCALLS
#ifdef creat64
#undef creat64
#endif

__alias(creat, creat64);
#endif /* UK_LIBC_SYSCALLS */
