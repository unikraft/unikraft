/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Internal syscalls for manipulating file metadata */

#include <string.h>
#include <sys/sysmacros.h>

#include <uk/posix-fdio.h>
#include <uk/posix-time.h>
#include <uk/timeutil.h>

#include "fdio-impl.h"

#define statx_time(ts) ((struct uk_statx_timestamp){ \
	.tv_sec = (ts).tv_sec, \
	.tv_nsec = (ts).tv_nsec \
})

static inline
void timecpy(struct timespec *d, const struct uk_statx_timestamp *s)
{
	d->tv_sec = s->tv_sec;
	d->tv_nsec = s->tv_nsec;
}

void uk_fdio_statx_cpyout(struct stat *s, const struct uk_statx *sx)
{
	unsigned int mask = sx->stx_mask;

	memset(s, 0, sizeof(*s));
	s->st_dev = makedev(sx->stx_dev_major, sx->stx_dev_minor);
	s->st_rdev = makedev(sx->stx_rdev_major, sx->stx_rdev_minor);
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
	if (unlikely(!statxbuf))
		return -EFAULT;
	if (unlikely(mask & UK_STATX__RESERVED))
		return -EINVAL;

	return uk_file_getstat(of->file, mask, statxbuf);
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
	uk_fdio_statx_cpyout(statbuf, &statx);
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
	unsigned int mask = 0;
	const int iolock = _SHOULD_LOCK(of->mode);

	if (owner != (uid_t)-1)
		mask |= UK_STATX_UID;
	if (group != (gid_t)-1)
		mask |= UK_STATX_GID;
	if (iolock)
		uk_file_wlock(of->file);
	r = uk_file_setstat(of->file, mask, &(const struct uk_statx){
		.stx_uid = owner,
		.stx_gid = group
	});
	if (iolock)
		uk_file_wunlock(of->file);
	return r;
}

int uk_sys_futimens(struct uk_ofile *of, const struct timespec *times)
{
	int r;
	unsigned int mask;
	struct uk_statx sx;
	struct timespec t;
	const int iolock = _SHOULD_LOCK(of->mode);

	/* TODO: check if uid is root or file owner for custom times */

	if (!times ||
	    times[0].tv_nsec == UTIME_NOW || times[1].tv_nsec == UTIME_NOW) {
		r = uk_sys_clock_gettime(CLOCK_REALTIME, &t);
		if (unlikely(r)) {
			UK_ASSERT(r < 0);
			/* The futime(n)s manpages do not specify an error case
			 * for failing to get the current time, and failure of
			 * the above call may indicate a misconfigured kernel.
			 * Print a warning before erroring out.
			 */
			uk_pr_warn("Failed to get current time: %d\n", r);
			return r;
		}
	}

	if (!times) {
		mask = UK_STATX_ATIME | UK_STATX_MTIME;
		sx.stx_atime = statx_time(t);
		sx.stx_mtime = statx_time(t);
	} else {
		mask = 0;
		if (times[0].tv_nsec != UTIME_OMIT) {
			mask |= UK_STATX_ATIME;
			if (times[0].tv_nsec == UTIME_NOW)
				sx.stx_atime = statx_time(t);
			else
				sx.stx_atime = statx_time(times[0]);
		}
		if (times[1].tv_nsec != UTIME_OMIT) {
			mask |= UK_STATX_MTIME;
			if (times[1].tv_nsec == UTIME_NOW)
				sx.stx_mtime = statx_time(t);
			else
				sx.stx_mtime = statx_time(times[1]);
		}
	}

	if (iolock)
		uk_file_wlock(of->file);
	r = uk_file_setstat(of->file, mask, &sx);
	if (iolock)
		uk_file_wunlock(of->file);
	return r;
}

int uk_sys_futimes(struct uk_ofile *of, const struct timeval *tv)
{
	return uk_sys_futimens(of, tv ? &uk_time_spec_from_val(tv) : NULL);
}
