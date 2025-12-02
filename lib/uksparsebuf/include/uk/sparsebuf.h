/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Sparse buffers
 *
 * Sparse buffers are data structures that provide a logical memory buffer of
 * indeterminate size, composed of both allocated and sparse areas.
 *
 * Allocated memory is managed at the granularity of "pages" -- contiguous areas
 * of virtual memory of `UK_PAGING_PAGE_SIZE` -- with a number of consecutive
 * pages forming a "slice". Sparse buffers are optimized for fast lookup and
 * iteration over consecutive slices.
 *
 * Furthermore, reference counting is provided for allocated regions at page
 * granularity, allowing callers to keep track of which parts of the sparse
 * buffer are in use.
 *
 * Callers provide the sparse buffer implementation with bespoke callbacks for
 * allocating and freeing memory regions, as well for managing memory for
 * internal objects.
 * Using these callbacks, sparse buffers provide the following operations:
 * - lookup -- find the slice backing a particular page offset (if any)
 * - iteration -- efficiently iterate over consecutive slices
 * - fill & scoop -- populate a range and make it sparse, respectively
 * - insert & collapse -- add or remove memory areas mid-buffer
 * - assign -- use caller-provided memory to represent part of buffer
 */

#ifndef __UK_SPARSEBUF_H__
#define __UK_SPARSEBUF_H__

#include <uk/alloc.h>
#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/paging.h>

/**
 * Return the sparse buffer page offset of byte offset `off`.
 */
static inline
__sz uk_sparsebuf_pgoff(__sz off)
{
	return off / UK_PAGING_PAGE_SIZE;
}

/**
 * Sparse buffer slice; represents one contiguous allocated region starting
 * at a particular logical page offset.
 *
 * Reference counting is done at slice granularity.
 * The memory backing this region may have been either allocated automatically
 * by the sparse buffer, or provided by the caller using `assign`. This latter
 * case is signaled by the `external` flag.
 */
struct uk_sparsebuf_slice {
	/* Page offset in sparse buffer that this region maps */
	__u64 pgoff;
	struct {
		/* != 0 if caller-assigned, 0 if automatically allocated */
		unsigned char external:1;
		/* Number of contiguous pages that this region spans */
		__u32 npages:31;
	};
	/* Reference count for all pages in this region */
	__u32 refcnt;
	/* Pointer to memory backing this region */
	char *buf;
};

/* Max slice size is 2^31 pages (8 TiB w/ 4K pages) */
#define UK_SPARSEBUF_ALLOC_MAX_PAGES ((1ULL << 31) - 1)
#define UK_SPARSEBUF_SLICE_MAX_REFS __U32_MAX

/* Basic inlines for size and bounds checking */

/**
 * Return the size of slice `sl` in bytes.
 */
static inline
__sz uk_sparsebuf_slice_size(const struct uk_sparsebuf_slice *sl)
{
	return sl->npages * UK_PAGING_PAGE_SIZE;
}

/**
 * Return the offset into the sparse buffer of the first byte in `sl`.
 */
static inline
__sz uk_sparsebuf_slice_offset(const struct uk_sparsebuf_slice *sl)
{
	return sl->pgoff * UK_PAGING_PAGE_SIZE;
}

/**
 * Return the page offset within a sparse buffer of the first page beyond the
 * end of slice `sl`.
 */
static inline
__u64 uk_sparsebuf_slice_pgend(const struct uk_sparsebuf_slice *sl)
{
	return sl->pgoff + sl->npages;
}

/* Relative to a slice `sl`, a sparse buffer page frame `pg` can be:
 * - ahead: `pg` is before `sl` with a gap of at least one frame
 * - prev: `pg` is the immediate frame preceding `sl`
 * - first: `pg` is the first page of `sl`
 * - mid: `pg` is within `sl` but neither first nor last
 * - last: `pg` is the last page of `sl`
 * - next: `pg` is the very next frame following `sl`
 * - beyond: `pg` is after `sl` with a gap of at least one frame
 *
 * Using a combination of these, the following can be expressed:
 * - before: ahead || prev
 * - within: first || mid || last
 * - intersects: mid || last
 * - after: next || beyond
 */

/**
 * Return non-zero if `pg` is before `sl` with a gap.
 */
static inline
int uk_sparsebuf_pg_ahead(__u64 pg, const struct uk_sparsebuf_slice *sl)
{
	return pg + 1 < sl->pgoff;
}

