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

#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/file.h>
#include <stdarg.h>
#include <uk/fdtab/fd.h>
#include <uk/print.h>
#include <uk/errptr.h>
#include <uk/trace.h>
#include <uk/syscall.h>
#include <uk/essentials.h>
#include "syscalls.h"

/* This macro is for defining an alias of the 64bit version of a
 * syscall to the regular one. We only do this when libc-style
 * syscall wrappers are requested to be created.
 * NOTE: When using LFS64(), remember to undefine possible rename
 *       macros created by included libc headers (e.g., <fcntl.h>):
 *       ```
 *       #ifdef openat64
 *       #undef openat64
 *       #endif
 *
 *       LFS64(openat)
 *       ```
 */
#if UK_LIBC_SYSCALLS
#define LFS64(x)				\
	__alias(x, x##64)
#else
#define LFS64(x)
#endif /* !UK_LIBC_SYSCALLS */

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
		(error != -EWOULDBLOCK && error != -EINTR));
}

UK_TRACEPOINT(trace_vfs_close, "%d", int);
UK_TRACEPOINT(trace_vfs_close_ret, "");
UK_TRACEPOINT(trace_vfs_close_err, "%d", int);

static int fdclose(struct fdtab_table *tab, int fd)
{
	struct fdtab_file *fp;
	int error;

	fp = fdtab_get_file(tab, fd);
	if (!fp)
		return -EBADF;

	error = fdtab_put_fd(tab, fd);
	if (!error)
		fdtab_fdrop(fp);

	return error;
}

UK_SYSCALL_R_DEFINE(int, close, int, fd)
{
	int error;
	struct fdtab_table *tab;

	tab = fdtab_get_active();

	trace_vfs_close(fd);
	error = fdclose(tab, fd);
	if (error)
		goto out_error;

	trace_vfs_close_ret();
	return 0;

out_error:
	trace_vfs_close_err(error);
	UK_ASSERT(error < 0);
	return error;
}

UK_TRACEPOINT(trace_vfs_lseek, "%d 0x%x %d", int, off_t, int);
UK_TRACEPOINT(trace_vfs_lseek_ret, "0x%x", off_t);
UK_TRACEPOINT(trace_vfs_lseek_err, "%d", int);

UK_SYSCALL_R_DEFINE(off_t, lseek, int, fd, off_t, offset, int, whence)
{
	struct fdtab_file *fp;
	off_t org;
	int error;

	trace_vfs_lseek(fd, offset, whence);
	error = fdtab_fget(fd, &fp);
	if (error)
		goto out_error;

	error = sys_lseek(fp, offset, whence, &org);
	fdtab_fdrop(fp);

	if (error)
		goto out_error;
	trace_vfs_lseek_ret(org);
	return org;

out_error:
	trace_vfs_lseek_err(error);
	UK_ASSERT(error < 0);
	return error;
}

#ifdef lseek64
#undef lseek64
#endif

LFS64(lseek);

/**
 * Return:
 * = 0, Success and the nr of bytes read is returned in bytes parameter.
 * < 0, error code.
 */
static ssize_t do_preadv(struct fdtab_file *fp, const struct iovec *iov,
			 int iovcnt, off_t offset, ssize_t *bytes)
{
	size_t cnt;
	int error;

	UK_ASSERT(fp && iov);

	error = sys_read(fp, iov, iovcnt, offset, &cnt);

	if (has_error(error, cnt))
		goto out_error;

	*bytes = cnt;
	return 0;

out_error:
	UK_ASSERT(error < 0);
	return error;
}

UK_TRACEPOINT(trace_vfs_preadv, "%d %p 0x%x 0x%x", int, const struct iovec*,
	      int, off_t);
UK_TRACEPOINT(trace_vfs_preadv_ret, "0x%x", ssize_t);
UK_TRACEPOINT(trace_vfs_preadv_err, "%d", int);

UK_SYSCALL_R_DEFINE(ssize_t, preadv, int, fd, const struct iovec*, iov,
	int, iovcnt, off_t, offset)
{
	struct fdtab_file *fp;
	ssize_t bytes;
	int error;

	trace_vfs_preadv(fd, iov, iovcnt, offset);
	error = fdtab_fget(fd, &fp);
	if (error)
		goto out_error;

	error = do_preadv(fp, iov, iovcnt, offset, &bytes);

	fdtab_fdrop(fp);

	if (error < 0)
		goto out_error;

	trace_vfs_preadv_ret(bytes);
	return bytes;

out_error:
	trace_vfs_preadv_err(error);
	return error;
}

