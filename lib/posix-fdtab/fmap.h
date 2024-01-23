/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Lock-free data structure specialized in mapping integers to pointers */

#ifndef __UK_FDTAB_FMAP_H__
#define __UK_FDTAB_FMAP_H__

#include <string.h>

#include <uk/atomic.h>
#include <uk/assert.h>
#include <uk/bitops.h>
#include <uk/bitmap.h>
#include <uk/essentials.h>
#include <uk/thread.h>

/**
 * Bitmap telling if a file descriptor is used or not.
 */
struct uk_bmap {
	/* Lock-free bitmap, with ones representing a free index */
	volatile unsigned long *bitmap;
	/* Size of the bitmap */
	size_t size;
};

/**
 * Gets the size of the bitmap in bytes.
 *
 * @param s
 *   Number of elements in the bitmap
 * @return
 *   Size of the bitmap in bytes
 */
#define UK_BMAP_SZ(s) (UK_BITS_TO_LONGS(s) * sizeof(unsigned long))


/**
 * Initializes the bitmap (fills it with ones);
 * must not be called concurrently with other functions.
 *
 * @param bm
 *   Bitmap to be initialized
 */
static inline void uk_bmap_init(const struct uk_bmap *bm)
{
	uk_bitmap_fill((void *)bm->bitmap, bm->size);
}

/**
 * Marks `idx` as used, and returns whether we were the ones to do so.
 *
 * @param bm
 *   Bitmap in which to mark
 * @param idx
 *   Index to mark as used
 * @return
 *   0 if we marked `idx` as used, non-zero otherwise
 */
static inline int uk_bmap_reserve(const struct uk_bmap *bm, int idx)
{
	unsigned long mask;
	unsigned long v;

	if (!((idx >= 0) && IN_RANGE((size_t)idx, 0, bm->size)))
		return -1;
	mask = UK_BIT_MASK(idx);
	v = uk_and(&bm->bitmap[UK_BIT_WORD(idx)], ~mask);
	return !(v & mask);
}

/**
 * Marks `idx` as free, and returns whether we were the ones to do so.
 *
 * @param bm
 *   Bitmap in which to mark
 * @param idx
 *   Index to mark as free
 * @return
 *   0 if we freed `idx`, non-zero if it was already free or out of range
 */
static inline int uk_bmap_free(const struct uk_bmap *bm, int idx)
{
	unsigned long mask;
	unsigned long v;

	if (!((idx >= 0) && IN_RANGE((size_t)idx, 0, bm->size)))
		return -1;
	mask = UK_BIT_MASK(idx);
	v = uk_or(&bm->bitmap[UK_BIT_WORD(idx)], mask);
	return !!(v & mask);
}

/**
 * Checks if `idx` is free or not.
 *
 * @param bm
 *   Bitmap in which to check
 * @param idx
 *   Index to check
 * @return
 *   0 if `idx` is free, non-zero otherwise
 */
static inline int uk_bmap_isfree(const struct uk_bmap *bm, int idx)
{
	if (!((idx >= 0) && IN_RANGE((size_t)idx, 0, bm->size)))
		return -1;
	return uk_test_bit(idx, bm->bitmap);
}

/**
 * Allocates and returns the smallest free index larger than `min`.
 *
 * @param bm
 *   Bitmap in which to search for the next free index
 * @param min
 *   Starting value from which to search the next free index
 * @return
 *   The allocated index or `>= bm->size` if map full.
 */
static inline int uk_bmap_request(const struct uk_bmap *bm, int min)
{
	size_t pos;

	do {
		/* Seems safe to cast away volatility, revisit if problem */
		pos = uk_find_next_bit((unsigned long *)bm->bitmap,
				       bm->size, min);
		if (pos >= bm->size)
			return bm->size;
	/* If bit was already cleared, we lost the race, retry */
	} while (uk_bmap_reserve(bm, pos));

	return pos;
}

/**
 * Data structure mapping between integers and open file descriptions.
 */
struct uk_fmap {
	/* Bitmap describing which file descriptors are free */
	struct uk_bmap bmap;
	/* Map of pointers to open file descriptions */
	void *volatile *map;
};

/**
 * Gets the size of the map in bytes.
 *
 * @param s
 *   Number of elements in the map
 * @return
 *   Size of the map in bytes
 */
#define UK_FMAP_SZ(s) ((s) * sizeof(void *))

/**
 * Checks if the index given is in the range of the map.
 *
 * @param m
 *   fmap that gives us the maximum value that the index can have
 * @param i
 *   Index that we check if it is in range from 0 to size of the bitmap
 * @return
 *   0 if the index is not in the range, 1 otherwise
 */
#define _FMAP_INRANGE(m, i) ((i >= 0) && IN_RANGE((size_t)i, 0, (m)->bmap.size))

/**
 * Initializes the memory for a uk_fmap.
 *
 * The `size` field must be correctly set and the map buffers allocated.
 *
 * @param m
 *   fmap to be initialized
 */
static inline void uk_fmap_init(const struct uk_fmap *m)
{
	memset((void *)m->map, 0, m->bmap.size * sizeof(void *));
	uk_bmap_init(&m->bmap);
}

/**
 * Looks up and returns the entry at `idx`.
 *
 * WARNING: Use of this function is vulnerable to use-after-free race conditions
 * for entry objects whose lifetime depends on their membership in this data
 * structure (e.g., refcounting on put, take, and after lookups).
 * For that case please use the `uk_fmap_critical_*` functions.
 *
 * @param m
 *   fmap in which to search
 * @param idx
 *   Index from which to return the entry
 * @return
 *   The entry at `idx`, or NULL if out of range.
 */
