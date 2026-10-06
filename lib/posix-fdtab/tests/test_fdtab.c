/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Kartik Lolla <kartiklolla.1@gmail.com> */

/**
 * Unit tests for posix-fdtab: the file descriptor table.
 *
 * Tests exercise the error-handling paths for the fd table's internal
 * syscall interface, verifying that invalid inputs are rejected without
 * crashing.  All tests use fd numbers that are negative, out of the
 * valid range, or known to be unmapped, so no live open file descriptor
 * is required.
 *
 * Return-value convention used throughout: uk_sys_* and uk_fdtab_*
 * functions return a negative errno on failure (eg. -EBADF).
 */

#include <fcntl.h>
#include<errno.h>

#include <uk/posix-fdtab.h>
#include <uk/test.h>

/*
 * I chose the file descriptor number 980 because it is
 * in the valid range of the fd numbers
 * (upto 1024) but is not mapped to any new files by default
 * so it is safe to assume that it is unmapped.
 */
#define TEST_UNMAPPED_FD 980

/* ---- uk_sys_close ---- */

UK_TESTCASE(posix_fdtab_suite, test_close_negative_fd)
{
	/* fd < 0 is always out of range */
	UK_TEST_EXPECT_SNUM_EQ(uk_sys_close(-1), -EBADF);
}

UK_TESTCASE(posix_fdtab_suite, test_close_out_of_range_fd)
{
	/* fd == MAXFDS is one past the end of the table */
	UK_TEST_EXPECT_SNUM_EQ(uk_sys_close(CONFIG_LIBPOSIX_FDTAB_MAXFDS), -EBADF);
}

UK_TESTCASE(posix_fdtab_suite, test_close_unmapped_fd)
{
	/* fd is in range but has never been opened */
	UK_TEST_EXPECT_SNUM_EQ(uk_sys_close(TEST_UNMAPPED_FD), -EBADF);
}

/* ---- uk_sys_dup ---- */

UK_TESTCASE(posix_fdtab_suite, test_dup_negative_fd)
{
	UK_TEST_EXPECT_SNUM_EQ(uk_sys_dup(-1), -EBADF);
}

UK_TESTCASE(posix_fdtab_suite, test_dup_out_of_range_fd)
{
	UK_TEST_EXPECT_SNUM_EQ(uk_sys_dup(CONFIG_LIBPOSIX_FDTAB_MAXFDS), -EBADF);
}

UK_TESTCASE(posix_fdtab_suite, test_dup_unmapped_fd)
{
	UK_TEST_EXPECT_SNUM_EQ(uk_sys_dup(TEST_UNMAPPED_FD), -EBADF);
}

/* ---- uk_sys_dup2 ---- */

UK_TESTCASE(posix_fdtab_suite, test_dup2_same_unmapped_fd)
{
	/*
	 * POSIX: dup2(fd, fd) returns fd if fd is open, EBADF if not.
	 * TEST_UNMAPPED_FD is not open, so we expect -EBADF.
	 */
	UK_TEST_EXPECT_SNUM_EQ(uk_sys_dup2(TEST_UNMAPPED_FD, TEST_UNMAPPED_FD), -EBADF);
}

UK_TESTCASE(posix_fdtab_suite, test_dup2_negative_oldfd)
{
	UK_TEST_EXPECT_SNUM_EQ(uk_sys_dup2(-1, 0), -EBADF);
}

UK_TESTCASE(posix_fdtab_suite, test_dup2_negative_newfd)
{
	UK_TEST_EXPECT_SNUM_EQ(uk_sys_dup2(0, -1), -EBADF);
}

/* ---- uk_sys_dup3 ---- */

UK_TESTCASE(posix_fdtab_suite, test_dup3_same_fd)
{
	/*
	 * dup3 with oldfd == newfd must return -EINVAL regardless of
	 * whether the fd is open (this differs from dup2 rules).
	 */
	UK_TEST_EXPECT_SNUM_EQ(uk_sys_dup3(1, 1, 0), -EINVAL);
}

