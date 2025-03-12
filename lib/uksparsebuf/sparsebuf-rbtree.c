/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Rank-balanced tree sparse buffer implementation */

#include <uk/errptr.h>
#include <uk/sparsebuf.h>

#include "impl-common.h"

UK_RB_HEAD(sparsebuf_map, uk_sparsebuf_blk);

static
__u64 sparsebuf_map_key(struct uk_sparsebuf_blk *blk)
{
	return blk->sl.pgoff;
}

static
int sparsebuf_map_cmp(__u64 a, __u64 b)
{
	return a - b;
}

UK_RB_KEY_GENERATE_STATIC(sparsebuf_map, uk_sparsebuf_blk, rb_entry,
			  sparsebuf_map_cmp, sparsebuf_map_key);

/* Internal implementations of lookup & iteration; called from inlines */

struct uk_sparsebuf_blk *uk_sparsebuf_rb_next(struct uk_sparsebuf_blk *blk)
{
	return UK_RB_NEXT(sparsebuf_map, __NULL, blk);
}

/* The sparsebuf RB-tree head is defined like:
 * struct sparsebuf_map {
 *     struct uk_sparsebuf_blk *rbh_root;
 * };
 *
 * Due to this layout, a (struct uk_sparsebuf_blk **) can cast safely to a
 * (struct sparsebuf_map *) for the purposes of accessing rbh_root.
 *
 * The following assert makes sure this assumption continues to hold.
 */
UK_CTASSERT(__offsetof(struct sparsebuf_map, rbh_root) == 0);

struct uk_sparsebuf_blk *uk_sparsebuf_rb_find(struct uk_sparsebuf_blk **headp,
					      __u64 pgoff)
{
	/* The compile-time assert above ensures we can cast this safely. */
	struct sparsebuf_map *const map = (struct sparsebuf_map *)headp;
	struct uk_sparsebuf_blk *ret;

	if (UK_RB_EMPTY(map))
		return __NULL;
	ret = UK_RB_PFIND(sparsebuf_map, map, pgoff);
	if (!ret)
		ret = UK_RB_MIN(sparsebuf_map, map);
	return ret;
}

/* RBtree helpers */

static inline
void sparsebuf_rb_insert(struct sparsebuf_map *map,
			 struct uk_sparsebuf_blk *blk)
{
	struct uk_sparsebuf_blk *prev __maybe_unused;

	prev = UK_RB_INSERT(sparsebuf_map, map, blk);
	UK_ASSERT(!prev);
}

static inline
void sparsebuf_rb_insert_prev(struct sparsebuf_map *map,
			      struct uk_sparsebuf_blk *ref,
			      struct uk_sparsebuf_blk *blk)
{
	UK_RB_INSERT_PREV(sparsebuf_map, map, ref, blk);
	UK_ASSERT(UK_RB_NEXT(sparsebuf_map, map, blk) == ref);
}

static inline
void sparsebuf_rb_insert_next(struct sparsebuf_map *map,
			      struct uk_sparsebuf_blk *ref,
			      struct uk_sparsebuf_blk *blk)
{
	UK_RB_INSERT_NEXT(sparsebuf_map, map, ref, blk);
	UK_ASSERT(UK_RB_NEXT(sparsebuf_map, map, ref) == blk);
}

/* API ops */

static
int sparsebuf_cut(struct uk_sparsebuf_blk **headp,
		  const struct uk_sparsebuf_cur *cur, __u64 pgoff,
		  struct uk_alloc *alloc)
{
	/* The compile-time assert above ensures we can cast this safely. */
	struct sparsebuf_map *const map = (struct sparsebuf_map *)headp;
	struct uk_sparsebuf_slice *const sl = uk_sparsebuf_slice_at(cur);
	const __u32 lnp = pgoff - sl->pgoff;
	const __u32 rnp = uk_sparsebuf_slice_pgend(sl) - pgoff;
	struct uk_sparsebuf_blk *blk;

	UK_ASSERT(uk_sparsebuf_pg_intersects(pgoff, sl));
	blk = uk_malloc(alloc, sizeof(*blk));
	if (unlikely(!blk))
		return -ENOMEM;

