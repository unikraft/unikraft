#include <uk/semaphore.h>
#if CONFIG_LIBPOSIX_PROCESS_MULTITHREADING
#include <uk/process.h>
#endif /* CONFIG_LIBPOSIX_PROCESS_MULTITHREADING */

void uk_semaphore_init(struct uk_semaphore *s, long count)
{
	uk_spin_init(&(s->sl));
	s->count = count;
	uk_waitq_init(&s->wait);

#ifdef UK_SEMAPHORE_DEBUG
	uk_pr_debug("Initialized semaphore %p with %ld\n",
		    s, s->count);
#endif
}

#if CONFIG_LIBPOSIX_PROCESS_MULTITHREADING
/* parent and child share System V semaphores */
static
int uk_posix_clone_sysvsem(void *arg __unused)
{
	UK_WARN_STUBBED();
	return UK_EVENT_HANDLED_CONT;
}

POSIX_PROCESS_CLONE_HANDLER(CLONE_SYSVSEM, uk_posix_clone_sysvsem);
#endif /* CONFIG_LIBPOSIX_PROCESS_MULTITHREADING */
