/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Kartik Lolla <kartiklolla.1@gmail.com> */

/**
 * Unit tests for posix-fdio: file I/O and control internal syscalls.
 *
 * Tests cover the error-handling paths for the internal syscall
 * interface, verifying that invalid inputs are rejected without
 * crashing.  All tests use a minimal stack-allocated uk_ofile with
 * only the .mode field set. No live file descriptor is required.
 *
 * Return-value convention: uk_sys_* functions return a negative errno
 * on failure (e.g. -EINVAL, -EFAULT, -ESPIPE).
 */

#include <errno.h>
#include <fcntl.h>

#include <uk/posix-fd.h>
#include <uk/posix-fdio.h>
#include <uk/test.h>

/*
 * RWF_NOWAIT is defined locally in fdio.c and has no shared header.
 * Used the same value here so the preadv2 test can reference it.
 */
#ifndef RWF_NOWAIT
#define RWF_NOWAIT	0x08
#endif /* RWF_NOWAIT */

/* ---- uk_sys_preadv ---- */

UK_TESTCASE(posix_fdio_suite, test_preadv_wronly_mode)
{
	/*
	 * O_WRONLY open file is not readable;
	 * returns -EINVAL before touching of->file.
	 */
	struct uk_ofile of = { .mode = O_WRONLY };

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_preadv(&of, NULL, 0, 0), -EINVAL);
}

UK_TESTCASE(posix_fdio_suite, test_preadv_negative_iovcnt)
{
	struct uk_ofile of = { .mode = 0 };

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_preadv(&of, NULL, -1, 0), -EINVAL);
}

UK_TESTCASE(posix_fdio_suite, test_preadv_null_iov_nonzero_cnt)
{
	/* NULL iov with non-zero iovcnt must return -EFAULT */
	struct uk_ofile of = { .mode = 0 };

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_preadv(&of, NULL, 1, 0), -EFAULT);
}

UK_TESTCASE(posix_fdio_suite, test_preadv_negative_offset)
{
	struct uk_ofile of = { .mode = 0 };

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_preadv(&of, NULL, 0, -1), -EINVAL);
}

/* ---- uk_sys_readv ---- */

UK_TESTCASE(posix_fdio_suite, test_readv_wronly_mode)
{
	struct uk_ofile of = { .mode = O_WRONLY };

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_readv(&of, NULL, 0), -EINVAL);
}

UK_TESTCASE(posix_fdio_suite, test_readv_negative_iovcnt)
{
	struct uk_ofile of = { .mode = 0 };

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_readv(&of, NULL, -1), -EINVAL);
}

UK_TESTCASE(posix_fdio_suite, test_readv_null_iov_nonzero_cnt)
{
	struct uk_ofile of = { .mode = 0 };

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_readv(&of, NULL, 1), -EFAULT);
}

/* ---- uk_sys_preadv2 ---- */

UK_TESTCASE(posix_fdio_suite, test_preadv2_nowait_flag)
{
	/*
	 * RWF_NOWAIT is an unsupported flag. The check fires before
	 * any field of 'of' is accessed.
	 */
	struct uk_ofile of = { .mode = 0 };
	ssize_t r;

	r = uk_sys_preadv2(&of, NULL, 0, 0, RWF_NOWAIT);
	UK_TEST_EXPECT_SNUM_EQ(r, -EINVAL);
}

UK_TESTCASE(posix_fdio_suite, test_preadv2_wronly_mode)
{
	struct uk_ofile of = { .mode = O_WRONLY };

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_preadv2(&of, NULL, 0, 0, 0), -EINVAL);
}

UK_TESTCASE(posix_fdio_suite, test_preadv2_negative_iovcnt)
{
	struct uk_ofile of = { .mode = 0 };

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_preadv2(&of, NULL, -1, 0, 0), -EINVAL);
}

UK_TESTCASE(posix_fdio_suite, test_preadv2_null_iov_nonzero_cnt)
{
	struct uk_ofile of = { .mode = 0 };

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_preadv2(&of, NULL, 1, 0, 0), -EFAULT);
}

UK_TESTCASE(posix_fdio_suite, test_preadv2_negative_offset)
{
	struct uk_ofile of = { .mode = 0 };

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_preadv2(&of, NULL, 0, -2, 0), -EINVAL);
}

/* ---- uk_sys_pwritev ---- */

UK_TESTCASE(posix_fdio_suite, test_pwritev_rdonly_mode)
{
	/* O_RDONLY (in mode 0) cannot write: gives -EINVAL */
	struct uk_ofile of = { .mode = 0 };

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_pwritev(&of, NULL, 0, 0), -EINVAL);
}

UK_TESTCASE(posix_fdio_suite, test_pwritev_negative_iovcnt)
{
	struct uk_ofile of = { .mode = O_RDWR };

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_pwritev(&of, NULL, -1, 0), -EINVAL);
}

UK_TESTCASE(posix_fdio_suite, test_pwritev_null_iov_nonzero_cnt)
{
	struct uk_ofile of = { .mode = O_RDWR };

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_pwritev(&of, NULL, 1, 0), -EFAULT);
}

UK_TESTCASE(posix_fdio_suite, test_pwritev_negative_offset)
{
	struct uk_ofile of = { .mode = O_RDWR };

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_pwritev(&of, NULL, 0, -1), -EINVAL);
}

/* ---- uk_sys_writev ---- */

UK_TESTCASE(posix_fdio_suite, test_writev_rdonly_mode)
{
	struct uk_ofile of = { .mode = 0 };

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_writev(&of, NULL, 0), -EINVAL);
}

UK_TESTCASE(posix_fdio_suite, test_writev_negative_iovcnt)
{
	struct uk_ofile of = { .mode = O_RDWR };

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_writev(&of, NULL, -1), -EINVAL);
}