	/* No failures beyond this point */
	/* Shorten original slice */
	sl->npages = lnp;
	/* Populate new slice */
	blk->sl = (struct uk_sparsebuf_slice){
		.pgoff = pgoff,
		.external = sl->external,
		.npages = rnp,
		.refcnt = sl->refcnt,
		.buf = sl->buf + lnp * PAGE_SIZE
	};
	/* Insert after original */
	sparsebuf_rb_insert_next(map, cur->p, blk);
	return 0;
}

static
int sparsebuf_separate(struct uk_sparsebuf_blk **headp,
		       __u64 pgoff, __u32 npages,
		       struct uk_sparsebuf_cur *cur, struct uk_alloc *alloc)
{
	const __u64 epgoff = pgoff + npages;
	struct uk_sparsebuf_slice *const sl = uk_sparsebuf_slice_at(cur);
	struct uk_sparsebuf_cur ecur;
	int r;

	if (uk_sparsebuf_pg_intersects(pgoff, sl)) {
		r = sparsebuf_cut(headp, cur, pgoff, alloc);
		if (unlikely(r))
			return r;
		uk_sparsebuf_advance(cur);
	}
	r = uk_sparsebuf_lookup(headp, epgoff, &ecur);
	UK_ASSERT(r); /* Buffer cannot have been empty */
	if (uk_sparsebuf_pg_intersects(epgoff, uk_sparsebuf_slice_at(&ecur))) {
		r = sparsebuf_cut(headp, &ecur, epgoff, alloc);
		if (unlikely(r))
			return r;
	}
	return 0;
}

static
void sparsebuf_freerange(const struct uk_sparsebuf_ctx *ctx,
			 struct uk_sparsebuf_blk **headp,
			 struct uk_sparsebuf_cur *cur, __u64 pgend)
{
	/* The compile-time assert above ensures we can cast this safely. */
	struct sparsebuf_map *const map = (struct sparsebuf_map *)headp;
	struct uk_sparsebuf_blk *blk;
	struct uk_sparsebuf_blk *tmp = cur->p;

	/* Free slices in collapse region */
	UK_RB_FOREACH_FROM(blk, sparsebuf_map, tmp) {
		struct uk_sparsebuf_slice *sl = &blk->sl;

		if (sparsebuf_start_or_before(pgend, sl))
			break;
		UK_ASSERT(sparsebuf_can_release(sl));
		sparsebuf_slice_buffree(ctx, sl);
		UK_RB_REMOVE(sparsebuf_map, map, blk);
		uk_free(ctx->alloc, blk);
	}
}

static
int sparsebuf_newslice(struct uk_sparsebuf_blk **blkp,
		       const struct uk_sparsebuf_ctx *ctx,
		       __u64 pgoff, __u32 npages, __u32 refcnt)
{
	struct uk_sparsebuf_blk *blk = uk_malloc(ctx->alloc, sizeof(*blk));
	void *buf;

	UK_ASSERT(npages);
	if (unlikely(!blk))
		return -ENOMEM;
	buf = ctx->funcs->alloc_new(pgoff, npages, ctx->arg);
	if (unlikely(PTRISERR(buf))) {
		uk_free(ctx->alloc, blk);
		return PTR2ERR(buf);
	}
	blk->sl = (struct uk_sparsebuf_slice){
		.pgoff = pgoff,
		.external = 0,
		.npages = npages,
		.refcnt = refcnt,
		.buf = buf
	};
	*blkp = blk;
	return 0;
}

static
int sparsebuf_newslice_before(const struct uk_sparsebuf_ctx *ctx,
			      struct sparsebuf_map *map,
			      struct uk_sparsebuf_blk *next,
			      __u64 pgoff, __u32 npages, __u32 refs)
{
	/* TODO: attempt merge with next */
	struct uk_sparsebuf_blk *newblk;
	const int r = sparsebuf_newslice(&newblk, ctx, pgoff, npages, refs);

	if (unlikely(r))
		return r;
	sparsebuf_rb_insert_prev(map, next, newblk);
	return 0;
}

