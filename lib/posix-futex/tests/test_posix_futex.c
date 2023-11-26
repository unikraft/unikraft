/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Adina Smeu <adina.smeu@gmail.com>
 *
 *
 * Copyright (c) 2022 Adina Smeu <adina.smeu@gmail.com>
 *                     All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#include <uk/test.h>

#include <time.h>

#include <linux/futex.h>
#include <uk/syscall.h>
#include <uk/sched.h>

#if defined(__X86_32__) || defined(__x86_64__)
#define NR_FUTEX	202
#elif (defined __ARM_32__) || (defined __ARM_64__)
#define NR_FUTEX	240
#endif

struct test_args {
	uint32_t num_iterations;
	uint32_t val;
	uint32_t nr_wake;
	uint64_t nr_requeue;

	uint32_t *futex_val;
	uint32_t *requeue_futex_val;
	uint32_t *var_to_change;
	uint32_t *var_to_change_vals;
	struct timespec *timeout;

	int *rets;
};

/**
 * Check that the var is equal to the expected value.
 * Check if the return values are equal to the expected values. The waker return
 * value should be equal to the number of threads that were woken. The waiter
 * return value should be 0.
 *
 * @param rets			The return values of the futex syscall
 * @param var_to_change_vals	The intermediate values of the variable to
 *				change
 * @param num_iterations	The number of iterations
 * @param num_threads		The number of threads (one waker + more waiters)
 * @param var			The variable that is changed by the threads
 * @param ret_waker		The expected return value of the waker thread
 * @param check_waiter_val	If true, check the intermediary values of the
 *				variable changed by the waiter threads.
 */
#define CHECK_ITERATIONS(rets, var_to_change_vals, num_iterations, \
			 num_threads, var, ret_waker, check_waiter_val) \
	do { \
		UK_TEST_EXPECT_SNUM_EQ(var, num_threads * num_iterations); \
		for (i = 0; i < num_threads; ++i) { \
			for (j = 0; j < num_iterations; ++j) { \
				if (i == 0) \
					UK_TEST_EXPECT_SNUM_EQ(rets[i][j], \
							       ret_waker); \
				else \
					UK_TEST_EXPECT_SNUM_EQ(rets[i][j], 0); \
				if (i == 0 || check_waiter_val) \
					UK_TEST_EXPECT_SNUM_EQ( \
						var_to_change_vals[i][j], \
						j * num_threads + i + 1); \
			} \
		} \
	} while (0)

static int futex(uint32_t *uaddr, int futex_op, uint32_t val,
		 const struct timespec *timeout, uint32_t *uaddr2,
		 uint32_t val3)
{
	return uk_syscall(NR_FUTEX, uaddr, futex_op, val, timeout, uaddr2,
			  val3);
}

/**
 * Wait for the futex value to be equal to the iteration number and then
 * increment a variable.
 */
static __noreturn void waiter_func(void *arg)
{
	uint32_t i;
	struct test_args *args = (struct test_args *)arg;
	int *rets = args->rets;
	uint32_t *var_to_change_vals = args->var_to_change_vals;
	uint32_t num_iterations = args->num_iterations;

	for (i = 0; i < num_iterations; ++i) {
		rets[i] = futex(args->futex_val, FUTEX_WAIT, i, args->timeout,
				NULL, 0);

		if (rets[i])
			break;

		var_to_change_vals[i] = ++(*args->var_to_change);
	}
	uk_sched_thread_exit();
}

/**
 * Increment the variable and wake up the waiter threads.
 */
static __noreturn void waker_func(void *arg)
{
	uint32_t i;
	struct test_args *args = (struct test_args *)arg;
	int *rets = args->rets;
	uint32_t *var_to_change_vals = args->var_to_change_vals;
	uint32_t num_iterations = args->num_iterations;

	for (i = 0; i < num_iterations; ++i) {
		++(*args->futex_val);
		var_to_change_vals[i] = ++(*args->var_to_change);
		rets[i] = futex(args->futex_val, FUTEX_WAKE, args->nr_wake,
				NULL, NULL, 0);
		uk_sched_yield();
	}
	uk_sched_thread_exit();
}

/**
 * Wake up nr_wake threads and requeue nr_requeue threads.
 */
static __noreturn void requeuer_func(void *arg)
{
	uint32_t i;
	struct test_args *args = (struct test_args *)arg;
	int *rets = args->rets;
	uint32_t num_iterations = args->num_iterations;

	for (i = 0; i < num_iterations; ++i) {
		rets[i] = futex(args->futex_val, FUTEX_CMP_REQUEUE,
				args->nr_wake,
				(struct timespec *)args->nr_requeue,
				args->requeue_futex_val, args->val);
		uk_sched_yield();
	}
	uk_sched_thread_exit();
}

/**
 * TODO: replace this with pthread_join
 *
 * this is sufficient for now
 */
static void wait_thread(struct uk_thread *t)
{
	while (!uk_thread_is_exited(t))
		uk_sched_yield();
}

