#include <uk/rwlock.h>
#include <uk/assert.h>
#include <uk/config.h>

void uk_rwlock_init(struct uk_rwlock *rwl)
{
	UK_ASSERT(rwl);

	rwl->rwlock = UK_RW_UNLOCK;
#ifdef CONFIG_RWLOCK_WRITE_RECURSE
	rwl->write_recurse = 0;
#endif
	uk_waitq_init(&rwl->shared);
	uk_waitq_init(&rwl->exclusive);
}
