/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Sparse buffer utilities for API users.
 *
 * This covers common non-trivial functions that operate on byte granularity.
 *
 * Because these functions call into state-modifying functions of the API, they
 * are similarly not thread safe and callers must ensure mutual exclusion.
 */

#ifndef __UK_SPARSEBUF_UTIL_H__
#define __UK_SPARSEBUF_UTIL_H__

#include <uk/sparsebuf.h>

/**
 * Zero out `len` bytes starting at sparse buffer offset `off`, using `cur`.
 */
static inline
void uk_sparsebuf_memclear(struct uk_sparsebuf_cur *cur, __sz off, __sz len)
{
	char *buf = __NULL;
	__sz blen = uk_sparsebuf_memat(off, cur, &buf);

	if (blen && buf) {
		UK_ASSERT(blen >= len);
		while (len--)
			*(buf++) = 0;
	}
}

/**
 * Truncate a sparse buffer down to `size` bytes.
 *
 * All integral pages between `size` and the end of the sparse buffer will be
 * scooped, and any remaining bytes between `size` and the next page boundary
 * will be zeroed out.
 *
 * @param ctx Caller context
 * @param headp Head pointer of sparse buffer to operate on
 * @param size Number of bytes to truncate to
 *
 * @return
 *   == 0: Success
 *    < 0: Negative error code
 */
static inline
int uk_sparsebuf_truncate(const struct uk_sparsebuf_ctx *ctx,
			  struct uk_sparsebuf_blk **headp, __sz size)
{
	const __sz pglo = ALIGN_UP(size, PAGE_SIZE);
	const __sz rem = pglo - size;
	struct uk_sparsebuf_cur cur;
	__sz toscoop;
	int r;

	/* Get last slice by looking up highest possible offset */
	r = uk_sparsebuf_lookup(headp, __U64_MAX, &cur);
	if (unlikely(!r))
		/* Sparse buffer empty, nothing to do */
		return 0;

	toscoop = uk_sparsebuf_slice_pgend(uk_sparsebuf_slice_at(&cur)) - pglo;
	r = uk_sparsebuf_scoop(ctx, headp, uk_sparsebuf_pgoff(pglo),
			       toscoop / PAGE_SIZE);
	if (unlikely(r)) {
		UK_ASSERT(r < 0);
		return r;
	}
	/* Clear up to next page boundary & success */
	if (rem && uk_sparsebuf_lookup(headp, uk_sparsebuf_pgoff(size), &cur))
		uk_sparsebuf_memclear(&cur, size, rem);
	return 0;
}

/**
 * Punch a hole -- make sparse or zero out -- `len` bytes of a sparse buffer
 * starting at byte offset `off`.
 *
 * This operation is equivalent to scooping out all integral pages in the target
 * area, with remaining sub-page areas being zeroed out.
 *
 * @param ctx Caller context
 * @param headp Head pointer of sparse buffer to operate on
 * @param off Offset of the first byte in the punched hole
 * @param len Length of punched hole in bytes
 *
 * @return
 *   == 0: Success
 *    < 0: Negative error code
 */
static inline
int uk_sparsebuf_punch_hole(const struct uk_sparsebuf_ctx *ctx,
			    struct uk_sparsebuf_blk **headp,
			    __sz off, __sz len)
{
	const __sz end = off + len;
	const __sz pglo = ALIGN_UP(off, PAGE_SIZE);
	const __sz pgoff = uk_sparsebuf_pgoff(pglo);
	const __sz pghi = ALIGN_DOWN(end, PAGE_SIZE);
	const __sz npages = uk_sparsebuf_pgoff(pghi) - pgoff;
	const __sz remlo = pglo - off;
	const __sz remhi = end - pghi;
	struct uk_sparsebuf_cur cur;
	int r;

	r = uk_sparsebuf_scoop(ctx, headp, pgoff, npages);
	if (unlikely(r)) {
		UK_ASSERT(r < 0);
		return r;
	}

	if (remlo && uk_sparsebuf_lookup(headp, uk_sparsebuf_pgoff(off), &cur))
		uk_sparsebuf_memclear(&cur, off, remlo);
	if (remhi && uk_sparsebuf_lookup(headp, uk_sparsebuf_pgoff(end), &cur))
		uk_sparsebuf_memclear(&cur, pghi, remhi);
	return 0;
}

#endif /* __UK_SPARSEBUF_UTIL_H__ */
