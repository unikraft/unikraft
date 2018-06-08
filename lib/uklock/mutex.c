#include <uk/mutex.h>

void uk_mutex_init(struct uk_mutex *m)
{
	m->locked = 0;
	uk_waitq_init(&m->wait);
}
