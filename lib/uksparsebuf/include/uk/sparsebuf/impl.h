/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Sparse buffer implementation; do not include directly */

#ifndef __UK_SPARSEBUF_IMPL_H__
#define __UK_SPARSEBUF_IMPL_H__

#ifndef __UK_SPARSEBUF_H__
#error "Do not include this header directly; #include <uk/sparsebuf.h> instead"
#endif /* __UK_SPARSEBUF_H__ */

#if CONFIG_LIBUKSPARSEBUF_IMPL_RBTREE

#include <uk/tree.h>

/* RBTree sparse buffer blocks consist of a single slice + tree metadata */
struct uk_sparsebuf_blk {
	UK_RB_ENTRY(uk_sparsebuf_blk) rb_entry;
	struct uk_sparsebuf_slice sl;
};

/* RBTree sparse buffer headblocks don't have any embedded slices */
#define UK__SPARSEBUF_EMBED_HEADBLK(sname, _)				\
struct sname {								\
	struct uk_sparsebuf_blk *head;					\
}

/* We only initialize the head block pointer; no slices need init */
#define UK__SPARSEBUF_EMBED_HEADBLK_INITIALIZER(_) { .head = __NULL }

/* No-op, we can directly get a blk ** from the headblock */
#define UK__SPARSEBUF_EMBED_HEAD(_, emb)

#define UK__SPARSEBUF_EMBED_HEADP(_, emb) (&(emb)->head)

/* A block uniquely identifies a slice; single pointer is enough */
struct uk_sparsebuf_cur {
	struct uk_sparsebuf_blk *p; /* __NULL signifies end of buffer */
};

static inline
int uk_sparsebuf_valid(const struct uk_sparsebuf_cur *cur)
{
	/* Any non-NULL block points to a valid slice */
	return !!cur->p;
}

static inline
int uk_sparsebuf_finished(const struct uk_sparsebuf_cur *cur)
{
	/* Invalid (NULL) blocks always mean end of buffer */
	return !uk_sparsebuf_valid(cur);
}

static inline
struct uk_sparsebuf_slice *uk_sparsebuf_slice_at(
	const struct uk_sparsebuf_cur *cur)
{
	UK_ASSERT(uk_sparsebuf_valid(cur));
	return &cur->p->sl;
}

/* Internal implementation functions called by inlines */
struct uk_sparsebuf_blk *uk_sparsebuf_rb_next(struct uk_sparsebuf_blk *blk);
struct uk_sparsebuf_blk *uk_sparsebuf_rb_find(struct uk_sparsebuf_blk **headp,
					      __u64 pgoff);

static inline
void uk_sparsebuf_advance(struct uk_sparsebuf_cur *cur)
{
	if (!uk_sparsebuf_finished(cur))
		cur->p = uk_sparsebuf_rb_next(cur->p);
}

static inline
int uk_sparsebuf_lookup(struct uk_sparsebuf_blk **headp, __u64 pgoff,
			struct uk_sparsebuf_cur *cur)
{
	struct uk_sparsebuf_blk *ret = uk_sparsebuf_rb_find(headp, pgoff);

	if (ret)
		cur->p = ret;
	return !!ret;
}

#endif /* CONFIG_LIBUKSPARSEBUF_IMPL_RBTREE */

#endif /* __UK_SPARSEBUF_IMPL_H__ */
