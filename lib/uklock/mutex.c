#include <uk/mutex.h>

#ifdef CONFIG_LIBUKLOCK_MUTEX_METRICS
#include <uk/init.h>
#include <uk/assert.h>

struct uk_mutex_metrics _uk_mutex_metrics = { 0 };
__spinlock              _uk_mutex_metrics_lock;
#endif /* CONFIG_LIBUKLOCK_MUTEX_METRICS */

void uk_mutex_init_config(struct uk_mutex *m, unsigned int flags)
{
	m->lock_count = 0;
	m->flags = flags;
	m->owner = NULL;
	uk_waitq_init(&m->wait);

#ifdef CONFIG_LIBUKLOCK_MUTEX_METRICS
	ukarch_spin_lock(&_uk_mutex_metrics_lock);
	_uk_mutex_metrics.active_unlocked++;
	ukarch_spin_unlock(&_uk_mutex_metrics_lock);
#endif /* CONFIG_LIBUKLOCK_MUTEX_METRICS */
}

#ifdef CONFIG_LIBUKLOCK_MUTEX_METRICS
/**
 * Initializes the mutex metric counters and its spinlock.
 * @return 0 on success.
 *
 * NOTE: this function is registered as a CTOR for this library.
 */
static int mutex_metrics_ctor(void)
{
	ukarch_spin_init(&_uk_mutex_metrics_lock);

	return 0;
}
uk_lib_initcall_prio(mutex_metrics_ctor, 1);

/**
 * Makes a copy of mutex metrics to avoid direct user access.
 * @dst : destination buffer (must have been already allocated)
 */
void uk_mutex_get_metrics(struct uk_mutex_metrics *dst)
{
	UK_ASSERT(dst);

	ukarch_spin_lock(&_uk_mutex_metrics_lock);
	memcpy(dst, &_uk_mutex_metrics, sizeof(*dst));
	ukarch_spin_unlock(&_uk_mutex_metrics_lock);
}
#endif /* CONFIG_LIBUKLOCK_MUTEX_METRICS */