static
int sparsebuf_newslice_after(const struct uk_sparsebuf_ctx *ctx,
			     struct sparsebuf_map *map,
			     struct uk_sparsebuf_blk *prev,
			     struct uk_sparsebuf_blk *next __maybe_unused,
			     __u64 pgoff, __u32 npages, __u32 refs)
{
	/* TODO: attempt merge with prev */
	struct uk_sparsebuf_blk *newblk;
	const int r = sparsebuf_newslice(&newblk, ctx, pgoff, npages, refs);

	if (unlikely(r))
		return r;
	sparsebuf_rb_insert_next(map, prev, newblk);
	UK_ASSERT(UK_RB_NEXT(sparsebuf_map, map, newblk) == next);
	return 0;
}

__ssz uk_sparsebuf_fill(const struct uk_sparsebuf_ctx *ctx,
			struct uk_sparsebuf_blk **headp,
			__u64 pgoff, __u32 npages, int takeref)
{
	/* The compile-time assert above ensures we can cast this safely. */
	struct sparsebuf_map *const map = (struct sparsebuf_map *)headp;
	const __u32 nrefs = !!takeref;
	struct uk_sparsebuf_cur cur;
	struct uk_sparsebuf_blk *blk;
	int r;
	__ssz ret = 0;

	if (unlikely(!npages))
		return 0;

	r = uk_sparsebuf_lookup(headp, pgoff, &cur);
	if (unlikely(!r)) {
		/* Buffer empty, single initial alloc */
		r = sparsebuf_newslice(&blk, ctx, pgoff, npages, nrefs);
		if (unlikely(r))
			return r;

		sparsebuf_rb_insert(map, blk);
		return npages;
	}
	blk = cur.p;
	if (takeref) {
		r = sparsebuf_separate(headp, pgoff, npages, &cur, ctx->alloc);
		if (unlikely(r))
			return r;
	}
	/* Walk through slices ensuring filled/refd */
	while (npages) {
		struct uk_sparsebuf_slice *const sl = &blk->sl;
		struct uk_sparsebuf_blk *next = blk;
		__u32 np;

		UK_ASSERT(blk);
		if (uk_sparsebuf_pg_before(pgoff, sl)) {
			/* New slice before */
			np = MIN(npages, sl->pgoff - pgoff);
			r = sparsebuf_newslice_before(ctx, map, blk,
						      pgoff, np, nrefs);
			if (unlikely(r))
				break;
		} else if (uk_sparsebuf_pg_within(pgoff, sl)) {
			/* Already filled */
			UK_ASSERT(uk_sparsebuf_pg_first(pgoff, sl) || !takeref);
			np = MIN(npages, uk_sparsebuf_slice_pgend(sl) - pgoff);
			if (takeref)
				sparsebuf_slice_refup(sl);
		} else {
			/* New slice after; we can advance blk */
			next = UK_RB_NEXT(sparsebuf_map, map, blk);
			np = next ? MIN(npages, next->sl.pgoff - pgoff)
				  : npages;
			if (np) {
				r = sparsebuf_newslice_after(ctx, map,
							     blk, next,
							     pgoff, np, nrefs);
				if (unlikely(r))
					break;
			}
		}
		npages -= np;
		pgoff += np;
		ret += np;
		blk = next;
	}
	UK_ASSERT(!npages || r < 0); /* If not done, it must be due to error */
	UK_ASSERT(ret || r < 0); /* Ensure we never return 0 */
	return ret ? ret : r;
}

