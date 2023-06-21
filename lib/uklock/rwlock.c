/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/atomic.h>
#include <uk/assert.h>
#include <uk/rwlock.h>
#include <uk/assert.h>
#include <uk/config.h>

void uk_rwlock_init_config(struct uk_rwlock *rwl, unsigned int config_flags)
{
	UK_ASSERT(rwl);

	rwl->nactive = 0;
	rwl->npending_reads = 0;
	rwl->npending_writes = 0;
	rwl->config_flags = config_flags;

	/* TODO: The implementation does not support recursive write locking */
	UK_ASSERT(!uk_rwlock_is_write_recursive(rwl));

	uk_spin_init(&rwl->sl);
	uk_waitq_init(&rwl->shared);
	uk_waitq_init(&rwl->exclusive);
}

void uk_rwlock_rlock(struct uk_rwlock *rwl)
{
	UK_ASSERT(rwl);

	uk_spin_lock(&rwl->sl);
	rwl->npending_reads++;

	/* We let readers wait when there are writers pending. This is
	 * necessary to avoid a situation where new readers continuously enter
	 * the critical section while other readers are still in - thereby
	 * starving the writers. However, at the same time we must not starve
	 * readers when there are writers. Thus we always wake up readers when
	 * a writer unlocks. The first wait() functions as a queue where we
	 * enqueue readers that arrive while a writer had the lock (or is going
	 * to get the lock next). When the writer unblock the readers, they
	 * fall to the second wait() and eventually can enter. New readers,
	 * however, block again on the first wait(), thus giving priority to
	 * waiting writers.
	 */
	if (rwl->npending_writes > 0)
		uk_waitq_wait_locked(&rwl->shared,
				     uk_spin_lock,
				     uk_spin_unlock,
				     &rwl->sl);

	/* Wait in case a writer acquired the lock (in the meantime) */
	uk_waitq_wait_event_locked(&rwl->shared,
				   rwl->nactive >= 0,
				   uk_spin_lock,
				   uk_spin_unlock,
				   &rwl->sl);

	rwl->nactive++;
	rwl->npending_reads--;
	uk_spin_unlock(&rwl->sl);
}

void uk_rwlock_wlock(struct uk_rwlock *rwl)
{
	UK_ASSERT(rwl);

	uk_spin_lock(&rwl->sl);
	rwl->npending_writes++;

	/* Wait for all readers to have left the lock. New readers will
	 * block in uk_rwlock_rlock while we are waiting.
	 */
	uk_waitq_wait_event_locked(&rwl->exclusive,
				   rwl->nactive == 0,
				   uk_spin_lock,
				   uk_spin_unlock,
				   &rwl->sl);

	UK_ASSERT(rwl->npending_writes > 0);
	UK_ASSERT(rwl->nactive == 0);

	rwl->npending_writes--;
	rwl->nactive = -1;
	uk_spin_unlock(&rwl->sl);
}

void uk_rwlock_runlock(struct uk_rwlock *rwl)
{
	int wake_writers;

	UK_ASSERT(rwl);

	uk_spin_lock(&rwl->sl);
	UK_ASSERT(rwl->nactive > 0);

	/* Remove this thread from the active readers. We wake up a writer if
	 * this was the last reader and there are writers waiting. If there
	 * are no writers pending, readers can always enter. We make sure that
	 * readers are not starving by prioritizing readers on write unlocks.
	 */
	rwl->nactive--;
	wake_writers = (rwl->nactive == 0 && rwl->npending_writes > 0);
	uk_spin_unlock(&rwl->sl);

	if (wake_writers)
		uk_waitq_wake_up_one(&rwl->exclusive);
}

void uk_rwlock_wunlock(struct uk_rwlock *rwl)
{
	int wake_readers;

	UK_ASSERT(rwl);

	uk_spin_lock(&rwl->sl);
	UK_ASSERT(rwl->nactive == -1);

	/* We are the writer. When we unlock we give priority to readers
	 * instead of writers so they do not starve. We avoid starvation of
	 * writers in uk_rwlock_rlock().
	 */
	rwl->nactive = 0;
	wake_readers = (rwl->npending_reads > 0);
	uk_spin_unlock(&rwl->sl);

	if (wake_readers)
		uk_waitq_wake_up(&rwl->shared);
	else
		uk_waitq_wake_up_one(&rwl->exclusive);
}

