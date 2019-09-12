/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2005-2007, Kohsuke Ohtani
 * Copyright (C) 2013 Cloudius Systems, Ltd.
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

#define _GNU_SOURCE

#include <sys/statvfs.h>
#include <sys/stat.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <vfscore/prex.h>
#include <vfscore/vnode.h>
#include "vfs.h"
#include <sys/file.h>
#include <stdarg.h>
#include <vfscore/file.h>
#include <vfscore/mount.h>
#include <vfscore/fs.h>
#include <uk/errptr.h>
#include <uk/ctors.h>
#include <uk/trace.h>
#include <uk/syscall.h>

#ifdef DEBUG_VFS
int	vfs_debug = VFSDB_FLAGS;
#endif

/* This macro is for defining an alias of the 64bit version of a
 * syscall to the regular one. It seams we can make the logic which is
 * choosing the right call simpler then in common libc.
 *
 * Let's keep LFS64 calls just in case in future we will find out that
 * these aliases are need.
 */
#define LFS64(x)

static mode_t global_umask = S_IWGRP | S_IWOTH;

/* TODO: these macro does not belong here
 * NOTE: borrowed from OSv
 */
#define DO_ONCE(thing) do {				\
	static int _x;					\
	if (!_x) {					\
	    _x = 1;					\
	    thing ;					\
	}						\
} while (0)
#define WARN_STUBBED() DO_ONCE(uk_pr_warn("%s() stubbed\n", __func__))

static inline int libc_error(int err)
{
    errno = err;
    return -1;
}

static inline mode_t apply_umask(mode_t mode)
{
	return mode & ~ukarch_load_n(&global_umask);
}

UK_TRACEPOINT(trace_vfs_open, "\"%s\" 0x%x 0%0o", const char*, int, mode_t);
UK_TRACEPOINT(trace_vfs_open_ret, "%d", int);
UK_TRACEPOINT(trace_vfs_open_err, "%d", int);

struct task *main_task;	/* we only have a single process */

int open(const char *pathname, int flags, ...)
{
	mode_t mode = 0;
	if (flags & O_CREAT) {
		va_list ap;
		va_start(ap, flags);
		mode = apply_umask(va_arg(ap, mode_t));
		va_end(ap);
	}

	trace_vfs_open(pathname, flags, mode);

	struct task *t = main_task;
	char path[PATH_MAX];
	struct vfscore_file *fp;
	int fd, error;
	int acc;

	acc = 0;
	switch (flags & O_ACCMODE) {
	case O_RDONLY:
		acc = VREAD;
		break;
	case O_WRONLY:
		acc = VWRITE;
		break;
	case O_RDWR:
		acc = VREAD | VWRITE;
		break;
	}

	error = task_conv(t, pathname, acc, path);
	if (error)
		goto out_errno;

	error = sys_open(path, flags, mode, &fp);
	if (error)
		goto out_errno;

	error = fdalloc(fp, &fd);
	if (error)
		goto out_fput;
	fdrop(fp);
	trace_vfs_open_ret(fd);
	return fd;

	out_fput:
	fdrop(fp);
	out_errno:
	errno = error;
	trace_vfs_open_err(error);
	return -1;
}

LFS64(open);

int openat(int dirfd, const char *pathname, int flags, ...)
{
	mode_t mode = 0;
	if (flags & O_CREAT) {
		va_list ap;
		va_start(ap, flags);
		mode = apply_umask(va_arg(ap, mode_t));
		va_end(ap);
	}

	if (pathname[0] == '/' || dirfd == AT_FDCWD) {
		return open(pathname, flags, mode);
	}

	struct vfscore_file *fp;
	int error = fget(dirfd, &fp);
	if (error) {
		errno = error;
		return -1;
	}

	struct vnode *vp = fp->f_dentry->d_vnode;
	vn_lock(vp);

	char p[PATH_MAX];

	/* build absolute path */
	strlcpy(p, fp->f_dentry->d_mount->m_path, PATH_MAX);
	strlcat(p, fp->f_dentry->d_path, PATH_MAX);
	strlcat(p, "/", PATH_MAX);
	strlcat(p, pathname, PATH_MAX);

	error = open(p, flags, mode);

	vn_unlock(vp);
	fdrop(fp);

	return error;
}
LFS64(openat);

int creat(const char *pathname, mode_t mode)
{
	return open(pathname, O_CREAT|O_WRONLY|O_TRUNC, mode);
}
LFS64(creat);

UK_TRACEPOINT(trace_vfs_close, "%d", int);
UK_TRACEPOINT(trace_vfs_close_ret, "");
UK_TRACEPOINT(trace_vfs_close_err, "%d", int);

int fdclose(int fd)
{
	struct vfscore_file *fp;
	int error;

	fp = vfscore_get_file(fd);
	if (!fp)
		return EBADF;

	error = vfscore_put_fd(fd);
	if (!error)
		fdrop(fp);

	return error;
}

int close(int fd)
{
	int error;

	trace_vfs_close(fd);
	error = fdclose(fd);
	if (error)
		goto out_errno;

	trace_vfs_close_ret();
	return 0;

	out_errno:
	trace_vfs_close_err(error);
	errno = error;
	return -1;
}

UK_TRACEPOINT(trace_vfs_mknod, "\"%s\" 0%0o 0x%x", const char*, mode_t, dev_t);
UK_TRACEPOINT(trace_vfs_mknod_ret, "");
UK_TRACEPOINT(trace_vfs_mknod_err, "%d", int);

int __xmknod(int ver, const char *pathname, mode_t mode, dev_t *dev __unused)
{
	UK_ASSERT(ver == 0); // On x86-64 Linux, _MKNOD_VER_LINUX is 0.
	struct task *t = main_task;
	char path[PATH_MAX];
	int error;

	trace_vfs_mknod(pathname, mode, *dev);
	if ((error = task_conv(t, pathname, VWRITE, path)) != 0)
		goto out_errno;

	error = sys_mknod(path, mode);
	if (error)
		goto out_errno;

	trace_vfs_mknod_ret();
	return 0;

	out_errno:
	trace_vfs_mknod_err(error);
	errno = error;
	return -1;
}

int mknod(const char *pathname, mode_t mode, dev_t dev)
{
	return __xmknod(0, pathname, mode, &dev);
}


UK_TRACEPOINT(trace_vfs_lseek, "%d 0x%x %d", int, off_t, int);
UK_TRACEPOINT(trace_vfs_lseek_ret, "0x%x", off_t);
UK_TRACEPOINT(trace_vfs_lseek_err, "%d", int);

off_t lseek(int fd, off_t offset, int whence)
{
	struct vfscore_file *fp;
	off_t org;
	int error;

	trace_vfs_lseek(fd, offset, whence);
	error = fget(fd, &fp);
	if (error)
		goto out_errno;

	error = sys_lseek(fp, offset, whence, &org);
	fdrop(fp);

	if (error)
		goto out_errno;
	trace_vfs_lseek_ret(org);
	return org;

	out_errno:
	trace_vfs_lseek_err(error);
	errno = error;
	return -1;
}