static
void sparsebuf_ref_release(const struct uk_sparsebuf_ctx *ctx,
			   struct uk_sparsebuf_blk **headp,
			   struct uk_sparsebuf_cur *cur, __u64 endpgoff,
			   int free_last)
{
	/* The compile-time assert above ensures we can cast this safely. */
	struct sparsebuf_map *map = (struct sparsebuf_map *)headp;
	struct uk_sparsebuf_blk *blk;
	struct uk_sparsebuf_blk *tmp = cur->p;

	UK_RB_FOREACH_FROM(blk, sparsebuf_map, tmp) {
		struct uk_sparsebuf_slice *const sl = &blk->sl;

		if (sparsebuf_start_or_before(endpgoff, sl))
			break;
		if (!sparsebuf_slice_refdn(sl) && free_last) {
			/* Last ref dropped; free & remove */
			sparsebuf_slice_buffree(ctx, sl);
			UK_RB_REMOVE(sparsebuf_map, map, blk);
			uk_free(ctx->alloc, blk);
		}
	}
}

static
void sparsebuf_scoop(const struct uk_sparsebuf_ctx *ctx,
		     struct uk_sparsebuf_blk **headp,
		     struct uk_sparsebuf_cur *cur, __u64 endpgoff)
{
	/* The compile-time assert above ensures we can cast this safely. */
	struct sparsebuf_map *map = (struct sparsebuf_map *)headp;
	struct uk_sparsebuf_blk *blk;
	struct uk_sparsebuf_blk *tmp = cur->p;

	UK_RB_FOREACH_FROM(blk, sparsebuf_map, tmp) {
		struct uk_sparsebuf_slice *const sl = &blk->sl;

		if (sparsebuf_start_or_before(endpgoff, sl))
			break;

		if (sparsebuf_can_release(sl)) {
			/* Free completely */
			sparsebuf_slice_buffree(ctx, sl);
			UK_RB_REMOVE(sparsebuf_map, map, blk);
			uk_free(ctx->alloc, blk);
		} else {
			/* Has active references, drop */
			sparsebuf_slice_bufdrop(ctx, sl);
		}
	}
}

void uk_sparsebuf_clear(const struct uk_sparsebuf_ctx *ctx,
			struct uk_sparsebuf_blk **headp)
{
	/* The compile-time assert above ensures we can cast this safely. */
	struct sparsebuf_map *map = (struct sparsebuf_map *)headp;
	struct uk_sparsebuf_blk *blk;
	struct uk_sparsebuf_blk *tmp;

	UK_ASSERT(headp);
	UK_RB_FOREACH_SAFE(blk, sparsebuf_map, map, tmp) {
		struct uk_sparsebuf_slice *const sl = &blk->sl;

		UK_ASSERT(sparsebuf_can_release(sl));
		sparsebuf_slice_buffree(ctx, sl);
		uk_free(ctx->alloc, blk);
	}
	UK_RB_ROOT(map) = __NULL;
}

int uk_sparsebuf_assign(const struct uk_sparsebuf_ctx *ctx,
			struct uk_sparsebuf_blk **headp,
			__u64 pgoff, __u32 npages, void *buf)
{
	/* The compile-time assert above ensures we can cast this safely. */
	struct sparsebuf_map *map = (struct sparsebuf_map *)headp;
	const __u64 pgend = pgoff + npages;
	struct uk_sparsebuf_cur cur;
	struct uk_sparsebuf_blk *blk;
	int r;

	if (unlikely(!npages))
		return 0;

	/* Alloc new blk */
	blk = uk_malloc(ctx->alloc, sizeof(*blk));
	if (unlikely(!blk))
		return -ENOMEM;

	r = uk_sparsebuf_lookup(headp, pgoff, &cur);
	if (r) {
		/* Separate & check if releasable */
		r = sparsebuf_separate(headp, pgoff, npages, &cur, ctx->alloc);
		if (unlikely(r))
			goto err;
		if (unlikely(sparsebuf_check_release(&cur, pgend))) {
			r = -EBUSY;
			goto err;
		}

		/* No failures beyond this point */
		/* Free range */
		sparsebuf_freerange(ctx, headp, &cur, pgend);
	}
	/* Insert new slice w/ gifted mem */
	blk->sl = (struct uk_sparsebuf_slice){
		.pgoff = pgoff,
		.external = 1,
		.npages = npages,
		.refcnt = 0,
		.buf = buf
	};
	sparsebuf_rb_insert(map, blk);
	return 0;

err:
	uk_free(ctx->alloc, blk);
	return r;
}
