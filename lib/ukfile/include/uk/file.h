/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Fundamental abstraction for files in Unikraft. */

#ifndef __UKFILE_FILE_H__
#define __UKFILE_FILE_H__

#include <fcntl.h>
#include <sys/uio.h>

#include <uk/rwlock.h>
#include <uk/file/pollqueue.h>
#include <uk/file/statx.h>

#if CONFIG_LIBUKFILE_FINALIZERS
#include <uk/file/final.h>
#else /* !CONFIG_LIBUKFILE_FINALIZERS */
#include <uk/weak_refcount.h>
#endif /* !CONFIG_LIBUKFILE_FINALIZERS */


struct uk_file;

/* File operations, to be provided by drivers */

/* I/O functions are non-blocking & return -EAGAIN when unable to perform.
 * The behavior of concurrent calls to these functions is driver-dependent and
 * no general assumptions can be made about their ordering and/or interleaving.
 * Callers should themselves use the state->iolock (and/or other locks)
 * as appropriate in order to provide the desired concurrency guarantees.
 */

/* I/O */
typedef ssize_t (*uk_file_io_func)(const struct uk_file *f,
				   const struct iovec *iov, int iovcnt,
				   off_t off, long flags);

/* Info (stat-like & chXXX-like) */
typedef int (*uk_file_getstat_func)(const struct uk_file *f,
				    unsigned int mask,
				    struct uk_statx *arg);
typedef int (*uk_file_setstat_func)(const struct uk_file *f,
				    unsigned int mask,
				    const struct uk_statx *arg);

/* Control */
/* Values for the `fam` argument of file_ctl */
#define UKFILE_CTL_FILE  0    /* File controls (sync, allocation, etc.) */
#define UKFILE_CTL_IOCTL 1    /* Linux-compatible ioctl() requests */

/*
 * SYNC((int)all, void, void)
 * Flush modified file data & metadata to storage.
 * If all is 0, flush minimum of metadata, if 1 flush all metadata.
 */
#define UKFILE_CTL_FILE_SYNC 0

/*
 * TRUNC((off_t)len, void, void)
 * Truncate file to `len` bytes.
 */
#define UKFILE_CTL_FILE_TRUNC 1

/*
 * FALLOC((int)mode, (off_t)offset, (off_t)len)
 * Linux-compatible fallocate operation.
 */
#define UKFILE_CTL_FILE_FALLOC 2

/*
 * FADVISE((off_t)offset, (off_t)len, (int)advice)
 * Linux-compatible fadvise operations.
 */
#define UKFILE_CTL_FILE_FADVISE 3

typedef int (*uk_file_ctl_func)(const struct uk_file *f, int fam, int req,
				uintptr_t arg1, uintptr_t arg2, uintptr_t arg3);

/* Destructor */
/* what - bitwise OR of what to release:
 * UK_FILE_RELEASE_RES - file resources
 * UK_FILE_RELEASE_OBJ - file object
 */
#define UK_FILE_RELEASE_RES UK_SWREFCOUNT_LAST_STRONG
#define UK_FILE_RELEASE_OBJ UK_SWREFCOUNT_LAST_REF

typedef void (*uk_file_release_func)(const struct uk_file *f, int what);

struct uk_file_ops {
	uk_file_io_func read;
	uk_file_io_func write;
	uk_file_getstat_func getstat;
	uk_file_setstat_func setstat;
	uk_file_ctl_func ctl;
};

/* File struct */

struct uk_file_state {
	/* Synchronization for higher-level operations */
	struct uk_rwlock iolock;
	/* Polling & events */
	struct uk_pollq pollq;
	/* Voluntary locks (flock) */
	/* TODO */
};

static inline void uk_file_state_rlock(struct uk_file_state *st)
{
	uk_rwlock_rlock(&st->iolock);
}

static inline void uk_file_state_runlock(struct uk_file_state *st)
{
	uk_rwlock_runlock(&st->iolock);
}

static inline void uk_file_state_wlock(struct uk_file_state *st)
{
	uk_rwlock_wlock(&st->iolock);
}

static inline void uk_file_state_wunlock(struct uk_file_state *st)
{
	uk_rwlock_wunlock(&st->iolock);
}

/*
 * We define initializers separate from an initial values.
 * The former can only be used in (static) variable initializations, while the
 * latter is meant for assigning to variables or as anonymous data structures.
 */
#define UK_FILE_STATE_INITIALIZER(name) { \
	.iolock = UK_RWLOCK_INITIALIZER((name).iolock, 0), \
	.pollq = UK_POLLQ_INITIALIZER((name).pollq) \
}
#define UK_FILE_STATE_INIT_VALUE(name) \
	((struct uk_file_state)UK_FILE_STATE_INITIALIZER(name))


static inline
uk_pollevent uk_file_state_event_clear(struct uk_file_state *st,
				       uk_pollevent ev)
{
	return uk_pollq_clear(&st->pollq, ev);
}

static inline
uk_pollevent uk_file_state_event_set(struct uk_file_state *st, uk_pollevent ev)
{
	return uk_pollq_set(&st->pollq, ev);
}

static inline
uk_pollevent uk_file_state_event_assign(struct uk_file_state *st,
					uk_pollevent ev)
{
	return uk_pollq_assign(&st->pollq, ev);
}

/*
 * Reference count type used by uk_file.
 *
 * The exact reference count is an implementation detail that we do not wish to
 * expose to consumers. Drivers may, however, need to allocate and initialize
 * this structure; we therefore provide a typedef and initializer.
 */
#if CONFIG_LIBUKFILE_FINALIZERS
typedef struct uk_file_finref uk_file_refcnt;