LFS64(lseek);

UK_TRACEPOINT(trace_vfs_pread, "%d %p 0x%x 0x%x", int, void*, size_t, off_t);
UK_TRACEPOINT(trace_vfs_pread_ret, "0x%x", ssize_t);
UK_TRACEPOINT(trace_vfs_pread_err, "%d", int);

// In BSD's internal implementation of read() and write() code, for example
// sosend_generic(), a partial read or write returns both an EWOULDBLOCK error
// *and* a non-zero number of written bytes. In that case, we need to zero the
// error, so the system call appear a successful partial read/write.
// In FreeBSD, dofilewrite() and dofileread() (sys_generic.c) do this too.
static inline int has_error(int error, int bytes)
{
	/* TODO: OSv checks also for ERESTART */
	return error && (
			(bytes == 0) ||
			(error != EWOULDBLOCK && error != EINTR));
}


ssize_t pread(int fd, void *buf, size_t count, off_t offset)
{
	trace_vfs_pread(fd, buf, count, offset);
	struct iovec iov = {
			.iov_base	= buf,
			.iov_len	= count,
	};
	struct vfscore_file *fp;
	size_t bytes;
	int error;

	error = fget(fd, &fp);
	if (error)
		goto out_errno;

	error = sys_read(fp, &iov, 1, offset, &bytes);
	fdrop(fp);

	if (has_error(error, bytes))
		goto out_errno;
	trace_vfs_pread_ret(bytes);
	return bytes;

	out_errno:
	trace_vfs_pread_err(error);
	errno = error;
	return -1;
}

LFS64(pread);

ssize_t read(int fd, void *buf, size_t count)
{
	return pread(fd, buf, count, -1);
}

UK_TRACEPOINT(trace_vfs_pwrite, "%d %p 0x%x 0x%x", int, const void*, size_t,
	      off_t);
UK_TRACEPOINT(trace_vfs_pwrite_ret, "0x%x", ssize_t);
UK_TRACEPOINT(trace_vfs_pwrite_err, "%d", int);

ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset)
{
	trace_vfs_pwrite(fd, buf, count, offset);
	struct iovec iov = {
			.iov_base	= (void *)buf,
			.iov_len	= count,
	};
	struct vfscore_file *fp;
	size_t bytes;
	int error;

	error = fget(fd, &fp);
	if (error)
		goto out_errno;

	error = sys_write(fp, &iov, 1, offset, &bytes);
	fdrop(fp);

	if (has_error(error, bytes))
		goto out_errno;
	trace_vfs_pwrite_ret(bytes);
	return bytes;

	out_errno:
	trace_vfs_pwrite_err(error);
	errno = error;
	return -1;
}

LFS64(pwrite);

ssize_t write(int fd, const void *buf, size_t count)
{
	return pwrite(fd, buf, count, -1);
}

ssize_t preadv(int fd, const struct iovec *iov, int iovcnt, off_t offset)
{
	struct vfscore_file *fp;
	size_t bytes;
	int error;

	error = fget(fd, &fp);
	if (error)
		goto out_errno;

	error = sys_read(fp, iov, iovcnt, offset, &bytes);
	fdrop(fp);

	if (has_error(error, bytes))
		goto out_errno;
	return bytes;

	out_errno:
	errno = error;
	return -1;
}

LFS64(preadv);

ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
{
	return preadv(fd, iov, iovcnt, -1);
}

UK_TRACEPOINT(trace_vfs_pwritev, "%d %p 0x%x 0x%x", int, const struct iovec*,
	      int, off_t);
UK_TRACEPOINT(trace_vfs_pwritev_ret, "0x%x", ssize_t);
UK_TRACEPOINT(trace_vfs_pwritev_err, "%d", int);

ssize_t pwritev(int fd, const struct iovec *iov, int iovcnt, off_t offset)
{
	struct vfscore_file *fp;
	size_t bytes;
	int error;

	trace_vfs_pwritev(fd, iov, iovcnt, offset);
	error = fget(fd, &fp);
	if (error)
		goto out_errno;

	error = sys_write(fp, iov, iovcnt, offset, &bytes);
	fdrop(fp);

	if (has_error(error, bytes))
		goto out_errno;
	trace_vfs_pwritev_ret(bytes);
	return bytes;

	out_errno:
	trace_vfs_pwritev_err(error);
	errno = error;
	return -1;
}
LFS64(pwritev);

ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
{
	return pwritev(fd, iov, iovcnt, -1);
}

UK_SYSCALL_DEFINE(writev, unsigned long, fd, const struct iovec *, vec,
		  unsigned long, vlen)
{
	return pwritev(fd, vec, vlen, -1);
}

UK_TRACEPOINT(trace_vfs_ioctl, "%d 0x%x", int, unsigned long);
UK_TRACEPOINT(trace_vfs_ioctl_ret, "");
UK_TRACEPOINT(trace_vfs_ioctl_err, "%d", int);

int ioctl(int fd, unsigned long int request, ...)
{
	struct vfscore_file *fp;
	int error;
	va_list ap;
	void* arg;

	trace_vfs_ioctl(fd, request);
	/* glibc ABI provides a variadic prototype for ioctl so we need to agree
	 * with it, since we now include sys/ioctl.h
	 * read the first argument and pass it to sys_ioctl() */
	va_start(ap, request);
	arg = va_arg(ap, void*);
	va_end(ap);

	error = fget(fd, &fp);
	if (error)
		goto out_errno;

	error = sys_ioctl(fp, request, arg);
	fdrop(fp);

	if (error)
		goto out_errno;
	trace_vfs_ioctl_ret();
	return 0;

	out_errno:
	trace_vfs_ioctl_err(error);
	errno = error;
	return -1;
}

UK_TRACEPOINT(trace_vfs_fsync, "%d", int);
UK_TRACEPOINT(trace_vfs_fsync_ret, "");
UK_TRACEPOINT(trace_vfs_fsync_err, "%d", int);

int fsync(int fd)
{
	struct vfscore_file *fp;
	int error;

	trace_vfs_fsync(fd);
	error = fget(fd, &fp);
	if (error)
		goto out_errno;

	error = sys_fsync(fp);
	fdrop(fp);

	if (error)
		goto out_errno;
	trace_vfs_fsync_ret();
	return 0;

	out_errno:
	trace_vfs_fsync_err(error);
	errno = error;
	return -1;
}

int fdatasync(int fd)
{
	// TODO: See if we can do less than fsync().
	return fsync(fd);
}

UK_TRACEPOINT(trace_vfs_fstat, "%d %p", int, struct stat*);
UK_TRACEPOINT(trace_vfs_fstat_ret, "");
UK_TRACEPOINT(trace_vfs_fstat_err, "%d", int);