#ifdef preadv64
#undef preadv64
#endif

LFS64(preadv);

UK_TRACEPOINT(trace_vfs_pread, "%d %p 0x%x 0x%x", int, void*, size_t, off_t);
UK_TRACEPOINT(trace_vfs_pread_ret, "0x%x", ssize_t);
UK_TRACEPOINT(trace_vfs_pread_err, "%d", int);

/*
 * Some libc's define some macros that remove the 64 suffix
 * from some system call function names (e.g., <unistd.h>, <fcntl.h>).
 * We need to undefine them here so that our system call
 * registration does not fail in such a case.
 */
#ifdef pread64
#undef pread64
#endif

UK_LLSYSCALL_R_DEFINE(ssize_t, pread64, int, fd,
		      void *, buf, size_t, count, off_t, offset)
{
	trace_vfs_pread(fd, buf, count, offset);
	struct iovec iov = {
			.iov_base	= buf,
			.iov_len	= count,
	};
	ssize_t bytes;

	bytes = uk_syscall_r_preadv((long) fd, (long) &iov,
				    1, (long) offset);
	if (bytes < 0)
		trace_vfs_pread_err(bytes);
	else
		trace_vfs_pread_ret(bytes);
	return bytes;
}

#if UK_LIBC_SYSCALLS
ssize_t pread(int fd, void *buf, size_t count, off_t offset)
{
	return uk_syscall_e_pread64((long) fd, (long) buf,
				    (long) count, (long) offset);
}

LFS64(pread);
#endif /* UK_LIBC_SYSCALLS */

UK_TRACEPOINT(trace_vfs_readv, "%d %p 0x%x", int, const struct iovec*,
	      int);
UK_TRACEPOINT(trace_vfs_readv_ret, "0x%x", ssize_t);
UK_TRACEPOINT(trace_vfs_readv_err, "%d", int);

UK_SYSCALL_R_DEFINE(ssize_t, readv,
		  int, fd, const struct iovec *, iov, int, iovcnt)
{
	struct fdtab_file *fp;
	ssize_t bytes;
	int error;

	trace_vfs_readv(fd, iov, iovcnt);
	error = fdtab_fget(fd, &fp);
	if (error)
		goto out_error;

	error = do_preadv(fp, iov, iovcnt, -1, &bytes);

	fdtab_fdrop(fp);

	if (error < 0)
		goto out_error;

	trace_vfs_readv_ret(bytes);
	return bytes;

out_error:
	trace_vfs_readv_err(error);
	return error;
}

UK_TRACEPOINT(trace_vfs_read, "%d %p %d", int, void *, int);
UK_TRACEPOINT(trace_vfs_read_ret, "0x%x", ssize_t);
UK_TRACEPOINT(trace_vfs_read_err, "%d", int);

UK_SYSCALL_R_DEFINE(ssize_t, read, int, fd, void *, buf, size_t, count)
{
	ssize_t bytes;

	UK_ASSERT(buf);

	struct iovec iov = {
			.iov_base	= buf,
			.iov_len	= count,
	};

	trace_vfs_read(fd, buf, count);

	bytes = uk_syscall_r_readv((long) fd, (long) &iov, 1);
	if (bytes < 0)
		trace_vfs_read_err(bytes);
	else
		trace_vfs_read_ret(bytes);
	return bytes;
}

static int do_pwritev(struct fdtab_file *fp, const struct iovec *iov,
		      int iovcnt, off_t offset, ssize_t *bytes)
{
	int error;
	size_t cnt;

	UK_ASSERT(bytes);

	error = sys_write(fp, iov, iovcnt, offset, &cnt);

	if (has_error(error, cnt))
		goto out_error;

	*bytes = cnt;
	return 0;

out_error:
	UK_ASSERT(error < 0);
	*bytes = -1;
	return error;
}

UK_TRACEPOINT(trace_vfs_pwritev, "%d %p 0x%x 0x%x", int, const struct iovec*,
	      int, off_t);
UK_TRACEPOINT(trace_vfs_pwritev_ret, "0x%x", ssize_t);
UK_TRACEPOINT(trace_vfs_pwritev_err, "%d", int);

