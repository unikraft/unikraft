/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Pseudo-files: files with constant input and discarding output */

#include <string.h>

#include <uk/assert.h>
#include <uk/file.h>
#include <uk/file/nops.h>
#include <uk/posix-fd.h>

/* Null file: input is always at EOF */
static const char NULL_VOLID[] = "null_vol";
/* Void file: behaves as if it is always waiting for input */
static const char VOID_VOLID[] = "void_vol";
/* Zero file: outputs all zeros */
static const char ZERO_VOLID[] = "zero_vol";

static ssize_t null_read(const struct uk_file *f,
			 const struct iovec *iov __unused, int iovcnt __unused,
			 off_t off __unused, long flags __unused)
{
	UK_ASSERT(f->vol == NULL_VOLID);
	return 0;
}

static ssize_t void_read(const struct uk_file *f,
			 const struct iovec *iov __unused, int iovcnt __unused,
			 off_t off __unused, long flags __unused)
{
	UK_ASSERT(f->vol == VOID_VOLID);
	return -EAGAIN;
}

static ssize_t zero_read(const struct uk_file *f,
			 const struct iovec *iov, int iovcnt,
			 off_t off __unused, long flags __unused)
{
	ssize_t total = 0;

	UK_ASSERT(f->vol == ZERO_VOLID);

	for (int i = 0; i < iovcnt; i++) {
		if (unlikely(!iov[i].iov_base && iov[i].iov_len))
			return -EFAULT;
		memset(iov[i].iov_base, 0, iov[i].iov_len);
		total += iov[i].iov_len;
	}
	return total;
}

static ssize_t null_write(const struct uk_file *f __unused,
			  const struct iovec *iov, int iovcnt,
			  off_t off __unused, long flags __unused)
{
	ssize_t total = 0;

	UK_ASSERT(f->vol == NULL_VOLID || f->vol == ZERO_VOLID ||
		  f->vol == VOID_VOLID);

	for (int i = 0; i < iovcnt; i++)
		total += iov[i].iov_len;
	return total;
}

static const struct uk_file_ops null_ops = {
	.read = null_read,
	.write = null_write,
	.getstat = uk_file_nop_getstat,
	.setstat = uk_file_nop_setstat,
	.ctl = uk_file_nop_ctl,
};

static const struct uk_file_ops void_ops = {
	.read = void_read,
	.write = null_write,
	.getstat = uk_file_nop_getstat,
	.setstat = uk_file_nop_setstat,
	.ctl = uk_file_nop_ctl,
};

static const struct uk_file_ops zero_ops = {
	.read = zero_read,
	.write = null_write,
	.getstat = uk_file_nop_getstat,
	.setstat = uk_file_nop_setstat,
	.ctl = uk_file_nop_ctl,
};

static uk_file_refcnt null_ref = UK_FILE_REFCNT_INITIALIZER(null_ref);
static uk_file_refcnt void_ref = UK_FILE_REFCNT_INITIALIZER(void_ref);
static uk_file_refcnt zero_ref = UK_FILE_REFCNT_INITIALIZER(zero_ref);

static struct uk_file_state null_state = UK_FILE_STATE_EVENTS_INITIALIZER(
	null_state, UKFD_POLLIN|UKFD_POLLOUT);
static struct uk_file_state void_state = UK_FILE_STATE_EVENTS_INITIALIZER(
	void_state, UKFD_POLLOUT);
static struct uk_file_state zero_state = UK_FILE_STATE_EVENTS_INITIALIZER(
	zero_state, UKFD_POLLIN|UKFD_POLLOUT);

static const struct uk_file null_file = {
	.vol = NULL_VOLID,
	.node = NULL,
	.ops = &null_ops,
	.refcnt = &null_ref,
	.state = &null_state,
	._release = uk_file_static_release
};

static const struct uk_file void_file = {
	.vol = VOID_VOLID,
	.node = NULL,
	.ops = &void_ops,
	.refcnt = &void_ref,
	.state = &void_state,
	._release = uk_file_static_release
};

static const struct uk_file zero_file = {
	.vol = ZERO_VOLID,
	.node = NULL,
	.ops = &zero_ops,
	.refcnt = &zero_ref,
	.state = &zero_state,
	._release = uk_file_static_release
};


const struct uk_file *uk_nullfile_create(void)
{
	const struct uk_file *f = &null_file;

	uk_file_acquire(f);
	return f;
}

const struct uk_file *uk_voidfile_create(void)
{
	const struct uk_file *f = &void_file;

	uk_file_acquire(f);
	return f;
}

const struct uk_file *uk_zerofile_create(void)
{
	const struct uk_file *f = &zero_file;

	uk_file_acquire(f);
	return f;
}
