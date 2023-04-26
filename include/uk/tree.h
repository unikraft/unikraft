/*	$NetBSD: tree.h,v 1.8 2004/03/28 19:38:30 provos Exp $	*/
/*	$OpenBSD: tree.h,v 1.7 2002/10/17 21:51:54 art Exp $	*/

/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright 2002 Niels Provos <provos@citi.umich.edu>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	__UK_TREE_H__
#define	__UK_TREE_H__

#include <uk/essentials.h>

/*
 * This file defines data structures for different types of trees:
 * splay trees and rank-balanced trees.
 *
 * A splay tree is a self-organizing data structure.  Every operation
 * on the tree causes a splay to happen.  The splay moves the requested
 * node to the root of the tree and partly rebalances it.
 *
 * This has the benefit that request locality causes faster lookups as
 * the requested nodes move to the top of the tree.  On the other hand,
 * every lookup causes memory writes.
 *
 * The Balance Theorem bounds the total access time for m operations
 * and n inserts on an initially empty tree as O((m + n)lg n).  The
 * amortized cost for a sequence of m accesses to a splay tree is O(lg n);
 *
 * A rank-balanced tree is a binary search tree with an integer
 * rank-difference as an attribute of each pointer from parent to child.
 * The sum of the rank-differences on any path from a node down to null is
 * the same, and defines the rank of that node. The rank of the null node
 * is -1.
 *
 * Different additional conditions define different sorts of balanced trees,
 * including "red-black" and "AVL" trees.  The set of conditions applied here
 * are the "weak-AVL" conditions of Haeupler, Sen and Tarjan presented in in
 * "Rank Balanced Trees", ACM Transactions on Algorithms Volume 11 Issue 4 June
 * 2015 Article No.: 30pp 1–26 https://doi.org/10.1145/2689412 (the HST paper):
 *	- every rank-difference is 1 or 2.
 *	- the rank of any leaf is 1.
 *
 * For historical reasons, rank differences that are even are associated
 * with the color red (Rank-Even-Difference), and the child that a red edge
 * points to is called a red child.
 *
 * Every operation on a rank-balanced tree is bounded as O(lg n).
 * The maximum height of a rank-balanced tree is 2lg (n+1).
 */

#define UK_SPLAY_HEAD(name, type)					\
struct name {								\
	struct type *sph_root; /* root of the tree */			\
}

#define UK_SPLAY_INITIALIZER(root)					\
	{ __NULL }

#define UK_SPLAY_INIT(root) do {					\
	(root)->sph_root = __NULL;					\
} while (/*CONSTCOND*/ 0)

#define UK_SPLAY_ENTRY(type)						\
struct {								\
	struct type *spe_left; /* left element */			\
	struct type *spe_right; /* right element */			\
}

#define UK_SPLAY_LEFT(elm, field)	(elm)->field.spe_left
#define UK_SPLAY_RIGHT(elm, field)	(elm)->field.spe_right
#define UK_SPLAY_ROOT(head)		(head)->sph_root
#define UK_SPLAY_EMPTY(head)		(UK_SPLAY_ROOT(head) == __NULL)

/* UK_SPLAY_ROTATE_{LEFT,RIGHT} expect that tmp hold UK_SPLAY_{RIGHT,LEFT} */
#define UK_SPLAY_ROTATE_RIGHT(head, tmp, field) do {			\
	UK_SPLAY_LEFT((head)->sph_root, field) = UK_SPLAY_RIGHT(tmp, field);\
	UK_SPLAY_RIGHT(tmp, field) = (head)->sph_root;			\
	(head)->sph_root = tmp;						\
} while (/*CONSTCOND*/ 0)

#define UK_SPLAY_ROTATE_LEFT(head, tmp, field) do {			\
	UK_SPLAY_RIGHT((head)->sph_root, field) = UK_SPLAY_LEFT(tmp, field);\
	UK_SPLAY_LEFT(tmp, field) = (head)->sph_root;			\
	(head)->sph_root = tmp;						\
} while (/*CONSTCOND*/ 0)

#define UK_SPLAY_LINKLEFT(head, tmp, field) do {			\
	UK_SPLAY_LEFT(tmp, field) = (head)->sph_root;			\
	tmp = (head)->sph_root;						\
	(head)->sph_root = UK_SPLAY_LEFT((head)->sph_root, field);	\
} while (/*CONSTCOND*/ 0)

#define UK_SPLAY_LINKRIGHT(head, tmp, field) do {			\
	UK_SPLAY_RIGHT(tmp, field) = (head)->sph_root;			\
	tmp = (head)->sph_root;						\
	(head)->sph_root = UK_SPLAY_RIGHT((head)->sph_root, field);	\
} while (/*CONSTCOND*/ 0)

#define UK_SPLAY_ASSEMBLE(head, node, left, right, field) do {		\
	UK_SPLAY_RIGHT(left, field) = UK_SPLAY_LEFT((head)->sph_root, field);\
	UK_SPLAY_LEFT(right, field) = UK_SPLAY_RIGHT((head)->sph_root, field);\
	UK_SPLAY_LEFT((head)->sph_root, field) = UK_SPLAY_RIGHT(node, field);\
	UK_SPLAY_RIGHT((head)->sph_root, field) = UK_SPLAY_LEFT(node, field);\
} while (/*CONSTCOND*/ 0)

/* Generates prototypes and inline functions */

#define UK_SPLAY_PROTOTYPE(name, type, field, cmp)			\
void name##_SPLAY(struct name *, struct type *);			\
void name##_SPLAY_MINMAX(struct name *, int);				\
struct type *name##_SPLAY_INSERT(struct name *, struct type *);		\
struct type *name##_SPLAY_REMOVE(struct name *, struct type *);		\
									\
/* Finds the node with the same key as elm */				\
static __unused __inline struct type *					\
name##_SPLAY_FIND(struct name *head, struct type *elm)			\
{									\
	if (UK_SPLAY_EMPTY(head))					\
		return(__NULL);						\
	name##_SPLAY(head, elm);					\
	if ((cmp)(elm, (head)->sph_root) == 0)				\
		return (head->sph_root);				\
	return (__NULL);						\
}									\
									\
