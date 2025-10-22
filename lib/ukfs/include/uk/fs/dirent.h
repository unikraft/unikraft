/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Utilities for handling directory entries to be used by filesystem drivers */

#ifndef __UKFS_FS_DIRENT_H__
#define __UKFS_FS_DIRENT_H__

#include <dirent.h>

#include <uk/fs.h>

/* Identical to libc struct dirent64, but with dynamically-sized name field */
struct uk_fs_dirent {
	ino_t d_ino;
	off_t d_off;
	unsigned short d_reclen;
	unsigned char d_type;
	char d_name[];
};

/* Asserts to ensure dirent64 & uk_fs_dirent have identical layouts in memory */
UK_CTASSERT(__alignof__(struct uk_fs_dirent) == __alignof__(struct dirent64));
UK_CTASSERT(__offsetof(struct uk_fs_dirent, d_ino) ==
	    __offsetof(struct dirent64, d_ino));
UK_CTASSERT(__offsetof(struct uk_fs_dirent, d_off) ==
	    __offsetof(struct dirent64, d_off));
UK_CTASSERT(__offsetof(struct uk_fs_dirent, d_reclen) ==
	    __offsetof(struct dirent64, d_reclen));
UK_CTASSERT(__offsetof(struct uk_fs_dirent, d_type) ==
	    __offsetof(struct dirent64, d_type));
UK_CTASSERT(__offsetof(struct uk_fs_dirent, d_name) ==
	    __offsetof(struct dirent64, d_name));

/**
 * Compute the record length for a dirent64 with name of length `namelen`.
 *
 * This takes into account the terminating NUL in the file name as well as
 * alignment requirements for a following dirent64.
 */
#define UKFS_DIRENT_RECLEN(namelen) \
	ALIGN_UP(sizeof(struct uk_fs_dirent) + (namelen) + 1, \
		 __alignof__(struct dirent64))

/**
 * Compute the length of the string in the `d_name` field, using `d_reclen`.
 *
 * Assumes a single terminating NUL, and arbitrary padding bytes up to the
 * alignment requirements for a dirent.
 */
static inline
size_t uk_fs_dirent_namelen(struct uk_fs_dirent *dp)
{
	size_t len;

	UK_ASSERT(IS_ALIGNED(dp->d_reclen, __alignof__(struct dirent64)));

	/* Start off with the maximum length according to the dirent */
	len = dp->d_reclen - __offsetof(struct uk_fs_dirent, d_name);
	if (len < __alignof__(struct dirent64))
		/* String is short, just start from the beginning */
		len = 0;
	else
		/* Roll back one alignment to guarantee we're in the string */
		len -= __alignof__(struct uk_fs_dirent);

	/* Advance until terminating NUL */
	while (dp->d_name[len] != '\0')
		len++;
	return len;
}

/**
 * Get the next dirent after entry `dp` of size `sz`.
 */
#define UKFS_DIRENT_NEXT(dp, sz) \
	((struct uk_fs_dirent *)((char *)(dp) + (sz)))

/**
 * Output a record for entry "." into `out` with inode number `inode`.
 *
 * This macro fills in only the essential fields; drivers are free to write out
 * any additional dirent fields they wish.
 */
#define UKFS_DIRENT_OUT_DOT(out, inode) \
do { \
	(out)->d_ino = (inode); \
	(out)->d_reclen = UKFS_DIRENT_RECLEN(1); \
	(out)->d_type = DT_DIR; \
	(out)->d_name[0] = '.'; \
	(out)->d_name[1] = '\0'; \
} while (0)

/**
 * Output a record for entry ".." into `out` with inode number `inode`.
 *
 * This macro fills in only the essential fields; drivers are free to write out
 * any additional dirent fields they wish.
 */
#define UKFS_DIRENT_OUT_DOTDOT(out, inode) \
do { \
	(out)->d_ino = (inode); \
	(out)->d_reclen = UKFS_DIRENT_RECLEN(2); \
	(out)->d_type = DT_DIR; \
	(out)->d_name[0] = '.'; \
	(out)->d_name[1] = '.'; \
	(out)->d_name[2] = '\0'; \
} while (0)

#endif /* __UKFS_FS_DIRENT_H__ */