int __fxstat(int ver __unused, int fd, struct stat *st)
{
	struct vfscore_file *fp;
	int error;

	trace_vfs_fstat(fd, st);

	error = fget(fd, &fp);
	if (error)
		goto out_errno;

	error = sys_fstat(fp, st);
	fdrop(fp);

	if (error)
		goto out_errno;
	trace_vfs_fstat_ret();
	return 0;

	out_errno:
	trace_vfs_fstat_err(error);
	errno = error;
	return -1;
}

LFS64(__fxstat);

int fstat(int fd __unused, struct stat *st)
{
	return __fxstat(1, fd, st);
}

LFS64(fstat);

int __fxstatat(int ver __unused, int dirfd, const char *pathname, struct stat *st,
		int flags)
{
	if (flags & AT_SYMLINK_NOFOLLOW) {
		UK_CRASH("UNIMPLEMENTED: fstatat() with AT_SYMLINK_NOFOLLOW");
	}

	if (pathname[0] == '/' || dirfd == AT_FDCWD) {
		return stat(pathname, st);
	}
	// If AT_EMPTY_PATH and pathname is an empty string, fstatat() operates on
	// dirfd itself, and in that case it doesn't have to be a directory.
	if ((flags & AT_EMPTY_PATH) && !pathname[0]) {
		return fstat(dirfd, st);
	}

	struct vfscore_file *fp;
	int error = fget(dirfd, &fp);
	if (error) {
		errno = error;
		return -1;
	}

	struct vnode *vp = fp->f_dentry->d_vnode;
	vn_lock(vp);

	char p[PATH_MAX];
	/* build absolute path */
	strlcpy(p, fp->f_dentry->d_mount->m_path, PATH_MAX);
	strlcat(p, fp->f_dentry->d_path, PATH_MAX);
	strlcat(p, "/", PATH_MAX);
	strlcat(p, pathname, PATH_MAX);

	error = stat(p, st);

	vn_unlock(vp);
	fdrop(fp);

	return error;
}

LFS64(__fxstatat);

int fstatat(int dirfd, const char *path, struct stat *st, int flags)
{
	return __fxstatat(1, dirfd, path, st, flags);
}

LFS64(fstatat);

int flock(int fd, int operation)
{
	struct vfscore_file *file;

	if (!fget(fd, &file)) {
		return libc_error(EBADF);
	}

	switch (operation) {
	case LOCK_SH:
	case LOCK_SH | LOCK_NB:
	case LOCK_EX:
	case LOCK_EX | LOCK_NB:
	case LOCK_UN:
		break;
	default:
		return libc_error(EINVAL);
	}

	return 0;
}

UK_TRACEPOINT(trace_vfs_readdir, "%d %p", int, struct dirent*);
UK_TRACEPOINT(trace_vfs_readdir_ret, "");
UK_TRACEPOINT(trace_vfs_readdir_err, "%d", int);

struct __dirstream
{
	int fd;
};

DIR *opendir(const char *path)
{
	DIR *dir;
	struct stat st;

	dir = malloc(sizeof(*dir));
	if (!dir) {
		errno = ENOMEM;
		goto out_err;
	}

	dir->fd = open(path, O_RDONLY);
	if (dir->fd < 0)
		goto out_free_dir;

	if (fstat(dir->fd, &st) < 0)
		goto out_free_dir;

	if (!S_ISDIR(st.st_mode)) {
		errno = ENOTDIR;
		goto out_free_dir;
	}

	return dir;

out_free_dir:
	free(dir);
out_err:
	return NULL;
}

DIR *fdopendir(int fd)
{
	DIR *dir;
	struct stat st;
	if (fstat(fd, &st) < 0) {
		return NULL;
	}
	if (!S_ISDIR(st.st_mode)) {
		errno = ENOTDIR;
		return NULL;
	}
	dir = malloc(sizeof(*dir));
	if (!dir) {
		errno = ENOMEM;
		return NULL;
	}
	dir->fd = fd;
	return dir;

}

int dirfd(DIR *dirp)
{
	if (!dirp) {
		return libc_error(EINVAL);
	}

	return dirp->fd;
}

int closedir(DIR *dir)
{
	close(dir->fd);
	free(dir);
	return 0;
}

struct dirent *readdir(DIR *dir)
{
	static __thread struct dirent entry, *result;
	int ret;

	ret = readdir_r(dir, &entry, &result);
	if (ret)
		return ERR2PTR(-ret);

	errno = 0;
	return result;
}

int readdir_r(DIR *dir, struct dirent *entry, struct dirent **result)
{
	int error;
	struct vfscore_file *fp;

	trace_vfs_readdir(dir->fd, entry);
	error = fget(dir->fd, &fp);
	if (!error) {
		error = sys_readdir(fp, entry);
		fdrop(fp);
		if (error) {
			trace_vfs_readdir_err(error);
		} else {
			trace_vfs_readdir_ret();
		}
	}
	// Our dirent has (like Linux) a d_reclen field, but a constant size.
	entry->d_reclen = sizeof(*entry);

	if (error) {
		*result = NULL;
	} else {
		*result = entry;
	}
	return error == ENOENT ? 0 : error;
}

// FIXME: in 64bit dirent64 and dirent are identical, so it's safe to alias
#undef readdir64_r
int readdir64_r(DIR *dir, struct dirent64 *entry,
		struct dirent64 **result)
		__attribute__((alias("readdir_r")));

#undef readdir64
struct dirent *readdir64(DIR *dir) __attribute__((alias("readdir")));

void rewinddir(DIR *dirp)
{
	struct vfscore_file *fp;

	int error = fget(dirp->fd, &fp);
	if (error) {
		// POSIX specifies that what rewinddir() does in the case of error
		// is undefined...
		return;
	}

	sys_rewinddir(fp);
	// Again, error code from sys_rewinddir() is ignored.
	fdrop(fp);
}

long telldir(DIR *dirp)
{
	struct vfscore_file *fp;
	int error = fget(dirp->fd, &fp);
	if (error) {
		return libc_error(error);
	}

	long loc;
	error = sys_telldir(fp, &loc);
	fdrop(fp);
	if (error) {
		return libc_error(error);
	}
	return loc;
}

void seekdir(DIR *dirp, long loc)
{
	struct vfscore_file *fp;
	int error = fget(dirp->fd, &fp);
	if (error) {
		// POSIX specifies seekdir() cannot return errors.
		return;
	}
	sys_seekdir(fp, loc);
	// Again, error code from sys_seekdir() is ignored.
	fdrop(fp);
}

UK_TRACEPOINT(trace_vfs_mkdir, "\"%s\" 0%0o", const char*, mode_t);
UK_TRACEPOINT(trace_vfs_mkdir_ret, "");
UK_TRACEPOINT(trace_vfs_mkdir_err, "%d", int);

