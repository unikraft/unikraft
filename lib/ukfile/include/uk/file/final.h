/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UKFILE_FINAL_H__
#define __UKFILE_FINAL_H__

#include <uk/mutex.h>
#include <uk/weak_refcount.h>

/*
 * Strong/weak reference counting with support for registering finalizers.
 * Finalizers are called when the last strong reference is released.
 */

typedef void (*uk_file_finalize_func)(void *);

/* Finalizer registration block */
struct uk_file_finalize_cb {
	struct uk_file_finalize_cb *next;
	uk_file_finalize_func fin; /* Finalizer */
	void *arg; /* Argument to pass to .fin() */
};

struct uk_file_finref {
	struct uk_swrefcount cnt;
	struct uk_file_finalize_cb *fins;
	struct uk_mutex lock; /* (Un)registration lock */
};

#define UK_FILE_FINREF_INITIALIZER(name, v) { \
	.cnt = UK_SWREFCOUNT_INITIALIZER((v), (v)), \
	.fins = NULL, \
	.lock = UK_MUTEX_INITIALIZER((name).lock) \
}

#define UK_FILE_FINREF_INIT_VALUE(name, v) \
	((struct uk_file_finref)UK_FILE_FINREF_INITIALIZER((name), (v)))

static inline
void uk_file_finref_acquire(struct uk_file_finref *r)
{
	uk_swrefcount_acquire(&r->cnt);
}

static inline
void uk_file_finref_acquire_weak(struct uk_file_finref *r)
{
	uk_swrefcount_acquire_weak(&r->cnt);
}

static inline
int uk_file_finref_release(struct uk_file_finref *r)
{
	return uk_swrefcount_release(&r->cnt);
}

static inline
void uk_file_finref_finalize(struct uk_file_finref *r)
{
	struct uk_file_finalize_cb *fin = r->fins;

	while (fin) {
		/* The call might free fin, read next before */
		struct uk_file_finalize_cb *next = fin->next;

		fin->fin(fin->arg);
		fin = next;
	}
}

static inline
int uk_file_finref_release_weak(struct uk_file_finref *r)
{
	return uk_swrefcount_release_weak(&r->cnt);
}

static inline
void uk_file_finref_register(struct uk_file_finref *r,
			     struct uk_file_finalize_cb *cb)
{
	UK_ASSERT(uk_refcount_read(&r->cnt.strong));
	uk_mutex_lock(&r->lock);
	cb->next = r->fins;
	r->fins = cb;
	uk_mutex_unlock(&r->lock);
}

static inline
void uk_file_finref_unregister(struct uk_file_finref *r,
			       struct uk_file_finalize_cb *cb)
{
	if (uk_refcount_read(&r->cnt.strong)) {
		struct uk_file_finalize_cb **p;

		uk_mutex_lock(&r->lock);
		for (p = &r->fins; *p && *p != cb; p = &(*p)->next);
		if (*p)
			*p = (*p)->next;
		uk_mutex_unlock(&r->lock);
	}
}

#endif /* __UKFILE_FINAL_H__ */
