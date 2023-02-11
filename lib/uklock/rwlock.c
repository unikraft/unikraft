#include <uk/thread.h>
#include <uk/rwlock.h>
#include <uk/assert.h>
#include <uk/config.h>

#define __IS_CONFIG_SPIN(flag)			(flag & UK_RW_CONFIG_SPIN)
#define __IS_CONFIG_WRITE_RECURSE(flag)	(flag & UK_RW_CONFIG_WRITE_RECURSE)

void __uk_rwlock_init(struct uk_rwlock *rwl, uint8_t config_flags)
{
	UK_ASSERT(rwl);

	rwl->rwlock = UK_RW_UNLOCK;
	rwl->owner = NULL;
	rwl->write_recurse = 0;
	rwl->config_flags = config_flags;
	uk_waitq_init(&rwl->shared);
	uk_waitq_init(&rwl->exclusive);
}

void uk_rwlock_rlock(struct uk_rwlock *rwl)
{
	uint32_t v, setv;

	UK_ASSERT(rwl);

	for (;;) {

		/* Try to increment the lock until we are in the read mode */
		v = rwl->rwlock;
		if (_rw_can_read(v)) {
			setv = v + UK_RW_ONE_READER;
			if (ukarch_compare_exchange_sync(&rwl->rwlock, v, setv) == setv)
				break;
			continue;
		}

		/* Set the read waiter flag if previously it is unset */
		if (!(rwl->rwlock & UK_RWLOCK_READ_WAITERS))
			ukarch_or(&rwl->rwlock, UK_RWLOCK_READ_WAITERS);

		/* Wait for the unlock event */
		if (__IS_CONFIG_SPIN(rwl->config_flags))
			while (_rw_can_read(rwl->rwlock) == 0)
				;
		else
			uk_waitq_wait_event(&rwl->shared, _rw_can_read(rwl->rwlock));
	}
}

void uk_rwlock_wlock(struct uk_rwlock *rwl)
{
	uint32_t v, setv;
	struct uk_thread *current_thread, *owner;


	UK_ASSERT(rwl);

	current_thread = uk_thread_current();
	owner = rwl->owner;

	v = rwl->rwlock;

	/* If the lock is not held by reader and owner of the lock is current
	 * thread then simply increment the write recurse count
	 */

	if (__IS_CONFIG_WRITE_RECURSE(rwl->config_flags)
			&& !(v & UK_RWLOCK_READ) && owner == current_thread) {
		ukarch_inc(&(rwl->write_recurse));
		return;
	} else {
		UK_ASSERT(owner != current_thread);
	}

	for (;;) {

		/* If there is no reader or no owner and read flag is set then we can
		 * acquire that lock
		 * Try to set the owner field and flag mask except reader flag
		 */

		v = rwl->rwlock;
		setv = v & UK_RWLOCK_WAITERS;

		if (_rw_can_write(v)) {
			if (ukarch_compare_exchange_sync(&rwl->rwlock, v, setv) == setv) {
				if (__IS_CONFIG_WRITE_RECURSE(rwl->config_flags))
					ukarch_inc(&(rwl->write_recurse));
				break;
			}
			continue;
		}

		/* If the acquire operation fails set the write waiters flag*/
		ukarch_or(&rwl->rwlock, UK_RWLOCK_WRITE_WAITERS);

		/* Wait for the unlock event */
		if (__IS_CONFIG_SPIN(rwl->config_flags))
			while (_rw_can_write(rwl->rwlock) == 0)
				;
		else
			uk_waitq_wait_event(&rwl->exclusive, _rw_can_write(rwl->rwlock));
	}

	ukarch_store_n(&rwl->owner, current_thread);
}

/* FIXME: lost wakeup problem
 *			one solution: use deadline base waking up
 */
void uk_rwlock_runlock(struct uk_rwlock *rwl)
{
	uint32_t v, setv;
	struct uk_waitq *queue = NULL;

	UK_ASSERT(rwl);

	v = rwl->rwlock;


	if (!(v & UK_RWLOCK_READ) || UK_RW_READERS(v) == 0)
		return;

	for (;;) {

		/* Decrement the reader count and unset the waiters*/
		setv = (v - UK_RW_ONE_READER) & ~(UK_RWLOCK_WAITERS);

		/* If there are waiters then select the appropriate queue
		 * Since we are giving priority try to select writer's queue
		 */
		if (!__IS_CONFIG_SPIN(rwl->config_flags)
				&& UK_RW_READERS(setv) == 0 && (v & UK_RWLOCK_WAITERS)) {

			queue = &rwl->shared;
			if (v & UK_RWLOCK_WRITE_WAITERS) {
				setv |= (v & UK_RWLOCK_READ_WAITERS);
				queue = &rwl->exclusive;
			}
		} else {
			setv |= (v & UK_RWLOCK_WAITERS);
		}

		/* Try to unlock*/
		if (ukarch_compare_exchange_sync(&rwl->rwlock, v, setv) == setv)
			break;

		v = rwl->rwlock;
	}

	/* wakeup the relevant queue */
	if (queue)
		uk_waitq_wake_up(queue);
}