int
mkdir(const char *pathname, mode_t mode)
{
	struct task *t = main_task;
	char path[PATH_MAX];
	int error;

	mode = apply_umask(mode);

	trace_vfs_mkdir(pathname, mode);
	if ((error = task_conv(t, pathname, VWRITE, path)) != 0)
		goto out_errno;

	error = sys_mkdir(path, mode);
	if (error)
		goto out_errno;
	trace_vfs_mkdir_ret();
	return 0;
	out_errno:
	trace_vfs_mkdir_err(error);
	errno = error;
	return -1;
}

UK_TRACEPOINT(trace_vfs_rmdir, "\"%s\"", const char*);
UK_TRACEPOINT(trace_vfs_rmdir_ret, "");
UK_TRACEPOINT(trace_vfs_rmdir_err, "%d", int);

int rmdir(const char *pathname)
{
	struct task *t = main_task;
	char path[PATH_MAX];
	int error;

	trace_vfs_rmdir(pathname);
	error = ENOENT;
	if (pathname == NULL)
		goto out_errno;
	if ((error = task_conv(t, pathname, VWRITE, path)) != 0)
		goto out_errno;

	error = sys_rmdir(path);
	if (error)
		goto out_errno;
	trace_vfs_rmdir_ret();
	return 0;
	out_errno:
	trace_vfs_rmdir_err(error);
	errno = error;
	return -1;
}

static void
get_last_component(const char *path, char *dst)
{
	int pos = strlen(path) - 1;

	while (pos >= 0 && path[pos] == '/')
		pos--;

	int component_end = pos;

	while (pos >= 0 && path[pos] != '/')
		pos--;

	int component_start = pos + 1;

	int len = component_end - component_start + 1;
	memcpy(dst, path + component_start, len);
	dst[len] = 0;
}

static int null_or_empty(const char *str)
{
	return str == NULL || *str == '\0';
}

UK_TRACEPOINT(trace_vfs_rename, "\"%s\" \"%s\"", const char*, const char*);
UK_TRACEPOINT(trace_vfs_rename_ret, "");
UK_TRACEPOINT(trace_vfs_rename_err, "%d", int);

int rename(const char *oldpath, const char *newpath)
{
	trace_vfs_rename(oldpath, newpath);
	struct task *t = main_task;
	char src[PATH_MAX];
	char dest[PATH_MAX];
	int error;

	error = ENOENT;
	if (null_or_empty(oldpath) || null_or_empty(newpath))
		goto out_errno;

	get_last_component(oldpath, src);
	if (!strcmp(src, ".") || !strcmp(src, "..")) {
		error = EINVAL;
		goto out_errno;
	}

	get_last_component(newpath, dest);
	if (!strcmp(dest, ".") || !strcmp(dest, "..")) {
		error = EINVAL;
		goto out_errno;
	}

	if ((error = task_conv(t, oldpath, VREAD, src)) != 0)
		goto out_errno;

	if ((error = task_conv(t, newpath, VWRITE, dest)) != 0)
		goto out_errno;

	error = sys_rename(src, dest);
	if (error)
		goto out_errno;
	trace_vfs_rename_ret();
	return 0;
	out_errno:
	trace_vfs_rename_err(error);
	errno = error;
	return -1;
}

UK_TRACEPOINT(trace_vfs_chdir, "\"%s\"", const char*);
UK_TRACEPOINT(trace_vfs_chdir_ret, "");
UK_TRACEPOINT(trace_vfs_chdir_err, "%d", int);

static int
__do_fchdir(struct vfscore_file *fp, struct task *t)
{
	struct vfscore_file *old = NULL;

	UK_ASSERT(t);

	if (t->t_cwdfp) {
		old = t->t_cwdfp;
	}

	/* Do the actual chdir operation here */
	int error = sys_fchdir(fp, t->t_cwd);

	t->t_cwdfp = fp;
	if (old) {
		fdrop(old);
	}

	return error;
}

int chdir(const char *pathname)
{
	trace_vfs_chdir(pathname);
	struct task *t = main_task;
	char path[PATH_MAX];
	struct vfscore_file *fp;
	int error;

	error = ENOENT;
	if (pathname == NULL)
		goto out_errno;

	if ((error = task_conv(t, pathname, VREAD, path)) != 0)
		goto out_errno;

	/* Check if directory exits */
	error = sys_open(path, O_DIRECTORY, 0, &fp);
	if (error) {
		goto out_errno;
	}

	error = __do_fchdir(fp, t);
	if (error) {
		fdrop(fp);
		goto out_errno;
	}

	trace_vfs_chdir_ret();
	return 0;
	out_errno:
	errno = error;
	trace_vfs_chdir_err(errno);
	return -1;
}

UK_TRACEPOINT(trace_vfs_fchdir, "%d", int);
UK_TRACEPOINT(trace_vfs_fchdir_ret, "");
UK_TRACEPOINT(trace_vfs_fchdir_err, "%d", int);

int fchdir(int fd)
{
	trace_vfs_fchdir(fd);
	struct task *t = main_task;
	struct vfscore_file *fp;
	int error;

	error = fget(fd, &fp);
	if (error)
		goto out_errno;

	error = __do_fchdir(fp, t);
	if (error) {
		fdrop(fp);
		goto out_errno;
	}

	trace_vfs_fchdir_ret();
	return 0;

	out_errno:
	trace_vfs_fchdir_err(error);
	errno = error;
	return -1;
}

UK_TRACEPOINT(trace_vfs_link, "\"%s\" \"%s\"", const char*, const char*);
UK_TRACEPOINT(trace_vfs_link_ret, "");
UK_TRACEPOINT(trace_vfs_link_err, "%d", int);

int link(const char *oldpath, const char *newpath)
{
	struct task *t = main_task;
	char path1[PATH_MAX];
	char path2[PATH_MAX];
	int error;

	trace_vfs_link(oldpath, newpath);

	error = ENOENT;
	if (oldpath == NULL || newpath == NULL)
		goto out_errno;
	if ((error = task_conv(t, oldpath, VWRITE, path1)) != 0)
		goto out_errno;
	if ((error = task_conv(t, newpath, VWRITE, path2)) != 0)
		goto out_errno;

	error = sys_link(path1, path2);
	if (error)
		goto out_errno;
	trace_vfs_link_ret();
	return 0;
	out_errno:
	trace_vfs_link_err(error);
	errno = error;
	return -1;
}


UK_TRACEPOINT(trace_vfs_symlink, "oldpath=%s, newpath=%s", const char*,
	      const char*);
UK_TRACEPOINT(trace_vfs_symlink_ret, "");
UK_TRACEPOINT(trace_vfs_symlink_err, "errno=%d", int);

