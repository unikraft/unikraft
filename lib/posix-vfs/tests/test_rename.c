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

/* TODO: import these into nolibc */
#ifndef RENAME_NOREPLACE
#define RENAME_NOREPLACE 1
#endif /* RENAME_NOREPLACE */

#ifndef RENAME_EXCHANGE
#define RENAME_EXCHANGE 2
#endif /* RENAME_EXCHANGE */

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

UK_TESTCASE(posix_vfs_rename_testsuite, posix_vfs_test_rename_file_over_file)
{
	if (unlikely(rn_skip))
		return;

	UK_TEST_EXPECT_ZERO(rn_mkdir(RN_ROOT "/ff"));
	UK_TEST_EXPECT_ZERO(rn_mkfile(RN_ROOT "/ff/a"));
	UK_TEST_EXPECT_ZERO(rn_mkfile(RN_ROOT "/ff/b"));

	/* A file may replace another file */
	UK_TEST_EXPECT_ZERO(uk_sys_rename(RN_ROOT "/ff/a", RN_ROOT "/ff/b"));
	UK_TEST_EXPECT(!rn_exists(RN_ROOT "/ff/a"));
	UK_TEST_EXPECT(rn_exists(RN_ROOT "/ff/b"));
}

UK_TESTCASE(posix_vfs_rename_testsuite, posix_vfs_test_rename_file_over_dir)
{
	if (unlikely(rn_skip))
		return;

	UK_TEST_EXPECT_ZERO(rn_mkdir(RN_ROOT "/fd"));
	UK_TEST_EXPECT_ZERO(rn_mkdir(RN_ROOT "/fd/full"));
	UK_TEST_EXPECT_ZERO(rn_mkfile(RN_ROOT "/fd/full/keep"));
	UK_TEST_EXPECT_ZERO(rn_mkdir(RN_ROOT "/fd/empty"));
	UK_TEST_EXPECT_ZERO(rn_mkfile(RN_ROOT "/fd/f"));

	/* A non-directory must never replace a directory, empty or not */
	UK_TEST_EXPECT_SNUM_EQ(uk_sys_rename(RN_ROOT "/fd/f",
					     RN_ROOT "/fd/full"),
			       -EISDIR);
	UK_TEST_EXPECT_SNUM_EQ(uk_sys_rename(RN_ROOT "/fd/f",
					     RN_ROOT "/fd/empty"),
			       -EISDIR);
	UK_TEST_EXPECT(rn_exists(RN_ROOT "/fd/f"));
	UK_TEST_EXPECT(rn_exists(RN_ROOT "/fd/full/keep"));
	UK_TEST_EXPECT(rn_exists(RN_ROOT "/fd/empty"));
}

UK_TESTCASE(posix_vfs_rename_testsuite, posix_vfs_test_rename_dir_over_file)
{
	if (unlikely(rn_skip))
		return;

	UK_TEST_EXPECT_ZERO(rn_mkdir(RN_ROOT "/df"));
	UK_TEST_EXPECT_ZERO(rn_mkdir(RN_ROOT "/df/d"));
	UK_TEST_EXPECT_ZERO(rn_mkfile(RN_ROOT "/df/d/keep"));
	UK_TEST_EXPECT_ZERO(rn_mkfile(RN_ROOT "/df/f"));

	/* A directory must never replace a non-directory */
	UK_TEST_EXPECT_SNUM_EQ(uk_sys_rename(RN_ROOT "/df/d",
					     RN_ROOT "/df/f"),
			       -ENOTDIR);
	UK_TEST_EXPECT(rn_exists(RN_ROOT "/df/d/keep"));
	UK_TEST_EXPECT(rn_exists(RN_ROOT "/df/f"));
}

