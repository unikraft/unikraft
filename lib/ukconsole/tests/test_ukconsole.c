/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/prio.h>
#include <uk/console.h>
#include <uk/test.h>

#define UK_CONSOLE_TEST_BUF_SIZE 512
static char uk_console_test_out_buf[UK_CONSOLE_TEST_BUF_SIZE];
static char uk_console_test_in_buf[UK_CONSOLE_TEST_BUF_SIZE] =
	"Hello in!";

__ssz uk_console_test_out(struct uk_console *dev __unused,
			  const char *buf, __sz len)
{
	UK_ASSERT(len <= UK_CONSOLE_TEST_BUF_SIZE);

	for (__sz i = 0; i < len; i++)
		uk_console_test_out_buf[i] = buf[i];

	return len;
}

__ssz uk_console_test_in(struct uk_console *dev __unused,
			 char *buf, __sz len)
{
	UK_ASSERT(len <= UK_CONSOLE_TEST_BUF_SIZE);

	for (__sz i = 0; i < len; i++)
		buf[i] = uk_console_test_in_buf[i];

	return len;
}

static struct uk_console_ops uk_console_test_ops = {
	.out = uk_console_test_out,
	.in = uk_console_test_in
};

static struct uk_console uk_console_test_dev =
	UK_CONSOLE("test", &uk_console_test_ops, 0);

UK_TESTCASE(uk_console_testsuite, uk_console_devices)
{
	char buf[UK_CONSOLE_TEST_BUF_SIZE];

	uk_console_register(&uk_console_test_dev);

	UK_TEST_EXPECT(uk_console_get(uk_console_test_dev.id)
		== &uk_console_test_dev);

	uk_console_out_with(uk_console_get(uk_console_test_dev.id),
			    "Hello out with!", 15);
	UK_TEST_EXPECT_BYTES_EQ(uk_console_test_out_buf, "Hello out with!", 15);

	memset(buf, 0, UK_CONSOLE_TEST_BUF_SIZE);
	uk_console_in_with(uk_console_get(uk_console_test_dev.id), buf, 9);
	UK_TEST_EXPECT_BYTES_EQ(buf, "Hello in!", 9);
}

UK_TESTCASE(uk_console_testsuite, uk_console_test_enumerate_devices)
{
	__u16 dev_cnt = uk_console_get_device_count();
	__u16 i;

	/* All devices with IDs in the range [0; dev_cnt) should exist */

	for (i = 0; i < dev_cnt; i++)
		UK_TEST_EXPECT_NOT_NULL(uk_console_get(i));
}

uk_testsuite_register(uk_console_testsuite, NULL);