UK_TESTCASE(posix_fdio_suite, test_writev_null_iov_nonzero_cnt)
{
	struct uk_ofile of = { .mode = O_RDWR };

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_writev(&of, NULL, 1), -EFAULT);
}

/* ---- uk_sys_pwritev2 ---- */

UK_TESTCASE(posix_fdio_suite, test_pwritev2_rdonly_mode)
{
	struct uk_ofile of = { .mode = 0 };

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_pwritev2(&of, NULL, 0, 0, 0), -EINVAL);
}

UK_TESTCASE(posix_fdio_suite, test_pwritev2_negative_iovcnt)
{
	struct uk_ofile of = { .mode = O_RDWR };

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_pwritev2(&of, NULL, -1, 0, 0), -EINVAL);
}

UK_TESTCASE(posix_fdio_suite, test_pwritev2_null_iov_nonzero_cnt)
{
	struct uk_ofile of = { .mode = O_RDWR };

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_pwritev2(&of, NULL, 1, 0, 0), -EFAULT);
}

UK_TESTCASE(posix_fdio_suite, test_pwritev2_negative_offset)
{
	struct uk_ofile of = { .mode = O_RDWR };

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_pwritev2(&of, NULL, 0, -1, 0), -EINVAL);
}

/* ---- uk_sys_lseek ---- */

UK_TESTCASE(posix_fdio_suite, test_lseek_invalid_whence)
{
	/*
	 * Whence must be SEEK_SET, SEEK_CUR, or SEEK_END.
	 * Any other value returns -EINVAL before of->mode is read.
	 */
	struct uk_ofile of = { .mode = 0 };

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_lseek(&of, 0, -1), -EINVAL);
}

UK_TESTCASE(posix_fdio_suite, test_lseek_non_seekable)
{
	/* UKFD_O_NOSEEK marks pipes/sockets. Seeking returns -ESPIPE */
	struct uk_ofile of = { .mode = UKFD_O_NOSEEK };

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_lseek(&of, 0, SEEK_SET), -ESPIPE);
}

/* ---- uk_sys_fstatx ---- */

UK_TESTCASE(posix_fdio_suite, test_fstatx_null_buf)
{
	/* NULL statxbuf returns -EFAULT before of->file is used */
	struct uk_ofile of = { .mode = 0 };

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_fstatx(&of, 0, NULL), -EFAULT);
}

UK_TESTCASE(posix_fdio_suite, test_fstatx_reserved_mask)
{
	/*
	 * Any mask bit in UK_STATX__RESERVED must return -EINVAL
	 * before of->file is used.
	 */
	struct uk_statx sx = {0};
	struct uk_ofile of = { .mode = 0 };
	int r;

	r = uk_sys_fstatx(&of, UK_STATX__RESERVED, &sx);
	UK_TEST_EXPECT_SNUM_EQ(r, -EINVAL);
}

/* ---- uk_sys_fstat ---- */

UK_TESTCASE(posix_fdio_suite, test_fstat_null_buf)
{
	/* NULL statbuf returns -EFAULT before of->file is used */
	struct uk_ofile of = { .mode = 0 };

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_fstat(&of, NULL), -EFAULT);
}

/* ---- uk_sys_fcntl ---- */

UK_TESTCASE(posix_fdio_suite, test_fcntl_getlk_null_arg)
{
	/*
	 * F_GETLK with a NULL flock pointer must return -EFAULT
	 * before of->file is touched.
	 */
	struct uk_ofile of = { .mode = 0 };

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_fcntl(&of, F_GETLK, 0), -EFAULT);
}

UK_TESTCASE(posix_fdio_suite, test_fcntl_unknown_cmd)
{
	/* Default branch: unrecognised cmd returns -EINVAL */
	struct uk_ofile of = { .mode = 0 };

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_fcntl(&of, 0x7fff, 0), -EINVAL);
}

/* ---- uk_sys_flock ---- */

UK_TESTCASE(posix_fdio_suite, test_flock_invalid_cmd)
{
	/*
	 * uk_sys_flock does not use 'of' at all (it is a stub).
	 * Any cmd outside LOCK_SH/EX/UN (with or without NB)
	 * reaches the default branch and returns -EINVAL.
	 */
	struct uk_ofile of = { .mode = 0 };

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_flock(&of, 0), -EINVAL);
}

/* ---- uk_sys_ftruncate ---- */

UK_TESTCASE(posix_fdio_suite, test_ftruncate_negative_len)
{
	/* len < 0 is rejected before of->mode is read */
	struct uk_ofile of = { .mode = 0 };

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_ftruncate(&of, -1), -EINVAL);
}

UK_TESTCASE(posix_fdio_suite, test_ftruncate_rdonly_mode)
{
	/* Read-only open file cannot be truncated */
	struct uk_ofile of = { .mode = 0 };

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_ftruncate(&of, 0), -EINVAL);
}

/* ---- uk_sys_fallocate ---- */

UK_TESTCASE(posix_fdio_suite, test_fallocate_negative_offset)
{
	struct uk_ofile of = { .mode = 0 };

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_fallocate(&of, 0, -1, 1), -EINVAL);
}

UK_TESTCASE(posix_fdio_suite, test_fallocate_zero_len)
{
	/* len == 0 violates the len > 0 requirement */
	struct uk_ofile of = { .mode = 0 };

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_fallocate(&of, 0, 0, 0), -EINVAL);
}

UK_TESTCASE(posix_fdio_suite, test_fallocate_negative_len)
{
	struct uk_ofile of = { .mode = 0 };

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_fallocate(&of, 0, 0, -1), -EINVAL);
}

uk_testsuite_register(posix_fdio_suite, NULL);
