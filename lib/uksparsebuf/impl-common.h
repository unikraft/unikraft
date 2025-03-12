/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Common utils & ops for sparsebuf implementations.
 *
 * Do not include in more than one source file per build.
 */

#ifndef __UK_SPARSEBUF_IMPL_COMMON_H__
#define __UK_SPARSEBUF_IMPL_COMMON_H__

/* We define these errnos here so as not to pull in a full libc */

#ifndef ENOMEM
#define ENOMEM 12
#endif /* ENOMEM */

#ifndef EBUSY
#define EBUSY 16
#endif /* EBUSY */

static inline
int sparsebuf_should_free(const struct uk_sparsebuf_slice *sl)
{
	/* We should free all memory that is not externally assigned */
	return !sl->external;
}

static inline
int sparsebuf_can_release(const struct uk_sparsebuf_slice *sl)
{
	/* We can release only if there are no held refcounts */
	return !sl->refcnt;
}

static inline
int sparsebuf_start_or_before(__u64 pg, const struct uk_sparsebuf_slice *sl)
{
	return uk_sparsebuf_pg_before(pg, sl) || uk_sparsebuf_pg_first(pg, sl);
}

/**
 * Take a reference on slice `sl`.
 */
static inline
void sparsebuf_slice_refup(struct uk_sparsebuf_slice *sl)
{
	UK_ASSERT(sl->refcnt != UK_SPARSEBUF_SLICE_MAX_REFS);
	sl->refcnt++;
}

/**
 * Release a reference on slice `sl` and return the number remaining.
 */
static inline
__u32 sparsebuf_slice_refdn(struct uk_sparsebuf_slice *sl)
{
	UK_ASSERT(sl->refcnt);
	return --(sl->refcnt);
}

/**
 * Free the buffer of slice `sl` if applicable, using context `ctx`.
 */
static inline
void sparsebuf_slice_buffree(const struct uk_sparsebuf_ctx *ctx,
			     const struct uk_sparsebuf_slice *sl)
{
	if (sparsebuf_should_free(sl))
		ctx->funcs->alloc_free(sl->buf, sl->pgoff, sl->npages,
				       ctx->arg);
}

/**
 * Drop the buffer of slice `sl`, using context `ctx`.
 */
static inline
void sparsebuf_slice_bufdrop(const struct uk_sparsebuf_ctx *ctx,
			     const struct uk_sparsebuf_slice *sl)
{
	ctx->funcs->alloc_drop(sl->buf, sl->pgoff, sl->npages, ctx->arg);
}

/**
 * Check whether all slices starting at `cur` until and excluding page offset
 * `end` can be safely released (i.e., have no references acquired).
 *
 * @return
 *   == 0: OK
 *   != 0: Fail
 */
static
int sparsebuf_check_release(struct uk_sparsebuf_cur *cur, __u64 end)
{
	struct uk_sparsebuf_cur tmp;

	UK_SPARSEBUF_FOREACH_FROM(cur, &tmp) {
		struct uk_sparsebuf_slice *sl = uk_sparsebuf_slice_at(&tmp);

		if (sparsebuf_start_or_before(end, sl))
			break;
		if (!sparsebuf_can_release(sl))
			return -1;
	}
	return 0;
}

/* Common API ops use the following funcs provided by the sparsebuf impl */

/**
 * Split the slice referenced by `cur` in two, cutting at page offset `pgoff`.
 *
 * @param headp Sparse buffer to operate on
 * @param cur Sparse buffer cursor pointing to target slice
 * @param pgoff Page offset within target slice where to cut
 * @param alloc Object allocator for new slice blocks
 *
 * @return
 *   == 0: Success
 *    < 0: Negative error code
 */
static int sparsebuf_cut(struct uk_sparsebuf_blk **headp,
			 const struct uk_sparsebuf_cur *cur, __u64 pgoff,
			 struct uk_alloc *alloc);

