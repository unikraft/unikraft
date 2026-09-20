/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <sys/mount.h>
#include <uk/errptr.h>
#include <uk/posix-fd.h>
#include <uk/posix-vfs.h>
#include <uk/test.h>

/* All test cases operate below this directory of the (writable) root fs */
#define RN_ROOT "/uktest-rename"

/* Set if the tests cannot run because there is no writable root filesystem */
static int rn_skip;

static int rn_mkdir(const char *path)
{
	return uk_sys_mkdir(path, 0755);
}

static int rn_mkfile(const char *path)
{
	struct uk_ofile *of;

	of = uk_sys_openat(NULL, path, O_CREAT | O_EXCL | O_RDWR, 0644);
	if (PTRISERR(of))
		return PTR2ERR(of);
	uk_ofile_release(of);
	return 0;
}

static int rn_exists(const char *path)
{
	return uk_sys_access(path, F_OK) == 0;
}

/* Mount a new RAMfs instance on the new directory `path` */
static int rn_mount(const char *path, unsigned long flags)
{
	int r;

	r = rn_mkdir(path);
	if (unlikely(r))
		return r;
	return uk_sys_mount("none", path, "ramfs", flags, NULL);
}

static int rn_suite_init(struct uk_testsuite *suite __unused)
{
	int r = rn_mkdir(RN_ROOT);

	/* Failing the init would abort the boot; skip the test cases instead */
	if (unlikely(r && r != -EEXIST)) {
		uk_test_printf("Skipping rename tests, no writable root: %d\n",
			       r);
		rn_skip = 1;
	}
	return 0;
}

UK_TESTCASE(posix_vfs_rename_testsuite, posix_vfs_test_rename_same_mount)
{
	if (unlikely(rn_skip))
		return;

	UK_TEST_EXPECT_ZERO(rn_mkdir(RN_ROOT "/same"));
	UK_TEST_EXPECT_ZERO(rn_mkdir(RN_ROOT "/same/a"));
	UK_TEST_EXPECT_ZERO(rn_mkdir(RN_ROOT "/same/b"));
	UK_TEST_EXPECT_ZERO(rn_mkfile(RN_ROOT "/same/a/f"));

	/* Renaming between directories of a single mount must succeed */
	UK_TEST_EXPECT_ZERO(uk_sys_rename(RN_ROOT "/same/a/f",
					  RN_ROOT "/same/b/g"));
	UK_TEST_EXPECT(!rn_exists(RN_ROOT "/same/a/f"));
	UK_TEST_EXPECT(rn_exists(RN_ROOT "/same/b/g"));
}

UK_TESTCASE(posix_vfs_rename_testsuite, posix_vfs_test_rename_exdev_mount)
{
	if (unlikely(rn_skip))
		return;

	UK_TEST_EXPECT_ZERO(rn_mkdir(RN_ROOT "/xdev"));
	UK_TEST_EXPECT_ZERO(rn_mount(RN_ROOT "/xdev/mnt", 0));
	UK_TEST_EXPECT_ZERO(rn_mkfile(RN_ROOT "/xdev/f"));
	UK_TEST_EXPECT_ZERO(rn_mkfile(RN_ROOT "/xdev/mnt/g"));

	/* Renaming across mounts must fail in both directions */
	UK_TEST_EXPECT_SNUM_EQ(uk_sys_rename(RN_ROOT "/xdev/f",
					     RN_ROOT "/xdev/mnt/f"),
			       -EXDEV);
	UK_TEST_EXPECT(rn_exists(RN_ROOT "/xdev/f"));
	UK_TEST_EXPECT(!rn_exists(RN_ROOT "/xdev/mnt/f"));

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_rename(RN_ROOT "/xdev/mnt/g",
					     RN_ROOT "/xdev/g"),
			       -EXDEV);
	UK_TEST_EXPECT(rn_exists(RN_ROOT "/xdev/mnt/g"));
	UK_TEST_EXPECT(!rn_exists(RN_ROOT "/xdev/g"));

	UK_TEST_EXPECT_ZERO(uk_sys_umount(RN_ROOT "/xdev/mnt", 0));
}

UK_TESTCASE(posix_vfs_rename_testsuite, posix_vfs_test_rename_exdev_rdonly)
{
	if (unlikely(rn_skip))
		return;

	UK_TEST_EXPECT_ZERO(rn_mkdir(RN_ROOT "/rofs"));
	UK_TEST_EXPECT_ZERO(rn_mount(RN_ROOT "/rofs/ro", MS_RDONLY));
	UK_TEST_EXPECT_ZERO(rn_mkfile(RN_ROOT "/rofs/f"));

	/* A read-only mount must not gain entries through rename */
	UK_TEST_EXPECT_SNUM_EQ(uk_sys_rename(RN_ROOT "/rofs/f",
					     RN_ROOT "/rofs/ro/f"),
			       -EXDEV);
	UK_TEST_EXPECT(rn_exists(RN_ROOT "/rofs/f"));
	UK_TEST_EXPECT(!rn_exists(RN_ROOT "/rofs/ro/f"));

	UK_TEST_EXPECT_ZERO(uk_sys_umount(RN_ROOT "/rofs/ro", 0));
}

UK_TESTCASE(posix_vfs_rename_testsuite, posix_vfs_test_rename_exdev_bind)
{
	if (unlikely(rn_skip))
		return;

	UK_TEST_EXPECT_ZERO(rn_mkdir(RN_ROOT "/bind"));
	UK_TEST_EXPECT_ZERO(rn_mkdir(RN_ROOT "/bind/src"));
	UK_TEST_EXPECT_ZERO(rn_mkdir(RN_ROOT "/bind/dst"));
	UK_TEST_EXPECT_ZERO(rn_mkfile(RN_ROOT "/bind/f"));
	UK_TEST_EXPECT_ZERO(uk_sys_mount(RN_ROOT "/bind/src",
					 RN_ROOT "/bind/dst", NULL, MS_BIND,
					 NULL));

	/* Bind mounts are distinct mounts, so rename must not cross them */
	UK_TEST_EXPECT_SNUM_EQ(uk_sys_rename(RN_ROOT "/bind/f",
					     RN_ROOT "/bind/dst/f"),
			       -EXDEV);
	UK_TEST_EXPECT(rn_exists(RN_ROOT "/bind/f"));

	UK_TEST_EXPECT_ZERO(uk_sys_umount(RN_ROOT "/bind/dst", 0));
}

uk_testsuite_register(posix_vfs_rename_testsuite, rn_suite_init);
