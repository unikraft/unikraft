/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/prio.h>
#include <uk/console.h>
#include <uk/console/driver.h>
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

static struct uk_console uk_console_test_dev;

UK_TESTCASE(uk_console_testsuite, uk_console_devices)
{
	char buf[UK_CONSOLE_TEST_BUF_SIZE];

	uk_console_init(&uk_console_test_dev, "test", &uk_console_test_ops, 0,
			UK_CONSOLE_CLASS_NONE);
	uk_console_register(&uk_console_test_dev);

	UK_TEST_EXPECT(uk_console_get(uk_console_test_dev.id)
		== &uk_console_test_dev);

	uk_console_out_direct(uk_console_get(uk_console_test_dev.id),
			      "Hello out direct!", 15);
	UK_TEST_EXPECT_BYTES_EQ(uk_console_test_out_buf, "Hello out direct!",
				15);

	memset(buf, 0, UK_CONSOLE_TEST_BUF_SIZE);
	uk_console_in_direct(uk_console_get(uk_console_test_dev.id), buf, 9);
	UK_TEST_EXPECT_BYTES_EQ(buf, "Hello in!", 9);
}

UK_TESTCASE(uk_console_testsuite, uk_console_test_enumerate_devices)
{
	__u16 dev_cnt = uk_console_count();
	__u16 i;

	/* All devices with IDs in the range [0; dev_cnt) should exist */

	for (i = 0; i < dev_cnt; i++)
		UK_TEST_EXPECT_NOT_NULL(uk_console_get(i));

	UK_TEST_EXPECT_NULL(uk_console_get(dev_cnt));
}

/* Devices that append what they get, one with an err op, one that takes a
 * byte per call: registered as STDOUT devices only for a capture.
 */
#define UK_CONSOLE_TEST_SINK_SIZE 2048

struct uk_console_test_sink {
	char buf[UK_CONSOLE_TEST_SINK_SIZE];
	__sz len;
};

static struct uk_console_test_sink uk_console_test_sink_out;
static struct uk_console_test_sink uk_console_test_sink_err;
static struct uk_console_test_sink uk_console_test_sink_plain;
static struct uk_console_test_sink uk_console_test_sink_slow;

static __ssz uk_console_test_sink_put(struct uk_console_test_sink *sink,
				      const char *buf, __sz len)
{
	for (__sz i = 0; i < len && sink->len < UK_CONSOLE_TEST_SINK_SIZE; i++)
		sink->buf[sink->len++] = buf[i];
	return len;
}

static __ssz uk_console_test_split_out(struct uk_console *dev __unused,
				       const char *buf, __sz len)
{
	return uk_console_test_sink_put(&uk_console_test_sink_out, buf, len);
}

static __ssz uk_console_test_split_err(struct uk_console *dev __unused,
				       const char *buf, __sz len)
{
	return uk_console_test_sink_put(&uk_console_test_sink_err, buf, len);
}

static __ssz uk_console_test_plain_out(struct uk_console *dev __unused,
				       const char *buf, __sz len)
{
	return uk_console_test_sink_put(&uk_console_test_sink_plain, buf, len);
}

static __ssz uk_console_test_slow_out(struct uk_console *dev __unused,
				      const char *buf, __sz len)
{
	return uk_console_test_sink_put(&uk_console_test_sink_slow, buf,
					len ? 1 : 0);
}

static const struct uk_console_ops uk_console_test_split_ops = {
	.out = uk_console_test_split_out,
	.err = uk_console_test_split_err,
};

static const struct uk_console_ops uk_console_test_plain_ops = {
	.out = uk_console_test_plain_out,
};

static const struct uk_console_ops uk_console_test_slow_ops = {
	.out = uk_console_test_slow_out,
};

static struct uk_console uk_console_test_split_dev;
static struct uk_console uk_console_test_plain_dev;
static struct uk_console uk_console_test_slow_dev;

static void uk_console_test_capture(void)
{
	uk_console_init(&uk_console_test_split_dev, "test-split",
			&uk_console_test_split_ops, 0, UK_CONSOLE_CLASS_NONE);
	uk_console_register(&uk_console_test_split_dev);
	uk_console_init(&uk_console_test_plain_dev, "test-plain",
			&uk_console_test_plain_ops, 0, UK_CONSOLE_CLASS_NONE);
	uk_console_register(&uk_console_test_plain_dev);
	uk_console_init(&uk_console_test_slow_dev, "test-slow",
			&uk_console_test_slow_ops, 0, UK_CONSOLE_CLASS_NONE);
	uk_console_register(&uk_console_test_slow_dev);

	/* STDOUT devices only now, so registering (and the message it may
	 * print) does not reach the sinks; the slow device is written to
	 * directly.
	 */
	uk_console_test_sink_out.len = 0;
	uk_console_test_sink_err.len = 0;
	uk_console_test_sink_plain.len = 0;
	uk_console_test_sink_slow.len = 0;
	uk_console_test_split_dev.flags |= UK_CONSOLE_FLAG_STDOUT;
	uk_console_test_plain_dev.flags |= UK_CONSOLE_FLAG_STDOUT;
}

