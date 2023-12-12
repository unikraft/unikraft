/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Internal syscalls for manipulating file metadata */

#include <string.h>

#include <uk/posix-fdio.h>

#include "fdio-impl.h"

static inline
dev_t nums2dev(__u32 major, __u32 minor)
{
	return ((dev_t)major << 32) | minor;
}

static inline
void timecpy(struct timespec *d, const struct uk_statx_timestamp *s)
{
	d->tv_sec = s->tv_sec;
	d->tv_nsec = s->tv_nsec;
}

static inline
void statx_cpyout(struct stat *s, const struct uk_statx *sx)
{
	unsigned int mask = sx->stx_mask;

	memset(s, 0, sizeof(*s));
	s->st_dev = nums2dev(sx->stx_dev_major, sx->stx_dev_minor);
	s->st_rdev = nums2dev(sx->stx_rdev_major, sx->stx_rdev_minor);
	s->st_blksize = sx->stx_blksize;
	if (mask & UK_STATX_INO)
		s->st_ino = sx->stx_ino;
	if (mask & (UK_STATX_TYPE|UK_STATX_MODE))
		s->st_mode = sx->stx_mode;
	if (mask & UK_STATX_NLINK)
		s->st_nlink = sx->stx_nlink;
	if (mask & UK_STATX_UID)
		s->st_uid = sx->stx_uid;
	if (mask & UK_STATX_GID)
		s->st_gid = sx->stx_gid;
	if (mask & UK_STATX_SIZE)
		s->st_size = sx->stx_size;
	if (mask & UK_STATX_BLOCKS)
		s->st_blocks = sx->stx_blocks;
	if (mask & UK_STATX_ATIME)
		timecpy(&s->st_atim, &sx->stx_atime);
	if (mask & UK_STATX_MTIME)
		timecpy(&s->st_mtim, &sx->stx_mtime);
	if (mask & UK_STATX_CTIME)
		timecpy(&s->st_ctim, &sx->stx_ctime);
}


int uk_sys_fstatx(struct uk_ofile *of, unsigned int mask,
		  struct uk_statx *statxbuf)
{
	int ret;
	int iolock;

	if (unlikely(!statxbuf))
		return -EFAULT;
	if (unlikely(mask & UK_STATX__RESERVED))
		return -EINVAL;

	iolock = _SHOULD_LOCK(of->mode);
	if (iolock)
		uk_file_rlock(of->file);
	ret = uk_file_getstat(of->file, mask, statxbuf);
	if (iolock)
		uk_file_runlock(of->file);
	return ret;
}

int uk_sys_fstat(struct uk_ofile *of, struct stat *statbuf)
{
	int r;
	struct uk_statx statx;

	if (unlikely(!statbuf))
		return -EFAULT;

	r = uk_sys_fstatx(of, UK_STATX_BASIC_STATS, &statx);
	if (unlikely(r))
		return r;
	statx_cpyout(statbuf, &statx);
	return 0;
}

int uk_sys_fchmod(struct uk_ofile *of, mode_t mode)
{
	int r;
	const int iolock = _SHOULD_LOCK(of->mode);

	if (iolock)
		uk_file_wlock(of->file);
	r = uk_file_setstat(of->file, UK_STATX_MODE, &(const struct uk_statx){
		.stx_mode = mode
	});
	if (iolock)
		uk_file_wunlock(of->file);
	return r;
}

int uk_sys_fchown(struct uk_ofile *of, uid_t owner, gid_t group)
{
	int r;
	const int iolock = _SHOULD_LOCK(of->mode);

	if (iolock)
		uk_file_wlock(of->file);
	r = uk_file_setstat(of->file, UK_STATX_UID|UK_STATX_GID,
		&(const struct uk_statx){
			.stx_uid = owner,
			.stx_gid = group
	});
	if (iolock)
		uk_file_wunlock(of->file);
	return r;
}
