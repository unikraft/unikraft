/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <fcntl.h>

#include <uk/essentials.h>
#include <uk/file/nops.h>
#include <uk/posix-poll.h>
#include <uk/test.h>

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

static int test_select(int nfds, fd_set *rd, fd_set *wr)
{
	struct timespec ts = { 0 };

	return uk_sys_pselect(nfds, rd, wr, NULL, &ts, NULL);
}

UK_TESTCASE(posix_poll_select, err_goes_to_read_and_write_sets)
{
	int fd = test_open(0, EPOLLERR);
	fd_set rd, wr;

	FD_ZERO(&rd);
	FD_ZERO(&wr);
	FD_SET(fd, &rd);
	FD_SET(fd, &wr);

	UK_TEST_EXPECT_SNUM_EQ(test_select(fd + 1, &rd, &wr), 2);
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

	UK_TEST_EXPECT_SNUM_EQ(test_select(fd + 1, &rd, &wr), 1);
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

	UK_TEST_EXPECT_SNUM_EQ(test_select(MAX(rw, err) + 1, &rd, &wr), 3);
	UK_TEST_EXPECT(FD_ISSET(rw, &rd));
	UK_TEST_EXPECT(FD_ISSET(rw, &wr));
	UK_TEST_EXPECT(!FD_ISSET(err, &rd));
	UK_TEST_EXPECT(FD_ISSET(err, &wr));

	uk_sys_close(rw);
	uk_sys_close(err);
}

uk_testsuite_register(posix_poll_select, NULL);
