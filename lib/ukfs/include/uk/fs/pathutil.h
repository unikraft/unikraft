/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Path utils */

#ifndef __UKFS_FS_PATHUTIL_H__
#define __UKFS_FS_PATHUTIL_H__

#include <stddef.h>
#include <string.h>

/**
 * Check if the first path component in string `path` of length `len` is ".".
 */
static inline
int uk_fs_path_isdot(const char *path, size_t len)
{
	return len >= 1 && path[0] == '.' && (len == 1 || path[1] == '/');
}

/**
 * Check if the first path component in string `path` of length `len` is "..".
 */
static inline
int uk_fs_path_isdotdot(const char *path, size_t len)
{
	return len >= 2 && path[0] == '.' && path[1] == '.' &&
	       (len == 2 || path[2] == '/');
}

/**
 * Return the offset of the next path separator ('/') in `path` of length `len`,
 * or `len` if no separator is found.
 *
 * @param path String containing path (does not need NUL terminator)
 * @param len Length of path
 *
 * @return
 *   < `len`: Offset of first path separator in `path`
 *  == `len`: Path separator not found
 */
static inline
size_t uk_fs_path_sep(const char *path, size_t len)
{
	const char *const p = memchr(path, '/', len);

	if (!p)
		return len;

	UK_ASSERT(p >= path);
	return (size_t)(p - path);
}

#endif /* __UKFS_FS_PATHUTIL_H__ */