UK_TESTCASE(posix_vfs_rename_testsuite, posix_vfs_test_rename_dir_over_dir)
{
	if (unlikely(rn_skip))
		return;

	UK_TEST_EXPECT_ZERO(rn_mkdir(RN_ROOT "/dd"));
	UK_TEST_EXPECT_ZERO(rn_mkdir(RN_ROOT "/dd/src"));
	UK_TEST_EXPECT_ZERO(rn_mkfile(RN_ROOT "/dd/src/s"));
	UK_TEST_EXPECT_ZERO(rn_mkdir(RN_ROOT "/dd/full"));
	UK_TEST_EXPECT_ZERO(rn_mkfile(RN_ROOT "/dd/full/keep"));
	UK_TEST_EXPECT_ZERO(rn_mkdir(RN_ROOT "/dd/empty"));

	/* A directory must not replace a non-empty directory */
	UK_TEST_EXPECT_SNUM_EQ(uk_sys_rename(RN_ROOT "/dd/src",
					     RN_ROOT "/dd/full"),
			       -ENOTEMPTY);
	UK_TEST_EXPECT(rn_exists(RN_ROOT "/dd/src/s"));
	UK_TEST_EXPECT(rn_exists(RN_ROOT "/dd/full/keep"));

	/* ... but may replace an empty one */
	UK_TEST_EXPECT_ZERO(uk_sys_rename(RN_ROOT "/dd/src",
					  RN_ROOT "/dd/empty"));
	UK_TEST_EXPECT(!rn_exists(RN_ROOT "/dd/src"));
	UK_TEST_EXPECT(rn_exists(RN_ROOT "/dd/empty/s"));
}

UK_TESTCASE(posix_vfs_rename_testsuite, posix_vfs_test_rename_dir_over_parent)
{
	if (unlikely(rn_skip))
		return;

	UK_TEST_EXPECT_ZERO(rn_mkdir(RN_ROOT "/par"));
	UK_TEST_EXPECT_ZERO(rn_mkdir(RN_ROOT "/par/a"));
	UK_TEST_EXPECT_ZERO(rn_mkdir(RN_ROOT "/par/a/b"));
	UK_TEST_EXPECT_ZERO(rn_mkfile(RN_ROOT "/par/a/f"));

	/* The destination is the (non-empty) parent of the source */
	UK_TEST_EXPECT_SNUM_EQ(uk_sys_rename(RN_ROOT "/par/a/b",
					     RN_ROOT "/par/a"),
			       -ENOTEMPTY);
	UK_TEST_EXPECT_SNUM_EQ(uk_sys_rename(RN_ROOT "/par/a/f",
					     RN_ROOT "/par/a"),
			       -EISDIR);
	UK_TEST_EXPECT(rn_exists(RN_ROOT "/par/a/b"));
	UK_TEST_EXPECT(rn_exists(RN_ROOT "/par/a/f"));
}

UK_TESTCASE(posix_vfs_rename_testsuite, posix_vfs_test_rename_dir_into_itself)
{
	if (unlikely(rn_skip))
		return;

	UK_TEST_EXPECT_ZERO(rn_mkdir(RN_ROOT "/loop"));
	UK_TEST_EXPECT_ZERO(rn_mkdir(RN_ROOT "/loop/a"));
	UK_TEST_EXPECT_ZERO(rn_mkdir(RN_ROOT "/loop/a/b"));
	UK_TEST_EXPECT_ZERO(rn_mkdir(RN_ROOT "/loop/a/b/c"));
	UK_TEST_EXPECT_ZERO(rn_mkfile(RN_ROOT "/loop/a/b/c/f"));

	/* A directory cannot be moved below itself, at any depth */
	UK_TEST_EXPECT_SNUM_EQ(uk_sys_rename(RN_ROOT "/loop/a",
					     RN_ROOT "/loop/a/x"),
			       -EINVAL);
	UK_TEST_EXPECT_SNUM_EQ(uk_sys_rename(RN_ROOT "/loop/a",
					     RN_ROOT "/loop/a/b/x"),
			       -EINVAL);
	UK_TEST_EXPECT_SNUM_EQ(uk_sys_rename(RN_ROOT "/loop/a",
					     RN_ROOT "/loop/a/b/c/x"),
			       -EINVAL);
	UK_TEST_EXPECT_SNUM_EQ(uk_sys_rename(RN_ROOT "/loop/a/b",
					     RN_ROOT "/loop/a/b/c/x"),
			       -EINVAL);

	/* The tree must be left untouched */
	UK_TEST_EXPECT(rn_exists(RN_ROOT "/loop/a/b/c/f"));
}

