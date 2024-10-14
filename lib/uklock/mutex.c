#include <uk/mutex.h>

#ifdef CONFIG_LIBUKLOCK_MUTEX_METRICS
#include <uk/init.h>
#include <uk/assert.h>

struct uk_mutex_metrics _uk_mutex_metrics = { 0 };
__spinlock              _uk_mutex_metrics_lock;
#endif /* CONFIG_LIBUKLOCK_MUTEX_METRICS */

/* We need the lower bits for our mutex flags */
UK_CTASSERT(__alignof(struct uk_thread) >= 4);

void uk_mutex_init_config(struct uk_mutex *m, unsigned int flags)
{
	m->lock_count = 0;
	m->flags = flags;
	m->lock = _UK_MUTEX_UNOWNED;
	uk_waitq_init(&m->wait);

#ifdef CONFIG_LIBUKLOCK_MUTEX_METRICS
	ukarch_spin_lock(&_uk_mutex_metrics_lock);
	_uk_mutex_metrics.active_unlocked++;
	ukarch_spin_unlock(&_uk_mutex_metrics_lock);
#endif /* CONFIG_LIBUKLOCK_MUTEX_METRICS */
}

void _uk_mutex_lock_wait(struct uk_mutex *m, __uptr tid, __uptr v)
{
	__uptr tmp;

	/* If the owner is the current thread, just increment the lock count */
	if (unlikely(uk_mutex_is_recursive(m) &&
		     (tid & ~_UK_MUTEX_STATE_MASK) ==
		     (v & ~_UK_MUTEX_STATE_MASK))) {
		UK_ASSERT(m->lock_count > 0);
		m->lock_count++;
		return;
	}

	UK_ASSERT(tid != (v & ~_UK_MUTEX_STATE_MASK));

	for (;;) {
		if (v == _UK_MUTEX_UNOWNED) {
			if (_uk_mutex_lock_fetch(m, &v, tid))
				break;
			continue;
		}

		UK_ASSERT(v != _UK_MUTEX_UNOWNED);

		if ((v & _UK_MUTEX_CONTESTED) == 0 &&
		    !__atomic_compare_exchange_n(&m->lock, &v,
						 v | _UK_MUTEX_CONTESTED,
						 0,
						 __ATOMIC_SEQ_CST,
						 __ATOMIC_SEQ_CST)) {
			continue;
		}

		/* From this point, we know that the other side will also go
		 * through the waitq spinlock.
		 * Wait until something changed, and retry locking the mutex.
		 */
		uk_waitq_wait_event(
			&m->wait,
			(tmp = __atomic_load_n(&m->lock, __ATOMIC_RELAXED)) !=
			v);

		v = tmp;
	}

	UK_ASSERT(m->lock_count == 0);
	m->lock_count = 1;
}

int _uk_mutex_trylock_wait(struct uk_mutex *m, __uptr tid, __uptr v)
{
	/* If the owner is the current thread, just increment the lock count */
	if (unlikely(uk_mutex_is_recursive(m) &&
		     (tid & ~_UK_MUTEX_STATE_MASK) ==
		     (v & ~_UK_MUTEX_STATE_MASK))) {
		UK_ASSERT(m->lock_count > 0);
		m->lock_count++;

#ifdef CONFIG_LIBUKLOCK_MUTEX_METRICS
		ukarch_spin_lock(&_uk_mutex_metrics_lock);
		_uk_mutex_metrics.total_ok_trylocks++;
		ukarch_spin_unlock(&_uk_mutex_metrics_lock);
#endif /* CONFIG_LIBUKLOCK_MUTEX_METRICS */

		return 1;
	}

	return 0;
}

void _uk_mutex_unlock_wait(struct uk_mutex *m)
{
	__atomic_store_n(&m->lock, _UK_MUTEX_UNOWNED, __ATOMIC_RELEASE);
	uk_waitq_wake_up(&m->wait);
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
uk_lib_initcall_prio(mutex_metrics_ctor, 0x0, 1);

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