static __unused __inline struct type *					\
name##_SPLAY_NEXT(struct name *head, struct type *elm)			\
{									\
	name##_SPLAY(head, elm);					\
	if (UK_SPLAY_RIGHT(elm, field) != __NULL) {			\
		elm = UK_SPLAY_RIGHT(elm, field);			\
		while (UK_SPLAY_LEFT(elm, field) != __NULL) {		\
			elm = UK_SPLAY_LEFT(elm, field);		\
		}							\
	} else								\
		elm = __NULL;						\
	return (elm);							\
}									\
									\
static __unused __inline struct type *					\
name##_SPLAY_MIN_MAX(struct name *head, int val)			\
{									\
	name##_SPLAY_MINMAX(head, val);					\
	return (UK_SPLAY_ROOT(head));					\
}

/* Main splay operation.
 * Moves node close to the key of elm to top
 */
#define UK_SPLAY_GENERATE(name, type, field, cmp)			\
struct type *								\
name##_SPLAY_INSERT(struct name *head, struct type *elm)		\
{									\
    if (UK_SPLAY_EMPTY(head)) {						\
	    UK_SPLAY_LEFT(elm, field) = UK_SPLAY_RIGHT(elm, field) = __NULL;\
    } else {								\
	    __typeof(cmp(__NULL, __NULL)) __comp;			\
	    name##_SPLAY(head, elm);					\
	    __comp = (cmp)(elm, (head)->sph_root);			\
	    if (__comp < 0) {						\
		    UK_SPLAY_LEFT(elm, field) = UK_SPLAY_LEFT((head)->sph_root, field);\
		    UK_SPLAY_RIGHT(elm, field) = (head)->sph_root;	\
		    UK_SPLAY_LEFT((head)->sph_root, field) = __NULL;	\
	    } else if (__comp > 0) {					\
		    UK_SPLAY_RIGHT(elm, field) = UK_SPLAY_RIGHT((head)->sph_root, field);\
		    UK_SPLAY_LEFT(elm, field) = (head)->sph_root;	\
		    UK_SPLAY_RIGHT((head)->sph_root, field) = __NULL;	\
	    } else							\
		    return ((head)->sph_root);				\
    }									\
    (head)->sph_root = (elm);						\
    return (__NULL);							\
}									\
									\
struct type *								\
name##_SPLAY_REMOVE(struct name *head, struct type *elm)		\
{									\
	struct type *__tmp;						\
	if (UK_SPLAY_EMPTY(head))					\
		return (__NULL);					\
	name##_SPLAY(head, elm);					\
	if ((cmp)(elm, (head)->sph_root) == 0) {			\
		if (UK_SPLAY_LEFT((head)->sph_root, field) == __NULL) {	\
			(head)->sph_root = UK_SPLAY_RIGHT((head)->sph_root, field);\
		} else {						\
			__tmp = UK_SPLAY_RIGHT((head)->sph_root, field);\
			(head)->sph_root = UK_SPLAY_LEFT((head)->sph_root, field);\
			name##_SPLAY(head, elm);			\
			UK_SPLAY_RIGHT((head)->sph_root, field) = __tmp;\
		}							\
		return (elm);						\
	}								\
	return (__NULL);						\
}									\
									\
void									\
name##_SPLAY(struct name *head, struct type *elm)			\
{									\
	struct type __node, *__left, *__right, *__tmp;			\
	__typeof(cmp(__NULL, __NULL)) __comp;				\
\
	UK_SPLAY_LEFT(&__node, field) = UK_SPLAY_RIGHT(&__node, field) = __NULL;\
	__left = __right = &__node;					\
\
	while ((__comp = (cmp)(elm, (head)->sph_root)) != 0) {		\
		if (__comp < 0) {					\
			__tmp = UK_SPLAY_LEFT((head)->sph_root, field);	\
			if (__tmp == __NULL)				\
				break;					\
			if ((cmp)(elm, __tmp) < 0){			\
				UK_SPLAY_ROTATE_RIGHT(head, __tmp, field);\
				if (UK_SPLAY_LEFT((head)->sph_root, field) == __NULL)\
					break;				\
			}						\
			UK_SPLAY_LINKLEFT(head, __right, field);	\
		} else if (__comp > 0) {				\
			__tmp = UK_SPLAY_RIGHT((head)->sph_root, field);\
			if (__tmp == __NULL)				\
				break;					\
			if ((cmp)(elm, __tmp) > 0){			\
				UK_SPLAY_ROTATE_LEFT(head, __tmp, field);\
				if (UK_SPLAY_RIGHT((head)->sph_root, field) == __NULL)\
					break;				\
			}						\
			UK_SPLAY_LINKRIGHT(head, __left, field);	\
		}							\
	}								\
	UK_SPLAY_ASSEMBLE(head, &__node, __left, __right, field);	\
}									\
									\
/* Splay with either the minimum or the maximum element			\
 * Used to find minimum or maximum element in tree.			\
 */									\
void name##_SPLAY_MINMAX(struct name *head, int __comp) \
{									\
	struct type __node, *__left, *__right, *__tmp;			\
\
	UK_SPLAY_LEFT(&__node, field) = UK_SPLAY_RIGHT(&__node, field) = __NULL;\
	__left = __right = &__node;					\
\
	while (1) {							\
		if (__comp < 0) {					\
			__tmp = UK_SPLAY_LEFT((head)->sph_root, field);	\
			if (__tmp == __NULL)				\
				break;					\
			if (__comp < 0){				\
				UK_SPLAY_ROTATE_RIGHT(head, __tmp, field);\
				if (UK_SPLAY_LEFT((head)->sph_root, field) == __NULL)\
					break;				\
			}						\
			UK_SPLAY_LINKLEFT(head, __right, field);	\
		} else if (__comp > 0) {				\
			__tmp = UK_SPLAY_RIGHT((head)->sph_root, field);\
			if (__tmp == __NULL)				\
				break;					\
			if (__comp > 0) {				\
				UK_SPLAY_ROTATE_LEFT(head, __tmp, field);\
				if (UK_SPLAY_RIGHT((head)->sph_root, field) == __NULL)\
					break;				\
			}						\
			UK_SPLAY_LINKRIGHT(head, __left, field);	\
		}							\
	}								\
	UK_SPLAY_ASSEMBLE(head, &__node, __left, __right, field);	\
}

#define UK_SPLAY_NEGINF	-1
#define UK_SPLAY_INF	1