int symlink(const char *oldpath, const char *newpath)
{
	int error;

	trace_vfs_symlink(oldpath, newpath);

	error = ENOENT;
	if (oldpath == NULL || newpath == NULL) {
		errno = ENOENT;
		trace_vfs_symlink_err(error);
		return (-1);
	}

	error = sys_symlink(oldpath, newpath);
	if (error) {
		errno = error;
		trace_vfs_symlink_err(error);
		return (-1);
	}

	trace_vfs_symlink_ret();
	return 0;
}

UK_TRACEPOINT(trace_vfs_unlink, "\"%s\"", const char*);
UK_TRACEPOINT(trace_vfs_unlink_ret, "");
UK_TRACEPOINT(trace_vfs_unlink_err, "%d", int);

int unlink(const char *pathname)
{
	trace_vfs_unlink(pathname);
	struct task *t = main_task;
	char path[PATH_MAX];
	int error;

	error = ENOENT;
	if (pathname == NULL)
		goto out_errno;
	if ((error = task_conv(t, pathname, VWRITE, path)) != 0)
		goto out_errno;

	error = sys_unlink(path);
	if (error)
		goto out_errno;
	trace_vfs_unlink_ret();
	return 0;
	out_errno:
	trace_vfs_unlink_err(error);
	errno = error;
	return -1;
}

UK_TRACEPOINT(trace_vfs_stat, "\"%s\" %p", const char*, struct stat*);
UK_TRACEPOINT(trace_vfs_stat_ret, "");
UK_TRACEPOINT(trace_vfs_stat_err, "%d", int);

int __xstat(int ver __unused, const char *pathname, struct stat *st)
{
	struct task *t = main_task;
	char path[PATH_MAX];
	int error;

	trace_vfs_stat(pathname, st);

	error = task_conv(t, pathname, 0, path);
	if (error)
		goto out_errno;

	error = sys_stat(path, st);
	if (error)
		goto out_errno;
	trace_vfs_stat_ret();
	return 0;

	out_errno:
	trace_vfs_stat_err(error);
	errno = error;
	return -1;
}

LFS64(__xstat);

int stat(const char *pathname, struct stat *st)
{
	if (!pathname) {
		errno = EINVAL;
		return -1;
	}
	return __xstat(1, pathname, st);
}

LFS64(stat);

UK_TRACEPOINT(trace_vfs_lstat, "pathname=%s, stat=%p", const char*,
	      struct stat*);
UK_TRACEPOINT(trace_vfs_lstat_ret, "");
UK_TRACEPOINT(trace_vfs_lstat_err, "errno=%d", int);

int __lxstat(int ver __unused, const char *pathname, struct stat *st)
{
	struct task *t = main_task;
	char path[PATH_MAX];
	int error;

	trace_vfs_lstat(pathname, st);

	error = task_conv(t, pathname, 0, path);
	if (error) {
		errno = error;
		trace_vfs_lstat_err(error);
		return (-1);
	}

	error = sys_lstat(path, st);
	if (error) {
		errno = error;
		trace_vfs_lstat_err(error);
		return (-1);
	}

	trace_vfs_lstat_ret();
	return 0;
}

LFS64(__lxstat);

int lstat(const char *pathname, struct stat *st)
{
	return __lxstat(1, pathname, st);
}

LFS64(lstat);

UK_TRACEPOINT(trace_vfs_statfs, "\"%s\" %p", const char*, struct statfs*);
UK_TRACEPOINT(trace_vfs_statfs_ret, "");
UK_TRACEPOINT(trace_vfs_statfs_err, "%d", int);

int __statfs(const char *pathname, struct statfs *buf)
{
	trace_vfs_statfs(pathname, buf);
	struct task *t = main_task;
	char path[PATH_MAX];
	int error;

	error = task_conv(t, pathname, 0, path);
	if (error)
		goto out_errno;

	error = sys_statfs(path, buf);
	if (error)
		goto out_errno;
	trace_vfs_statfs_ret();
	return 0;
	out_errno:
	trace_vfs_statfs_err(error);
	errno = error;
	return -1;
}
__weak_alias(__statfs, statfs);

LFS64(statfs);

UK_TRACEPOINT(trace_vfs_fstatfs, "\"%s\" %p", int, struct statfs*);
UK_TRACEPOINT(trace_vfs_fstatfs_ret, "");
UK_TRACEPOINT(trace_vfs_fstatfs_err, "%d", int);

int __fstatfs(int fd, struct statfs *buf)
{
	struct vfscore_file *fp;
	int error;

	trace_vfs_fstatfs(fd, buf);
	error = fget(fd, &fp);
	if (error)
		goto out_errno;

	error = sys_fstatfs(fp, buf);
	fdrop(fp);

	if (error)
		goto out_errno;
	trace_vfs_fstatfs_ret();
	return 0;

	out_errno:
	trace_vfs_fstatfs_err(error);
	errno = error;
	return -1;
}
__weak_alias(__fstatfs, fstatfs);

LFS64(fstatfs);

static int
statfs_to_statvfs(struct statvfs *dst, struct statfs *src)
{
	dst->f_bsize = src->f_bsize;
	dst->f_frsize = src->f_bsize;
	dst->f_blocks = src->f_blocks;
	dst->f_bfree = src->f_bfree;
	dst->f_bavail = src->f_bavail;
	dst->f_files = src->f_files;
	dst->f_ffree = src->f_ffree;
	dst->f_favail = 0;
	dst->f_fsid = src->f_fsid.__val[0];
	dst->f_flag = src->f_flags;
	dst->f_namemax = src->f_namelen;
	return 0;
}

int
statvfs(const char *pathname, struct statvfs *buf)
{
	struct statfs st;

	if (__statfs(pathname, &st) < 0)
		return -1;
	return statfs_to_statvfs(buf, &st);
}

LFS64(statvfs);

int
fstatvfs(int fd, struct statvfs *buf)
{
	struct statfs st;

	if (__fstatfs(fd, &st) < 0)
		return -1;
	return statfs_to_statvfs(buf, &st);
}

LFS64(fstatvfs);


UK_TRACEPOINT(trace_vfs_getcwd, "%p %d", char*, size_t);
UK_TRACEPOINT(trace_vfs_getcwd_ret, "\"%s\"", const char*);
UK_TRACEPOINT(trace_vfs_getcwd_err, "%d", int);

char *getcwd(char *path, size_t size)
{
	trace_vfs_getcwd(path, size);
	struct task *t = main_task;
	size_t len = strlen(t->t_cwd) + 1;
	int error;

	if (!path) {
		if (!size)
			size = len;
		path = (char*)malloc(size);
		if (!path) {
			error = ENOMEM;
			goto out_errno;
		}
	} else {
		if (!size) {
			error = EINVAL;
			goto out_errno;
		}
	}

	if (size < len) {
		error = ERANGE;
		goto out_errno;
	}

	memcpy(path, t->t_cwd, len);
	trace_vfs_getcwd_ret(path);
	return path;

	out_errno:
	trace_vfs_getcwd_err(error);
	errno = error;
	return NULL;
}

