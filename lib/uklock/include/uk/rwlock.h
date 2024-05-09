/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_RWLOCK_H__
#define __UK_RWLOCK_H__

#include <uk/config.h>

#if CONFIG_LIBUKLOCK_RWLOCK
#include <uk/essentials.h>
#include <uk/spinlock.h>
#include <uk/wait.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UK_RWLOCK_CONFIG_WRITE_RECURSE 0x01 /* Recursive locking for writers */

struct __align(8) uk_rwlock {
	/** Number of active readers, -1 if writer active */
	volatile int nactive;
	/** Number of readers waiting */
	volatile unsigned int npending_reads;
	/** Number of writers waiting */
	volatile unsigned int npending_writes;
	/** Configuration flag for lock (see UK_RWLOCK_CONFIG_*) */
	unsigned int config_flags;
	/** Spinlock to synchronize this lock */
	struct uk_spinlock sl;
	/** Wait queue for readers */
	struct uk_waitq shared;
	/** Wait queue for writers */
	struct uk_waitq exclusive;
};

static inline int uk_rwlock_is_write_recursive(const struct uk_rwlock *rwl)
{
	return (rwl->config_flags & UK_RWLOCK_CONFIG_WRITE_RECURSE);
}

/**
 * Initialize the reader-writer lock with the given configuration flags
 *
 * @param rwl
 *   Reader-writer lock to operate on
 * @param config_flags
 *   Configuration flags for reader-writer lock (see UK_RWLOCK_CONFIG_*)
 */
void uk_rwlock_init_config(struct uk_rwlock *rwl, unsigned int config_flags);

#define uk_rwlock_init(rwl) uk_rwlock_init_config(rwl, 0)

#define UK_RWLOCK_INITIALIZER(name, flags) \
	((struct uk_rwlock){ \
		.nactive = 0, \
		.npending_reads = 0, \
		.npending_writes = 0, \
		.config_flags = flags, \
		.sl = UK_SPINLOCK_INITIALIZER(), \
		.shared = UK_WAIT_QUEUE_INITIALIZER((name).shared), \
		.exclusive = UK_WAIT_QUEUE_INITIALIZER((name).exclusive), \
	})

/**
 * Acquire the reader-writer lock for reading. Multiple readers can
 * acquire the lock at the same time
 *
 * @param rwl
 *   Reader-writer lock to be acquired
 */
void uk_rwlock_rlock(struct uk_rwlock *rwl);

/**
 * Acquire the reader-writer lock for writing. Only a single writer can
 * acquire the lock at the same time
 *
 * @param rwl
 *   Reader-writer lock to be acquired
 */
void uk_rwlock_wlock(struct uk_rwlock *rwl);

/**
 * Release the reader-writer lock, which has previously been acquired by this
 * thread for reading. The lock can also be a downgraded write lock
 *
 * @param rwl
 *   Reader-writer lock to be released
 */
void uk_rwlock_runlock(struct uk_rwlock *rwl);

/**
 * Release the reader-writer lock, which has previously been acquired by this
 * thread for writing. The lock can also be an upgraded read lock
 *
 * @param rwl
 *   Reader-writer lock to be released
 */
void uk_rwlock_wunlock(struct uk_rwlock *rwl);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CONFIG_LIBUKLOCK_RWLOCK */

#endif /* __UK_RWLOCK_H__ */