UK_TESTCASE(posix_futex_testsuite, test_wait_different_value)
{
	uint32_t futex_val = 10;
	uint32_t val = 20;

	int ret = futex(&futex_val, FUTEX_WAIT, val, NULL, NULL, 0);

	UK_TEST_EXPECT_SNUM_EQ(ret, -1);
	UK_TEST_EXPECT_SNUM_EQ(errno, EAGAIN);
}

UK_TESTCASE(posix_futex_testsuite, test_wait_timeout)
{
	uint32_t futex_val = 10;
	uint32_t val = 10;
	struct timespec tm = {.tv_sec = 0, .tv_nsec = 1000000000};

	int ret = futex(&futex_val, FUTEX_WAIT, val, &tm, NULL, 0);

	UK_TEST_EXPECT_SNUM_EQ(ret, -1);
	UK_TEST_EXPECT_SNUM_EQ(errno, ETIMEDOUT);
}

UK_TESTCASE(posix_futex_testsuite, test_wake_no_waiters)
{
	uint32_t futex_val = 10;
	uint32_t nr_wake = 1;

	int ret = futex(&futex_val, FUTEX_WAKE, nr_wake, NULL, NULL, 0);

	UK_TEST_EXPECT_ZERO(ret);
}

UK_TESTCASE(posix_futex_testsuite, test_one_waiter_one_waker)
{
	uint32_t i, j;
	uint32_t futex_val = 0;
	uint32_t var_to_change = 0;
	uint32_t num_threads = 2;
	uint32_t num_iterations = 5;

	struct uk_thread *threads[num_threads];
	struct test_args args[num_threads];
	uint32_t var_to_change_vals[num_threads][num_iterations];
	int rets[num_threads][num_iterations];

	/* Initialize common arguments */
	for (i = 0; i < num_threads; ++i)
		args[i] = (struct test_args){
			.futex_val = &futex_val,
			.var_to_change = &var_to_change,
			.var_to_change_vals = var_to_change_vals[i],
			.rets = rets[i],
			.num_iterations = num_iterations,
			.nr_wake = 1,
			.timeout = NULL,
		};

	/* Create the two threads */
	threads[1] = uk_sched_thread_create(uk_sched_current(),
			waiter_func, args + 1, "Waiter");
	threads[0] = uk_sched_thread_create(uk_sched_current(),
			waker_func, args + 0, "Waker");

	/* Wait for the threads to finish */
	for (i = 0; i < num_threads; ++i)
		wait_thread(threads[i]);

	CHECK_ITERATIONS(rets, var_to_change_vals, num_iterations, num_threads,
			 var_to_change, 1, true);
}

UK_TESTCASE(posix_futex_testsuite, test_two_waiters_one_waker)
{
	uint32_t i, j;
	uint32_t futex_val = 0;
	uint32_t var_to_change = 0;
	uint32_t num_threads = 3;
	uint32_t num_iterations = 5;

	struct uk_thread *threads[num_threads];
	struct test_args args[num_threads];
	uint32_t var_to_change_vals[num_threads][num_iterations];
	int rets[num_threads][num_iterations];

	/* Initialize common arguments */
	for (i = 0; i < num_threads; ++i)
		args[i] = (struct test_args){
			.futex_val = &futex_val,
			.var_to_change = &var_to_change,
			.var_to_change_vals = var_to_change_vals[i],
			.rets = rets[i],
			.num_iterations = num_iterations,
			.nr_wake = 2,
			.timeout = NULL,
		};

	/* Create the two threads */
	threads[1] = uk_sched_thread_create(uk_sched_current(),
			waiter_func, args + 1, "Waiter 1");
	threads[2] = uk_sched_thread_create(uk_sched_current(),
			waiter_func, args + 2, "Waiter 2");
	threads[0] = uk_sched_thread_create(uk_sched_current(),
			waker_func, args + 0, "Waker");

	/* Wait for the threads to finish */
	for (i = 0; i < num_threads; ++i)
		wait_thread(threads[i]);

	CHECK_ITERATIONS(rets, var_to_change_vals, num_iterations, num_threads,
			 var_to_change, 2, false);
}

UK_TESTCASE(posix_futex_testsuite, test_cmp_requeue_different_val)
{
	uint32_t futex_val = 10;
	uint32_t requeue_futex_val = 15;
	uint32_t val = 20;
	uint32_t nr_wake = 1;
	uint64_t nr_requeue = 1;

	int ret = futex(&futex_val, FUTEX_CMP_REQUEUE, nr_wake,
			(struct timespec *)nr_requeue, &requeue_futex_val, val);

	UK_TEST_EXPECT_SNUM_EQ(ret, -1);
	UK_TEST_EXPECT_SNUM_EQ(errno, EAGAIN);
}