UK_SYSCALL_R_DEFINE(ssize_t, pwritev, int, fd, const struct iovec*, iov,
			int, iovcnt, off_t, offset)
{
	struct fdtab_file *fp;
	ssize_t bytes;
	int error;

	trace_vfs_pwritev(fd, iov, iovcnt, offset);
	error = fdtab_fget(fd, &fp);
	if (error)
		goto out_error;

	error = do_pwritev(fp, iov, iovcnt, offset, &bytes);

	fdtab_fdrop(fp);

	if (error < 0)
		goto out_error;

	trace_vfs_pwritev_ret(bytes);
	return bytes;

out_error:
	trace_vfs_pwritev_err(error);
	return error;
}

#ifdef pwritev64
#undef pwritev64
#endif

LFS64(pwritev);

UK_TRACEPOINT(trace_vfs_pwrite, "%d %p 0x%x 0x%x", int, const void*, size_t,
	      off_t);
UK_TRACEPOINT(trace_vfs_pwrite_ret, "0x%x", ssize_t);
UK_TRACEPOINT(trace_vfs_pwrite_err, "%d", int);

/*
 * Some libc's define some macros that remove the 64 suffix
 * from some system call function names (e.g., <unistd.h>, <fcntl.h>).
 * We need to undefine them here so that our system call
 * registration does not fail in such a case.
 */
#ifdef pwrite64
#undef pwrite64
#endif

UK_LLSYSCALL_R_DEFINE(ssize_t, pwrite64, int, fd,
		      const void *, buf, size_t, count, off_t, offset)
{
	trace_vfs_pwrite(fd, buf, count, offset);
	struct iovec iov = {
			.iov_base	= (void *)buf,
			.iov_len	= count,
	};
	ssize_t bytes;

	bytes = uk_syscall_r_pwritev((long) fd, (long) &iov,
				     1, (long) offset);
	if (bytes < 0)
		trace_vfs_pwrite_err(bytes);
	else
		trace_vfs_pwrite_ret(bytes);
	return bytes;
}

#if UK_LIBC_SYSCALLS
ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset)
{
	return uk_syscall_e_pwrite64((long) fd, (long) buf,
				     (long) count, (long) offset);
}

LFS64(pwrite);
#endif /* UK_LIBC_SYSCALLS */

UK_TRACEPOINT(trace_vfs_writev, "%d %p 0x%x 0x%x", int, const struct iovec*,
	      int);
UK_TRACEPOINT(trace_vfs_writev_ret, "0x%x", ssize_t);
UK_TRACEPOINT(trace_vfs_writev_err, "%d", int);

UK_SYSCALL_R_DEFINE(ssize_t, writev,
		  int, fd, const struct iovec *, vec, int, vlen)
{
	struct fdtab_file *fp;
	ssize_t bytes;
	int error;

	trace_vfs_writev(fd, vec, vlen);
	error = fdtab_fget(fd, &fp);
	if (error)
		goto out_error;

	error = do_pwritev(fp, vec, vlen, -1, &bytes);

	fdtab_fdrop(fp);

	if (error < 0)
		goto out_error;

	trace_vfs_pwritev_ret(bytes);
	return bytes;

out_error:
	trace_vfs_pwritev_err(error);
	return error;
}

UK_TRACEPOINT(trace_vfs_write, "%d %p 0x%x 0x%x", int, const void *,
	      size_t);
UK_TRACEPOINT(trace_vfs_write_ret, "0x%x", ssize_t);
UK_TRACEPOINT(trace_vfs_write_err, "%d", int);

UK_SYSCALL_R_DEFINE(ssize_t, write, int, fd, const void *, buf, size_t, count)
{
	ssize_t bytes;

	UK_ASSERT(buf);

	struct iovec iov = {
			.iov_base	= (void *)buf,
			.iov_len	= count,
	};
	trace_vfs_write(fd, buf, count);
	bytes = uk_syscall_r_writev((long) fd, (long) &iov, 1);
	if (bytes < 0)
		trace_vfs_write_err(errno);
	else
		trace_vfs_write_ret(bytes);
	return bytes;
}

