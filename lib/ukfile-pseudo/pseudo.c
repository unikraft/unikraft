/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Pseudo-files with constant input and discarding output */

#include <poll.h>
#include <limits.h>
#include <sys/stat.h>

#include <uk/assert.h>
#include <uk/file-pseudo.h>
#include <uk/file/nops.h>
#include <uk/file/iovutil.h>

static const char NULL_VOLID[] = "null_vol";
static const char VOID_VOLID[] = "void_vol";
static const char ZERO_VOLID[] = "zero_vol";

static ssize_t null_read(const struct uk_file *f __maybe_unused,
			 const struct iovec *iov __unused,
			 size_t iovcnt __unused,
			 size_t off __unused, long flags __unused)
{
	UK_ASSERT(f->vol == NULL_VOLID);
	return 0;
}

static ssize_t void_read(const struct uk_file *f __maybe_unused,
			 const struct iovec *iov __unused,
			 size_t iovcnt __unused,
			 size_t off __unused, long flags __unused)
{
	UK_ASSERT(f->vol == VOID_VOLID);
	return -EAGAIN;
}

static ssize_t zero_read(const struct uk_file *f __maybe_unused,
			 const struct iovec *iov, size_t iovcnt,
			 size_t off __unused, long flags __unused)
{
	size_t iovi = 0;
	size_t cur = 0;

	UK_ASSERT(f->vol == ZERO_VOLID);
	if (unlikely(!iov))
		return -EFAULT;

	return uk_iov_zero(iov, iovcnt, SSIZE_MAX, &iovi, &cur);
}

static ssize_t null_write(const struct uk_file *f __maybe_unused,
			  const struct iovec *iov, size_t iovcnt,
			  size_t off __unused, long flags __unused)
{
	UK_ASSERT(f->vol == NULL_VOLID || f->vol == ZERO_VOLID ||
		  f->vol == VOID_VOLID);
	if (unlikely(!iov))
		return -EFAULT;

	return uk_iov_len(iov, iovcnt);
}

static int pseudo_getstat(const struct uk_file *f __maybe_unused,
			  unsigned int mask __unused, struct uk_statx *arg)
{
	UK_ASSERT(f->vol == NULL_VOLID || f->vol == VOID_VOLID ||
		  f->vol == ZERO_VOLID);

	/* Since all information is immediately available, ignore mask arg */
	arg->stx_mask = UK_STATX_TYPE | UK_STATX_MODE | UK_STATX_NLINK |
			UK_STATX_INO  | UK_STATX_SIZE;
	arg->stx_mode = S_IFCHR | 0666;
	arg->stx_nlink = 1;
	arg->stx_ino = 1;
	arg->stx_size = 0;

	/* Following fields are always filled in, not in stx_mask */
	arg->stx_dev_major = 0;
	arg->stx_dev_minor = 5; /* Same value Linux returns for /dev/null */
	arg->stx_rdev_major = 0;
	arg->stx_rdev_minor = 0;
	arg->stx_blksize = 0x1000;
	return 0;
}

static const struct uk_file_ops null_ops = {
	.read = null_read,
	.write = null_write,
	.mem = uk_file_nop_mem,
	.getstat = pseudo_getstat,
	.setstat = uk_file_nop_setstat,
	.ctl = uk_file_nop_ctl,
};

static const struct uk_file_ops void_ops = {
	.read = void_read,
	.write = null_write,
	.mem = uk_file_nop_mem,
	.getstat = pseudo_getstat,
	.setstat = uk_file_nop_setstat,
	.ctl = uk_file_nop_ctl,
};

static const struct uk_file_ops zero_ops = {
	.read = zero_read,
	.write = null_write,
	.mem = uk_file_nop_mem,
	.getstat = pseudo_getstat,
	.setstat = uk_file_nop_setstat,
	.ctl = uk_file_nop_ctl,
};

/* Refcounts initialized to 1 to count the static lifetime reference */
static uk_file_refcnt null_ref = UK_FILE_REFCNT_INITIALIZER(null_ref);
static uk_file_refcnt void_ref = UK_FILE_REFCNT_INITIALIZER(void_ref);
static uk_file_refcnt zero_ref = UK_FILE_REFCNT_INITIALIZER(zero_ref);

static struct uk_file_state null_state = UK_FILE_STATE_EVENTS_INITIALIZER(
	null_state, POLLIN | POLLOUT);
static struct uk_file_state void_state = UK_FILE_STATE_EVENTS_INITIALIZER(
	void_state, POLLOUT);
static struct uk_file_state zero_state = UK_FILE_STATE_EVENTS_INITIALIZER(
	zero_state, POLLIN | POLLOUT);

const struct uk_file uk_file_null = {
	.vol = NULL_VOLID,
	.node = NULL,
	.ops = &null_ops,
	.refcnt = &null_ref,
	.state = &null_state,
	._release = uk_file_static_release
};

const struct uk_file uk_file_void = {
	.vol = VOID_VOLID,
	.node = NULL,
	.ops = &void_ops,
	.refcnt = &void_ref,
	.state = &void_state,
	._release = uk_file_static_release
};

const struct uk_file uk_file_zero = {
	.vol = ZERO_VOLID,
	.node = NULL,
	.ops = &zero_ops,
	.refcnt = &zero_ref,
	.state = &zero_state,
	._release = uk_file_static_release
};
