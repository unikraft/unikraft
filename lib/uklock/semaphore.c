#include <uk/semaphore.h>

void uk_semaphore_init(struct uk_semaphore *s, long count)
{
	s->count = count;
	uk_waitq_init(&s->wait);

	uk_pr_info("Initialized semaphore %p with %ld\n",
		   s, s->count);
}
