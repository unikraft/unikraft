/* SPDX-License-Identifier: BSD-3-Clause */

#include <uk/atomic.h>
#include <uk/rwlock.h>
#include <uk/sched.h>
#include <uk/semaphore.h>
#include <uk/test.h>

#define ITERATIONS 256U
#define WAIT_NS 2000000000ULL

static struct {
	struct uk_rwlock lock;
	struct uk_semaphore start_readers;
	struct uk_semaphore start_writers;
	struct uk_semaphore done;
	unsigned int readers;
	unsigned int first_readers;
	unsigned int active_readers;
	unsigned int active_writers;
	unsigned int writes;
	unsigned int errors;
	unsigned long long cpu_mask;
} probe;

static __noreturn void rw_writer(void *unused __unused)
{
	uk_semaphore_down(&probe.start_writers);
	uk_or(&probe.cpu_mask, 1ULL << uk_sched_current()->lcpu_idx);
	for (unsigned int i = 0; i < ITERATIONS; ++i) {
		uk_rwlock_wlock(&probe.lock);
		if (uk_inc(&probe.active_writers) || uk_load_n(&probe.active_readers))
			uk_inc(&probe.errors);
		uk_inc(&probe.writes);
		uk_dec(&probe.active_writers);
		uk_rwlock_wunlock(&probe.lock);
	}
	uk_semaphore_up(&probe.done);
	uk_sched_thread_exit();
}

static __noreturn void rw_reader(void *unused __unused)
{
	uk_semaphore_down(&probe.start_readers);
	uk_or(&probe.cpu_mask, 1ULL << uk_sched_current()->lcpu_idx);
	for (unsigned int i = 0; i < ITERATIONS; ++i) {
		unsigned int writes;

		uk_rwlock_rlock(&probe.lock);
		uk_inc(&probe.active_readers);
		writes = uk_load_n(&probe.writes);
		if (!i) {
			__nsec deadline = ukplat_monotonic_clock() + WAIT_NS;

			uk_inc(&probe.first_readers);
			while (uk_load_n(&probe.first_readers) < probe.readers &&
			       ukplat_monotonic_clock() < deadline)
				uk_sched_yield();
			if (uk_load_n(&probe.first_readers) != probe.readers)
				uk_inc(&probe.errors);
		}
		if (uk_load_n(&probe.active_writers) || writes != uk_load_n(&probe.writes))
			uk_inc(&probe.errors);
		uk_dec(&probe.active_readers);
		uk_rwlock_runlock(&probe.lock);
	}
	uk_semaphore_up(&probe.done);
	uk_sched_thread_exit();
}

static unsigned int pending_waiters(volatile unsigned int *value, unsigned int target)
{
	__nsec deadline = ukplat_monotonic_clock() + WAIT_NS;
	unsigned int pending;

	do {
		uk_spin_lock(&probe.lock.sl);
		pending = *value;
		uk_spin_unlock(&probe.lock.sl);
		if (pending >= target)
			break;
		uk_sched_yield();
	} while (ukplat_monotonic_clock() < deadline);
	return pending;
}

UK_TESTCASE(rwlock_smp, reader_writer_contention)
{
	const unsigned int workers = uk_sched_started_count();
	const unsigned int writers = workers / 2;
	unsigned long long expected_mask = 0;

	UK_TEST_EXPECT(workers >= 3 && workers <= 64);
	if (workers < 3 || workers > 64)
		return;
	probe.readers = workers - writers;
	uk_rwlock_init(&probe.lock);
	uk_semaphore_init(&probe.start_readers, 0);
	uk_semaphore_init(&probe.start_writers, 0);
	uk_semaphore_init(&probe.done, 0);
	uk_rwlock_rlock(&probe.lock);
	for (unsigned int slot = 0; slot < workers; ++slot) {
		struct uk_sched *sched = uk_sched_select_round_robin(uk_sched_current());
		uk_thread_fn1_t fn = slot < writers ? rw_writer : rw_reader;

		UK_TEST_EXPECT(sched->lcpu_idx >= 0 && sched->lcpu_idx < 64);
		if (sched->lcpu_idx < 0 || sched->lcpu_idx >= 64) {
			uk_rwlock_runlock(&probe.lock);
			return;
		}
		UK_TEST_EXPECT(!(expected_mask & (1ULL << sched->lcpu_idx)));
		expected_mask |= 1ULL << sched->lcpu_idx;
		UK_TEST_EXPECT_NOT_NULL(uk_sched_thread_create(sched, fn, NULL, "rwlock-smp"));
	}
	for (unsigned int i = 0; i < writers; ++i)
		uk_semaphore_up(&probe.start_writers);
	UK_TEST_EXPECT_SNUM_EQ(pending_waiters(&probe.lock.npending_writes, writers), writers);
	for (unsigned int i = 0; i < probe.readers; ++i)
		uk_semaphore_up(&probe.start_readers);
	UK_TEST_EXPECT_SNUM_EQ(pending_waiters(&probe.lock.npending_reads, probe.readers), probe.readers);
	uk_rwlock_runlock(&probe.lock);
	for (unsigned int i = 0; i < workers; ++i)
		UK_TEST_EXPECT(uk_semaphore_down_to(&probe.done, WAIT_NS) != __NSEC_MAX);
	UK_TEST_EXPECT_SNUM_EQ(uk_load_n(&probe.cpu_mask), expected_mask);
	UK_TEST_EXPECT_SNUM_EQ(uk_load_n(&probe.writes), writers * ITERATIONS);
	UK_TEST_EXPECT_ZERO(uk_load_n(&probe.errors));
	UK_TEST_EXPECT(uk_waitq_empty(&probe.lock.shared));
	UK_TEST_EXPECT(uk_waitq_empty(&probe.lock.exclusive));
}

_UK_TESTSUITE(rwlock_smp, NULL);

/* Application constructors run after the secondary schedulers have started. */
static void __constructor run_rwlock_smp(void)
{
	uk_testsuite_run(&_UK_TESTSUITE_NAME(rwlock_smp));
}