/**
 * Return non-zero if `pg` is previous to `sl`.
 */
static inline
int uk_sparsebuf_pg_prev(__u64 pg, const struct uk_sparsebuf_slice *sl)
{
	return pg + 1 == sl->pgoff;
}

/**
 * Return non-zero if `pg` is the first frame in `sl`.
 */
static inline
int uk_sparsebuf_pg_first(__u64 pg, const struct uk_sparsebuf_slice *sl)
{
	return pg == sl->pgoff;
}

/**
 * Return non-zero if `pg` is in the middle of `sl`.
 */
static inline
int uk_sparsebuf_pg_mid(__u64 pg, const struct uk_sparsebuf_slice *sl)
{
	return pg > sl->pgoff && pg + 1 < uk_sparsebuf_slice_pgend(sl);
}

/**
 * Return non-zero if `pg` is the last frame of `sl`.
 */
static inline
int uk_sparsebuf_pg_last(__u64 pg, const struct uk_sparsebuf_slice *sl)
{
	return pg + 1 == uk_sparsebuf_slice_pgend(sl);
}

/**
 * Return non-zero if `pg` is the next frame after `sl`.
 */
static inline
int uk_sparsebuf_pg_next(__u64 pg, const struct uk_sparsebuf_slice *sl)
{
	return pg == uk_sparsebuf_slice_pgend(sl);
}

/**
 * Return non-zero if `pg` is after `sl` with a gap.
 */
static inline
int uk_sparsebuf_pg_beyond(__u64 pg, const struct uk_sparsebuf_slice *sl)
{
	return pg > uk_sparsebuf_slice_pgend(sl);
}

/**
 * Return non-zero if `pg` is before `sl`.
 */
static inline
int uk_sparsebuf_pg_before(__u64 pg, const struct uk_sparsebuf_slice *sl)
{
	return pg < sl->pgoff;
}

/**
 * Return non-zero if `pg` is contained within `sl`.
 */
static inline
int uk_sparsebuf_pg_within(__u64 pg, const struct uk_sparsebuf_slice *sl)
{
	return pg >= sl->pgoff && pg < uk_sparsebuf_slice_pgend(sl);
}

/**
 * Return non-zero if the start of `pg` intersects the body of `sl`.
 */
static inline
int uk_sparsebuf_pg_intersects(__u64 pg, const struct uk_sparsebuf_slice *sl)
{
	return pg > sl->pgoff && pg < uk_sparsebuf_slice_pgend(sl);
}

/**
 * Return non-zero if `pg` is after `sl`.
 */
static inline
int uk_sparsebuf_pg_after(__u64 pg, const struct uk_sparsebuf_slice *sl)
{
	return pg >= uk_sparsebuf_slice_pgend(sl);
}

/* Type declarations */

/* Sparse buffers manage the memory required for bookkeeping in units of blocks,
 * each block managing the metadata for a number of slices.
 * These blocks are opaque to the caller, with a pointer to an initial (head)
 * block serving as a reference to the sparse buffer data structure as a whole.
 */

/**
 * Internal opaque structure used to store and lookup slices.
 */
struct uk_sparsebuf_blk;

/* While blocks will in most cases be dynamically allocated and freed, callers
 * may desire to preallocate/embed a particularly-sized block as part of the
 * sparse buffer head.
 *
 * For this purpose the following macros are provided for declaring,
 * initializing and accessing such embedded blocks.
 */

/**
 * Declare a struct named `sname` containing an embedded block of `size_hint`
 * slices, usable as a sparse buffer head.
 *
 * As suggested by the name, `size_hint` is merely a guiding value, and the
 * sparse buffer implementation may embed any convenient number of slices,
 * including none at all.
 */
#define UK_SPARSEBUF_EMBED_HEADBLK(sname, size_hint) \
	UK__SPARSEBUF_EMBED_HEADBLK(sname, size_hint)

/**
 * Initializer for a variable of the struct type declared by
 * `UK_SPARSEBUF_EMBED_HEADBLK(..., size_hint)`.
 */
#define UK_SPARSEBUF_EMBED_HEADBLK_INITIALIZER(size_hint) \
	UK__SPARSEBUF_EMBED_HEADBLK_INITIALIZER(size_hint)

