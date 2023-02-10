#ifndef __UK_RWLOCK_H__
#define __UK_RWLOCK_H__

#include <uk/config.h>
#include <uk/essentials.h>
#include <uk/arch/atomic.h>
#include <stddef.h>
#include <uk/assert.h>
#include <uk/wait.h>
#include <uk/wait_types.h>
#include <uk/lock-common.h>

#include <uk/print.h>

#define FLAG_BITS			(4)
#define READERS_SHIFT		(FLAG_BITS)
#define FLAG_MASK			((1 << READERS_SHIFT) - 1)

#define UK_RWLOCK_READ				(0x01)
#define UK_RWLOCK_READ_WAITERS		(0x02)
#define UK_RWLOCK_WRITE_WAITERS		(0x04)
#define UK_RWLOCK_WAITERS			(UK_RWLOCK_READ_WAITERS | \
									UK_RWLOCK_WRITE_WAITERS)

#define UK_RW_READERS_LOCK(x)		((x) << READERS_SHIFT | UK_RWLOCK_READ)
#define UK_RW_ONE_READER			(1 << READERS_SHIFT)
#define UK_RW_READERS(x)			((x) >> READERS_SHIFT)

#define UK_RW_UNLOCK				(UK_RW_READERS_LOCK(0))
#define OWNER(x)					((x) & ~FLAG_MASK)

#define _rw_can_read(rwlock)	\
	((rwlock & (UK_RWLOCK_READ | UK_RWLOCK_WRITE_WAITERS)) == UK_RWLOCK_READ)

#define _rw_can_write(rwlock)	\
	((rwlock & ~(UK_RWLOCK_WAITERS)) == UK_RW_UNLOCK)

#define _rw_can_upgrade(rwlock) \
	((rwlock & UK_RWLOCK_READ) && UK_RW_READERS(rwlock) == 1)

#define UK_RWLOCK_INITIALIZER(name) {			\
	UK_RW_UNLOCK,								\
	0,											\
	0,											\
	__WAIT_QUEUE_INITIALIZER((name.shared)),	\
	__WAIT_QUEUE_INITIALIZER((name.exclusive)) }

struct __align(8) uk_rwlock {
	uintptr_t rwlock;
#define UK_RW_CONFIG_SPIN			0x01
#define UK_RW_CONFIG_WRITE_RECURSE 	0x02
	uint8_t config_flags;
	unsigned int write_recurse;
	struct uk_waitq shared;
	struct uk_waitq exclusive;
};

void __uk_rwlock_init(struct uk_rwlock *rwl, uint8_t config_flags);

void uk_rwlock_rlock(struct uk_rwlock *rwl);

void uk_rwlock_wlock(struct uk_rwlock *rwl);

void uk_rwlock_runlock(struct uk_rwlock *rwl);

void uk_rwlock_wunlock(struct uk_rwlock *rwl);

void uk_rwlock_upgrade(struct uk_rwlock *rwl);

void uk_rwlock_downgrade(struct uk_rwlock *rwl);

#define uk_rwlock_init(rwl) __uk_rwlock_init(rwl, 0)

#define uk_rwlock_init_config(rwl, config_flags) \
	__uk_rwlock_init(rwl, config_flags)

_LOCK_IRQF(struct uk_rwlock *,  uk_rwlock_rlock)
_LOCK_IRQF(struct uk_rwlock *,  uk_rwlock_wlock)

_UNLOCK_IRQF(struct uk_rwlock *, uk_rwlock_runlock)
_UNLOCK_IRQF(struct uk_rwlock *, uk_rwlock_wunlock)

#endif /* __UK_RWLOCK_H__ */