void uk_rwlock_wunlock(struct uk_rwlock *rwl)
{
	uint32_t v, setv;
	struct uk_thread *owner, *current_thread;
	struct uk_waitq *queue;

	UK_ASSERT(rwl);

	v = rwl->rwlock;
	owner = rwl->owner;
	current_thread = uk_thread_current();

	queue = NULL;

	if ((v & UK_RWLOCK_READ) || owner != current_thread)
		return;

	/* Handle recursion
	 * If the number of recursive writers are not zero then return
	 */
	if (__IS_CONFIG_WRITE_RECURSE(rwl->config_flags) &&
			ukarch_sub_fetch(&(rwl->write_recurse), 1) != 0)
		return;

	ukarch_store_n(&rwl->owner, NULL);

	/* All recusive locks have been released, time to unlock*/
	for (;;) {

		setv = UK_RW_UNLOCK;

		/* Check if there are waiters, if yes select the appropriate queue */
		if (!__IS_CONFIG_SPIN(rwl->config_flags) && (v & UK_RWLOCK_WAITERS)) {
			queue = &rwl->shared;
			if (v & UK_RWLOCK_WRITE_WAITERS) {
				setv |= (v & UK_RWLOCK_READ_WAITERS);
				queue = &rwl->exclusive;
			}
		}
		/* Try to set the lock */
		if (ukarch_compare_exchange_sync(&rwl->rwlock, v, setv) == setv)
			break;

		v = rwl->rwlock;
	}

	/* Wakeup the waiters */
	if (queue)
		uk_waitq_wake_up(queue);
}

void uk_rwlock_upgrade(struct uk_rwlock *rwl)
{
	uint32_t v, setv;

	UK_ASSERT(rwl);

	v = rwl->rwlock;

	/* If there are no readers then upgrade is invalid */
	if (UK_RW_READERS(v) == 0)
		return;

	for (;;) {

		/* Try to set the owner and relevant waiter flags */
		setv = v & UK_RWLOCK_WAITERS;
		v = rwl->rwlock;

		if (_rw_can_upgrade(v)) {
			if (ukarch_compare_exchange_sync(&rwl->rwlock, v, setv) == setv) {
				if (__IS_CONFIG_WRITE_RECURSE(rwl->config_flags))
					ukarch_inc(&(rwl->write_recurse));
				break;
			}
			continue;
		}
		/* If we cannot upgrade wait till readers count is 0 */
		ukarch_or(&rwl->rwlock, UK_RWLOCK_WRITE_WAITERS);
		if (__IS_CONFIG_SPIN(rwl->config_flags))
			while (_rw_can_upgrade(rwl->rwlock) == 0)
				;
		else
			uk_waitq_wait_event(&rwl->exclusive, _rw_can_upgrade(rwl->rwlock));
	}

	rwl->owner = uk_thread_current();
}

void uk_rwlock_downgrade(struct uk_rwlock *rwl)
{
	uint32_t v, setv;
	struct uk_thread *current_thread, *owner;
	struct uk_waitq *queue = NULL;

	UK_ASSERT(rwl);

	current_thread = uk_thread_current();
	owner = rwl->owner;
	v = rwl->rwlock;

	if ((v & UK_RWLOCK_READ) || owner != current_thread)
		return;

	if (__IS_CONFIG_WRITE_RECURSE(rwl->config_flags))
		rwl->write_recurse = 0;

	ukarch_store_n(&rwl->owner, NULL);

	for (;;) {
		if (!__IS_CONFIG_SPIN(rwl->config_flags)
				&& (v & UK_RWLOCK_READ_WAITERS)) {
			queue = &rwl->shared;
		}
		/* Set only write waiter flags since we are waking up the reads */
		setv = (UK_RW_UNLOCK + UK_RW_ONE_READER)
				| (v & UK_RWLOCK_WRITE_WAITERS);

		if (ukarch_compare_exchange_sync(&rwl->rwlock, v, setv) == setv)
			break;

		v = rwl->rwlock;
	}

	if (queue)
		uk_waitq_wake_up(queue);
}
