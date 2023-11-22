/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Internal syscalls for file I/O */

#include <poll.h>

#include <uk/posix-fdio.h>

#include "fdio-impl.h"

/* Stable mode bits to pass onto the read/write implementations */
#define _READ_MODEMASK 0
#define _WRITE_MODEMASK (O_SYNC|O_DSYNC)


#define _buf2iov(buf, count) \
	((struct iovec){ .iov_base = (buf), .iov_len = (count) })

#define _SHOULD_BLOCK(r, m) ((r) == -EAGAIN && _IS_BLOCKING((m)))


static inline
ssize_t fdio_get_eof(const struct uk_file *f)
{
	struct uk_statx statx;
	int r = uk_file_getstat(f, UK_STATX_SIZE, &statx);

	if (unlikely(r))
		return r;
	else
		return statx.stx_size;
}


ssize_t uk_sys_preadv(struct uk_ofile *of, const struct iovec *iov, int iovcnt,
		      off_t offset)
{
	ssize_t r;
	long flags;
	int iolock;
	const struct uk_file *f;

	unsigned int mode = of->mode;

	if (unlikely(!_CAN_READ(mode)))
		return -EINVAL;
	if (unlikely(iovcnt < 0))
		return -EINVAL;
	if (unlikely(!iov && iovcnt))
		return -EFAULT;
	if (unlikely(offset < 0))
		return -EINVAL;

	flags = mode & _READ_MODEMASK;
	iolock = _SHOULD_LOCK(mode);
	f = of->file;
	for (;;) {
		if (iolock)
			uk_file_rlock(f);
		r = uk_file_read(f, iov, iovcnt, offset, flags);
		if (iolock)
			uk_file_runlock(f);
		if (!_SHOULD_BLOCK(r, mode))
			break;
		uk_file_poll(f, UKFD_POLLIN|UKFD_POLL_ALWAYS);
	}
	return r;
}

ssize_t uk_sys_pread(struct uk_ofile *of, void *buf, size_t count, off_t offset)
{
	return uk_sys_preadv(of, &_buf2iov(buf, count), 1, offset);
}

ssize_t uk_sys_readv(struct uk_ofile *of, const struct iovec *iov, int iovcnt)
{
	ssize_t r;
	long flags;
	off_t off;
	const struct uk_file *f;
	int seekable;
	int iolock;

	unsigned int mode = of->mode;

	if (unlikely(!_CAN_READ(mode)))
		return -EINVAL;
	if (unlikely(iovcnt < 0))
		return -EINVAL;
	if (unlikely(!iov && iovcnt))
		return -EFAULT;

	seekable = _IS_SEEKABLE(mode);
	iolock = _SHOULD_LOCK(mode);

	off = 0;
	flags = mode & _READ_MODEMASK;
	f = of->file;
	for (;;) {
		if (seekable) {
			_of_lock(of);
			off = of->pos;
		}

		if (iolock)
			uk_file_rlock(f);
		r = uk_file_read(f, iov, iovcnt, off, flags);
		if (iolock)
			uk_file_runlock(f);
		if (!_SHOULD_BLOCK(r, mode))
			break;
		if (seekable)
			_of_unlock(of);
		uk_file_poll(f, UKFD_POLLIN|UKFD_POLL_ALWAYS);
	}

	if (seekable) {
		if (r > 0)
			of->pos = off + r;
		_of_unlock(of);
	}

	return r;
}

ssize_t uk_sys_read(struct uk_ofile *of, void *buf, size_t count)
{
	return uk_sys_readv(of, &_buf2iov(buf, count), 1);
}

ssize_t uk_sys_preadv2(struct uk_ofile *of, const struct iovec *iov, int iovcnt,
		       off_t offset, int flags __maybe_unused)
{
	ssize_t r;
	off_t off;
	long xflags;
	const struct uk_file *f;
	unsigned int mode;
	int seekable;
	int use_pos;
	int iolock;

#if 0 /* RWF flags not supported by our libc's yet */
	if (unlikely(flags & RWF_NOWAIT)) {
		uk_pr_warn_once("STUB: preadv2 flag RWF_NOWAIT not supported");
		return -EINVAL; /* Not supported */
	}
#endif

	mode = of->mode;

	if (unlikely(!_CAN_READ(mode)))
		return -EINVAL;
	if (unlikely(iovcnt < 0))
		return -EINVAL;
	if (unlikely(!iov && iovcnt))
		return -EFAULT;
	if (unlikely(offset < 0))
		return -EINVAL;

	seekable = _IS_SEEKABLE(mode);
	use_pos = offset == -1 && seekable;
	iolock = _SHOULD_LOCK(mode);

	off = offset != -1 ? offset : 0;
	xflags = mode & _READ_MODEMASK;
	f = of->file;

	for (;;) {
		if (use_pos) {
			_of_lock(of);
			off = of->pos;
		}

		if (iolock)
			uk_file_rlock(f);
		r = uk_file_read(f, iov, iovcnt, off, xflags);
		if (iolock)
			uk_file_runlock(f);
		if (!_SHOULD_BLOCK(r, mode))
			break;
		if (use_pos)
			_of_unlock(of);
		uk_file_poll(f, UKFD_POLLIN|UKFD_POLL_ALWAYS);
	}

	if (use_pos) {
		if (r > 0)
			of->pos = off + r;
		_of_unlock(of);
	}

	return r;
}