#define UK_SPLAY_INSERT(name, x, y)	name##_SPLAY_INSERT(x, y)
#define UK_SPLAY_REMOVE(name, x, y)	name##_SPLAY_REMOVE(x, y)
#define UK_SPLAY_FIND(name, x, y)		name##_SPLAY_FIND(x, y)
#define UK_SPLAY_NEXT(name, x, y)		name##_SPLAY_NEXT(x, y)
#define UK_SPLAY_MIN(name, x)		(UK_SPLAY_EMPTY(x) ? __NULL	\
					: name##_SPLAY_MIN_MAX(x, UK_SPLAY_NEGINF))
#define UK_SPLAY_MAX(name, x)		(UK_SPLAY_EMPTY(x) ? __NULL	\
					: name##_SPLAY_MIN_MAX(x, UK_SPLAY_INF))

#define UK_SPLAY_FOREACH(x, name, head)					\
	for ((x) = UK_SPLAY_MIN(name, head);				\
	     (x) != __NULL;						\
	     (x) = UK_SPLAY_NEXT(name, head, x))

/* Macros that define a rank-balanced tree */
#define UK_RB_HEAD(name, type)						\
struct name {								\
	struct type *rbh_root; /* root of the tree */			\
}

#define UK_RB_INITIALIZER(root)						\
	{ __NULL }

#define UK_RB_INIT(root) do {						\
	(root)->rbh_root = __NULL;					\
} while (/*CONSTCOND*/ 0)

#define UK_RB_ENTRY(type)						\
struct {								\
	struct type *rbe_link[3];					\
}

/* Work-around for type-punning mechanism of the rb tree implementation */
typedef __uptr __may_alias _rb_uptr_ma;

/*
 * With the expectation that any object of struct type has an
 * address that is a multiple of 4, and that therefore the
 * 2 least significant bits of a pointer to struct type are
 * always zero, this implementation sets those bits to indicate
 * that the left or right child of the tree node is "red".
 */
#define UK__RB_LINK(elm, dir, field)	(elm)->field.rbe_link[dir]
#define UK__RB_UP(elm, field)		UK__RB_LINK(elm, 2, field)
#define UK__RB_L			((__uptr)1)
#define UK__RB_R			((__uptr)2)
#define UK__RB_LR			((__uptr)3)
#define UK__RB_BITS(elm)		(*(_rb_uptr_ma *)&elm)
#define UK__RB_BITSUP(elm, field)	UK__RB_BITS(UK__RB_UP(elm, field))
#define UK__RB_PTR(elm)			(__typeof(elm))			\
					((__uptr)elm & ~UK__RB_LR)

#define UK_RB_PARENT(elm, field)	UK__RB_PTR(UK__RB_UP(elm, field))
#define UK_RB_LEFT(elm, field)		UK__RB_LINK(elm, UK__RB_L-1, field)
#define UK_RB_RIGHT(elm, field)		UK__RB_LINK(elm, UK__RB_R-1, field)
#define UK_RB_ROOT(head)		(head)->rbh_root
#define UK_RB_EMPTY(head)		(UK_RB_ROOT(head) == __NULL)

#define UK_RB_SET_PARENT(dst, src, field) do {				\
	UK__RB_BITSUP(dst, field) = (__uptr)src |			\
	    (UK__RB_BITSUP(dst, field) & UK__RB_LR);			\
} while (/*CONSTCOND*/ 0)

#define UK_RB_SET(elm, parent, field) do {				\
	UK__RB_UP(elm, field) = parent;					\
	UK_RB_LEFT(elm, field) = UK_RB_RIGHT(elm, field) = __NULL;	\
} while (/*CONSTCOND*/ 0)

/*
 * Either UK_RB_AUGMENT or UK_RB_AUGMENT_CHECK is invoked in a loop at the root of
 * every modified subtree, from the bottom up to the root, to update augmented
 * node data.  UK_RB_AUGMENT_CHECK returns true only when the update changes the
 * node data, so that updating can be stopped short of the root when it returns
 * false.
 */
#ifndef UK_RB_AUGMENT_CHECK
#ifndef UK_RB_AUGMENT
#define UK_RB_AUGMENT_CHECK(x) 0
#else
#define UK_RB_AUGMENT_CHECK(x) (UK_RB_AUGMENT(x), 1)
#endif
#endif

#define UK_RB_UPDATE_AUGMENT(elm, field) do {				\
	__typeof(elm) rb_update_tmp = (elm);				\
	while (UK_RB_AUGMENT_CHECK(rb_update_tmp) &&			\
	    (rb_update_tmp = UK_RB_PARENT(rb_update_tmp, field)) != __NULL)\
		;							\
} while (0)

#define UK_RB_SWAP_CHILD(head, par, out, in, field) do {		\
	if (par == __NULL)						\
		UK_RB_ROOT(head) = (in);				\
	else if ((out) == UK_RB_LEFT(par, field))			\
		UK_RB_LEFT(par, field) = (in);				\
	else								\
		UK_RB_RIGHT(par, field) = (in);				\
} while (/*CONSTCOND*/ 0)

/*
 * UK_RB_ROTATE macro partially restructures the tree to improve balance. In the
 * case when dir is UK__RB_L, tmp is a right child of elm.  After rotation, elm
 * is a left child of tmp, and the subtree that represented the items between
 * them, which formerly hung to the left of tmp now hangs to the right of elm.
 * The parent-child relationship between elm and its former parent is not
 * changed; where this macro once updated those fields, that is now left to the
 * caller of UK_RB_ROTATE to clean up, so that a pair of rotations does not twice
 * update the same pair of pointer fields with distinct values.
 */
#define UK_RB_ROTATE(elm, tmp, dir, field) do {				\
	if ((UK__RB_LINK(elm, (dir ^ UK__RB_LR)-1, field) =		\
	    UK__RB_LINK(tmp, dir-1, field)) != __NULL)			\
		UK_RB_SET_PARENT(UK__RB_LINK(tmp, dir-1, field), elm, field);\
	UK__RB_LINK(tmp, dir-1, field) = (elm);				\
	UK_RB_SET_PARENT(elm, tmp, field);				\
} while (/*CONSTCOND*/ 0)

/* Generates prototypes and inline functions */
#define	UK_RB_PROTOTYPE(name, type, field, cmp)				\
	UK_RB_PROTOTYPE_INTERNAL(name, type, field, cmp,)
#define	UK_RB_PROTOTYPE_STATIC(name, type, field, cmp)			\
	UK_RB_PROTOTYPE_INTERNAL(name, type, field, cmp, __unused static)