static inline void *uk_fmap_lookup(const struct uk_fmap *m, int idx)
{
	void *got;

	if (!_FMAP_INRANGE(m, idx))
		return NULL;

	do {
		got = m->map[idx];
		if (!got) {
			if (uk_bmap_isfree(&m->bmap, idx))
				break; /* Entry is actually free */
			uk_sched_yield(); /* Lost race, retry */
		}
	} while (!got);
	return got;
}

/**
 * Puts a new entry in the map and returns its index.
 *
 * @param m
 *   fmap in which to put the entry
 * @param p
 *   New entry to put in the map
 * @param min
 *   Start value from which we search the next free index
 * @return
 *   newly allocated index, or out of range if map full
 */
static inline
int uk_fmap_put(const struct uk_fmap *m, const void *p, int min)
{
	void *got __maybe_unused;
	int pos;

	pos = uk_bmap_request(&m->bmap, min);
	if (!_FMAP_INRANGE(m, pos))
		return pos; /* Map full */

	got = uk_exchange_n(&m->map[pos], (void *)p);
	UK_ASSERT(got == NULL); /* There can't be stuff in there, abort */

	return pos;
}

/**
 * Takes the entry at `idx` out of the map and returns it.
 *
 * @param m
 *   fmap from which to take the entry
 * @param idx
 *   Index of the entry
 * @return
 *   Previous entry, or NULL if empty or out of range
 */
static inline void *uk_fmap_take(const struct uk_fmap *m, int idx)
{
	int v __maybe_unused;
	void *got;

	if (!_FMAP_INRANGE(m, idx))
		return NULL;

	do {
		if (uk_bmap_isfree(&m->bmap, idx))
			return NULL; /* Already free */

		/* At most one take thread gets the previous non-NULL value */
		got = uk_exchange_n(&m->map[idx], NULL);
		if (!got)
			/* We lost the race with a (critical) take, retry */
			uk_sched_yield();
	} while (!got);

	/* We are that one thread; nobody else can set the bitmap */
	v = uk_bmap_free(&m->bmap, idx);
	UK_ASSERT(!v);
	return got;
}

/**
 * Takes out an entry, without marking its `idx` as free.
 *
 * Calling this function is akin to taking a lock on `idx` which should be
 * released as soon as practical by a matching call to `uk_fmap_critical_put`.
 * Take care to not block inbetween these two calls.
 *
 * @param m
 *   fmap from which to take the entry
 * @param idx
 *   Index of the entry
 * @return
 *   The entry at `idx`, or NULL if not present or out of range.
 */
static inline
void *uk_fmap_critical_take(const struct uk_fmap *m, int idx)
{
	void *got;

	if (!_FMAP_INRANGE(m, idx))
		return NULL;
	do {
		got = uk_exchange_n(&m->map[idx], NULL);
		if (!got) {
			if (uk_bmap_isfree(&m->bmap, idx))
				/* idx is actually empty */
				break;
			/* Lost race with (critical) take, retry */
			uk_sched_yield();
		}
	} while (!got);
	return got;
}

/**
 * Places an entry at `idx`, following a call to `uk_fmap_critical_take`.
 *
 * Calling on an `idx` without a matching call to `uk_fmap_critical_take`
 * is undefined. The value of `p` need not match the value previously taken out.
 *
 * @param m
 *   fmap in which to place the entry
 * @param idx
 *   Index where to place the entry
 * @param p
 *	 Entry to be placed in fmap
 * @return
 *   0 on success, non-zero if `idx` out of range
 */
static inline
int uk_fmap_critical_put(const struct uk_fmap *m, int idx, const void *p)
{
	void *got __maybe_unused;

	if (!_FMAP_INRANGE(m, idx))
		return -1;

	(void)uk_bmap_reserve(&m->bmap, idx);
	got = uk_exchange_n(&m->map[idx], p);
	UK_ASSERT(got == NULL);
	return 0;
}

/**
 * Atomically exhanges the entry at `idx` with `p`,
 * returning previous in `prev`.
 *
 * If `idx` is free, it is marked as used and `*prev` is set to NULL.
 *
 * @param m
 *   fmap in which to exchange the entries
 * @param idx
 *   Index where we want to exchange the entries
 * @param p
 *   Entry to be exchanged with
 * @param prev
 *   Previous entry that has been replaced
 * @return
 *   0 on success, non-zero if `idx` out of range
 */
static inline
int uk_fmap_xchg(const struct uk_fmap *m, int idx,
		 const void *p, void **prev)
{
	void *got;

	UK_ASSERT(p); /* Cannot exchange with NULL, use uk_fmap_take instead */
	if (!_FMAP_INRANGE(m, idx))
		return -1;

	/* Exchanging entries directly is problematic, must use take & put */
	for (;;) {
		int r = uk_bmap_reserve(&m->bmap, idx);

		if (r) {
			/* There was already something there */
			got = uk_fmap_critical_take(m, idx);
			if (got) {
				(void)uk_fmap_critical_put(m, idx, p);
				*prev = got;
				return 0;
			}
			/* Lost race with (critical) take, retry */
			uk_sched_yield();
		} else {
			/* idx was free, we're basically a put now */
			got = uk_exchange_n(&m->map[idx], p);
			UK_ASSERT(got == NULL);
			return 0;
		}
	}
}

#endif /* __UK_FDTAB_FMAP_H__ */