UK_TRACEPOINT(trace_vfs_dup, "%d", int);
UK_TRACEPOINT(trace_vfs_dup_ret, "\"%s\"", int);
UK_TRACEPOINT(trace_vfs_dup_err, "%d", int);
/*
 * Duplicate a file descriptor
 */
int dup(int oldfd)
{
	struct vfscore_file *fp;
	int newfd;
	int error;

	trace_vfs_dup(oldfd);
	error = fget(oldfd, &fp);
	if (error)
		goto out_errno;

	error = fdalloc(fp, &newfd);
	if (error)
		goto out_fdrop;

	fdrop(fp);
	trace_vfs_dup_ret(newfd);
	return newfd;

	out_fdrop:
	fdrop(fp);
	out_errno:
	trace_vfs_dup_err(error);
	errno = error;
	return -1;
}

UK_TRACEPOINT(trace_vfs_dup3, "%d %d 0x%x", int, int, int);
UK_TRACEPOINT(trace_vfs_dup3_ret, "%d", int);
UK_TRACEPOINT(trace_vfs_dup3_err, "%d", int);
/*
 * Duplicate a file descriptor to a particular value.
 */
int dup3(int oldfd, int newfd, int flags)
{
	struct vfscore_file *fp;
	int error;

	trace_vfs_dup3(oldfd, newfd, flags);
	/*
	 * Don't allow any argument but O_CLOEXEC.  But we even ignore
	 * that as we don't support exec() and thus don't care.
	 */
	if ((flags & ~O_CLOEXEC) != 0) {
		error = EINVAL;
		goto out_errno;
	}

	if (oldfd == newfd) {
		error = EINVAL;
		goto out_errno;
	}

	error = fget(oldfd, &fp);
	if (error)
		goto out_errno;

	error = vfscore_install_fd(newfd, fp);
	if (error) {
		fdrop(fp);
		goto out_errno;
	}

	fdrop(fp);
	trace_vfs_dup3_ret(newfd);
	return newfd;

	out_errno:
	trace_vfs_dup3_err(error);
	errno = error;
	return -1;
}

int dup2(int oldfd, int newfd)
{
	if (oldfd == newfd)
		return newfd;

	return dup3(oldfd, newfd, 0);
}

/*
 * The file control system call.
 */
#define SETFL (O_APPEND | O_ASYNC | O_DIRECT | O_NOATIME | O_NONBLOCK)

UK_TRACEPOINT(trace_vfs_fcntl, "%d %d 0x%x", int, int, int);
UK_TRACEPOINT(trace_vfs_fcntl_ret, "\"%s\"", int);
UK_TRACEPOINT(trace_vfs_fcntl_err, "%d", int);

int fcntl(int fd, int cmd, ...)
{
	int arg;
	va_list ap;
	struct vfscore_file *fp;
	int ret = 0, error;
	int tmp;

	va_start(ap, cmd);
	arg = va_arg(ap, int);
	va_end(ap);

	trace_vfs_fcntl(fd, cmd, arg);
	error = fget(fd, &fp);
	if (error)
		goto out_errno;

	// An important note about our handling of FD_CLOEXEC / O_CLOEXEC:
	// close-on-exec shouldn't have been a file flag (fp->f_flags) - it is a
	// file descriptor flag, meaning that that two dup()ed file descriptors
	// could have different values for FD_CLOEXEC. Our current implementation
	// *wrongly* makes close-on-exec an f_flag (using the bit O_CLOEXEC).
	// There is little practical difference, though, because this flag is
	// ignored in OSv anyway, as it doesn't support exec().
	switch (cmd) {
	case F_DUPFD:
		error = fdalloc(fp, &ret);
		if (error)
			goto out_errno;
		break;
	case F_GETFD:
		ret = (fp->f_flags & O_CLOEXEC) ? FD_CLOEXEC : 0;
		break;
	case F_SETFD:
		FD_LOCK(fp);
		fp->f_flags = (fp->f_flags & ~O_CLOEXEC) |
				((arg & FD_CLOEXEC) ? O_CLOEXEC : 0);
		FD_UNLOCK(fp);
		break;
	case F_GETFL:
		// As explained above, the O_CLOEXEC should have been in f_flags,
		// and shouldn't be returned. Linux always returns 0100000 ("the
		// flag formerly known as O_LARGEFILE) so let's do it too.
		ret = (vfscore_oflags(fp->f_flags) & ~O_CLOEXEC) | 0100000;
		break;
	case F_SETFL:
		FD_LOCK(fp);
		fp->f_flags = vfscore_fflags((vfscore_oflags(fp->f_flags) & ~SETFL) |
				(arg & SETFL));
		FD_UNLOCK(fp);

#if defined(FIONBIO) && defined(FIOASYNC)
		/* Sync nonblocking/async state with file flags */
		tmp = fp->f_flags & FNONBLOCK;
		vfs_ioctl(fp, FIONBIO, &tmp);
		tmp = fp->f_flags & FASYNC;
		vfs_ioctl(fp, FIOASYNC, &tmp);
#endif
		break;
	case F_DUPFD_CLOEXEC:
		error = fdalloc(fp, &ret);
		if (error)
			goto out_errno;
		fp->f_flags |= O_CLOEXEC;
		break;
	case F_SETLK:
		uk_pr_warn("fcntl(F_SETLK) stubbed\n");
		break;
	case F_GETLK:
		uk_pr_warn("fcntl(F_GETLK) stubbed\n");
		break;
	case F_SETLKW:
		uk_pr_warn("fcntl(F_SETLKW) stubbed\n");
		break;
	case F_SETOWN:
		uk_pr_warn("fcntl(F_SETOWN) stubbed\n");
		break;
	default:
		uk_pr_err("unsupported fcntl cmd 0x%x\n", cmd);
		error = EINVAL;
	}

	fdrop(fp);
	if (error)
		goto out_errno;
	trace_vfs_fcntl_ret(ret);
	return ret;

out_errno:
	trace_vfs_fcntl_err(error);
	errno = error;
	return -1;
}

UK_TRACEPOINT(trace_vfs_access, "\"%s\" 0%0o", const char*, int);
UK_TRACEPOINT(trace_vfs_access_ret, "");
UK_TRACEPOINT(trace_vfs_access_err, "%d", int);

/*
 * Check permission for file access
 */
int access(const char *pathname, int mode)
{
	trace_vfs_access(pathname, mode);
	struct task *t = main_task;
	char path[PATH_MAX];
	int acc, error = 0;

	acc = 0;
	if (mode & R_OK)
		acc |= VREAD;
	if (mode & W_OK)
		acc |= VWRITE;

	if ((error = task_conv(t, pathname, acc, path)) != 0)
		goto out_errno;

	error = sys_access(path, mode);
	if (error)
		goto out_errno;
	trace_vfs_access_ret();
	return 0;
	out_errno:
	errno = error;
	trace_vfs_access_err(error);
	return -1;
}

