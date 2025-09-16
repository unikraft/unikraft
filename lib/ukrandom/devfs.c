/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <poll.h>
#include <sys/stat.h>

#include <uk/devfs.h>
#include <uk/init.h>
#include <uk/file.h>
#include <uk/file/nops.h>
#include <uk/fs.h>
#include <uk/fs/prio.h>
#include <uk/random.h>
#include <uk/random/driver.h>

#include "swrand.h"

extern struct uk_random_driver *driver;

static const char SWRANDOM_VOLID[] = "swrandom_vol";
static const char HWRANDOM_VOLID[] = "hwrandom_vol";

static
ssize_t urandom_read(const struct uk_file *f __maybe_unused,
		     const struct iovec *iov, size_t iovcnt,
		     size_t off __unused, long flags __unused)
{
	ssize_t nbytes = 0;
	int r;

	UK_ASSERT(f->vol == SWRANDOM_VOLID);

	for (size_t i = 0; i < iovcnt; i++) {
		r = uk_random_fill_buffer(iov[i].iov_base, iov[i].iov_len);
		if (unlikely(r))
			return r;
		nbytes += iov[i].iov_len;
	}
	return nbytes;
}

static
ssize_t hwrng_read(const struct uk_file *f __maybe_unused,
		   const struct iovec *iov, size_t iovcnt,
		   size_t off __unused, long flags __unused)
{
	ssize_t nbytes = 0;
	int r;

	UK_ASSERT(f->vol == HWRANDOM_VOLID);
	/* Should never be called if we don't have a proper hwrng driver */
	UK_ASSERT(driver);
	UK_ASSERT(driver != (void *)UK_SWRAND_DRIVER_NONE);

	for (size_t i = 0; i < iovcnt; i++) {
		r = driver->ops->random_bytes(iov[i].iov_base, iov[i].iov_len);
		if (unlikely(r))
			return r;
		nbytes += iov[i].iov_len;
	}
	return nbytes;
}

static
int urandom_getstat(const struct uk_file *f __maybe_unused,
		    unsigned int mask __unused, struct uk_statx *arg)
{
	UK_ASSERT(f->vol == SWRANDOM_VOLID);

	/* Since all information is immediately available, ignore mask arg */
	arg->stx_mask = UK_STATX_TYPE | UK_STATX_MODE | UK_STATX_NLINK |
			UK_STATX_INO  | UK_STATX_SIZE;
	arg->stx_mode = S_IFCHR | 0666;
	arg->stx_nlink = 1;
	arg->stx_ino = 9;
	arg->stx_size = 0;

	/* Following fields are always filled in, not in stx_mask */
	/* Same major/minor numbers Linux uses */
	arg->stx_dev_major = 1;
	arg->stx_dev_minor = 9;
	arg->stx_rdev_major = 0;
	arg->stx_rdev_minor = 0;
	arg->stx_blksize = 0x1000;
	return 0;
}

static
int hwrng_getstat(const struct uk_file *f __maybe_unused,
		  unsigned int mask __unused, struct uk_statx *arg)
{
	UK_ASSERT(f->vol == HWRANDOM_VOLID);

	/* Since all information is immediately available, ignore mask arg */
	arg->stx_mask = UK_STATX_TYPE | UK_STATX_MODE | UK_STATX_NLINK |
			UK_STATX_INO  | UK_STATX_SIZE;
	arg->stx_mode = S_IFCHR | 0666;
	arg->stx_nlink = 1;
	arg->stx_ino = 83;
	arg->stx_size = 0;

	/* Following fields are always filled in, not in stx_mask */
	/* major/minor numbers grabbed off a Linux box; should be arbitrary */
	arg->stx_dev_major = 10;
	arg->stx_dev_minor = 183;
	arg->stx_rdev_major = 0;
	arg->stx_rdev_minor = 0;
	arg->stx_blksize = 0x1000;
	return 0;
}

static
ssize_t deny_write(const struct uk_file *f __unused,
		   const struct iovec *iov __unused, size_t iovcnt __unused,
		   size_t off __unused, long flags __unused)
{
	return -EPERM;
}

