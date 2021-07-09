#include <uk/futex.h>
#include <uk/syscall.h>
#include <uk/arch/atomic.h>
#include <uk/thread.h>
#include <uk/list.h>
#include <uk/assert.h>
#include <uk/print.h>

/* TODO: Hash bucket */
#define add_futex(addr, fthread) \
	struct uk_futex *f = uk_malloc(uk_alloc_get_default(), \
							sizeof(*f)); \
	f->futex = addr; \
	f->thread = fthread; \
	uk_list_add_tail(&f->list_node, &futex_list)

/* TODO: timed out futexes not removed from the list */
static UK_LIST_HEAD(futex_list);

static int futex_wait(uint32_t *uaddr, uint32_t val, const struct timespec *tm)
{
	if (ukarch_load_n(uaddr) == val) {
		add_futex(uaddr, uk_thread_current());
		if (tm)
			/* Block for at least x nanosecs */
			uk_thread_block_timeout(uk_thread_current(),
				tm->tv_nsec);
		else
			/* Block undefinitely */
			uk_thread_block(uk_thread_current());

		uk_pr_debug("Futex sleep\n");

		uk_sched_yield();

		/* Thread is no longer blocked */
		return 0;
	}

	/* futex word does not contain expected val */
	return -EAGAIN;
}

static int futex_wake(uint32_t *uaddr, uint32_t val)
{
	int count = 0; // waken-up threads

	struct uk_list_head *i, *tmp;
	struct uk_futex *f;

	uk_list_for_each_safe(i, tmp, &futex_list) {

		/* Wakes at most val thread */
		if ((uint32_t)count >= val)
			break;

		f = uk_list_entry(i, struct uk_futex, list_node);

		if (f->futex == uaddr) {
			++count;
			uk_thread_wake(f->thread);
			uk_list_del(i);
		}
	}

	return count;
}

long do_futex(uint32_t *uaddr, int futex_op, uint32_t val,
				const struct timespec *timeout,
				uint32_t *uaddr2, int val3)
{
	switch (futex_op) {

	case FUTEX_WAIT:
		return futex_wait(uaddr, val, timeout);

	case FUTEX_WAKE:
		return futex_wake(uaddr, val);

	default:
		return -ENOSYS;
	}
}

UK_SYSCALL_R_DEFINE(long, futex, uint32_t *, uaddr, int, futex_op,
	uint32_t, val, const struct timespec *, timeout,
	uint32_t *, uaddr2, uint32_t, val3)
{
	ssize_t ret;

	ret = do_futex(uaddr, futex_op, val, timeout, uaddr2, val3);
	return ret;
}