UK_TESTCASE(posix_vfs_rename_testsuite, posix_vfs_test_rename_dir_exchange_loop)
{
	if (unlikely(rn_skip))
		return;

	UK_TEST_EXPECT_ZERO(rn_mkdir(RN_ROOT "/xl"));
	UK_TEST_EXPECT_ZERO(rn_mkdir(RN_ROOT "/xl/a"));
	UK_TEST_EXPECT_ZERO(rn_mkdir(RN_ROOT "/xl/a/b"));
	UK_TEST_EXPECT_ZERO(rn_mkfile(RN_ROOT "/xl/a/b/f"));

	/* Exchanging a directory with one of its descendants is a loop, too */
	UK_TEST_EXPECT_SNUM_EQ(uk_sys_renameat(NULL, RN_ROOT "/xl/a", NULL,
					       RN_ROOT "/xl/a/b",
					       RENAME_EXCHANGE),
			       -EINVAL);
	UK_TEST_EXPECT_SNUM_EQ(uk_sys_renameat(NULL, RN_ROOT "/xl/a/b", NULL,
					       RN_ROOT "/xl/a",
					       RENAME_EXCHANGE),
			       -EINVAL);
	UK_TEST_EXPECT(rn_exists(RN_ROOT "/xl/a/b/f"));
}

UK_TESTCASE(posix_vfs_rename_testsuite, posix_vfs_test_rename_dir_move)
{
	if (unlikely(rn_skip))
		return;

	UK_TEST_EXPECT_ZERO(rn_mkdir(RN_ROOT "/mv"));
	UK_TEST_EXPECT_ZERO(rn_mkdir(RN_ROOT "/mv/a"));
	UK_TEST_EXPECT_ZERO(rn_mkdir(RN_ROOT "/mv/a/b"));
	UK_TEST_EXPECT_ZERO(rn_mkfile(RN_ROOT "/mv/a/b/f"));
	UK_TEST_EXPECT_ZERO(rn_mkdir(RN_ROOT "/mv/c"));

	/* Moving a directory up, down and sideways in the tree is fine */
	UK_TEST_EXPECT_ZERO(uk_sys_rename(RN_ROOT "/mv/a/b", RN_ROOT "/mv/b"));
	UK_TEST_EXPECT_ZERO(uk_sys_rename(RN_ROOT "/mv/b", RN_ROOT "/mv/c/b"));
	UK_TEST_EXPECT_ZERO(uk_sys_rename(RN_ROOT "/mv/c", RN_ROOT "/mv/a/c"));
	UK_TEST_EXPECT(rn_exists(RN_ROOT "/mv/a/c/b/f"));
	UK_TEST_EXPECT(rn_exists(RN_ROOT "/mv/a/c/b/../b/f"));
	UK_TEST_EXPECT(!rn_exists(RN_ROOT "/mv/c"));
}

UK_TESTCASE(posix_vfs_rename_testsuite, posix_vfs_test_rename_flags)
{
	if (unlikely(rn_skip))
		return;

	UK_TEST_EXPECT_ZERO(rn_mkdir(RN_ROOT "/flg"));
	UK_TEST_EXPECT_ZERO(rn_mkdir(RN_ROOT "/flg/d"));
	UK_TEST_EXPECT_ZERO(rn_mkfile(RN_ROOT "/flg/d/inner"));
	UK_TEST_EXPECT_ZERO(rn_mkfile(RN_ROOT "/flg/f"));

	/* RENAME_NOREPLACE fails on an existing destination */
	UK_TEST_EXPECT_SNUM_EQ(uk_sys_renameat(NULL, RN_ROOT "/flg/f", NULL,
					       RN_ROOT "/flg/d",
					       RENAME_NOREPLACE),
			       -EEXIST);

	/* RENAME_EXCHANGE may swap a file with a directory */
	UK_TEST_EXPECT_ZERO(uk_sys_renameat(NULL, RN_ROOT "/flg/f", NULL,
					    RN_ROOT "/flg/d",
					    RENAME_EXCHANGE));
	UK_TEST_EXPECT(rn_exists(RN_ROOT "/flg/f/inner"));
	UK_TEST_EXPECT(rn_exists(RN_ROOT "/flg/d"));
	UK_TEST_EXPECT(!rn_exists(RN_ROOT "/flg/d/inner"));
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