#define UK_FILE_REFCNT_INITIALIZER(name) UK_FILE_FINREF_INITIALIZER((name), 1)
#define UK_FILE_REFCNT_INIT_VALUE(name) UK_FILE_FINREF_INIT_VALUE((name), 1)

#define uk_file_refcnt_acquire		uk_file_finref_acquire
#define uk_file_refcnt_acquire_weak	uk_file_finref_acquire_weak
#define uk_file_refcnt_release		uk_file_finref_release
#define uk_file_refcnt_release_weak	uk_file_finref_release_weak

#else /* !CONFIG_LIBUKFILE_FINALIZERS */
typedef struct uk_swrefcount uk_file_refcnt;

#define UK_FILE_REFCNT_INITIALIZER(name) UK_SWREFCOUNT_INITIALIZER(1, 1)
#define UK_FILE_REFCNT_INIT_VALUE(name) UK_SWREFCOUNT_INIT_VALUE(1, 1)

#define uk_file_refcnt_acquire		uk_swrefcount_acquire
#define uk_file_refcnt_acquire_weak	uk_swrefcount_acquire_weak
#define uk_file_refcnt_release		uk_swrefcount_release
#define uk_file_refcnt_release_weak	uk_swrefcount_release_weak

#endif /* !CONFIG_LIBUKFILE_FINALIZERS */
/* Files always get created with one strong reference held */
/* See above comment for file state on initializers vs initial values */

struct uk_file {
	/* Identity */
	const void *vol; /* Volume instance; needed to check file kind */
	void *node; /* Driver-specific inode data */
	/* Ops table */
	const struct uk_file_ops *ops;
	/* Mutable state (refcounting, poll events & locks) */
	uk_file_refcnt *refcnt;
	struct uk_file_state *state;
	/* Destructor, never call directly */
	uk_file_release_func _release;
};

/* Operations inlines */
static inline
ssize_t uk_file_read(const struct uk_file *f,
		     const struct iovec *iov, int iovcnt,
		     off_t off, long flags)
{
	return f->ops->read(f, iov, iovcnt, off, flags);
}

static inline
ssize_t uk_file_write(const struct uk_file *f,
		      const struct iovec *iov, int iovcnt,
		      off_t off, long flags)
{
	return f->ops->write(f, iov, iovcnt, off, flags);
}

static inline
int uk_file_getstat(const struct uk_file *f,
		    unsigned int mask, struct uk_statx *arg)
{
	return f->ops->getstat(f, mask, arg);
}

static inline
int uk_file_setstat(const struct uk_file *f,
		    unsigned int mask, const struct uk_statx *arg)
{
	return f->ops->setstat(f, mask, arg);
}

static inline
int uk_file_ctl(const struct uk_file *f, int fam, int req,
		uintptr_t arg1, uintptr_t arg2, uintptr_t arg3)
{
	return f->ops->ctl(f, fam, req, arg1, arg2, arg3);
}

/* Refcounting & destruction */

static inline
void uk_file_acquire(const struct uk_file *f)
{
	uk_file_refcnt_acquire(f->refcnt);
}

static inline
void uk_file_acquire_weak(const struct uk_file *f)
{
	uk_file_refcnt_acquire_weak(f->refcnt);
}

static inline
void uk_file_release(const struct uk_file *f)
{
	int r = uk_file_refcnt_release(f->refcnt);

	if (r)
		f->_release(f, r);
}

static inline
void uk_file_release_weak(const struct uk_file *f)
{
	int r = uk_file_refcnt_release_weak(f->refcnt);

	if (r)
		f->_release(f, r);
}

#if CONFIG_LIBUKFILE_FINALIZERS
static inline
void uk_file_finalizer_register(const struct uk_file *f,
				struct uk_file_finalize_cb *cb)
{
	uk_file_finref_register(f->refcnt, cb);
}

static inline
void uk_file_finalizer_unregister(const struct uk_file *f,
				  struct uk_file_finalize_cb *cb)
{
	uk_file_finref_unregister(f->refcnt, cb);
}
#endif /* CONFIG_LIBUKFILE_FINALIZERS */

/* High-level I/O locking */

static inline void uk_file_rlock(const struct uk_file *f)
{
	uk_file_state_rlock(f->state);
}

static inline void uk_file_runlock(const struct uk_file *f)
{
	uk_file_state_runlock(f->state);
}

static inline void uk_file_wlock(const struct uk_file *f)
{
	uk_file_state_wlock(f->state);
}

static inline void uk_file_wunlock(const struct uk_file *f)
{
	uk_file_state_wunlock(f->state);
}

/* Events & polling */

static inline
uk_pollevent uk_file_poll_immediate(const struct uk_file *f, uk_pollevent req)
{
	return uk_pollq_poll_immediate(&f->state->pollq, req);
}

static inline
uk_pollevent uk_file_poll_until(const struct uk_file *f, uk_pollevent req,
				__nsec deadline)
{
	return uk_pollq_poll_until(&f->state->pollq, req, deadline);
}

static inline
uk_pollevent uk_file_poll(const struct uk_file *f, uk_pollevent req)
{
	return uk_file_poll_until(f, req, 0);
}

static inline
uk_pollevent uk_file_event_clear(const struct uk_file *f, uk_pollevent clr)
{
	return uk_file_state_event_clear(f->state, clr);
}

static inline
uk_pollevent uk_file_event_set(const struct uk_file *f, uk_pollevent set)
{
	return uk_file_state_event_set(f->state, set);
}

static inline
uk_pollevent uk_file_event_assign(const struct uk_file *f, uk_pollevent set)
{
	return uk_file_state_event_assign(f->state, set);
}

#endif /* __UKFILE_FILE_H__ */
