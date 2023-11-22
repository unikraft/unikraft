/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Internal syscalls for file control operations */

#include <sys/ioctl.h>

#include <uk/arch/atomic.h>
#include <uk/posix-fdio.h>
#include <uk/print.h>

#include "fdio-impl.h"


int uk_sys_ioctl(struct uk_ofile *of, int cmd, void *arg)
{
	int r;
	int iolock;
	const struct uk_file *f;

	if (cmd == FIONBIO) {
		int val = *(int *)arg;

		if (val)
			ukarch_or(&of->mode, O_NONBLOCK);
		else
			ukarch_and(&of->mode, ~O_NONBLOCK);
		return 0;
	}

	iolock = _SHOULD_LOCK(of->mode);
	f = of->file;
	if (iolock)
		uk_file_wlock(f);
	r = uk_file_ctl(f, UKFILE_CTL_IOCTL, cmd, (uintptr_t)arg, 0, 0);
	if (iolock)
		uk_file_wunlock(f);
	return r;
}

/* File flags that fcntl can change, according to POSIX */
#define _FCNTL_MASK (O_APPEND|O_ASYNC|O_DIRECT|O_NOATIME|O_NONBLOCK)
/* File flags we actually support & care about changing */
#define _SETFL_MASK (_FCNTL_MASK & UKFD_MODE_MASK)

int uk_sys_fcntl(struct uk_ofile *of, int cmd, unsigned long arg)
{
	switch (cmd) {
	case F_GETFL:
		return of->mode & UKFD_MODE_MASK;
	case F_SETFL:
	{
		unsigned int mode = of->mode;
		unsigned int newmode;

		do {
			newmode = mode & ~_SETFL_MASK;
			newmode |= *((unsigned int *)arg) & _SETFL_MASK;
		} while (!ukarch_compare_exchange_n(&of->mode, &mode, newmode));
		return 0;
	}
	default:
		uk_pr_warn("STUB: fcntl(%d)\n", cmd);
		return -EINVAL;
	}
}

int uk_sys_flock(struct uk_ofile *of, int cmd)
{
	uk_pr_warn_once("STUB: flock\n");
	return -ENOSYS;
}

int uk_sys_fsync(struct uk_ofile *of)
{
	return uk_file_ctl(of->file, UKFILE_CTL_FILE, UKFILE_CTL_FILE_SYNC,
			   -1, 0, 0);
}

int uk_sys_fdatasync(struct uk_ofile *of)
{
	return uk_file_ctl(of->file, UKFILE_CTL_FILE, UKFILE_CTL_FILE_SYNC,
			   0, 0, 0);
}

int uk_sys_ftruncate(struct uk_ofile *of, off_t len)
{
	unsigned int mode;
	const struct uk_file *f;
	int r;
	int iolock;

	if (len < 0)
		return -EINVAL;

	mode = of->mode;
	if (!_CAN_WRITE(mode))
		return -EINVAL;

	iolock = _SHOULD_LOCK(mode);
	f = of->file;
	if (iolock)
		uk_file_wlock(f);
	r = uk_file_ctl(f, UKFILE_CTL_FILE, UKFILE_CTL_FILE_TRUNC,
			len, 0, 0);
	if (iolock)
		uk_file_wunlock(f);
	return r;
}

int uk_sys_fallocate(struct uk_ofile *of, int mode, off_t offset, off_t len)
{
	const struct uk_file *f;
	int r;
	int iolock;

	if (offset < 0 || len <= 0)
		return -EINVAL;

	iolock = _SHOULD_LOCK(of->mode);
	f = of->file;
	if (iolock)
		uk_file_wlock(f);
	r = uk_file_ctl(f, UKFILE_CTL_FILE, UKFILE_CTL_FILE_FALLOC,
			mode, offset, len);
	if (iolock)
		uk_file_wunlock(f);
	return r;
}

int uk_sys_fadvise(struct uk_ofile *of, off_t offset, off_t len, int advice)
{
	return uk_file_ctl(of->file, UKFILE_CTL_FILE, UKFILE_CTL_FILE_FADVISE,
			   offset, len, advice);
}