#define UK_RB_PROTOTYPE_INTERNAL(name, type, field, cmp, attr)		\
	UK_RB_PROTOTYPE_RANK(name, type, attr)				\
	UK_RB_PROTOTYPE_DO_INSERT_COLOR(name, type, attr);		\
	UK_RB_PROTOTYPE_INSERT_COLOR(name, type, attr);			\
	UK_RB_PROTOTYPE_REMOVE_COLOR(name, type, attr);			\
	UK_RB_PROTOTYPE_INSERT_FINISH(name, type, attr);		\
	UK_RB_PROTOTYPE_INSERT(name, type, attr);			\
	UK_RB_PROTOTYPE_REMOVE(name, type, attr);			\
	UK_RB_PROTOTYPE_FIND(name, type, attr);				\
	UK_RB_PROTOTYPE_NFIND(name, type, attr);			\
	UK_RB_PROTOTYPE_NEXT(name, type, attr);				\
	UK_RB_PROTOTYPE_INSERT_NEXT(name, type, attr);			\
	UK_RB_PROTOTYPE_PREV(name, type, attr);				\
	UK_RB_PROTOTYPE_INSERT_PREV(name, type, attr);			\
	UK_RB_PROTOTYPE_MINMAX(name, type, attr);			\
	UK_RB_PROTOTYPE_REINSERT(name, type, attr);
#ifdef UK__RB_DIAGNOSTIC
#define UK_RB_PROTOTYPE_RANK(name, type, attr)				\
	attr int name##_RB_RANK(struct type *);
#else
#define UK_RB_PROTOTYPE_RANK(name, type, attr)
#endif
#define UK_RB_PROTOTYPE_DO_INSERT_COLOR(name, type, attr)		\
	attr struct type *name##_RB_DO_INSERT_COLOR(struct name *,	\
	    struct type *, struct type *)
#define UK_RB_PROTOTYPE_INSERT_COLOR(name, type, attr)			\
	attr struct type *name##_RB_INSERT_COLOR(struct name *, struct type *)
#define UK_RB_PROTOTYPE_REMOVE_COLOR(name, type, attr)			\
	attr struct type *name##_RB_REMOVE_COLOR(struct name *,		\
	    struct type *, struct type *)
#define UK_RB_PROTOTYPE_REMOVE(name, type, attr)			\
	attr struct type *name##_RB_REMOVE(struct name *, struct type *)
#define UK_RB_PROTOTYPE_INSERT_FINISH(name, type, attr)			\
	attr struct type *name##_RB_INSERT_FINISH(struct name *,	\
	    struct type *, struct type **, struct type *)
#define UK_RB_PROTOTYPE_INSERT(name, type, attr)			\
	attr struct type *name##_RB_INSERT(struct name *, struct type *)
#define UK_RB_PROTOTYPE_FIND(name, type, attr)				\
	attr struct type *name##_RB_FIND(struct name *, struct type *)
#define UK_RB_PROTOTYPE_NFIND(name, type, attr)				\
	attr struct type *name##_RB_NFIND(struct name *, struct type *)
#define UK_RB_PROTOTYPE_NEXT(name, type, attr)				\
	attr struct type *name##_RB_NEXT(struct type *)
#define UK_RB_PROTOTYPE_INSERT_NEXT(name, type, attr)			\
	attr struct type *name##_RB_INSERT_NEXT(struct name *,		\
	    struct type *, struct type *)
#define UK_RB_PROTOTYPE_PREV(name, type, attr)				\
	attr struct type *name##_RB_PREV(struct type *)
#define UK_RB_PROTOTYPE_INSERT_PREV(name, type, attr)			\
	attr struct type *name##_RB_INSERT_PREV(struct name *,		\
	    struct type *, struct type *)
#define UK_RB_PROTOTYPE_MINMAX(name, type, attr)			\
	attr struct type *name##_RB_MINMAX(struct name *, int)
#define UK_RB_PROTOTYPE_REINSERT(name, type, attr)			\
	attr struct type *name##_RB_REINSERT(struct name *, struct type *)

/* Main rb operation.
 * Moves node close to the key of elm to top
 */
#define	UK_RB_GENERATE(name, type, field, cmp)				\
	UK_RB_GENERATE_INTERNAL(name, type, field, cmp,)
#define	UK_RB_GENERATE_STATIC(name, type, field, cmp)			\
	UK_RB_GENERATE_INTERNAL(name, type, field, cmp, __unused static)
#define UK_RB_GENERATE_INTERNAL(name, type, field, cmp, attr)		\
	UK_RB_GENERATE_RANK(name, type, field, attr)			\
	UK_RB_GENERATE_DO_INSERT_COLOR(name, type, field, attr)		\
	UK_RB_GENERATE_INSERT_COLOR(name, type, field, attr)		\
	UK_RB_GENERATE_REMOVE_COLOR(name, type, field, attr)		\
	UK_RB_GENERATE_INSERT_FINISH(name, type, field, attr)		\
	UK_RB_GENERATE_INSERT(name, type, field, cmp, attr)		\
	UK_RB_GENERATE_REMOVE(name, type, field, attr)			\
	UK_RB_GENERATE_FIND(name, type, field, cmp, attr)		\
	UK_RB_GENERATE_NFIND(name, type, field, cmp, attr)		\
	UK_RB_GENERATE_NEXT(name, type, field, attr)			\
	UK_RB_GENERATE_INSERT_NEXT(name, type, field, cmp, attr)	\
	UK_RB_GENERATE_PREV(name, type, field, attr)			\
	UK_RB_GENERATE_INSERT_PREV(name, type, field, cmp, attr)	\
	UK_RB_GENERATE_MINMAX(name, type, field, attr)			\
	UK_RB_GENERATE_REINSERT(name, type, field, cmp, attr)

#ifdef UK__RB_DIAGNOSTIC
#ifndef UK_RB_AUGMENT
#define UK__RB_AUGMENT_VERIFY(x) UK_RB_AUGMENT_CHECK(x)
#else
#define UK__RB_AUGMENT_VERIFY(x) 0
#endif
#define UK_RB_GENERATE_RANK(name, type, field, attr)			\
/*									\
 * Return the rank of the subtree rooted at elm, or -1 if the subtree	\
 * is not rank-balanced, or has inconsistent augmentation data.
 */									\
