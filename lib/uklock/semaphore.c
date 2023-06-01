#include <uk/semaphore.h>
#if CONFIG_LIBPOSIX_PROCESS_CLONE
#include <uk/process.h>
#endif /* CONFIG_LIBPOSIX_PROCESS_CLONE */

void uk_semaphore_init(struct uk_semaphore *s, long count)
{
	s->count = count;
	uk_waitq_init(&s->wait);

#ifdef UK_SEMAPHORE_DEBUG
	uk_pr_debug("Initialized semaphore %p with %ld\n",
		    s, s->count);
#endif
}

#if CONFIG_LIBPOSIX_PROCESS_CLONE
/* parent and child share System V semaphores */
static int uk_posix_clone_sysvsem(const struct clone_args *cl_args __unused,
				  size_t cl_args_len __unused,
				  struct uk_thread *child __unused,
				  struct uk_thread *parent __unused)
{
	UK_WARN_STUBBED();
	return 0;
}
UK_POSIX_CLONE_HANDLER(CLONE_SYSVSEM, true, uk_posix_clone_sysvsem, 0x0);
#endif /* CONFIG_LIBPOSIX_PROCESS_CLONE */