static const struct uk_file_ops urandom_ops = {
	.read = urandom_read,
	.write = deny_write,
	.mem = uk_file_nop_mem,
	.getstat = urandom_getstat,
	.setstat = uk_file_nop_setstat,
	.ctl = uk_file_nop_ctl,
};

static const struct uk_file_ops hwrng_ops = {
	.read = hwrng_read,
	.write = deny_write,
	.mem = uk_file_nop_mem,
	.getstat = hwrng_getstat,
	.setstat = uk_file_nop_setstat,
	.ctl = uk_file_nop_ctl,
};

/* Refcounts initialized to 1 to count the static lifetime reference */
static uk_file_refcnt urandom_ref = UK_FILE_REFCNT_INITIALIZER(urandom_ref);
static uk_file_refcnt random_ref = UK_FILE_REFCNT_INITIALIZER(random_ref);
static uk_file_refcnt hwrng_ref = UK_FILE_REFCNT_INITIALIZER(hwrng_ref);

static struct uk_file_state urandom_state = UK_FILE_STATE_EVENTS_INITIALIZER(
	urandom_state, POLLIN);
static struct uk_file_state random_state = UK_FILE_STATE_EVENTS_INITIALIZER(
	random_state, POLLIN);
static struct uk_file_state hwrng_state = UK_FILE_STATE_EVENTS_INITIALIZER(
	hwrng_state, POLLIN);

static const struct uk_file urandom_file = {
	.vol = SWRANDOM_VOLID,
	.node = NULL,
	.ops = &urandom_ops,
	.refcnt = &urandom_ref,
	.state = &urandom_state,
	._release = uk_file_static_release
};

static const struct uk_file random_file = {
	.vol = SWRANDOM_VOLID,
	.node = NULL,
	/* /dev/random uses same ops as urandom at the moment */
	.ops = &urandom_ops,
	.refcnt = &random_ref,
	.state = &random_state,
	._release = uk_file_static_release
};

static const struct uk_file hwrng_file = {
	.vol = HWRANDOM_VOLID,
	.node = NULL,
	.ops = &hwrng_ops,
	.refcnt = &hwrng_ref,
	.state = &hwrng_state,
	._release = uk_file_static_release
};

static
int init_ukrandom_devfs(struct uk_init_ctx *ictx __unused)
{
	const union uk_fs_create_target urandom_target = {
		.file = &urandom_file
	};
	const union uk_fs_create_target random_target = {
		.file = &random_file
	};
	const union uk_fs_create_target hwrng_target = {
		.file = &hwrng_file
	};
	const void *r;

	/* We borrow the singleton static reference, no refcounting needed */
	UK_ASSERT(uk_fs_devfs_root);

	/* Do not create device nodes if ukrandom is not initialized */
	if (unlikely(!driver))
		return 0;

	/* We do not clean up created files on error, as they will be dropped
	 * when the devfs root is released on system shutdown.
	 */
	r = uk_fs_createat(uk_fs_devfs_root, "urandom", 7,
			   0666, O_EXCL, urandom_target);
	if (unlikely(PTRISERR(r))) {
		uk_pr_err("Failed to create /dev/urandom: %d\n",  PTR2ERR(r));
		return PTR2ERR(r);
	}
	r = uk_fs_createat(uk_fs_devfs_root, "random", 6,
			   0666, O_EXCL, random_target);
	if (unlikely(PTRISERR(r))) {
		uk_pr_err("Failed to create /dev/random: %d\n",  PTR2ERR(r));
		return PTR2ERR(r);
	}

#if CONFIG_LIBUKRANDOM_CMDLINE_SEED
	/* Skip /dev/hwrng there is no hardware device */
	if (driver == (void *)UK_SWRAND_DRIVER_NONE)
		return 0;
#endif /* CONFIG_LIBUKRANDOM_CMDLINE_SEED */

	r = uk_fs_createat(uk_fs_devfs_root, "hwrng", 5,
			   0666, O_EXCL, hwrng_target);
	if (unlikely(PTRISERR(r))) {
		uk_pr_err("Failed to create /dev/hwrng: %d\n",  PTR2ERR(r));
		return PTR2ERR(r);
	}

	return 0;
}

uk_rootfs_initcall_prio(init_ukrandom_devfs, 0, UK_FS_PRIO_FSAVAIL);