int faccessat(int dirfd, const char *pathname, int mode, int flags)
{
	if (flags & AT_SYMLINK_NOFOLLOW) {
		UK_CRASH("UNIMPLEMENTED: faccessat() with AT_SYMLINK_NOFOLLOW");
	}

	if (pathname[0] == '/' || dirfd == AT_FDCWD) {
		return access(pathname, mode);
	}

	struct vfscore_file *fp;
	int error = fget(dirfd, &fp);
	if (error) {
		errno = error;
		return -1;
	}

	struct vnode *vp = fp->f_dentry->d_vnode;
	vn_lock(vp);

	char p[PATH_MAX];

	/* build absolute path */
	strlcpy(p, fp->f_dentry->d_mount->m_path, PATH_MAX);
	strlcat(p, fp->f_dentry->d_path, PATH_MAX);
	strlcat(p, "/", PATH_MAX);
	strlcat(p, pathname, PATH_MAX);

	error = access(p, mode);

	vn_unlock(vp);
	fdrop(fp);

	return error;
}

int euidaccess(const char *pathname, int mode)
{
	return access(pathname, mode);
}

__weak_alias(euidaccess,eaccess);

#if 0
/*
 * Return if specified file is a tty
 */
int isatty(int fd)
{
	struct vfscore_file *fp;
	int istty = 0;

	trace_vfs_isatty(fd);
	fileref f(fileref_from_fd(fd));
	if (!f) {
		errno = EBADF;
		trace_vfs_isatty_err(errno);
		return -1;
	}

	fp = f.get();
	if (dynamic_cast<tty_file*>(fp) ||
		(fp->f_dentry && fp->f_dentry->d_vnode->v_flags & VISTTY)) {
		istty = 1;
	}

	trace_vfs_isatty_ret(istty);
	return istty;
}
#endif

UK_TRACEPOINT(trace_vfs_truncate, "\"%s\" 0x%x", const char*, off_t);
UK_TRACEPOINT(trace_vfs_truncate_ret, "");
UK_TRACEPOINT(trace_vfs_truncate_err, "%d", int);

int truncate(const char *pathname, off_t length)
{
	trace_vfs_truncate(pathname, length);
	struct task *t = main_task;
	char path[PATH_MAX];
	int error;

	error = ENOENT;
	if (pathname == NULL)
		goto out_errno;
	if ((error = task_conv(t, pathname, VWRITE, path)) != 0)
		goto out_errno;

	error = sys_truncate(path, length);
	if (error)
		goto out_errno;
	trace_vfs_truncate_ret();
	return 0;
	out_errno:
	errno = error;
	trace_vfs_truncate_err(error);
	return -1;
}

LFS64(truncate);

UK_TRACEPOINT(trace_vfs_ftruncate, "%d 0x%x", int, off_t);
UK_TRACEPOINT(trace_vfs_ftruncate_ret, "");
UK_TRACEPOINT(trace_vfs_ftruncate_err, "%d", int);

int ftruncate(int fd, off_t length)
{
	trace_vfs_ftruncate(fd, length);
	struct vfscore_file *fp;
	int error;

	error = fget(fd, &fp);
	if (error)
		goto out_errno;

	error = sys_ftruncate(fp, length);
	fdrop(fp);

	if (error)
		goto out_errno;
	trace_vfs_ftruncate_ret();
	return 0;

	out_errno:
	errno = error;
	trace_vfs_ftruncate_err(error);
	return -1;
}

LFS64(ftruncate);

ssize_t readlink(const char *pathname, char *buf, size_t bufsize)
{
	struct task *t = main_task;
	char path[PATH_MAX];
	int error;
	ssize_t size;

	error = -EINVAL;
	if (bufsize <= 0)
		goto out_errno;

	error = ENOENT;
	if (pathname == NULL)
		goto out_errno;
	error = task_conv(t, pathname, VWRITE, path);
	if (error)
		goto out_errno;

	size  = 0;
	error = sys_readlink(path, buf, bufsize, &size);

	if (error != 0)
		goto out_errno;

	return size;
	out_errno:
	errno = error;
	return -1;
}

UK_TRACEPOINT(trace_vfs_fallocate, "%d %d 0x%x 0x%x", int, int, loff_t, loff_t);
UK_TRACEPOINT(trace_vfs_fallocate_ret, "");
UK_TRACEPOINT(trace_vfs_fallocate_err, "%d", int);

int fallocate(int fd, int mode, loff_t offset, loff_t len)
{
	struct vfscore_file *fp;
	int error;

	trace_vfs_fallocate(fd, mode, offset, len);
	error = fget(fd, &fp);
	if (error)
		goto out_errno;

	error = sys_fallocate(fp, mode, offset, len);
	fdrop(fp);

	if (error)
		goto out_errno;
	trace_vfs_fallocate_ret();
	return 0;

	out_errno:
	trace_vfs_fallocate_err(error);
	errno = error;
	return -1;
}

LFS64(fallocate);

#if 0
int futimesat(int dirfd, const char *pathname, const struct timeval times[2])
{
	struct stat st;
	struct vfscore_file *fp;
	int error;
	char *absolute_path;

	if ((pathname && pathname[0] == '/') || dirfd == AT_FDCWD)
		return utimes(pathname, times);

	// Note: if pathname == NULL, futimesat operates on dirfd itself, and in
	// that case it doesn't have to be a directory.
	if (pathname) {
		error = fstat(dirfd, &st);
		if (error) {
			error = errno;
			goto out_errno;
		}

		if (!S_ISDIR(st.st_mode)){
			error = ENOTDIR;
			goto out_errno;
		}
	}

	error = fget(dirfd, &fp);
	if (error)
		goto out_errno;

	/* build absolute path */
	absolute_path = (char*)malloc(PATH_MAX);
	strlcpy(absolute_path, fp->f_dentry->d_mount->m_path, PATH_MAX);
	strlcat(absolute_path, fp->f_dentry->d_path, PATH_MAX);

	if (pathname) {
		strlcat(absolute_path, "/", PATH_MAX);
		strlcat(absolute_path, pathname, PATH_MAX);
	}

	error = utimes(absolute_path, times);
	free(absolute_path);

	fdrop(fp);

	if (error)
		goto out_errno;
	return 0;

	out_errno:
	errno = error;
	return -1;
}

UK_TRACEPOINT(trace_vfs_utimensat, "\"%s\"", const char*);
UK_TRACEPOINT(trace_vfs_utimensat_ret, "");
UK_TRACEPOINT(trace_vfs_utimensat_err, "%d", int);

int utimensat(int dirfd, const char *pathname, const struct timespec times[2], int flags)
{
	trace_vfs_utimensat(pathname);

	auto error = sys_utimensat(dirfd, pathname, times, flags);

	if (error) {
		trace_vfs_utimensat_err(error);
		errno = error;
		return -1;
	}

	trace_vfs_utimensat_ret();
	return 0;
}