attr int								\
name##_RB_RANK(struct type *elm)					\
{									\
	struct type *left, *right, *up;					\
	int left_rank, right_rank;					\
									\
	if (elm == __NULL)						\
		return (0);						\
	up = UK__RB_UP(elm, field);					\
	left = UK_RB_LEFT(elm, field);					\
	left_rank = ((UK__RB_BITS(up) & UK__RB_L) ? 2 : 1) +		\
	    name##_RB_RANK(left);					\
	right = UK_RB_RIGHT(elm, field);				\
	right_rank = ((UK__RB_BITS(up) & UK__RB_R) ? 2 : 1) +		\
	    name##_RB_RANK(right);					\
	if (left_rank != right_rank ||					\
	    (left_rank == 2 && left == __NULL && right == __NULL) ||	\
	    UK__RB_AUGMENT_VERIFY(elm))					\
		return (-1);						\
	return (left_rank);						\
}
#else
#define UK_RB_GENERATE_RANK(name, type, field, attr)
#endif

#define UK_RB_GENERATE_DO_INSERT_COLOR(name, type, field, attr)		\
attr struct type *							\
name##_RB_DO_INSERT_COLOR(struct name *head,				\
    struct type *parent, struct type *elm)				\
{									\
	/*								\
	 * Initially, elm is a leaf.  Either its parent was previously	\
	 * a leaf, with two black null children, or an interior node	\
	 * with a black non-null child and a red null child. The        \
	 * balance criterion "the rank of any leaf is 1" precludes the  \
	 * possibility of two red null children for the initial parent. \
	 * So the first loop iteration cannot lead to accessing an      \
	 * uninitialized 'child', and a later iteration can only happen \
	 * when a value has been assigned to 'child' in the previous    \
	 * one.								\
	 */								\
	struct type *child, *child_up, *gpar;				\
	__uptr elmdir, sibdir;						\
									\
	do {								\
		/* the rank of the tree rooted at elm grew */		\
		gpar = UK__RB_UP(parent, field);			\
		elmdir = UK_RB_RIGHT(parent, field) == elm ? UK__RB_R : UK__RB_L; \
		if (UK__RB_BITS(gpar) & elmdir) {			\
			/* shorten the parent-elm edge to rebalance */	\
			UK__RB_BITSUP(parent, field) ^= elmdir;		\
			return (__NULL);				\
		}							\
		sibdir = elmdir ^ UK__RB_LR;				\
		/* the other edge must change length */			\
		UK__RB_BITSUP(parent, field) ^= sibdir;			\
		if ((UK__RB_BITS(gpar) & UK__RB_LR) == 0) {		\
			/* both edges now short, retry from parent */	\
			child = elm;					\
			elm = parent;					\
			continue;					\
		}							\
		UK__RB_UP(parent, field) = gpar = UK__RB_PTR(gpar);	\
		if (UK__RB_BITSUP(elm, field) & elmdir) {		\
			/*						\
			 * Exactly one of the edges descending from elm \
			 * is long. The long one is in the same		\
			 * direction as the edge from parent to elm,	\
			 * so change that by rotation.  The edge from 	\
			 * parent to z was shortened above.  Shorten	\
			 * the long edge down from elm, and adjust	\
			 * other edge lengths based on the downward	\
			 * edges from 'child'.				\
			 *						\
			 *	     par		 par		\
			 *	    /	\		/   \		\
			 *	  elm	 z	       /     z		\
			 *	 /  \		     child		\
			 *	/  child	     /	 \		\
			 *     /   /  \		   elm 	  \		\
			 *    w	  /    \	  /   \    y		\
			 *     	 x      y	 w     \     		\
			 *				x		\
			 */						\
			UK_RB_ROTATE(elm, child, elmdir, field);	\
			child_up = UK__RB_UP(child, field);		\
			if (UK__RB_BITS(child_up) & sibdir)		\
				UK__RB_BITSUP(parent, field) ^= elmdir;	\
			if (UK__RB_BITS(child_up) & elmdir)		\
				UK__RB_BITSUP(elm, field) ^= UK__RB_LR;	\
			else						\
				UK__RB_BITSUP(elm, field) ^= elmdir;	\
			/* if child is a leaf, don't augment elm,	\
			 * since it is restored to be a leaf again. */	\
			if ((UK__RB_BITS(child_up) & UK__RB_LR) == 0)	\
				elm = child;				\
		} else							\
			child = elm;					\
									\
		/*							\
		 * The long edge descending from 'child' points back	\
		 * in the direction of 'parent'. Rotate to make		\
		 * 'parent' a child of 'child', then make both edges	\
		 * of 'child' short to rebalance.			\
		 *							\
		 *	     par		 child			\
		 *	    /	\		/     \			\
		 *	   /	 z	       x       par		\
		 *	child			      /	  \		\
		 *	 /  \			     /	   z		\
		 *	x    \			    y			\
		 *	      y						\
		 */							\
		UK_RB_ROTATE(parent, child, sibdir, field);		\
		UK__RB_UP(child, field) = gpar;				\
		UK_RB_SWAP_CHILD(head, gpar, parent, child, field);	\
		/*							\
		 * Elements rotated down have new, smaller subtrees,	\
		 * so update augmentation for them.			\
		 */							\
		if (elm != child)					\
			(void)UK_RB_AUGMENT_CHECK(elm);			\
		(void)UK_RB_AUGMENT_CHECK(parent);			\
		return (child);						\
	} while ((parent = gpar) != __NULL);				\
	return (__NULL);						\
}

#define UK_RB_GENERATE_INSERT_COLOR(name, type, field, attr)		\
attr struct type *							\
name##_RB_INSERT_COLOR(struct name *head, struct type *elm)		\
{									\
	struct type *parent, *tmp;					\
									\
	parent = UK_RB_PARENT(elm, field);				\
	if (parent != __NULL)						\
		tmp = name##_RB_DO_INSERT_COLOR(head, parent, elm);	\
	else								\
		tmp = __NULL;						\
	return (tmp);							\
}

#ifndef UK_RB_STRICT_HST
/*
 * In REMOVE_COLOR, the HST paper, in figure 3, in the single-rotate case, has
 * 'parent' with one higher rank, and then reduces its rank if 'parent' has
 * become a leaf.  This implementation always has the parent in its new position
 * with lower rank, to avoid the leaf check.  Define UK_RB_STRICT_HST to 1 to get
 * the behavior that HST describes.
 */
#define UK_RB_STRICT_HST 0
#endif

