/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Lock-free data structure specialized in mapping integers to pointers */

#ifndef __UK_FDTAB_FMAP_H__
#define __UK_FDTAB_FMAP_H__

#include <string.h>

#include <uk/arch/atomic.h>
#include <uk/assert.h>
#include <uk/bitops.h>
#include <uk/bitmap.h>
#include <uk/essentials.h>
#include <uk/thread.h>

/**
 * Lock-free bitmap, with ones representing a free index.
 */
struct uk_bmap {
	volatile unsigned long *bitmap;
	size_t size;
};

#define UK_BMAP_SZ(s) (UK_BITS_TO_LONGS(s) * sizeof(unsigned long))


/**
 * Initialize the bitmap; must not be called concurrently with other functions.
 */
static inline void uk_bmap_init(const struct uk_bmap *bm)
{
	uk_bitmap_fill((void *)bm->bitmap, bm->size);
}

/**
 * Mark `idx` as used, and return whether we were the ones to do so.
 *
 * @return
 *   0 if we marked `idx` as used, non-zero otherwise
 */
static inline int uk_bmap_reserve(const struct uk_bmap *bm, int idx)
{
	unsigned long mask;
	unsigned long v;

	if (!IN_RANGE(idx, 0, bm->size))
		return -1;
	mask = UK_BIT_MASK(idx);
	v = ukarch_and(&bm->bitmap[UK_BIT_WORD(idx)], ~mask);
	return !(v & mask);
}

/**
 * Mark `idx` as free, and return whether we were the ones to do so.
 *
 * @return
 *   0 if we freed `idx`, non-zero if it was already free or out of range
 */
static inline int uk_bmap_free(const struct uk_bmap *bm, int idx)
{
	unsigned long mask;
	unsigned long v;

	if (!IN_RANGE(idx, 0, bm->size))
		return -1;
	mask = UK_BIT_MASK(idx);
	v = ukarch_or(&bm->bitmap[UK_BIT_WORD(idx)], mask);
	return !!(v & mask);
}

static inline int uk_bmap_isfree(const struct uk_bmap *bm, int idx)
{
	if (!IN_RANGE(idx, 0, bm->size))
		return -1;
	return uk_test_bit(idx, bm->bitmap);
}

/**
 * Allocate and return the smallest free index larger than `min`.
 *
 * @return
 *   The allocated index or `>= bm->size` if map full.
 */
static inline int uk_bmap_request(const struct uk_bmap *bm, int min)
{
	int pos;

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
 * Mapping of integers to pointers.
 */
struct uk_fmap {
	struct uk_bmap bmap;
	void *volatile *map;
};

#define UK_FMAP_SZ(s) ((s) * sizeof(void *))


#define _FMAP_INRANGE(m, i) IN_RANGE(i, 0, (m)->bmap.size)

/**
 * Initialize the memory for a uk_fmap.
 *
 * The `size` field must be correctly set and the map buffers allocated.
 */
static inline void uk_fmap_init(const struct uk_fmap *m)
{
	memset((void *)m->map, 0, m->bmap.size * sizeof(void *));
	uk_bmap_init(&m->bmap);
}

/**
 * Lookup the entry at `idx`.
 *
 * WARNING: Use of this function is vulnerable to use-after-free race conditions
 * for entry objects whose lifetime depends on their membership in this data
 * structure (e.g., refcounting on put, take, and after lookups).
 * For that case please use the `uk_fmap_critical_*` functions.
 *
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
 * Put an entry in the map and return its index.
 *
 * @return
 *   newly allocated index, or out of range if map full
 */
static inline
int uk_fmap_put(const struct uk_fmap *m, const void *p, int min)
{
	void *got;
	int pos;

	pos = uk_bmap_request(&m->bmap, min);
	if (!_FMAP_INRANGE(m, pos))
		return pos; /* Map full */

	got = ukarch_exchange_n(&m->map[pos], (void *)p);
	UK_ASSERT(got == NULL); /* There can't be stuff in there, abort */

	return pos;
}

/**
 * Take the entry at `idx` out of the map and return it.
 *
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
		got = ukarch_exchange_n(&m->map[idx], NULL);
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
 * Take out an entry, without marking its `idx` as free.
 *
 * Calling this function is akin to taking a lock on `idx` which should be
 * released as soon as practical by a matching call to `uk_fmap_critical_put`.
 * Take care to not block inbetween these two calls.
 *
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
		got = ukarch_exchange_n(&m->map[idx], NULL);
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
 * Place an entry at `idx`, following a call to `uk_fmap_critical_take`.
 *
 * Calling on an `idx` without a matching call to `uk_fmap_critical_take`
 * is undefined. The value of `p` need not match the value previously taken out.
 *
 * @return
 *   0 on success, non-zero if `idx` out of range
 */
static inline
int uk_fmap_critical_put(const struct uk_fmap *m, int idx, const void *p)
{
	void *got;

	if (!_FMAP_INRANGE(m, idx))
		return -1;

	(void)uk_bmap_reserve(&m->bmap, idx);
	got = ukarch_exchange_n(&m->map[idx], p);
	UK_ASSERT(got == NULL);
	return 0;
}

/**
 * Atomically exhange the entry at `idx` with `p`, returning previous in `prev`.
 *
 * If `idx` is free, it is marked as used and `*prev` is set to NULL.
 *
 * @return
 *   0 on success, non-zero if `idx` out of range
 */
static inline
int uk_fmap_xchg(const struct uk_fmap *m, int idx,
		 const void *p, void **prev)
{
	void *got;

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
			got = ukarch_exchange_n(&m->map[idx], p);
			UK_ASSERT(got == NULL);
			return 0;
		}
	}
}

#endif /* __UK_FDTAB_FMAP_H__ */