UK_TRACEPOINT(trace_vfs_ioctl, "%d 0x%x", int, unsigned long);
UK_TRACEPOINT(trace_vfs_ioctl_ret, "");
UK_TRACEPOINT(trace_vfs_ioctl_err, "%d", int);

UK_LLSYSCALL_R_DEFINE(int, ioctl, int, fd, unsigned long int, request,
		void*, arg)
{
	struct fdtab_file *fp;
	int error;

	trace_vfs_ioctl(fd, request);
	/* glibc ABI provides a variadic prototype for ioctl so we need to agree
	 * with it, since we now include sys/ioctl.h
	 * read the first argument and pass it to sys_ioctl()
	 */

	error = fdtab_fget(fd, &fp);
	if (error)
		goto out_errno;

	error = sys_ioctl(fp, request, arg);
	fdtab_fdrop(fp);

	if (error < 0)
		goto out_errno;
	trace_vfs_ioctl_ret();
	return 0;

out_errno:
	trace_vfs_ioctl_err(error);
	UK_ASSERT(error < 0);
	return error;
}

#if UK_LIBC_SYSCALLS
int ioctl(int fd, unsigned long int request, ...)
{
	va_list ap;
	void *arg;

	va_start(ap, request);
	arg = va_arg(ap, void*);
	va_end(ap);

	return uk_syscall_e_ioctl((long) fd, (long) request, (long) arg);
}
#endif /* UK_LIBC_SYSCALLS */

UK_TRACEPOINT(trace_vfs_fsync, "%d", int);
UK_TRACEPOINT(trace_vfs_fsync_ret, "");
UK_TRACEPOINT(trace_vfs_fsync_err, "%d", int);

UK_SYSCALL_R_DEFINE(int, fsync, int, fd)
{
	struct fdtab_file *fp;
	int error;

	trace_vfs_fsync(fd);
	error = fdtab_fget(fd, &fp);
	if (error)
		goto out_error;

	error = sys_fsync(fp);
	fdtab_fdrop(fp);

	if (error)
		goto out_error;
	trace_vfs_fsync_ret();
	return 0;

out_error:
	trace_vfs_fsync_err(error);
	UK_ASSERT(error < 0);
	return error;
}

UK_SYSCALL_R_DEFINE(int, fdatasync, int, fd)
{
	// TODO: See if we can do less than fsync().
	return fsync(fd);
}

UK_TRACEPOINT(trace_vfs_fstat, "%d %p", int, struct stat*);
UK_TRACEPOINT(trace_vfs_fstat_ret, "");
UK_TRACEPOINT(trace_vfs_fstat_err, "%d", int);

static int __fxstat_helper(int ver __unused, int fd, struct stat *st)
{
	struct fdtab_file *fp;
	int error;

	trace_vfs_fstat(fd, st);

	error = fdtab_fget(fd, &fp);
	if (error)
		goto out_errno;

	error = sys_fstat(fp, st);
	fdtab_fdrop(fp);

	if (error)
		goto out_errno;
	trace_vfs_fstat_ret();
	return 0;

out_errno:
	trace_vfs_fstat_err(error);
	errno = -error;
	return -1;
}

#if UK_LIBC_SYSCALLS
int __fxstat(int ver __unused, int fd, struct stat *st)
{
	return __fxstat_helper(ver, fd, st);
}
#ifdef __fxstat64
#undef __fxstat64
#endif

LFS64(__fxstat);
#endif /* UK_LIBC_SYSCALLS */

UK_SYSCALL_DEFINE(int, fstat, int, fd, struct stat *, st)
{
	return __fxstat_helper(1, fd, st);
}

#ifdef fstat64
#undef fstat64
#endif

LFS64(fstat);

UK_SYSCALL_R_DEFINE(int, flock, int, fd, int, operation)
{
	struct fdtab_file *file;
	int error;

	error = fdtab_fget(fd, &file);
	if (error)
		goto out_error;

	switch (operation) {
	case LOCK_SH:
	case LOCK_SH | LOCK_NB:
	case LOCK_EX:
	case LOCK_EX | LOCK_NB:
	case LOCK_UN:
		break;
	default:
		error = -EINVAL;
		goto out_error;
	}

	return 0;

out_error:
	UK_ASSERT(error < 0);
	return error;
}

UK_TRACEPOINT(trace_vfs_dup, "%d", int);
UK_TRACEPOINT(trace_vfs_dup_ret, "\"%s\"", int);
UK_TRACEPOINT(trace_vfs_dup_err, "%d", int);

