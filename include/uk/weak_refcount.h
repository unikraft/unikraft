/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Strong/Weak reference counting */

#ifndef __UK_WEAK_REFCOUNT_H__
#define __UK_WEAK_REFCOUNT_H__

#include <uk/refcount.h>
#include <uk/assert.h>

struct uk_swrefcount {
	__atomic refcount; /* Total number of references */
	__atomic strong; /* Number of strong references; <= .refcount */
};

/*
 * We define initializers separate from an initial values.
 * The former can only be used in (static) variable initializations, while the
 * latter is meant for assigning to variables or as anonymous data structures.
 */
#define UK_SWREFCOUNT_INITIALIZER(r, s) { \
	.refcount = UK_REFCOUNT_INITIALIZER((r)), \
	.strong = UK_REFCOUNT_INITIALIZER((s)), \
}
#define UK_SWREFCOUNT_INIT_VALUE(r, s) \
	((struct uk_swrefcount)UK_SWREFCOUNT_INITIALIZER((r), (s)))

/**
 * Initialize refcount with `ref` references, of which `strong` are strong.
 */
static inline
void uk_swrefcount_init(struct uk_swrefcount *sw, __u32 ref, __u32 strong)
{
	UK_ASSERT(ref >= strong);
	uk_refcount_init(&sw->refcount, ref);
	uk_refcount_init(&sw->strong, strong);
}

/**
 * Acquire a weak reference on `sw`.
 */
static inline
void uk_swrefcount_acquire_weak(struct uk_swrefcount *sw)
{
	uk_refcount_acquire(&sw->refcount);
}

/**
 * Acquire a strong reference on `sw`.
 */
static inline
void uk_swrefcount_acquire(struct uk_swrefcount *sw)
{
	uk_swrefcount_acquire_weak(sw);
	uk_refcount_acquire(&sw->strong);
}

#define UK_SWREFCOUNT_LAST_STRONG 2 /* Released last strong reference */
#define UK_SWREFCOUNT_LAST_REF    1 /* Released last reference */

/**
 * Release a weak reference on `sw` and return whether it was the last.
 *
 * @return
 *   0 if there are more references left, or UK_SWREFCOUNT_LAST_REF if not.
 */
static inline
int uk_swrefcount_release_weak(struct uk_swrefcount *sw)
{
	return uk_refcount_release(&sw->refcount) ? UK_SWREFCOUNT_LAST_REF : 0;
}

/**
 * Release a strong reference on `sw` and return whether it was the last.
 *
 * @return
 *   0 if there are more references left, otherwise a bitwise OR of:
 *   UK_SWREFCOUNT_LAST_STRONG, if we released the last strong reference
 *   UK_SWREFCOUNT_LAST_REF, if we released the last reference
 */
static inline
int uk_swrefcount_release(struct uk_swrefcount *sw)
{
	int ret = 0;

	if (uk_refcount_release(&sw->strong))
		ret |= UK_SWREFCOUNT_LAST_STRONG;
	return ret | uk_swrefcount_release_weak(sw);
}

#endif /* __UK_WEAK_REFCOUNT_H__ */
