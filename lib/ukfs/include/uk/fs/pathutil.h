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
	return (size_t)(p - path);
}

/* Return tuple type combining a string length and position */
struct uk_fs_poslen {
	size_t pos;
	size_t len;
};

/**
 * Find path length and position of first separator (if any) in a single pass.
 *
 * If `ignore_lead` is non-zero, ignore any number of leading separators.
 *
 * @return
 *  .len = length of string `path`; same as reported by `strlen()`
 *  .pos = posision of first matching separator, or == .len if none found
 */
static inline
struct uk_fs_poslen uk_fs_path_len_lead(const char *path, int ignore_lead)
{
	size_t pos = 0;
	size_t len = 0;

	if (ignore_lead)
		while (path[len] == '/')
			len++;
	while (path[len]) {
		if (path[len] == '/')
			pos = len + 1;
		len++;
	}
	return (struct uk_fs_poslen){
		.pos = pos ? pos - 1 : len,
		.len = len
	};
}

/**
 * Find path length and position of last separator (if any) in a single pass.
 *
 * If `ignore_trail` is non-zero, ignore any number of trailing separators up to
 * a bare "/" path.
 *
 * @return
 *  .len = length of string `path`; same as reported by `strlen()`
 *  .pos = posision of last matching separator, or == .len if none found
 */
static inline
struct uk_fs_poslen uk_fs_path_len_trail(const char *path, int ignore_trail)
{
	size_t pos = 0;
	size_t len;

	for (len = 0; path[len]; len++)
		if (path[len] == '/')
			pos = len + 1;
	if (ignore_trail && len > 1 && pos == len) {
		size_t tmp = len - 1;

		while (tmp && path[tmp] == '/')
			tmp--;
		len = tmp + 1;
		pos = tmp;
		while (pos && path[pos - 1] != '/')
			pos--;
	}
	return (struct uk_fs_poslen){pos, len};
}

#endif /* __UKFS_FS_PATHUTIL_H__ */