/*
 * Duplicate a file descriptor
 */
UK_SYSCALL_R_DEFINE(int, dup, int, oldfd)
{
	struct fdtab_file *fp;
	int newfd;
	int error;

	trace_vfs_dup(oldfd);
	error = fdtab_fget(oldfd, &fp);
	if (error)
		goto out_error;

	error = fdtab_fdalloc(fp, &newfd);
	if (error)
		goto out_fdtab_fdrop;

	fdtab_fdrop(fp);
	trace_vfs_dup_ret(newfd);
	return newfd;

out_fdtab_fdrop:
	fdtab_fdrop(fp);

out_error:
	trace_vfs_dup_err(error);
	UK_ASSERT(error < 0);
	return error;
}

UK_TRACEPOINT(trace_vfs_dup3, "%d %d 0x%x", int, int, int);
UK_TRACEPOINT(trace_vfs_dup3_ret, "%d", int);
UK_TRACEPOINT(trace_vfs_dup3_err, "%d", int);
/*
 * Duplicate a file descriptor to a particular value.
 */
UK_SYSCALL_R_DEFINE(int, dup3, int, oldfd, int, newfd, int, flags)
{
	struct fdtab_file *fp, *fp_new;
	struct fdtab_table *tab;
	int error;

	tab = fdtab_get_active();

	trace_vfs_dup3(oldfd, newfd, flags);
	/*
	 * Don't allow any argument but O_CLOEXEC.  But we even ignore
	 * that as we don't support exec() and thus don't care.
	 */
	if ((flags & ~O_CLOEXEC) != 0) {
		error = EINVAL;
		goto out_error;
	}

	if (oldfd == newfd) {
		error = EINVAL;
		goto out_error;
	}

	error = fdtab_fget(oldfd, &fp);
	if (error)
		goto out_error;

	error = fdtab_fget(newfd, &fp_new);
	if (error == 0) {
		/* if newfd is open, then close it */
		error = close(newfd);
		if (error)
			goto out_error;
	}

	error = fdtab_reserve_fd(tab, newfd);
	if (error)
		goto out_error;

	error = fdtab_install_fd(tab, newfd, fp);
	if (error) {
		fdtab_fdrop(fp);
		goto out_error;
	}

	trace_vfs_dup3_ret(newfd);
	return newfd;

out_error:
	trace_vfs_dup3_err(error);
	UK_ASSERT(error < 0);
	return error;
}

UK_SYSCALL_R_DEFINE(int, dup2, int, oldfd, int, newfd)
{
	if (oldfd == newfd)
		return newfd;

	return uk_syscall_r_dup3(oldfd, newfd, 0);
}

/*
 * The file control system call.
 */
#define SETFL (O_APPEND | O_ASYNC | O_DIRECT | O_NOATIME | O_NONBLOCK)

UK_TRACEPOINT(trace_vfs_fcntl, "%d %d 0x%x", int, int, int);
UK_TRACEPOINT(trace_vfs_fcntl_ret, "\"%s\"", int);
UK_TRACEPOINT(trace_vfs_fcntl_err, "%d", int);