#define UK_RB_GENERATE_REMOVE_COLOR(name, type, field, attr)		\
attr struct type *							\
name##_RB_REMOVE_COLOR(struct name *head,				\
    struct type *parent, struct type *elm)				\
{									\
	struct type *gpar, *sib, *up;					\
	__uptr elmdir, sibdir;						\
									\
	if (UK_RB_RIGHT(parent, field) == elm &&			\
	    UK_RB_LEFT(parent, field) == elm) {				\
		/* Deleting a leaf that is an only-child creates a	\
		 * rank-2 leaf. Demote that leaf. */			\
		UK__RB_UP(parent, field) = UK__RB_PTR(UK__RB_UP(parent, field));\
		elm = parent;						\
		if ((parent = UK__RB_UP(elm, field)) == __NULL)		\
			return (__NULL);				\
	}								\
	do {								\
		/* the rank of the tree rooted at elm shrank */		\
		gpar = UK__RB_UP(parent, field);			\
		elmdir = UK_RB_RIGHT(parent, field) == elm ? UK__RB_R : UK__RB_L; \
		UK__RB_BITS(gpar) ^= elmdir;				\
		if (UK__RB_BITS(gpar) & elmdir) {			\
			/* lengthen the parent-elm edge to rebalance */	\
			UK__RB_UP(parent, field) = gpar;		\
			return (__NULL);				\
		}							\
		if (UK__RB_BITS(gpar) & UK__RB_LR) {			\
			/* shorten other edge, retry from parent */	\
			UK__RB_BITS(gpar) ^= UK__RB_LR;			\
			UK__RB_UP(parent, field) = gpar;		\
			gpar = UK__RB_PTR(gpar);			\
			continue;					\
		}							\
		sibdir = elmdir ^ UK__RB_LR;				\
		sib = UK__RB_LINK(parent, sibdir-1, field);		\
		up = UK__RB_UP(sib, field);				\
		UK__RB_BITS(up) ^= UK__RB_LR;				\
		if ((UK__RB_BITS(up) & UK__RB_LR) == 0) {		\
			/* shorten edges descending from sib, retry */	\
			UK__RB_UP(sib, field) = up;			\
			continue;					\
		}							\
		if ((UK__RB_BITS(up) & sibdir) == 0) {			\
			/*						\
			 * The edge descending from 'sib' away from	\
			 * 'parent' is long.  The short edge descending	\
			 * from 'sib' toward 'parent' points to 'elm*'	\
			 * Rotate to make 'sib' a child of 'elm*'	\
			 * then adjust the lengths of the edges		\
			 * descending from 'sib' and 'elm*'.		\
			 *						\
			 *	     par		 par		\
			 *	    /	\		/   \		\
			 *	   /	sib	      elm    \  	\
			 *	  /	/ \	            elm*	\
			 *	elm   elm* \	            /  \ 	\
			 *	      /	\   \	       	   /    \	\
			 *	     /   \   z	    	  /      \	\
			 *	    x	  y    		 x      sib 	\
			 *				        /  \	\
			 *				       /    z	\
			 *				      y 	\
			 */						\
			elm = UK__RB_LINK(sib, elmdir-1, field);	\
			/* elm is a 1-child.  First rotate at elm. */	\
			UK_RB_ROTATE(sib, elm, sibdir, field);		\
			up = UK__RB_UP(elm, field);			\
			UK__RB_BITSUP(parent, field) ^=			\
			    (UK__RB_BITS(up) & elmdir) ? UK__RB_LR : elmdir;\
			UK__RB_BITSUP(sib, field) ^=			\
			    (UK__RB_BITS(up) & sibdir) ? UK__RB_LR : sibdir;\
			UK__RB_BITSUP(elm, field) |= UK__RB_LR;		\
		} else {						\
			if ((UK__RB_BITS(up) & elmdir) == 0 &&		\
			    UK_RB_STRICT_HST && elm != __NULL) {	\
				/* if parent does not become a leaf,	\
				   do not demote parent yet. */		\
				UK__RB_BITSUP(parent, field) ^= sibdir;	\
				UK__RB_BITSUP(sib, field) ^= UK__RB_LR;	\
			} else if ((UK__RB_BITS(up) & elmdir) == 0) {	\
				/* demote parent. */			\
				UK__RB_BITSUP(parent, field) ^= elmdir;	\
				UK__RB_BITSUP(sib, field) ^= sibdir;	\
			} else						\
				UK__RB_BITSUP(sib, field) ^= sibdir;	\
			elm = sib;					\
		}							\
									\
		/*							\
		 * The edge descending from 'elm' away from 'parent'	\
		 * is short.  Rotate to make 'parent' a child of 'elm', \
		 * then lengthen the short edges descending from	\
		 * 'parent' and 'elm' to rebalance.			\
		 *							\
		 *	     par		 elm			\
		 *	    /	\		/   \			\
		 *	   e	 \	       /     \			\
		 *		 elm	      /	      \			\
		 *		/  \	    par	       s		\
		 *	       /    \	   /   \			\
		 *	      /	     \	  e	\			\
		 *	     x	      s		 x			\
		 */							\
		UK_RB_ROTATE(parent, elm, elmdir, field);		\
		UK_RB_SET_PARENT(elm, gpar, field);			\
		UK_RB_SWAP_CHILD(head, gpar, parent, elm, field);	\
		/*							\
		 * An element rotated down, but not into the search	\
		 * path has a new, smaller subtree, so update		\
		 * augmentation for it.					\
		 */							\
		if (sib != elm)						\
			(void)UK_RB_AUGMENT_CHECK(sib);			\
		return (parent);					\
	} while (elm = parent, (parent = gpar) != __NULL);		\
	return (__NULL);						\
}

#define UK__RB_AUGMENT_WALK(elm, match, field)				\
do {									\
	if (match == elm)						\
		match = __NULL;						\
} while (UK_RB_AUGMENT_CHECK(elm) &&					\
    (elm = UK_RB_PARENT(elm, field)) != __NULL)

