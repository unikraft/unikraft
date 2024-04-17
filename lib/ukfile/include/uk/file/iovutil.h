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
 * Zero out at most `len` bytes in memory regions described by `iov[iovcnt]`,
 * starting at `*curp` offset from the buffer at `iov[*iovip]`.
 *
 * If the total remaining space in `iov` is less than `len`, exit early.
 * After the call, `*iovip` and `*curp` are updated with the new positions.
 *
 * @return Number of bytes zeroed
 */
static inline
size_t uk_iov_zero(const struct iovec *iov, int iovcnt, size_t len,
		   int *iovip, size_t *curp)
{
	size_t ret = 0;
	int i = *iovip;
	size_t cur = *curp;

	UK_ASSERT(i < iovcnt);
	UK_ASSERT(cur < iov[i].iov_len);
	if (cur) {
		const size_t l = MIN(len, iov[i].iov_len - cur);

		memset((char *)iov[i].iov_base + cur, 0, l);
		ret += l;
		len -= l;
		cur += l;
		if (cur == iov[i].iov_len) {
			i++;
			cur = 0;
		}
	}
	if (len) {
		UK_ASSERT(!cur);
		while (i < iovcnt && len) {
			const size_t l = MIN(iov[i].iov_len, len);

			memset(iov[i].iov_base, 0, l);
			ret += l;
			len -= l;
			if (len)
				i++;
			else
				cur = l;
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
 * If the total remaining space in `iov` is less than `len`, exit early.
 * After the call, `*iovip` and `*curp` are updated with the new positions.
 *
 * @return Number of bytes copied
 */
static inline
size_t uk_iov_scatter(const struct iovec *iov, int iovcnt, const char *buf,
		      size_t len, int *iovip, size_t *curp)
{
	size_t ret = 0;
	int i = *iovip;
	size_t cur = *curp;

	UK_ASSERT(i < iovcnt);
	UK_ASSERT(cur < iov[i].iov_len);
	if (cur) {
		const size_t l = MIN(len, iov[i].iov_len - cur);

		memcpy((char *)iov[i].iov_base + cur, buf, l);
		ret += l;
		buf += l;
		len -= l;
		cur += l;
		if (cur == iov[i].iov_len) {
			i++;
			cur = 0;
		}
	}
	if (len) {
		UK_ASSERT(!cur);
		while (i < iovcnt && len) {
			const size_t l = MIN(iov[i].iov_len, len);

			memcpy(iov[i].iov_base, buf, l);
			ret += l;
			buf += l;
			len -= l;
			if (len)
				i++;
			else
				cur = l;
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
 * If the total remaining bytes in `iov` are less than `len`, exit early.
 * After the call, `*iovip` and `*curp` are updated with the new positions.
 *
 * @return Number of bytes copied
 */
static inline
size_t uk_iov_gather(char *buf, const struct iovec *iov, int iovcnt,
		     size_t len, int *iovip, size_t *curp)
{
	size_t ret = 0;
	int i = *iovip;
	size_t cur = *curp;

	UK_ASSERT(i < iovcnt);
	UK_ASSERT(cur < iov[i].iov_len);
	if (cur) {
		const size_t l = MIN(len, iov[i].iov_len - cur);

		memcpy(buf, (char *)iov[i].iov_base + cur, l);
		ret += l;
		buf += l;
		len -= l;
		cur += l;
		if (cur == iov[i].iov_len) {
			i++;
			cur = 0;
		}
	}
	if (len) {
		UK_ASSERT(!cur);
		while (i < iovcnt && len) {
			const size_t l = MIN(iov[i].iov_len, len);

			memcpy(buf, iov[i].iov_base, l);
			ret += l;
			buf += l;
			len -= l;
			if (len)
				i++;
			else
				cur = l;
		}
	}
	*iovip = i;
	*curp = cur;
	return ret;
}

#endif /* __UKFILE_IOVUTIL_H__ */