/**
 * Initialization rvalue, usable to assign to a variable of struct `sname` as
 * declared by `UK_SPARSEBUF_EMBED_HEADBLK(sname, size_hint)`.
 */
#define UK_SPARSEBUF_EMBED_HEADBLK_INIT_VALUE(sname, size_hint) \
	((struct sname)UK_SPARSEBUF_EMBED_HEADBLK_INITIALIZER(size_hint))

/**
 * Set up the necessary context to use `UK_SPARSEBUF_EMBED_HEADP` in the current
 * code block.
 *
 * @param tmpvar Temporary variable name; must be unused in the current context
 * @param emb Embedded headblock struct
 */
#define UK_SPARSEBUF_EMBED_HEAD(tmpvar, emb) \
	UK__SPARSEBUF_EMBED_HEAD(tmpvar, emb)

/**
 * Expression to retrieve the head pointer out of an embedded headblock.
 *
 * Requires previous use of `UK_SPARSEBUF_EMBED_HEAD` in current or enclosing
 * code block.
 *
 * @param tmpvar Temporary variable name; same as for UK_SPARSEBUF_EMBED_HEAD
 * @param emb Embedded headblock struct
 *
 * @return
 *   (struct uk_sparsebuf_blk **) Head pointer of sparse buffer.
 */
#define UK_SPARSEBUF_EMBED_HEADP(tmpvar, emb) \
	UK__SPARSEBUF_EMBED_HEADP(tmpvar, emb)

/**
 * Internal opaque structure pointing to one particular slice of a sparsebuf.
 */
struct uk_sparsebuf_cur;

/* Core sparsebuf lookup/iteration inline declarations */

/**
 * Check whether `cur` is valid.
 *
 * @return
 *  != 0: Valid
 *  == 0: Invalid
 */
static int uk_sparsebuf_valid(const struct uk_sparsebuf_cur *cur);

/**
 * Check whether `cur` points at or beyond the end of a sparse buffer.
 *
 * @return
 *  != 0: `cur` points past the end of the sparse buffer
 *  == 0: `cur` points before the end
 */
static int uk_sparsebuf_finished(const struct uk_sparsebuf_cur *cur);

/**
 * Return the slice pointed to by `cur`, which must be valid.
 */
static
struct uk_sparsebuf_slice *uk_sparsebuf_slice_at(
	const struct uk_sparsebuf_cur *cur);

/**
 * Advance `cur` to the next slice in the sparse buffer, if possible.
 */
static
void uk_sparsebuf_advance(struct uk_sparsebuf_cur *cur);

/**
 * Lookup `pgoff` in the sparsebuf described by `headp`.
 *
 * Fails if sparse buffer is entirely empty.
 * On success, `cur` is set to point to a valid slice with the highest
 * starting page offset that is lesser than or equal to `pgoff`, or to the very
 * first valid slice if the buffer is sparse up to and beyong `pgoff`.
 *
 * @return
 *  != 0: Success
 *  == 0: Sparse buffer is entirely empty
 */
static
int uk_sparsebuf_lookup(struct uk_sparsebuf_blk **headp, __u64 pgoff,
			struct uk_sparsebuf_cur *cur);

/**
 * For-like macro to iterate over all slices in a sparse buffer.
 *
 * Initializes and modifies `cur` in place.
 *
 * The sparse buffer must not be modified over the course of the iteration.
 */
#define UK_SPARSEBUF_FOREACH(headp, cur)				\
	for (const int uk_sb_nonempty = uk_sparsebuf_lookup((headp), 0, (cur));\
	     uk_sb_nonempty && !uk_sparsebuf_finished((cur));		\
	     uk_sparsebuf_advance((cur)))

/**
 * For-like macro to iterate over slices in a sparse buffer starting from a
 * particular position.
 *
 * `start` is left untouched, and `cur` is modified in-place.
 */
#define UK_SPARSEBUF_FOREACH_FROM(start, cur)				\
	for (*(cur) = *(start);						\
	     !uk_sparsebuf_finished((cur));				\
	     uk_sparsebuf_advance((cur)))

/**
 * Retrieve memory for accessing byte offset `off` of the sparse buffer,
 * starting at position `cur`, writing a pointer to this area into `bufp`.
 *
 * `cur` must point to a valid allocation or the end of buffer, and will be
 * advanced as necessary.
 *
 * If `off` is within a mapped area, `bufp` is set to a valid pointer.
 * If `off` is within a sparse area, `bufp` is set to __NULL.
 *
 * If the return value is 0, `off` lies beyond the last slice and `bufp` is
 * left untouched.
 *
 * @return The length in bytes of the returned buffer.
 */
