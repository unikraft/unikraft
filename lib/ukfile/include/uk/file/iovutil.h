/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Convenience operations for dealing with iovec */

#ifndef __UKFILE_IOVUTIL_H__
#define __UKFILE_IOVUTIL_H__

#include <string.h>
#include <sys/uio.h>

#include <uk/essentials.h>

/**
 * Count the number of bytes described by `iov[iovcnt]`.
 *
 * @return Total number of bytes described by `iov[iovcnt]`
 */
static inline
size_t uk_iov_len(const struct iovec *iov, size_t iovcnt)
{
	size_t total = 0;

	for (size_t i = 0; i < iovcnt; i++)
		total += iov[i].iov_len;
	return total;
}

/**
 * Count the number of bytes remaining in `iov[iovcnt]` starting at
 * `iov[iovi][cur]`.
 *
 * If `iovi` is in bounds (< `iovcnt`), `cur` must also be in bounds
 * (<= `iov[iovi].iov_len`).
 *
 * @return Total number of bytes remaining
 */
static inline
size_t uk_iov_remaining(const struct iovec *iov, size_t iovcnt,
			size_t iovi, size_t cur)
{
	if (iovi < iovcnt)
		return uk_iov_len(iov + iovi, iovcnt - iovi) - cur;
	return 0;
}

/**
 * Fast-forward the iovec cursors `iovip` and `curp` by at most `len` bytes.
 *
 * If `*iovip` is in bounds (< `iovcnt`), `*curp` must also be in bounds
 * (<= `iov[*iovip].iov_len`).
 *
 * @return Number of bytes actually fast-forwarded by
 */
static inline
size_t uk_iov_ff(const struct iovec *iov, size_t iovcnt, size_t len,
		 size_t *iovip, size_t *curp)
{
	size_t ret = 0;
	size_t i = *iovip;
	size_t cur = *curp;

	while (i < iovcnt && len) {
		const size_t sz = MIN(len, iov[i].iov_len - cur);

		UK_ASSERT(cur <= iov[i].iov_len);
		len -= sz;
		cur += sz;
		ret += sz;
		if (cur == iov[i].iov_len) {
			i++;
			cur = 0;
		}
	}
	*iovip = i;
	*curp = cur;
	return ret;
}

/**
 * Zero out at most `len` bytes in memory regions described by `iov[iovcnt]`,
 * starting at `*curp` offset from the buffer at `iov[*iovip]`.
 *
 * If `*iovip` is in bounds (< `iovcnt`), `*curp` must also be in bounds
 * (<= `iov[*iovip].iov_len`).
 *
 * If the total remaining space in `iov` is less than `len`, exit early.
 * After the call, `*iovip` and `*curp` are updated with the new positions.
 *
 * @return Number of bytes zeroed
 */
static inline
size_t uk_iov_zero(const struct iovec *iov, size_t iovcnt, size_t len,
		   size_t *iovip, size_t *curp)
{
	size_t ret = 0;
	size_t i = *iovip;
	size_t cur = *curp;

	while (i < iovcnt && len) {
		const size_t sz = MIN(len, iov[i].iov_len - cur);

		UK_ASSERT(cur <= iov[i].iov_len); /* cur must be in bounds */
		UK_ASSERT(sz || cur == iov[i].iov_len); /* must make progress */
		if (sz) {
			memset((char *)iov[i].iov_base + cur, 0, sz);
			ret += sz;
			len -= sz;
			cur += sz;
		}
		if (cur == iov[i].iov_len) {
			i++;
			cur = 0;
		}
	}

	*iovip = i;
	*curp = cur;
	return ret;
}

/**
 * Copy at most `len` bytes from `buf` into memory regions described by
 * `iov[iovcnt]`, starting at `*curp` offset from the buffer at `iov[*iovip]`.
 *
 * If `*iovip` is in bounds (< `iovcnt`), `*curp` must also be in bounds
 * (<= `iov[*iovip].iov_len`).
 *
 * If the total remaining space in `iov` is less than `len`, exit early.
 * After the call, `*iovip` and `*curp` are updated with the new positions.
 *
 * @return Number of bytes copied
 */
static inline
size_t uk_iov_scatter(const struct iovec *iov, size_t iovcnt, const char *buf,
		      size_t len, size_t *iovip, size_t *curp)
{
	size_t ret = 0;
	size_t i = *iovip;
	size_t cur = *curp;

	while (i < iovcnt && len) {
		const size_t sz = MIN(len, iov[i].iov_len - cur);

		UK_ASSERT(cur <= iov[i].iov_len); /* cur must be in bounds */
		UK_ASSERT(sz || cur == iov[i].iov_len); /* must make progress */
		if (sz) {
			memcpy((char *)iov[i].iov_base + cur, buf, sz);
			ret += sz;
			buf += sz;
			len -= sz;
			cur += sz;
		}
		if (cur == iov[i].iov_len) {
			i++;
			cur = 0;
		}
	}

	*iovip = i;
	*curp = cur;
	return ret;
}

/**
 * Copy at most `len` bytes from the memory regions described by `iov[iovcnt]`,
 * starting at `*curp` offset from the buffer at `iov[*iovip]`, into `buf`.
 *
 * If `*iovip` is in bounds (< `iovcnt`), `*curp` must also be in bounds
 * (<= `iov[*iovip].iov_len`).
 *
 * If the total remaining bytes in `iov` are less than `len`, exit early.
 * After the call, `*iovip` and `*curp` are updated with the new positions.
 *
 * @return Number of bytes copied
 */
static inline
size_t uk_iov_gather(char *buf, const struct iovec *iov, size_t iovcnt,
		     size_t len, size_t *iovip, size_t *curp)
{
	size_t ret = 0;
	size_t i = *iovip;
	size_t cur = *curp;

	while (i < iovcnt && len) {
		const size_t sz = MIN(len, iov[i].iov_len - cur);

		UK_ASSERT(cur <= iov[i].iov_len); /* cur must be in bounds */
		UK_ASSERT(sz || cur == iov[i].iov_len); /* must make progress */
		if (sz) {
			memcpy(buf, (char *)iov[i].iov_base + cur, sz);
			ret += sz;
			buf += sz;
			len -= sz;
			cur += sz;
		}
		if (cur == iov[i].iov_len) {
			i++;
			cur = 0;
		}
	}

	*iovip = i;
	*curp = cur;
	return ret;
}

#endif /* __UKFILE_IOVUTIL_H__ */