/**
 * Ensure the range of `npages` pages starting at page offset `pgoff` is backed
 * by separate slices from the rest of the buffer, allowing operations granular
 * to this page range.
 *
 * Cannot be called on an empty sparse buffer.
 *
 * Before the call `cur` must point to the slice containing `pgoff`.
 * After a successful call, `cur` is updated to point to the first slice of the
 * newly separated region, or to the end of buffer if `pgoff` is beyond the last
 * slice.
 *
 * @param headp Sparse buffer to operate on
 * @param pgoff Page offset of start of range
 * @param npages Number of pages in range
 * @param[in,out] cur Sparse buffer cursor
 * @param alloc Object allocator for new slice blocks
 *
 * @return
 *   == 0: Success
 *    < 0: Negative error code
 */
static int sparsebuf_separate(struct uk_sparsebuf_blk **headp,
			      __u64 pgoff, __u32 npages,
			      struct uk_sparsebuf_cur *cur,
			      struct uk_alloc *alloc);

/**
 * Free slices and all associated memory starting at `cur`, up to and not
 * including page offset `pgend`.
 *
 * Cannot be called on an empty buffer.
 *
 * The target region must be backed by separate slices from the rest of the
 * sparse buffer, and no references must be held.
 *
 * This function cannot fail.
 *
 * @param ctx Caller context
 * @param headp Sparse buffer to operate on
 * @param cur Sparse buffer cursor pointing to first slice to free
 * @param pgend Upper limit (non-inclusive) of region to free
 */
static void sparsebuf_freerange(const struct uk_sparsebuf_ctx *ctx,
				struct uk_sparsebuf_blk **headp,
				struct uk_sparsebuf_cur *cur, __u64 pgend);

/**
 * Release previously acquired reference on slices starting at `cur`, up to and
 * not including page offset `pgend`, optionally freeing slices.
 *
 * Cannot be called on an empty buffer.
 *
 * The target region must be backed by separate slices from the rest of the
 * sparse buffer.
 *
 * This function cannot fail.
 *
 * @param ctx Caller context
 * @param headp Sparse buffer to operate on
 * @param cur Sparse buffer cursor pointing to first slice to release ref for
 * @param pgend Upper limit (non-inclusive) of region to release ref for
 * @param free_last If non-zero, free slices with no remaining references
 */
static void sparsebuf_ref_release(const struct uk_sparsebuf_ctx *ctx,
				  struct uk_sparsebuf_blk **headp,
				  struct uk_sparsebuf_cur *cur, __u64 pgend,
				  int free_last);

/**
 * Scoop out (make sparse) all slices starting at `cur`, up to and not including
 * page offset `pgend`.
 *
 * Cannot be called on an empty buffer.
 *
 * The target region must be backed by separate slices from the rest of the
 * sparse buffer. Slices without held references will be freed, those with held
 * references will be dropped.
 *
 * This function cannot fail.
 *
 * @param ctx Caller context
 * @param headp Sparse buffer to operate on
 * @param cur Sparse buffer cursor pointing to first slice to scoop out
 * @param pgend Upper limit (non-inclusive) of region to scoop out
 */
static void sparsebuf_scoop(const struct uk_sparsebuf_ctx *ctx,
			    struct uk_sparsebuf_blk **headp,
			    struct uk_sparsebuf_cur *cur, __u64 pgend);

/* Common API ops */