UK_TESTCASE(posix_fdtab_suite, test_dup3_negative_oldfd)
{
	UK_TEST_EXPECT_SNUM_EQ(uk_sys_dup3(-1, 0, 0), -EBADF);
}

UK_TESTCASE(posix_fdtab_suite, test_dup3_negative_newfd)
{
	UK_TEST_EXPECT_SNUM_EQ(uk_sys_dup3(0, -1, 0), -EBADF);
}

UK_TESTCASE(posix_fdtab_suite, test_dup3_out_of_range_newfd)
{
	UK_TEST_EXPECT_SNUM_EQ(uk_sys_dup3(0, CONFIG_LIBPOSIX_FDTAB_MAXFDS, 0), -EBADF);
}

UK_TESTCASE(posix_fdtab_suite, test_dup3_invalid_flags)
{
	/*
	 * dup3 only accepts O_CLOEXEC in flags. Any other bit must
	 * return -EINVAL.  O_NONBLOCK is a valid file-status flag but
	 * is not meaningful for dup3.
	 */
	UK_TEST_EXPECT_SNUM_EQ(uk_sys_dup3(0, 5, O_NONBLOCK), -EINVAL);
}

/* ---- uk_sys_dup_min ---- */

UK_TESTCASE(posix_fdtab_suite, test_dup_min_negative_fd)
{
	UK_TEST_EXPECT_SNUM_EQ(uk_sys_dup_min(-1, 0, 0), -EBADF);
}

UK_TESTCASE(posix_fdtab_suite, test_dup_min_unmapped_fd)
{
	UK_TEST_EXPECT_SNUM_EQ(uk_sys_dup_min(TEST_UNMAPPED_FD, 0, 0), -EBADF);
}

UK_TESTCASE(posix_fdtab_suite, test_dup_min_invalid_flags)
{
	/* Same flag rule as dup3: only O_CLOEXEC is accepted */
	UK_TEST_EXPECT_SNUM_EQ(uk_sys_dup_min(0, 0, O_NONBLOCK), -EINVAL);
}

/* ---- uk_fdtab_get ---- */

UK_TESTCASE(posix_fdtab_suite, test_fdtab_get_negative_fd)
{
	UK_TEST_EXPECT_NULL(uk_fdtab_get(-1));
}

UK_TESTCASE(posix_fdtab_suite, test_fdtab_get_unmapped_fd)
{
	UK_TEST_EXPECT_NULL(uk_fdtab_get(TEST_UNMAPPED_FD));
}

/* ---- uk_fdtab_getflags ---- */

UK_TESTCASE(posix_fdtab_suite, test_fdtab_getflags_negative_fd)
{
	UK_TEST_EXPECT_SNUM_LT(uk_fdtab_getflags(-1), 0);
}

UK_TESTCASE(posix_fdtab_suite, test_fdtab_getflags_unmapped_fd)
{
	UK_TEST_EXPECT_SNUM_LT(uk_fdtab_getflags(TEST_UNMAPPED_FD), 0);
}

/* ---- uk_fdtab_setflags ---- */

UK_TESTCASE(posix_fdtab_suite, test_fdtab_setflags_negative_fd)
{
	UK_TEST_EXPECT_SNUM_LT(uk_fdtab_setflags(-1, 0), 0);
}

UK_TESTCASE(posix_fdtab_suite, test_fdtab_setflags_unmapped_fd)
{
	UK_TEST_EXPECT_SNUM_LT(uk_fdtab_setflags(TEST_UNMAPPED_FD, 0), 0);
}

UK_TESTCASE(posix_fdtab_suite, test_fdtab_setflags_invalid_flags)
{
	/*
	 * The flags check in uk_fdtab_setflags happens before the fd
	 * lookup, so -EINVAL is returned regardless of the fd value.
	 */
	UK_TEST_EXPECT_SNUM_EQ(uk_fdtab_setflags(0, O_NONBLOCK), -EINVAL);
}

uk_testsuite_register(posix_fdtab_suite, NULL);