static inline
__sz uk_sparsebuf_memat(__sz off, struct uk_sparsebuf_cur *cur, char **bufp)
{
	const __u64 pgoff = uk_sparsebuf_pgoff(off);

	while (!uk_sparsebuf_finished(cur)) {
		struct uk_sparsebuf_slice *sl = uk_sparsebuf_slice_at(cur);

		if (uk_sparsebuf_pg_before(pgoff, sl)) {
			/* Not mapped; return number of zero bytes */
			*bufp = __NULL;
			return sl->pgoff * UK_PAGING_PAGE_SIZE - off;
		} else if (uk_sparsebuf_pg_within(pgoff, sl)) {
			/* Mapped; return buffer & number of bytes */
			const __sz sloff = off - uk_sparsebuf_slice_offset(sl);

			*bufp = &sl->buf[sloff];
			return uk_sparsebuf_slice_size(sl) - sloff;
		}
		/* off is after sl, advance */
		uk_sparsebuf_advance(cur);
	}
	/* Not found */
	return 0;
}

/* Include inline implementations */
#include <uk/sparsebuf/impl.h>

/* Caller-provided functions for memory range allocation / free / drop */

/**
 * Allocate memory for a region starting at `pgoff` spanning `npages`.
 *
 * @return
 *  !PTRISERR: success; pointer to memory
 *  PTRISERR: failure; negative errno encoded in return
 */
typedef void *(*uk_sparsebuf_alloc_new_func)(__u64 pgoff, __u32 npages,
					     void *arg);

/**
 * Free memory at `addr` for a region starting at `pgoff` spanning `npages`.
 *
 * `addr` is guaranteed to be memory previously returned by `alloc_new`.
 */
typedef void (*uk_sparsebuf_alloc_free_func)(void *addr,
					     __u64 pgoff, __u32 npages,
					     void *arg);

/**
 * Drop memory at `addr` for a region starting at `pgoff` spanning `npages`.
 *
 * Drop is called instead of free when attempting to scoop regions that have
 * active references taken (i.e., non-zero refcounts).
 *
 * What precisely happens on drop is left to the implementation; the memory at
 * `addr` must however remain accessible after drop returns.
 */
typedef void (*uk_sparsebuf_alloc_drop_func)(void *addr,
					     __u64 pgoff, __u32 npages,
					     void *arg);

/* Caller-provided functions for managing sparse buffer memory */
struct uk_sparsebuf_alloc_funcs {
	uk_sparsebuf_alloc_new_func alloc_new;
	uk_sparsebuf_alloc_free_func alloc_free;
	uk_sparsebuf_alloc_drop_func alloc_drop;
};

/* Caller-provided context for calling into sparse buffer ops */
struct uk_sparsebuf_ctx {
	/* Memory management callbacks */
	const struct uk_sparsebuf_alloc_funcs *funcs;
	/* Object allocator for metadata */
	struct uk_alloc *alloc;
	/* Custom argument passed verbatim to memory management callbacks */
	void *arg;
};

/* Core sparse buffer operations */

/* The functions below modify state and are not reentrant nor thread safe.
 *
 * Callers are responsible for ensuring mutual exclusion as well as exclusion of
 * other read operations on the sparse buffer during calls to these functions.
 *
 * On failure, these functions leave the visible sparse buffer state unmodified.
 */

/**
 * Insert a sparse region of `npages` at page offset `pgoff`.
 *
 * Any mapped regions beyond `pgoff` will have their offsets adjusted upwards by
 * `npages`.
 *
 * @param ctx Caller context
 * @param headp Head pointer of sparse buffer to operate on
 * @param pgoff Page offset to insert sparse region at
 * @param npages Number of pages of inserted sparse region
 *
 * @return
 *   == 0: Success
 *    < 0: Negative error code
 */
int uk_sparsebuf_insert(const struct uk_sparsebuf_ctx *ctx,
			struct uk_sparsebuf_blk **headp,
			__u64 pgoff, __u32 npages);