int uk_sparsebuf_insert(const struct uk_sparsebuf_ctx *ctx,
			struct uk_sparsebuf_blk **headp,
			__u64 pgoff, __u32 npages)
{
	struct uk_sparsebuf_cur cur;
	struct uk_sparsebuf_slice *sl;
	int r;

	if (unlikely(!npages))
		return 0;
	r = uk_sparsebuf_lookup(headp, pgoff, &cur);
	if (unlikely(!r))
		return 0; /* Sparsebuf empty; nothing to do */
	sl = uk_sparsebuf_slice_at(&cur);

	if (uk_sparsebuf_pg_intersects(pgoff, sl)) {
		r = sparsebuf_cut(headp, &cur, pgoff, ctx->alloc);
		if (unlikely(r)) {
			UK_ASSERT(r < 0);
			return r;
		}
		uk_sparsebuf_advance(&cur);
		sl = uk_sparsebuf_slice_at(&cur);
	} else if (uk_sparsebuf_pg_after(pgoff, sl)) {
		uk_sparsebuf_advance(&cur);
	}

	/* No failure beyond this point */
	/* This loop assumes the data structure is stable w.r.t. changes in
	 * indexes that maintain relative order; all implementations must comply
	 */
	UK_ASSERT(sparsebuf_start_or_before(pgoff,
					    uk_sparsebuf_slice_at(&cur)));
	while (!uk_sparsebuf_finished(&cur)) {
		uk_sparsebuf_slice_at(&cur)->pgoff += npages;
		uk_sparsebuf_advance(&cur);
	}
	return 0;
}

int uk_sparsebuf_collapse(const struct uk_sparsebuf_ctx *ctx,
			  struct uk_sparsebuf_blk **headp,
			  __u64 pgoff, __u32 npages)
{
	const __u64 pgend = pgoff + npages;
	struct uk_sparsebuf_cur cur;
	int r;

	if (unlikely(!npages))
		return 0;

	r = uk_sparsebuf_lookup(headp, pgoff, &cur);
	if (unlikely(!r))
		return 0;

	r = sparsebuf_separate(headp, pgoff, npages, &cur, ctx->alloc);
	if (unlikely(r)) {
		UK_ASSERT(r < 0);
		return r;
	}

	if (unlikely(sparsebuf_check_release(&cur, pgend)))
		return -EBUSY;

	/* No failure beyond this point */
	/* Free slices in range */
	sparsebuf_freerange(ctx, headp, &cur, pgend);

	/* Adjust following slices downwards */
	/* This loop assumes the data structure is stable w.r.t. changes in
	 * indexes that maintain relative order; all implementations must comply
	 */
	UK_ASSERT(sparsebuf_start_or_before(pgend,
					    uk_sparsebuf_slice_at(&cur)));
	while (!uk_sparsebuf_finished(&cur)) {
		uk_sparsebuf_slice_at(&cur)->pgoff -= npages;
		uk_sparsebuf_advance(&cur);
	}
	return 0;
}

int uk_sparsebuf_ref_release(const struct uk_sparsebuf_ctx *ctx,
			     struct uk_sparsebuf_blk **headp,
			     __u64 pgoff, __u32 npages, int free_last)
{
	struct uk_sparsebuf_cur cur;
	int r;

	if (unlikely(!npages))
		return 0;

	r = uk_sparsebuf_lookup(headp, pgoff, &cur);
	UK_ASSERT(r); /* Cannot be called on empty buffer */

	r = sparsebuf_separate(headp, pgoff, npages, &cur, ctx->alloc);
	if (unlikely(r)) {
		UK_ASSERT(r < 0);
		return r;
	}

	/* No failure beyond this point */
	sparsebuf_ref_release(ctx, headp, &cur, pgoff + npages, free_last);
	return 0;
}

int uk_sparsebuf_scoop(const struct uk_sparsebuf_ctx *ctx,
		       struct uk_sparsebuf_blk **headp,
		       __u64 pgoff, __u32 npages)
{
	struct uk_sparsebuf_cur cur;
	int r;

	if (unlikely(!npages))
		return 0;

	r = uk_sparsebuf_lookup(headp, pgoff, &cur);
	if (unlikely(!r))
		return 0;

	r = sparsebuf_separate(headp, pgoff, npages, &cur, ctx->alloc);
	if (unlikely(r)) {
		UK_ASSERT(r < 0);
		return r;
	}

	/* No failure beyond this point */
	sparsebuf_scoop(ctx, headp, &cur, pgoff + npages);
	return 0;
}

#endif /* __UK_SPARSEBUF_IMPL_COMMON_H__ */
