#include <uk/test.h>
#include <stdio.h>
#include <uk/sched.h>
#include <unistd.h>
#include <uk/list.h>

static int FN0_CALLED;
static int FN1_CALLED;
static int FN2_CALLED;
static int DTOR_CALLED;
static int DTOR;

static long FN1_ARG;
static long FN2_ARG1;
static long FN2_ARG2;

static __thread long tls1 = 0xaabbccdd;
static __thread long tls2 = 0xaabbccdd;
static __thread long tls3 = 0xaabbccdd;

void generic_dtor(struct uk_thread *t)
{
	DTOR_CALLED += 1;
}

__noreturn void fn0(void)
{
	tls1 = 0xaaaaaaaa;
	FN0_CALLED = 1;
	DTOR += 1;
	uk_sched_thread_exit();
}


__noreturn void fn1(void *arg)
{
	tls2 = 0xbbbbbbbb;
	FN1_ARG = (long) arg;
	FN1_CALLED = 1;
	DTOR += 1;
	uk_sched_thread_exit();
}

__noreturn void fn2(void *arg1, void *arg2)
{
	tls3 = 0xcccccccc;
	FN2_ARG1 = (long) arg1;
	FN2_ARG2 = (long) arg2;
	FN2_CALLED = 1;
	DTOR += 1;
	uk_sched_thread_exit();
}


UK_TESTCASE(posix_process_test_tls_testsuite, multiple_threads)
{
	/* take the current scheduler used by the main thread */
	struct uk_sched *scheduler = uk_sched_current();
	struct uk_thread *itr;
	char buf[10000];
	int count = 0;
	int no_threads = 0;

	/* create a thread that will start from fn0, a function with 0 args */
	uk_sched_thread_create_fn0(scheduler,
				   fn0,
				   4096,
				   0,
				   0,
				   "Thread 1",
				   NULL,
				   generic_dtor);

	uk_sched_thread_create_fn1(scheduler,
				   fn1,
				   (void *) 0xdeadbeef,
				   4096,
				   0,
				   0,
				   "Thread 2",
				   NULL,
				   generic_dtor);

	uk_sched_thread_create_fn2(scheduler,
				   fn2,
				   (void *) 0xdeadbeef,
				   (void *) 0xbadbabee,
				   4096,
				   0,
				   0,
				   "Thread 3",
				   NULL,
				   generic_dtor);

	UK_TAILQ_FOREACH(itr, &scheduler->thread_list, thread_list) {
		count += snprintf(buf + count, sizeof(buf) - count,
			"\n\t%s at address %p", itr->name, itr);
		no_threads += 1;
	}

	UK_TEST_EXPECT_SNUM_EQ(no_threads, 5);

	sleep(2);

	UK_TEST_EXPECT_NOT_ZERO(FN0_CALLED);

	UK_TEST_EXPECT_NOT_ZERO(FN1_CALLED);
	UK_TEST_EXPECT_SNUM_EQ(FN1_ARG, 0xdeadbeef);

	UK_TEST_EXPECT_NOT_ZERO(FN2_CALLED);
	UK_TEST_EXPECT_SNUM_EQ(FN2_ARG1, 0xdeadbeef);
	UK_TEST_EXPECT_SNUM_EQ(FN2_ARG2, 0xbadbabee);

	UK_TEST_ASSERTF(DTOR_CALLED == DTOR,
		"expected to call generic_dtor for %d threads, called %d times",
		DTOR, DTOR_CALLED);

	UK_TEST_EXPECT(tls1 == 0xaabbccdd && tls2 == 0xaabbccdd
		&& tls3 == 0xaabbccdd);
}

uk_testsuite_register(posix_process_test_tls_testsuite, NULL);
