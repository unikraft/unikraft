/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <string.h>
#include <uk/test.h>
#include <uk/streambuf.h>
#include <uk/essentials.h>
#include <uk/hexdump.h>

UK_TESTCASE(ukstreambuf, streambuf_init)
{
	struct uk_streambuf sb;
	__u8 buf[8] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };

	uk_streambuf_init(&sb, buf, ARRAY_SIZE(buf), 0x0);

	UK_TEST_EXPECT_PTR_EQ(uk_streambuf_buf(&sb), buf);
	UK_TEST_EXPECT_ZERO(uk_streambuf_seek(&sb));
	UK_TEST_EXPECT_PTR_EQ(uk_streambuf_wptr(&sb), buf);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_len(&sb), 0);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_left(&sb), ARRAY_SIZE(buf));
	UK_TEST_EXPECT_ZERO(uk_streambuf_istruncated(&sb));
	UK_TEST_EXPECT_SNUM_EQ(buf[0], 0x11);
	UK_TEST_EXPECT_SNUM_EQ(buf[7], 0x88);

	uk_streambuf_init(&sb, buf, ARRAY_SIZE(buf), UK_STREAMBUF_C_WIPEZERO);

	UK_TEST_EXPECT_ZERO(buf[0]);
	UK_TEST_EXPECT_ZERO(buf[7]);
}

UK_TESTCASE(ukstreambuf, streambuf_init_termshift)
{
	struct uk_streambuf sb;
	char buf[8] = { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h' };

	uk_streambuf_init(&sb, buf, ARRAY_SIZE(buf), UK_STREAMBUF_C_TERMSHIFT);

	UK_TEST_EXPECT_PTR_EQ(uk_streambuf_buf(&sb), buf);
	UK_TEST_EXPECT_ZERO(uk_streambuf_seek(&sb));
	UK_TEST_EXPECT_PTR_EQ(uk_streambuf_wptr(&sb), buf);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_len(&sb), 0);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_left(&sb), ARRAY_SIZE(buf));
	UK_TEST_EXPECT_ZERO(uk_streambuf_istruncated(&sb));
	UK_TEST_EXPECT_SNUM_EQ(buf[0], '\0');
}

uk_testsuite_register(ukstreambuf, NULL);