#define UK_RB_GENERATE_REMOVE(name, type, field, attr)			\
attr struct type *							\
name##_RB_REMOVE(struct name *head, struct type *out)			\
{									\
	struct type *child, *in, *opar, *parent;			\
									\
	child = UK_RB_LEFT(out, field);					\
	in = UK_RB_RIGHT(out, field);					\
	opar = UK__RB_UP(out, field);					\
	if (in == __NULL || child == __NULL) {				\
		in = child = (in == __NULL ? child : in);		\
		parent = opar = UK__RB_PTR(opar);			\
	} else {							\
		parent = in;						\
		while (UK_RB_LEFT(in, field))				\
			in = UK_RB_LEFT(in, field);			\
		UK_RB_SET_PARENT(child, in, field);			\
		UK_RB_LEFT(in, field) = child;				\
		child = UK_RB_RIGHT(in, field);				\
		if (parent != in) {					\
			UK_RB_SET_PARENT(parent, in, field);		\
			UK_RB_RIGHT(in, field) = parent;		\
			parent = UK_RB_PARENT(in, field);		\
			UK_RB_LEFT(parent, field) = child;		\
		}							\
		UK__RB_UP(in, field) = opar;				\
		opar = UK__RB_PTR(opar);				\
	}								\
	UK_RB_SWAP_CHILD(head, opar, out, in, field);			\
	if (child != __NULL)						\
		UK__RB_UP(child, field) = parent;			\
	if (parent != __NULL) {						\
		opar = name##_RB_REMOVE_COLOR(head, parent, child);	\
		/* if rotation has made 'parent' the root of the same	\
		 * subtree as before, don't re-augment it. */		\
		if (parent == in && UK_RB_LEFT(parent, field) == __NULL) {\
			opar = __NULL;					\
			parent = UK_RB_PARENT(parent, field);		\
		}							\
		UK__RB_AUGMENT_WALK(parent, opar, field);		\
		if (opar != __NULL) {					\
			/*						\
			 * Elements rotated into the search path have	\
			 * changed subtrees, so update augmentation for	\
			 * them if AUGMENT_WALK didn't.			\
			 */						\
			(void)UK_RB_AUGMENT_CHECK(opar);		\
			(void)UK_RB_AUGMENT_CHECK(UK_RB_PARENT(opar, field));\
		}							\
	}								\
	return (out);							\
}

#define UK_RB_GENERATE_INSERT_FINISH(name, type, field, attr)		\
/* Inserts a node into the RB tree */					\
attr struct type *							\
name##_RB_INSERT_FINISH(struct name *head, struct type *parent,		\
    struct type **pptr, struct type *elm)				\
{									\
	struct type *tmp = __NULL;					\
									\
	UK_RB_SET(elm, parent, field);					\
	*pptr = elm;							\
	if (parent != __NULL)						\
		tmp = name##_RB_DO_INSERT_COLOR(head, parent, elm);	\
	UK__RB_AUGMENT_WALK(elm, tmp, field);				\
	if (tmp != __NULL)						\
		/*							\
		 * An element rotated into the search path has a	\
		 * changed subtree, so update augmentation for it if	\
		 * AUGMENT_WALK didn't.					\
		 */							\
		(void)UK_RB_AUGMENT_CHECK(tmp);				\
	return (__NULL);						\
}

#define UK_RB_GENERATE_INSERT(name, type, field, cmp, attr)		\
/* Inserts a node into the RB tree */					\
attr struct type *							\
name##_RB_INSERT(struct name *head, struct type *elm)			\
{									\
	struct type *tmp;						\
	struct type **tmpp = &UK_RB_ROOT(head);				\
	struct type *parent = __NULL;					\
									\
	while ((tmp = *tmpp) != __NULL) {				\
		parent = tmp;						\
		__typeof(cmp(__NULL, __NULL)) comp = (cmp)(elm, parent);\
		if (comp < 0)						\
			tmpp = &UK_RB_LEFT(parent, field);		\
		else if (comp > 0)					\
			tmpp = &UK_RB_RIGHT(parent, field);		\
		else							\
			return (parent);				\
	}								\
	return (name##_RB_INSERT_FINISH(head, parent, tmpp, elm));	\
}

#define UK_RB_GENERATE_FIND(name, type, field, cmp, attr)		\
/* Finds the node with the same key as elm */				\
attr struct type *							\
name##_RB_FIND(struct name *head, struct type *elm)			\
{									\
	struct type *tmp = UK_RB_ROOT(head);				\
	__typeof(cmp(__NULL, __NULL)) comp;				\
	while (tmp) {							\
		comp = cmp(elm, tmp);					\
		if (comp < 0)						\
			tmp = UK_RB_LEFT(tmp, field);			\
		else if (comp > 0)					\
			tmp = UK_RB_RIGHT(tmp, field);			\
		else							\
			return (tmp);					\
	}								\
	return (__NULL);						\
}

#define UK_RB_GENERATE_NFIND(name, type, field, cmp, attr)		\
/* Finds the first node greater than or equal to the search key */	\
attr struct type *							\
name##_RB_NFIND(struct name *head, struct type *elm)			\
{									\
	struct type *tmp = UK_RB_ROOT(head);				\
	struct type *res = __NULL;					\
	__typeof(cmp(__NULL, __NULL)) comp;				\
	while (tmp) {							\
		comp = cmp(elm, tmp);					\
		if (comp < 0) {						\
			res = tmp;					\
			tmp = UK_RB_LEFT(tmp, field);			\
		}							\
		else if (comp > 0)					\
			tmp = UK_RB_RIGHT(tmp, field);			\
		else							\
			return (tmp);					\
	}								\
	return (res);							\
}

#define UK_RB_GENERATE_NEXT(name, type, field, attr)			\
/* ARGSUSED */								\
attr struct type *							\
name##_RB_NEXT(struct type *elm)					\
{									\
	if (UK_RB_RIGHT(elm, field)) {					\
		elm = UK_RB_RIGHT(elm, field);				\
		while (UK_RB_LEFT(elm, field))				\
			elm = UK_RB_LEFT(elm, field);			\
	} else {							\
		while (UK_RB_PARENT(elm, field) &&			\
		    (elm == UK_RB_RIGHT(UK_RB_PARENT(elm, field), field)))\
			elm = UK_RB_PARENT(elm, field);			\
		elm = UK_RB_PARENT(elm, field);				\
	}								\
	return (elm);							\
}

#if defined(_KERNEL) && defined(DIAGNOSTIC)
#define UK__RB_ORDER_CHECK(cmp, lo, hi) do {				\
	KASSERT((cmp)(lo, hi) < 0, ("out of order insertion"));		\
} while (0)
#else
#define UK__RB_ORDER_CHECK(cmp, lo, hi) do {} while (0)
#endif

