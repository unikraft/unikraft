#include <uk/mutex.h>

#ifdef CONFIG_LIBUKLOCK_MUTEX_METRICS
#include <uk/init.h>

struct uk_mutex_metrics _uk_mutex_metrics;
__spinlock              _uk_mutex_metrics_lock;
#endif /* CONFIG_LIBUKLOCK_MUTEX_METRICS */

void uk_mutex_init(struct uk_mutex *m)
{
	m->lock_count = 0;
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
	memset(&_uk_mutex_metrics, 0, sizeof(_uk_mutex_metrics));
	ukarch_spin_init(&_uk_mutex_metrics_lock);

	return 0;
}
uk_lib_initcall_prio(mutex_metrics_ctor, 1);
#endif /* CONFIG_LIBUKLOCK_MUTEX_METRICS */

