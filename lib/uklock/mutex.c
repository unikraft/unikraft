#include <uk/mutex.h>

void uk_mutex_init(struct uk_mutex *m)
{
	m->lock_count = 0;
	m->owner = NULL;
	uk_waitq_init(&m->wait);
}