UK_LLSYSCALL_R_DEFINE(int, fcntl, int, fd, unsigned int, cmd, int, arg)
{
	struct fdtab_file *fp;
	int ret = 0, error;
#if defined(FIONBIO) && defined(FIOASYNC)
	int tmp;
#endif

	trace_vfs_fcntl(fd, cmd, arg);
	error = fdtab_fget(fd, &fp);
	if (error)
		goto out_errno;

	// An important note about our handling of FD_CLOEXEC / O_CLOEXEC:
	// close-on-exec shouldn't have been a file flag (fp->f_flags) - it is a
	// file descriptor flag, meaning that that two dup()ed file descriptors
	// could have different values for FD_CLOEXEC. Our current
	// implementation *wrongly* makes close-on-exec an f_flag (using the bit
	// O_CLOEXEC). There is little practical difference, though, because
	// this flag is ignored in OSv anyway, as it doesn't support exec().
	switch (cmd) {
	case F_DUPFD:
		error = fdtab_fdalloc(fp, &ret);
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
		ret = (fdtab_oflags(fp->f_flags) & ~O_CLOEXEC) | 0100000;
		break;
	case F_SETFL:
		FD_LOCK(fp);
		fp->f_flags = fdtab_fflags((fdtab_oflags(fp->f_flags) & ~SETFL)
					   | (arg & SETFL));
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
		error = fdtab_fdalloc(fp, &ret);
		if (error)
			goto out_errno;
		fp->f_flags |= O_CLOEXEC;
		break;
	case F_SETLK:
		uk_pr_warn_once("fcntl(F_SETLK) stubbed\n");
		break;
	case F_GETLK:
		uk_pr_warn_once("fcntl(F_GETLK) stubbed\n");
		break;
	case F_SETLKW:
		uk_pr_warn_once("fcntl(F_SETLKW) stubbed\n");
		break;
	case F_SETOWN:
		uk_pr_warn_once("fcntl(F_SETOWN) stubbed\n");
		break;
	default:
		uk_pr_err("unsupported fcntl cmd 0x%x\n", cmd);
		error = EINVAL;
	}

	fdtab_fdrop(fp);
	if (error)
		goto out_errno;
	trace_vfs_fcntl_ret(ret);
	return ret;

out_errno:
	trace_vfs_fcntl_err(error);
	UK_ASSERT(error < 0);
	return error;
}

#if UK_LIBC_SYSCALLS
int fcntl(int fd, int cmd, ...)
{
	int arg = 0;
	va_list ap;

	va_start(ap, cmd);
	if (cmd == F_SETFD ||
	    cmd == F_SETFL) {
		arg = va_arg(ap, int);
	}
	va_end(ap);

	return uk_syscall_e_fcntl(fd, cmd, arg);
}
#endif /* UK_LIBC_SYSCALLS */

UK_TRACEPOINT(trace_vfs_ftruncate, "%d 0x%x", int, off_t);
UK_TRACEPOINT(trace_vfs_ftruncate_ret, "");
UK_TRACEPOINT(trace_vfs_ftruncate_err, "%d", int);

UK_SYSCALL_R_DEFINE(int, ftruncate, int, fd, off_t, length)
{
	trace_vfs_ftruncate(fd, length);
	struct fdtab_file *fp;
	int error;

	error = fdtab_fget(fd, &fp);
	if (error)
		goto out_error;

	error = sys_ftruncate(fp, length);
	fdtab_fdrop(fp);

	if (error)
		goto out_error;
	trace_vfs_ftruncate_ret();
	return 0;

out_error:
	trace_vfs_ftruncate_err(error);
	UK_ASSERT(error < 0);
	return error;
}

#ifdef ftruncate64
#undef ftruncate64
#endif

LFS64(ftruncate);

UK_TRACEPOINT(trace_vfs_fallocate, "%d %d 0x%x 0x%x", int, int, loff_t, loff_t);
UK_TRACEPOINT(trace_vfs_fallocate_ret, "");
UK_TRACEPOINT(trace_vfs_fallocate_err, "%d", int);

UK_SYSCALL_R_DEFINE(int, fallocate, int, fd, int, mode, loff_t, offset, loff_t,
		    len)
{
	struct fdtab_file *fp;
	int error;

	trace_vfs_fallocate(fd, mode, offset, len);
	error = fdtab_fget(fd, &fp);
	if (error)
		goto out_error;

	error = sys_fallocate(fp, mode, offset, len);
	fdtab_fdrop(fp);

	if (error)
		goto out_error;
	trace_vfs_fallocate_ret();
	return 0;

out_error:
	trace_vfs_fallocate_err(error);
	return error;
}

#ifdef fallocate64
#undef fallocate64
#endif

LFS64(fallocate);

#if UK_LIBC_SYSCALLS
int posix_fadvise(int fd __unused, off_t offset __unused, off_t len __unused,
		int advice)
{
	switch (advice) {
	case POSIX_FADV_NORMAL:
	case POSIX_FADV_SEQUENTIAL:
	case POSIX_FADV_RANDOM:
	case POSIX_FADV_NOREUSE:
	case POSIX_FADV_WILLNEED:
	case POSIX_FADV_DONTNEED:
		return 0;
	default:
		return EINVAL;
	}
}

#ifdef posix_fadvise64
#undef posix_fadvise64
#endif

LFS64(posix_fadvise);
#endif /* UK_LIBC_SYSCALLS */