#define UK_RB_GENERATE_INSERT_NEXT(name, type, field, cmp, attr)	\
/* Inserts a node into the next position in the RB tree */		\
attr struct type *							\
name##_RB_INSERT_NEXT(struct name *head,				\
    struct type *elm, struct type *next)				\
{									\
	struct type *tmp;						\
	struct type **tmpp = &UK_RB_RIGHT(elm, field);			\
									\
	UK__RB_ORDER_CHECK(cmp, elm, next);				\
	if (name##_RB_NEXT(elm) != __NULL)				\
		UK__RB_ORDER_CHECK(cmp, next, name##_RB_NEXT(elm));	\
	while ((tmp = *tmpp) != __NULL) {				\
		elm = tmp;						\
		tmpp = &UK_RB_LEFT(elm, field);				\
	}								\
	return (name##_RB_INSERT_FINISH(head, elm, tmpp, next));	\
}

#define UK_RB_GENERATE_PREV(name, type, field, attr)			\
/* ARGSUSED */								\
attr struct type *							\
name##_RB_PREV(struct type *elm)					\
{									\
	if (UK_RB_LEFT(elm, field)) {					\
		elm = UK_RB_LEFT(elm, field);				\
		while (UK_RB_RIGHT(elm, field))				\
			elm = UK_RB_RIGHT(elm, field);			\
	} else {							\
		while (UK_RB_PARENT(elm, field) &&			\
		    (elm == UK_RB_LEFT(UK_RB_PARENT(elm, field), field)))\
			elm = UK_RB_PARENT(elm, field);			\
		elm = UK_RB_PARENT(elm, field);				\
	}								\
	return (elm);							\
}

#define UK_RB_GENERATE_INSERT_PREV(name, type, field, cmp, attr)	\
/* Inserts a node into the prev position in the RB tree */		\
attr struct type *							\
name##_RB_INSERT_PREV(struct name *head,				\
    struct type *elm, struct type *prev)				\
{									\
	struct type *tmp;						\
	struct type **tmpp = &UK_RB_LEFT(elm, field);			\
									\
	UK__RB_ORDER_CHECK(cmp, prev, elm);				\
	if (name##_RB_PREV(elm) != __NULL)				\
		UK__RB_ORDER_CHECK(cmp, name##_RB_PREV(elm), prev);	\
	while ((tmp = *tmpp) != __NULL) {				\
		elm = tmp;						\
		tmpp = &UK_RB_RIGHT(elm, field);			\
	}								\
	return (name##_RB_INSERT_FINISH(head, elm, tmpp, prev));	\
}

#define UK_RB_GENERATE_MINMAX(name, type, field, attr)			\
attr struct type *							\
name##_RB_MINMAX(struct name *head, int val)				\
{									\
	struct type *tmp = UK_RB_ROOT(head);				\
	struct type *parent = __NULL;					\
	while (tmp) {							\
		parent = tmp;						\
		if (val < 0)						\
			tmp = UK_RB_LEFT(tmp, field);			\
		else							\
			tmp = UK_RB_RIGHT(tmp, field);			\
	}								\
	return (parent);						\
}

#define	UK_RB_GENERATE_REINSERT(name, type, field, cmp, attr)		\
attr struct type *							\
name##_RB_REINSERT(struct name *head, struct type *elm)			\
{									\
	struct type *cmpelm;						\
	if (((cmpelm = UK_RB_PREV(name, head, elm)) != __NULL &&	\
	    cmp(cmpelm, elm) >= 0) ||					\
	    ((cmpelm = UK_RB_NEXT(name, head, elm)) != __NULL &&	\
	    cmp(elm, cmpelm) >= 0)) {					\
		/* XXXLAS: Remove/insert is heavy handed. */		\
		UK_RB_REMOVE(name, head, elm);				\
		return (UK_RB_INSERT(name, head, elm));			\
	}								\
	return (__NULL);						\
}									\

#define UK_RB_NEGINF	-1
#define UK_RB_INF	1

#define UK_RB_INSERT(name, x, y)	name##_RB_INSERT(x, y)
#define UK_RB_INSERT_NEXT(name, x, y, z)	name##_RB_INSERT_NEXT(x, y, z)
#define UK_RB_INSERT_PREV(name, x, y, z)	name##_RB_INSERT_PREV(x, y, z)
#define UK_RB_REMOVE(name, x, y)	name##_RB_REMOVE(x, y)
#define UK_RB_FIND(name, x, y)	name##_RB_FIND(x, y)
#define UK_RB_NFIND(name, x, y)	name##_RB_NFIND(x, y)
#define UK_RB_NEXT(name, x, y)	name##_RB_NEXT(y)
#define UK_RB_PREV(name, x, y)	name##_RB_PREV(y)
#define UK_RB_MIN(name, x)		name##_RB_MINMAX(x, UK_RB_NEGINF)
#define UK_RB_MAX(name, x)		name##_RB_MINMAX(x, UK_RB_INF)
#define UK_RB_REINSERT(name, x, y)	name##_RB_REINSERT(x, y)

#define UK_RB_FOREACH(x, name, head)					\
	for ((x) = UK_RB_MIN(name, head);				\
	     (x) != __NULL;						\
	     (x) = name##_RB_NEXT(x))

#define UK_RB_FOREACH_FROM(x, name, y)					\
	for ((x) = (y);							\
	    ((x) != __NULL) && ((y) = name##_RB_NEXT(x), (x) != __NULL);\
	     (x) = (y))

#define UK_RB_FOREACH_SAFE(x, name, head, y)				\
	for ((x) = UK_RB_MIN(name, head);				\
	    ((x) != __NULL) && ((y) = name##_RB_NEXT(x), (x) != __NULL);\
	     (x) = (y))

#define UK_RB_FOREACH_REVERSE(x, name, head)				\
	for ((x) = UK_RB_MAX(name, head);				\
	     (x) != __NULL;						\
	     (x) = name##_RB_PREV(x))

#define UK_RB_FOREACH_REVERSE_FROM(x, name, y)				\
	for ((x) = (y);							\
	    ((x) != __NULL) && ((y) = name##_RB_PREV(x), (x) != __NULL);\
	     (x) = (y))

#define UK_RB_FOREACH_REVERSE_SAFE(x, name, head, y)			\
	for ((x) = UK_RB_MAX(name, head);				\
	    ((x) != __NULL) && ((y) = name##_RB_PREV(x), (x) != __NULL);\
	     (x) = (y))

#endif	/* __UK_TREE_H__ */