UK_TRACEPOINT(trace_vfs_futimens, "%d", int);
UK_TRACEPOINT(trace_vfs_futimens_ret, "");
UK_TRACEPOINT(trace_vfs_futimens_err, "%d", int);

int futimens(int fd, const struct timespec times[2])
{
	trace_vfs_futimens(fd);

	int error = sys_futimens(fd, times);
	if (error) {
		trace_vfs_futimens_err(error);
		errno = error;
		return -1;
	}

	trace_vfs_futimens_ret();
	return 0;
}

static int do_utimes(const char *pathname, const struct timeval times[2], int flags)
{
	struct task *t = main_task;
	char path[PATH_MAX];
	int error;

	trace_vfs_utimes(pathname);

	error = task_conv(t, pathname, 0, path);
	if (error) {
		trace_vfs_utimes_err(error);
		return libc_error(error);
	}

	error = sys_utimes(path, times, flags);
	if (error) {
		trace_vfs_utimes_err(error);
		return libc_error(error);
	}

	trace_vfs_utimes_ret();
	return 0;
}

int utimes(const char *pathname, const struct timeval times[2])
{
	return do_utimes(pathname, times, 0);
}

int lutimes(const char *pathname, const struct timeval times[2])
{
	return do_utimes(pathname, times, AT_SYMLINK_NOFOLLOW);
}

int utime(const char *pathname, const struct utimbuf *t)
{
	using namespace std::chrono;

	struct timeval times[2];
	times[0].tv_usec = 0;
	times[1].tv_usec = 0;
	if (!t) {
		long int tsec = duration_cast<seconds>(osv::clock::wall::now().time_since_epoch()).count();
		times[0].tv_sec = tsec;
		times[1].tv_sec = tsec;
	} else {
		times[0].tv_sec = t->actime;
		times[1].tv_sec = t->modtime;
	}

	return utimes(pathname, times);
}
#endif

UK_TRACEPOINT(trace_vfs_chmod, "\"%s\" 0%0o", const char*, mode_t);
UK_TRACEPOINT(trace_vfs_chmod_ret, "");
UK_TRACEPOINT(trace_vfs_chmod_err, "%d", int);

int chmod(const char *pathname, mode_t mode)
{
	trace_vfs_chmod(pathname, mode);
	struct task *t = main_task;
	char path[PATH_MAX];
	int error = ENOENT;
	if (pathname == NULL)
		goto out_errno;
	if ((error = task_conv(t, pathname, VWRITE, path)) != 0)
		goto out_errno;
	error = sys_chmod(path, mode & UK_ALLPERMS);
	if (error)
		goto out_errno;
	trace_vfs_chmod_ret();
	return 0;
out_errno:
	trace_vfs_chmod_err(error);
	errno = error;
	return -1;
}

UK_TRACEPOINT(trace_vfs_fchmod, "\"%d\" 0%0o", int, mode_t);
UK_TRACEPOINT(trace_vfs_fchmod_ret, "");

int fchmod(int fd, mode_t mode)
{
	trace_vfs_fchmod(fd, mode);
	int error = sys_fchmod(fd, mode & UK_ALLPERMS);
	trace_vfs_fchmod_ret();
	if (error) {
		errno = error;
		return -1;
	} else {
		return 0;
	}
}

UK_TRACEPOINT(trace_vfs_fchown, "\"%d\" %d %d", int, uid_t, gid_t);
UK_TRACEPOINT(trace_vfs_fchown_ret, "");

int fchown(int fd __unused, uid_t owner __unused, gid_t group __unused)
{
	trace_vfs_fchown(fd, owner, group);
	WARN_STUBBED();
	trace_vfs_fchown_ret();
	return 0;
}

int chown(const char *path __unused, uid_t owner __unused, gid_t group __unused)
{
	WARN_STUBBED();
	return 0;
}

int lchown(const char *path __unused, uid_t owner __unused, gid_t group __unused)
{
	WARN_STUBBED();
	return 0;
}


#if 0
ssize_t sendfile(int out_fd, int in_fd, off_t *_offset, size_t count)
{
	struct vfscore_file *in_fp;
	struct vfscore_file *out_fp;
	fileref in_f{fileref_from_fd(in_fd)};
	fileref out_f{fileref_from_fd(out_fd)};

	if (!in_f || !out_f) {
		return libc_error(EBADF);
	}

	in_fp = in_f.get();
	out_fp = out_f.get();

	if (!in_fp->f_dentry) {
		return libc_error(EBADF);
	}

	if (!(in_fp->f_flags & UK_FREAD))
		return libc_error(EBADF);

	if (out_fp->f_type & DTYPE_VNODE) {
		if (!out_fp->f_dentry)
			return libc_error(EBADF);
		else if (!(out_fp->f_flags & UK_FWRITE))
			return libc_error(EBADF);
	}

	off_t offset ;

	if (_offset != nullptr) {
		offset = *_offset;
	} else {
		/* if _offset is nullptr, we need to read from the present position of in_fd */
		offset = lseek(in_fd, 0, SEEK_CUR);
	}

	// Constrain count to the extent of the file...
	struct stat st;
	if (fstat(in_fd, &st) < 0) {
		return -1;
	} else {
		if (offset >= st.st_size) {
			return 0;
		} else if ((offset + count) >= st.st_size) {
			count = st.st_size - offset;
			if (count == 0) {
				return 0;
			}
		}
	}

	size_t bytes_to_mmap = count + (offset % mmu::page_size);
	off_t offset_for_mmap =  align_down(offset, (off_t)mmu::page_size);

	char *src = static_cast<char *>(mmap(nullptr, bytes_to_mmap, PROT_READ, MAP_SHARED, in_fd, offset_for_mmap));

	if (src == MAP_FAILED) {
		return -1;
	}

	int ret = write(out_fd, src + (offset % PAGESIZE), count);

	if (ret < 0) {
		return libc_error(errno);
	} else if(_offset == nullptr) {
		lseek(in_fd, ret, SEEK_CUR);
	} else {
		*_offset += ret;
	}

	assert(munmap(src, count) == 0);

	return ret;
}

#undef sendfile64
LFS64(sendfile);
#endif

mode_t umask(mode_t newmask)
{
	return ukarch_exchange_n(&global_umask, newmask);
}

int
fs_noop(void)
{
	return 0;
}

int chroot(const char *path __unused)
{
	WARN_STUBBED();
	errno = ENOSYS;
	return -1;
}

static struct task _main_task_impl;
static void vfscore_init(void)
{
	memset(&_main_task_impl, 0, sizeof(_main_task_impl));
	strcpy(_main_task_impl.t_cwd, "/");
	main_task = &_main_task_impl;

	vnode_init();
	lookup_init();
}

UK_CTOR_FUNC(1, vfscore_init);