/**
 * Collapse (remove) the region of `npages` at page offset `pgoff`.
 *
 * Any mapped regions beyond `pgoff` will have their offsets adjusted downwards
 * by `npages`.
 *
 * Target region must not have active references held.
 * Mapped pages, except those previously supplied with `uk_sparsebuf_assign`,
 * will be freed.
 *
 * @param ctx Caller context
 * @param headp Head pointer of sparse buffer to operate on
 * @param pgoff Page offset to start collapse at
 * @param npages Number of pages to collapse
 *
 * @return
 *   == 0: Success
 *    < 0: Negative error code
 */
int uk_sparsebuf_collapse(const struct uk_sparsebuf_ctx *ctx,
			  struct uk_sparsebuf_blk **headp,
			  __u64 pgoff, __u32 npages);

/**
 * Ensure the region of `npages` starting at page offset `pgoff` is backed by
 * mapped areas of memory, acquiring refcounts in the process if `ref_acquire`.
 *
 * A successful call may fill in fewer pages than requested; callers should
 * check the return value.
 *
 * @param ctx Caller context
 * @param headp Head pointer of sparse buffer to operate on
 * @param pgoff Page offset to start filling at
 * @param npages Number of pages to fill
 * @param ref_acquire If non-zero, acquire a reference on pages in the fill area
 *
 * @return
 *    > 0: Success, number of pages filled
 *    < 0: Negative error code
 */
__ssz uk_sparsebuf_fill(const struct uk_sparsebuf_ctx *ctx,
			struct uk_sparsebuf_blk **headp,
			__u64 pgoff, __u32 npages, int ref_acquire);

/**
 * Release previously acquired reference to `npages` starting at `pgoff`.
 *
 * It is undefined behavior to release references without previously acquiring.
 *
 * @param ctx Caller context
 * @param headp Head pointer of sparse buffer to operate on
 * @param pgoff Page offset to start from
 * @param npages Number of pages to release references for
 * @param free_last If non-zero, free pages whose last reference was dropped
 *
 * @return
 *   == 0: Success
 *    < 0: Negative error code
 */
int uk_sparsebuf_ref_release(const struct uk_sparsebuf_ctx *ctx,
			     struct uk_sparsebuf_blk **headp,
			     __u64 pgoff, __u32 npages, int free_last);

/**
 * Scoop out (make sparse) the region of `npages` at page offset `pgoff`.
 *
 * Mapped pages with no active refereces will be freed. Those with references
 * will be dropped. Pages previously supplied through `uk_sparsebuf_assign`
 * are removed from the buffer but neither freed nor dropped.
 *
 * @param ctx Caller context
 * @param headp Head pointer of sparse buffer to operate on
 * @param pgoff Page offset to start scooping at
 * @param npages Number of pages to make sparse
 *
 * @return
 *   == 0: Success
 *    < 0: Negative error code
 */
int uk_sparsebuf_scoop(const struct uk_sparsebuf_ctx *ctx,
		       struct uk_sparsebuf_blk **headp,
		       __u64 pgoff, __u32 npages);

/**
 * Clear the sparse buffer, emptying it completely and freeing all memory.
 *
 * No active references must be held.
 *
 * @param ctx Caller context
 * @param headp Head pointer of sparse buffer to operate on
 */
void uk_sparsebuf_clear(const struct uk_sparsebuf_ctx *ctx,
			struct uk_sparsebuf_blk **headp);

/**
 * Assign the memory region of `npages` starting at `buf` to be the backing
 * for the sparse buffer page offset `pgoff`, replacing whatever had been there.
 *
 * `buf` must be valid as long as it remains part of the sparse buffer.
 * Any mapped regions must not have references held and will be freed.
 * Pages previously supplied through `uk_sparsebuf_assign` are not freed.
 *
 * The caller is responsible for keeping track of assigned buffers and managing
 * their lifetimes.
 *
 * @param ctx Caller context
 * @param headp Head pointer of sparse buffer to operate on
 * @param pgoff Page offset where to insert the region
 * @param npages Number of pages to assign
 * @param buf Buffer to use
 *
 * @return
 *   == 0: Success
 *    < 0: Negative error code
 */
int uk_sparsebuf_assign(const struct uk_sparsebuf_ctx *ctx,
			struct uk_sparsebuf_blk **headp,
			__u64 pgoff, __u32 npages, void *buf);

#endif /* __UK_SPARSEBUF_H__ */
