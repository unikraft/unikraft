/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <fcntl.h>

#include <uk/essentials.h>
#include <uk/file/nops.h>
#include <uk/plat/time.h>
#include <uk/posix-poll.h>
#include <uk/sched.h>
#include <uk/test.h>
#include <uk/timeutil.h>

struct test_file {
	struct uk_file f;
	struct uk_file_state st;
	uk_file_refcnt ref;
};

static struct test_file test_files[2];

static void test_file_release(const struct uk_file *f __unused,
			      int what __unused)
{
}

static int test_open(int i, uk_pollevent ev)
{
	struct test_file *tf = &test_files[i];

	tf->st = UK_FILE_STATE_EVENTS_INIT_VALUE(tf->st, ev);
	tf->ref = UK_FILE_REFCNT_INIT_VALUE(tf->ref);
	tf->f = (struct uk_file){
		.refcnt = &tf->ref,
		.state = &tf->st,
		.ops = &uk_file_nops,
		._release = test_file_release,
	};
	return uk_fdtab_open(&tf->f, O_RDWR);
}

static int test_select(int nfds, fd_set *rd, fd_set *wr, fd_set *ex)
{
	struct timespec ts = { 0 };

	return uk_sys_pselect(nfds, rd, wr, ex, &ts, NULL);
}

UK_TESTCASE(posix_poll_select, err_goes_to_read_and_write_sets)
{
	int fd = test_open(0, EPOLLERR);
	fd_set rd, wr;

	FD_ZERO(&rd);
	FD_ZERO(&wr);
	FD_SET(fd, &rd);
	FD_SET(fd, &wr);

	UK_TEST_EXPECT_SNUM_EQ(test_select(fd + 1, &rd, &wr, NULL), 2);
	UK_TEST_EXPECT(FD_ISSET(fd, &rd));
	UK_TEST_EXPECT(FD_ISSET(fd, &wr));

	uk_sys_close(fd);
}

UK_TESTCASE(posix_poll_select, hup_goes_to_read_set)
{
	int fd = test_open(0, EPOLLHUP);
	fd_set rd, wr;

	FD_ZERO(&rd);
	FD_ZERO(&wr);
	FD_SET(fd, &rd);
	FD_SET(fd, &wr);

	UK_TEST_EXPECT_SNUM_EQ(test_select(fd + 1, &rd, &wr, NULL), 1);
	UK_TEST_EXPECT(FD_ISSET(fd, &rd));
	UK_TEST_EXPECT(!FD_ISSET(fd, &wr));

	uk_sys_close(fd);
}

UK_TESTCASE(posix_poll_select, count_is_number_of_set_bits)
{
	int rw = test_open(0, EPOLLIN | EPOLLOUT);
	int err = test_open(1, EPOLLERR);
	fd_set rd, wr;

	FD_ZERO(&rd);
	FD_ZERO(&wr);
	FD_SET(rw, &rd);
	FD_SET(rw, &wr);
	FD_SET(err, &wr);

	UK_TEST_EXPECT_SNUM_EQ(test_select(MAX(rw, err) + 1, &rd, &wr, NULL),
			       3);
	UK_TEST_EXPECT(FD_ISSET(rw, &rd));
	UK_TEST_EXPECT(FD_ISSET(rw, &wr));
	UK_TEST_EXPECT(!FD_ISSET(err, &rd));
	UK_TEST_EXPECT(FD_ISSET(err, &wr));

	uk_sys_close(rw);
	uk_sys_close(err);
}

UK_TESTCASE(posix_poll_select, only_pri_goes_to_except_set)
{
	int pri = test_open(0, EPOLLPRI);
	int err = test_open(1, EPOLLERR | EPOLLHUP);
	fd_set ex;

	FD_ZERO(&ex);
	FD_SET(pri, &ex);
	FD_SET(err, &ex);

	UK_TEST_EXPECT_SNUM_EQ(test_select(MAX(pri, err) + 1, NULL, NULL, &ex),
			       1);
	UK_TEST_EXPECT(FD_ISSET(pri, &ex));
	UK_TEST_EXPECT(!FD_ISSET(err, &ex));

	uk_sys_close(pri);
	uk_sys_close(err);
}

UK_TESTCASE(posix_poll_select, no_set_bit_waits_for_timeout)
{
	struct timespec ts = { .tv_nsec = 10 * 1000 * 1000 };
	int fd = test_open(0, EPOLLHUP);
	__nsec t0;
	fd_set wr;

	FD_ZERO(&wr);
	FD_SET(fd, &wr);

	t0 = ukplat_monotonic_clock();
	UK_TEST_EXPECT_SNUM_EQ(uk_sys_pselect(fd + 1, NULL, &wr, NULL, &ts,
					      NULL), 0);
	UK_TEST_EXPECT_SNUM_GE(ukplat_monotonic_clock() - t0,
			       10 * 1000 * 1000);
	UK_TEST_EXPECT(!FD_ISSET(fd, &wr));
	UK_TEST_EXPECT_SNUM_EQ(uk_time_spec_to_nsec(&ts), 0);

	uk_sys_close(fd);
}

static __noreturn void test_set_out(void *f)
{
	uk_file_event_set(f, EPOLLOUT);
	uk_sched_thread_exit();
}

UK_TESTCASE(posix_poll_select, requested_event_ends_wait)
{
	struct timespec ts = { .tv_sec = 1 };
	int fd = test_open(0, EPOLLHUP);
	struct uk_thread *t;
	fd_set wr;

	FD_ZERO(&wr);
	FD_SET(fd, &wr);
	t = uk_sched_thread_create(uk_sched_current(), test_set_out,
				   &test_files[0].f, "test_set_out");

	UK_TEST_EXPECT_SNUM_EQ(uk_sys_pselect(fd + 1, NULL, &wr, NULL, &ts,
					      NULL), 1);
	UK_TEST_EXPECT(FD_ISSET(fd, &wr));

	while (!uk_thread_is_exited(t))
		uk_sched_yield();
	uk_sys_close(fd);
}

uk_testsuite_register(posix_poll_select, NULL);