ssize_t uk_sys_pwritev(struct uk_ofile *of, const struct iovec *iov, int iovcnt,
		       off_t offset)
{
	ssize_t r;
	long flags;
	const struct uk_file *f;
	int iolock;

	unsigned int mode = of->mode;

	if (!_CAN_WRITE(mode))
		return -EINVAL;
	if (unlikely(iovcnt < 0))
		return -EINVAL;
	if (unlikely(!iov && iovcnt))
		return -EFAULT;
	if (unlikely(offset < 0))
		return -EINVAL;

	flags = mode & _WRITE_MODEMASK;
	iolock = _SHOULD_LOCK(mode);
	f = of->file;
	for (;;) {
		if (iolock)
			uk_file_wlock(f);
		r = uk_file_write(f, iov, iovcnt, offset, flags);
		if (iolock)
			uk_file_wunlock(f);
		if (!_SHOULD_BLOCK(r, mode))
			break;
		uk_file_poll(f, UKFD_POLLOUT|UKFD_POLL_ALWAYS);
	}

	return r;
}

ssize_t uk_sys_pwrite(struct uk_ofile *of, const void *buf, size_t count,
		  off_t offset)
{
	return uk_sys_pwritev(of, &_buf2iov((void *)buf, count), 1, offset);
}

ssize_t uk_sys_writev(struct uk_ofile *of, const struct iovec *iov, int iovcnt)
{
	ssize_t r;
	off_t off;
	long flags;
	const struct uk_file *f;
	int seekable;
	int iolock;

	unsigned int mode = of->mode;

	if (!_CAN_WRITE(mode))
		return -EINVAL;
	if (unlikely(iovcnt < 0))
		return -EINVAL;
	if (unlikely(!iov && iovcnt))
		return -EFAULT;

	seekable = _IS_SEEKABLE(mode);
	iolock = _SHOULD_LOCK(mode);

	off = 0;
	flags = mode & _WRITE_MODEMASK;
	f = of->file;
	for (;;) {
		if (seekable)
			_of_lock(of);
		if (iolock)
			uk_file_wlock(f);

		if (seekable) {
			if (_IS_APPEND(mode))
				off = fdio_get_eof(f);
			else
				off = of->pos;
		}

		if (likely(off >= 0))
			r = uk_file_write(f, iov, iovcnt, off, flags);
		else
			r = off;

		if (iolock)
			uk_file_wunlock(f);
		if (!_SHOULD_BLOCK(r, mode))
			break;
		if (seekable)
			_of_unlock(of);
		uk_file_poll(f, UKFD_POLLOUT|UKFD_POLL_ALWAYS);
	}

	if (seekable) {
		if (r >= 0)
			of->pos = off + r;
		_of_unlock(of);
	}

	return r;
}

ssize_t uk_sys_write(struct uk_ofile *of, const void *buf, size_t count)
{
	return uk_sys_writev(of, &_buf2iov((void *)buf, count), 1);
}

ssize_t uk_sys_pwritev2(struct uk_ofile *of, const struct iovec *iov,
			int iovcnt, off_t offset, int flags __maybe_unused)
{
	ssize_t r;
	off_t off;
	long xflags;
	const struct uk_file *f;
	int seekable;
	int use_pos;
	int iolock;

	unsigned int mode = of->mode;

	if (!_CAN_WRITE(mode))
		return -EINVAL;
	if (unlikely(iovcnt < 0))
		return -EINVAL;
	if (unlikely(!iov && iovcnt))
		return -EFAULT;
	if (unlikely(offset < 0))
		return -EINVAL;

	seekable = _IS_SEEKABLE(mode);
	use_pos = seekable && offset == -1;
	iolock = _SHOULD_LOCK(mode);

	off = seekable ? offset : 0;
	xflags = mode & _WRITE_MODEMASK;
#if 0 /* RWF flags not supported by our libc's yet */
	if (flags & RWF_SYNC)
		xflags |= O_SYNC;
	if (flags & RWF_DSYNC)
		xflags |= O_DSYNC;
#endif
	f = of->file;

	for (;;) {
		if (use_pos)
			_of_lock(of);
		if (iolock)
			uk_file_wlock(f);

		if (seekable) {
#if 0 /* RWF flags not supported by our libc's yet */
			if (flags & RWF_APPEND)
				off = fdio_get_eof(f);
			else
#endif
				if (offset == -1) {
					if (_IS_APPEND(mode))
						off = fdio_get_eof(f);
					else
						off = of->pos;
				}
		}

		if (likely(off >= 0))
			r = uk_file_write(f, iov, iovcnt, off, xflags);
		else
			r = off;

		if (iolock)
			uk_file_wunlock(f);
		if (!_SHOULD_BLOCK(r, mode))
			break;
		if (use_pos)
			_of_unlock(of);
		uk_file_poll(f, UKFD_POLLOUT|UKFD_POLL_ALWAYS);
	}

	if (use_pos) {
		if (r >= 0)
			of->pos = off + r;
		_of_unlock(of);
	}

	return r;
}


off_t uk_sys_lseek(struct uk_ofile *of, off_t offset, int whence)
{
	unsigned int mode;
	int iolock;

	if (unlikely(!(whence == SEEK_SET ||
		       whence == SEEK_CUR ||
		       whence == SEEK_END)))
		return -EINVAL;

	mode = of->mode;

	if (unlikely(!_CAN_WRITE(mode)))
		return -EINVAL;
	if (unlikely(!_IS_SEEKABLE(mode)))
		return -ESPIPE;

	iolock = _SHOULD_LOCK(mode);

	_of_lock(of);
	if (whence == SEEK_END) {
		const struct uk_file *f = of->file;

		if (iolock)
			uk_file_rlock(f);
		offset = fdio_get_eof(f);
		if (iolock)
			uk_file_runlock(f);
	} else if (whence == SEEK_CUR) {
		offset += of->pos;
	}
	if (likely(offset >= 0))
		of->pos = offset;
	else
		offset = -EINVAL;
	_of_unlock(of);

	return offset;
}