static void uk_console_test_capture_end(void)
{
	uk_console_test_split_dev.flags &= ~UK_CONSOLE_FLAG_STDOUT;
	uk_console_test_plain_dev.flags &= ~UK_CONSOLE_FLAG_STDOUT;
	uk_console_unregister(&uk_console_test_slow_dev);
	uk_console_unregister(&uk_console_test_plain_dev);
	uk_console_unregister(&uk_console_test_split_dev);
}

UK_TESTCASE(uk_console_testsuite, uk_console_test_err)
{
	__ssz out_rc, err_rc, direct_rc;

	/* A device with err keeps standard error apart; one without it
	 * prints both through out.
	 */
	uk_console_test_capture();
	out_rc = uk_console_out("out", 3);
	err_rc = uk_console_err("err", 3);
	direct_rc = uk_console_err_direct(&uk_console_test_plain_dev, "!", 1);
	uk_console_test_capture_end();

	UK_TEST_EXPECT_SNUM_EQ(out_rc, 3);
	UK_TEST_EXPECT_SNUM_EQ(err_rc, 3);
	UK_TEST_EXPECT_SNUM_EQ(direct_rc, 1);
	UK_TEST_EXPECT_SNUM_EQ(uk_console_test_sink_out.len, 3);
	UK_TEST_EXPECT_BYTES_EQ(uk_console_test_sink_out.buf, "out", 3);
	UK_TEST_EXPECT_SNUM_EQ(uk_console_test_sink_err.len, 3);
	UK_TEST_EXPECT_BYTES_EQ(uk_console_test_sink_err.buf, "err", 3);
	UK_TEST_EXPECT_SNUM_EQ(uk_console_test_sink_plain.len, 7);
	UK_TEST_EXPECT_BYTES_EQ(uk_console_test_sink_plain.buf, "outerr!", 7);
}

#if CONFIG_LIBUKCONSOLE_ONLCR
#define UK_CONSOLE_TEST_ONLCR_WANT "a\r\n\r\nb"
#else /* !CONFIG_LIBUKCONSOLE_ONLCR */
#define UK_CONSOLE_TEST_ONLCR_WANT "a\n\nb"
#endif /* !CONFIG_LIBUKCONSOLE_ONLCR */

UK_TESTCASE(uk_console_testsuite, uk_console_test_onlcr)
{
	static const char want[] = UK_CONSOLE_TEST_ONLCR_WANT;
	const __sz want_len = sizeof(want) - 1;
	__ssz out_rc, err_rc;

	uk_console_test_capture();
	out_rc = uk_console_out_onlcr("a\n\nb", 4);
	uk_console_test_capture_end();
	UK_TEST_EXPECT_SNUM_EQ(out_rc, 4);
	UK_TEST_EXPECT_SNUM_EQ(uk_console_test_sink_out.len, want_len);
	UK_TEST_EXPECT_BYTES_EQ(uk_console_test_sink_out.buf, want, want_len);

	uk_console_test_capture();
	err_rc = uk_console_err_onlcr("a\n\nb", 4);
	uk_console_test_capture_end();
	UK_TEST_EXPECT_SNUM_EQ(err_rc, 4);
	UK_TEST_EXPECT_SNUM_EQ(uk_console_test_sink_err.len, want_len);
	UK_TEST_EXPECT_BYTES_EQ(uk_console_test_sink_err.buf, want, want_len);
	UK_TEST_EXPECT_SNUM_EQ(uk_console_test_sink_out.len, 0);
}

UK_TESTCASE(uk_console_testsuite, uk_console_test_direct_onlcr)
{
	static const char want[] = UK_CONSOLE_TEST_ONLCR_WANT;
	const __sz want_len = sizeof(want) - 1;
	const char *buf = "a\n\nb";
	__sz left = 4, calls = 0;
	__ssz rc;

	/* A device that takes a byte at a time: every call makes progress,
	 * and the bytes add up to the translation.
	 */
	uk_console_test_capture();
	do {
		rc = uk_console_out_direct_onlcr(&uk_console_test_slow_dev,
						 buf, left);
		if (rc <= 0)
			break;
		buf += rc;
		left -= rc;
	} while (left && ++calls < 16);
	uk_console_test_capture_end();

	UK_TEST_EXPECT_SNUM_EQ(left, 0);
	UK_TEST_EXPECT_SNUM_EQ(uk_console_test_sink_slow.len, want_len);
	UK_TEST_EXPECT_BYTES_EQ(uk_console_test_sink_slow.buf, want, want_len);

	/* With the err op, and all at once */
	uk_console_test_capture();
	rc = uk_console_err_direct_onlcr(&uk_console_test_split_dev,
					 "a\n\nb", 4);
	uk_console_test_capture_end();
	UK_TEST_EXPECT_SNUM_EQ(rc, 4);
	UK_TEST_EXPECT_SNUM_EQ(uk_console_test_sink_err.len, want_len);
	UK_TEST_EXPECT_BYTES_EQ(uk_console_test_sink_err.buf, want, want_len);
}

uk_testsuite_register(uk_console_testsuite, NULL);
