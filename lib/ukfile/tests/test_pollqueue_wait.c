/* SPDX-License-Identifier: BSD-3-Clause */

#include <uk/atomic.h>
#include <uk/file/pollqueue.h>
#include <uk/sched.h>
#include <uk/test.h>

static const uk_pollevent ready_event = 1;

struct poll_wait_test {
	struct uk_pollq queue;
	int reader_waited_on_writer;
};

static __noreturn void poll_writer(void *arg)
{
	struct poll_wait_test *test = arg;
	struct uk_pollq *queue = &test->queue;

	uk_rwlock_wlock(&queue->waitlock);
	uk_or(&queue->events, ready_event);
	uk_waitq_wake_up(&queue->waitq);

	/* Let the notified reader try to regain the lock before releasing it. */
	uk_sched_yield();

	uk_spin_lock(&queue->waitlock.sl);
	test->reader_waited_on_writer = queue->waitlock.npending_reads > 0;
	uk_spin_unlock(&queue->waitlock.sl);
	uk_rwlock_wunlock(&queue->waitlock);
	uk_sched_thread_exit();
}

UK_TESTCASE(pollqueue_wait, detach_before_contended_relock)
{
	struct poll_wait_test test = {0};
	struct uk_thread *writer;
	uk_pollevent events;

	test.queue = UK_POLLQ_MANAGED_INIT_VALUE(test.queue);
	writer = uk_sched_thread_create_fn1(uk_sched_current(), poll_writer,
					    &test, 0, 0, false, false,
					    "poll-writer", NULL, NULL);
	UK_TEST_EXPECT_NOT_NULL(writer);
	if (!writer)
		return;

	events = uk_pollq_poll_until(&test.queue, ready_event,
				     ukplat_monotonic_clock() + 1000000000ULL, NULL);
	UK_TEST_EXPECT_SNUM_EQ(events, ready_event);
	UK_TEST_EXPECT_SNUM_EQ(test.reader_waited_on_writer, 1);
	UK_TEST_EXPECT_SNUM_EQ(uk_waitq_empty(&test.queue.waitq), 1);
}

uk_testsuite_late_prio(pollqueue_wait, NULL, 8);