UK_TESTCASE(posix_futex_testsuite, test_cmp_requeue_two_waiters_no_requeue)
{
	uint32_t i;
	uint32_t futex_val = 0;
	uint32_t other_futex_val = 0;
	uint32_t requeue_futex_val = 15;
	uint32_t var_to_change = 0;
	uint32_t val = 0;
	uint32_t nr_wake = 1;
	uint32_t nr_requeue = 1;
	uint32_t num_threads = 3;
	uint32_t num_iterations = 1;

	struct timespec tm = {.tv_sec = 0, .tv_nsec = 1000000000};

	struct uk_thread *threads[num_threads];
	struct test_args args[num_threads];
	uint32_t var_to_change_vals[num_threads][num_iterations];
	int rets[num_threads][num_iterations];

	/* Initialize common arguments */
	for (i = 0; i < num_threads; ++i)
		args[i] = (struct test_args){
			.futex_val = &futex_val,
			.var_to_change = &var_to_change,
			.var_to_change_vals = var_to_change_vals[i],
			.rets = rets[i],
			.num_iterations = num_iterations,
			.timeout = NULL,
		};

	/* Initialize waiter on the other futex */
	args[1].futex_val = &other_futex_val;
	args[1].timeout = &tm;

	/* Initialize requeuer args */
	args[2].requeue_futex_val = &requeue_futex_val;
	args[2].val = val;
	args[2].nr_requeue = nr_requeue;
	args[2].nr_wake = nr_wake;

	/* Create the two threads */
	threads[0] = uk_sched_thread_create(uk_sched_current(),
			waiter_func, args + 0, "Waiter 1");
	threads[1] = uk_sched_thread_create(uk_sched_current(),
			waiter_func, args + 1, "Waiter 2");
	threads[2] = uk_sched_thread_create(uk_sched_current(),
			requeuer_func, args + 2, "Requeuer");

	/* Wait for the threads to finish */
	for (i = 0; i < num_threads; ++i)
		wait_thread(threads[i]);

	/* Requeuer should have woken up one waiter and requeued no thread */
	UK_TEST_EXPECT_SNUM_EQ(rets[2][0], 1);

	/* Waiter 1 should have been woken */
	UK_TEST_EXPECT_SNUM_EQ(rets[0][0], 0);

	/* Waiter 2 should have timed out */
	UK_TEST_EXPECT_SNUM_EQ(rets[1][0], -1);

	UK_TEST_EXPECT_SNUM_EQ(var_to_change, 1);
}

UK_TESTCASE(posix_futex_testsuite, test_cmp_requeue_two_waiters_one_waker)
{
	uint32_t i;
	uint32_t futex_val = 0;
	uint32_t requeue_futex_val = 15;
	uint32_t var_to_change = 0;
	uint32_t val = 0;
	uint32_t nr_wake = 1;
	uint32_t nr_requeue = 1;
	uint32_t num_threads = 4;
	uint32_t num_iterations = 1;

	struct uk_thread *threads[num_threads];
	struct test_args args[num_threads];
	uint32_t var_to_change_vals[num_threads][num_iterations];
	int rets[num_threads][num_iterations];

	/* Initialize common arguments */
	for (i = 0; i < num_threads; ++i)
		args[i] = (struct test_args){
			.futex_val = &futex_val,
			.var_to_change = &var_to_change,
			.var_to_change_vals = var_to_change_vals[i],
			.rets = rets[i],
			.num_iterations = num_iterations,
			.timeout = NULL,
		};

	/* Initialize requeuer args */
	args[2].requeue_futex_val = &requeue_futex_val;
	args[2].val = val;
	args[2].nr_requeue = nr_requeue;
	args[2].nr_wake = nr_wake;

	/* Initialize waker args */
	args[3].futex_val = &requeue_futex_val;
	args[3].nr_wake = nr_wake;

	threads[0] = uk_sched_thread_create(uk_sched_current(),
			waiter_func, args + 0, "Waiter 1");
	threads[1] = uk_sched_thread_create(uk_sched_current(),
			waiter_func, args + 1, "Waiter 2");
	threads[2] = uk_sched_thread_create(uk_sched_current(),
			requeuer_func, args + 2, "Requeuer");

	wait_thread(threads[2]);

	/* Requeuer should have woken up one waiter and requeued the other */
	UK_TEST_EXPECT_SNUM_EQ(rets[2][0], 2);

	threads[3] = uk_sched_thread_create(uk_sched_current(),
			waker_func, args + 3, "Waker");

	/* Wait for the threads to finish */
	for (i = 0; i < num_threads; ++i)
		if (i != 2)
			wait_thread(threads[i]);

	/* Waiters 1 and 2 should have been woken */
	UK_TEST_EXPECT_SNUM_EQ(rets[0][0], 0);
	UK_TEST_EXPECT_SNUM_EQ(rets[1][0], 0);

	/* Waker should have awoken one waiter */
	UK_TEST_EXPECT_SNUM_EQ(rets[3][0], 1);

	UK_TEST_EXPECT_SNUM_EQ(var_to_change, 3);
}

uk_testsuite_register(posix_futex_testsuite, NULL);
