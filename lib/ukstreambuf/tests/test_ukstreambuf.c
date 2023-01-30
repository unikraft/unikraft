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

UK_TESTCASE(ukstreambuf, streambuf_strcpy)
{
	struct uk_streambuf sb;
	char buf[10];
	const char result[10] = { 't', 'e', 's', 't', '1', '\0',
				  '-', '\0', 't', '\0' };
	__sz ret;

	uk_streambuf_init(&sb, buf, ARRAY_SIZE(buf), 0x0);

	/* First string */
	ret = uk_streambuf_strcpy(&sb, "test1");
	UK_TEST_EXPECT_SNUM_EQ(ret, 6); /* written size incl. '\0' */
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_seek(&sb), 6);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_len(&sb), 6);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_left(&sb), 4);
	UK_TEST_EXPECT_PTR_EQ(uk_streambuf_wptr(&sb), &buf[6]);
	UK_TEST_EXPECT_ZERO(uk_streambuf_istruncated(&sb));

	/* Append another string */
	ret = uk_streambuf_strcpy(&sb, "-");
	UK_TEST_EXPECT_SNUM_EQ(ret, 2);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_seek(&sb), 8);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_len(&sb), 8);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_left(&sb), 2);
	UK_TEST_EXPECT_PTR_EQ(uk_streambuf_wptr(&sb), &buf[8]);
	/* '\0'-termination should be kept between appended strings */
	UK_TEST_EXPECT_SNUM_EQ(buf[5], '\0');
	UK_TEST_EXPECT_ZERO(uk_streambuf_istruncated(&sb));

	/* Running out of space */
	ret = uk_streambuf_strcpy(&sb, "test2");
	UK_TEST_EXPECT_SNUM_EQ(ret, 2);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_seek(&sb), 10);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_len(&sb), 10);
	UK_TEST_EXPECT_ZERO(uk_streambuf_left(&sb));
	UK_TEST_EXPECT_PTR_EQ(uk_streambuf_wptr(&sb), &buf[10]);
	UK_TEST_EXPECT_SNUM_EQ(buf[7], '\0');
	UK_TEST_EXPECT_SNUM_EQ(buf[9], '\0');
	UK_TEST_EXPECT_NOT_ZERO(uk_streambuf_istruncated(&sb));

	/* No space left */
	ret = uk_streambuf_strcpy(&sb, "-");
	UK_TEST_EXPECT_SNUM_EQ(ret, 0);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_seek(&sb), 10);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_len(&sb), 10);
	UK_TEST_EXPECT_ZERO(uk_streambuf_left(&sb));
	UK_TEST_EXPECT_PTR_EQ(uk_streambuf_wptr(&sb), &buf[10]);
	UK_TEST_EXPECT_SNUM_EQ(buf[9], '\0');
	UK_TEST_EXPECT_NOT_ZERO(uk_streambuf_istruncated(&sb));

	/* Result on buf as expected? */
	UK_TEST_EXPECT_ZERO(memcmp(buf, result, ARRAY_SIZE(buf)));
}

UK_TESTCASE(ukstreambuf, streambuf_strcpy_termshift)
{
	struct uk_streambuf sb;
	char buf[10];
	const char result[10] = { 't', 'e', 's', 't', '1', '-',
				  't', 'e', 's', '\0' };
	__sz ret;

	/*
	 * NOTE: With TERMSHIFT, the wptr should move one position backwards
	 *       for successive writes (resulting in C-string concatenation).
	 */
	uk_streambuf_init(&sb, buf, ARRAY_SIZE(buf), UK_STREAMBUF_C_TERMSHIFT);

	/* First string */
	ret = uk_streambuf_strcpy(&sb, "test1");
	UK_TEST_EXPECT_SNUM_EQ(ret, 6); /* written size incl. '\0' */
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_seek(&sb), 5);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_len(&sb), 6);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_left(&sb), 5);
	UK_TEST_EXPECT_PTR_EQ(uk_streambuf_wptr(&sb), &buf[5]);
	UK_TEST_EXPECT_ZERO(uk_streambuf_istruncated(&sb));

	/* Append another string */
	ret = uk_streambuf_strcpy(&sb, "-");
	UK_TEST_EXPECT_SNUM_EQ(ret, 2);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_seek(&sb), 6);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_len(&sb), 7);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_left(&sb), 4);
	UK_TEST_EXPECT_PTR_EQ(uk_streambuf_wptr(&sb), &buf[6]);
	/* There should be no '\0'-termination between concatenated strings */
	UK_TEST_EXPECT_SNUM_NQ(buf[5], '\0');
	UK_TEST_EXPECT_ZERO(uk_streambuf_istruncated(&sb));

	/* Running out of space */
	ret = uk_streambuf_strcpy(&sb, "test2");
	UK_TEST_EXPECT_SNUM_EQ(ret, 4);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_seek(&sb), 9);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_len(&sb), 10);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_left(&sb), 1);
	UK_TEST_EXPECT_PTR_EQ(uk_streambuf_wptr(&sb), &buf[9]);
	UK_TEST_EXPECT_SNUM_NQ(buf[6], '\0');
	UK_TEST_EXPECT_SNUM_EQ(buf[9], '\0');
	UK_TEST_EXPECT_NOT_ZERO(uk_streambuf_istruncated(&sb));

	/* No space left */
	ret = uk_streambuf_strcpy(&sb, "-");
	UK_TEST_EXPECT_SNUM_EQ(ret, 1); /* '\0' written */
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_seek(&sb), 9);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_len(&sb), 10);
	/* Last '\0' stays over-writable (but only with '\0')*/
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_left(&sb), 1);
	UK_TEST_EXPECT_PTR_EQ(uk_streambuf_wptr(&sb), &buf[9]);
	UK_TEST_EXPECT_SNUM_EQ(buf[9], '\0');
	UK_TEST_EXPECT_NOT_ZERO(uk_streambuf_istruncated(&sb));

	/* Result on buf as expected? */
	UK_TEST_EXPECT_ZERO(memcmp(buf, result, ARRAY_SIZE(buf)));
}

UK_TESTCASE(ukstreambuf, streambuf_printf)
{
	struct uk_streambuf sb;
	char buf[10];
	const char result[10] = { 't', 'e', 's', 't', '1', '\0',
				  '-', '\0', 't', '\0' };
	__sz ret;

	uk_streambuf_init(&sb, buf, ARRAY_SIZE(buf), 0x0);

	/* First string */
	ret = uk_streambuf_printf(&sb, "%s", "test1");
	UK_TEST_EXPECT_SNUM_EQ(ret, 6); /* written size incl. '\0' */
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_seek(&sb), 6);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_len(&sb), 6);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_left(&sb), 4);
	UK_TEST_EXPECT_PTR_EQ(uk_streambuf_wptr(&sb), &buf[6]);
	UK_TEST_EXPECT_ZERO(uk_streambuf_istruncated(&sb));

	/* Append another string */
	ret = uk_streambuf_printf(&sb, "%s", "-");
	UK_TEST_EXPECT_SNUM_EQ(ret, 2);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_seek(&sb), 8);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_len(&sb), 8);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_left(&sb), 2);
	UK_TEST_EXPECT_PTR_EQ(uk_streambuf_wptr(&sb), &buf[8]);
	/* '\0'-termination should be kept between appended strings */
	UK_TEST_EXPECT_SNUM_EQ(buf[5], '\0');
	UK_TEST_EXPECT_ZERO(uk_streambuf_istruncated(&sb));

	/* Running out of space */
	ret = uk_streambuf_printf(&sb, "%s", "test2");
	UK_TEST_EXPECT_SNUM_EQ(ret, 2);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_seek(&sb), 10);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_len(&sb), 10);
	UK_TEST_EXPECT_ZERO(uk_streambuf_left(&sb));
	UK_TEST_EXPECT_PTR_EQ(uk_streambuf_wptr(&sb), &buf[10]);
	UK_TEST_EXPECT_SNUM_EQ(buf[7], '\0');
	UK_TEST_EXPECT_SNUM_EQ(buf[9], '\0');
	UK_TEST_EXPECT_NOT_ZERO(uk_streambuf_istruncated(&sb));

	/* No space left */
	ret = uk_streambuf_printf(&sb, "%s", "-");
	UK_TEST_EXPECT_SNUM_EQ(ret, 0);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_seek(&sb), 10);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_len(&sb), 10);
	UK_TEST_EXPECT_ZERO(uk_streambuf_left(&sb));
	UK_TEST_EXPECT_PTR_EQ(uk_streambuf_wptr(&sb), &buf[10]);
	UK_TEST_EXPECT_SNUM_EQ(buf[9], '\0');
	UK_TEST_EXPECT_NOT_ZERO(uk_streambuf_istruncated(&sb));

	/* Result on buf as expected? */
	UK_TEST_EXPECT_ZERO(memcmp(buf, result, ARRAY_SIZE(buf)));
}

UK_TESTCASE(ukstreambuf, streambuf_printf_termshift)
{
	struct uk_streambuf sb;
	char buf[10];
	const char result[10] = "test1-tes";
	__sz ret;

	/*
	 * NOTE: With TERMSHIFT, the wptr should move one position backwards
	 *       for successive writes (resulting in C-string concatenation).
	 */
	uk_streambuf_init(&sb, buf, ARRAY_SIZE(buf), UK_STREAMBUF_C_TERMSHIFT);

	/* First string */
	ret = uk_streambuf_printf(&sb, "%s", "test1");
	UK_TEST_EXPECT_SNUM_EQ(ret, 6); /* written size incl. '\0' */
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_seek(&sb), 5);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_len(&sb), 6);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_left(&sb), 5);
	UK_TEST_EXPECT_PTR_EQ(uk_streambuf_wptr(&sb), &buf[5]);
	UK_TEST_EXPECT_SNUM_EQ(buf[5], '\0');
	UK_TEST_EXPECT_ZERO(uk_streambuf_istruncated(&sb));

	/* Append another string */
	ret = uk_streambuf_printf(&sb, "%s", "-");
	UK_TEST_EXPECT_SNUM_EQ(ret, 2);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_seek(&sb), 6);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_len(&sb), 7);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_left(&sb), 4);
	UK_TEST_EXPECT_PTR_EQ(uk_streambuf_wptr(&sb), &buf[6]);
	/* There should be no '\0'-termination between concatenated strings */
	UK_TEST_EXPECT_SNUM_NQ(buf[5], '\0');
	UK_TEST_EXPECT_ZERO(uk_streambuf_istruncated(&sb));

	/* Running out of space */
	ret = uk_streambuf_printf(&sb, "%s", "test2");
	UK_TEST_EXPECT_SNUM_EQ(ret, 4);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_seek(&sb), 9);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_len(&sb), 10);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_left(&sb), 1);
	UK_TEST_EXPECT_PTR_EQ(uk_streambuf_wptr(&sb), &buf[9]);
	UK_TEST_EXPECT_SNUM_NQ(buf[6], '\0');
	UK_TEST_EXPECT_SNUM_EQ(buf[9], '\0');
	UK_TEST_EXPECT_NOT_ZERO(uk_streambuf_istruncated(&sb));

	/* No space left */
	ret = uk_streambuf_printf(&sb, "%s", "-");
	UK_TEST_EXPECT_SNUM_EQ(ret, 1); /* '\0' written */
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_seek(&sb), 9);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_len(&sb), 10);
	/* Last '\0' stays over-writable (but only with '\0')*/
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_left(&sb), 1);
	UK_TEST_EXPECT_PTR_EQ(uk_streambuf_wptr(&sb), &buf[9]);
	UK_TEST_EXPECT_SNUM_EQ(buf[9], '\0');
	UK_TEST_EXPECT_NOT_ZERO(uk_streambuf_istruncated(&sb));

	/* Result on buf as expected? */
	UK_TEST_EXPECT_ZERO(memcmp(buf, result, ARRAY_SIZE(buf)));
}

UK_TESTCASE(ukstreambuf, streambuf_memcpy)
{
	struct uk_streambuf sb;
	char buf[10];
	const char str_test1[] = "test1";
	const char str_minus[] = "-";
	const char str_test2[] = "test2";
	const char result[10] = { 't', 'e', 's', 't', '1', '\0',
				  '-', '\0', 't', 'e' };
	__sz ret;

	uk_streambuf_init(&sb, buf, ARRAY_SIZE(buf), 0x0);

	/* First memcpy */
	ret = uk_streambuf_memcpy(&sb, str_test1, 6);
	UK_TEST_EXPECT_SNUM_EQ(ret, 6);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_seek(&sb), 6);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_len(&sb), 6);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_left(&sb), 4);
	UK_TEST_EXPECT_PTR_EQ(uk_streambuf_wptr(&sb), &buf[6]);
	UK_TEST_EXPECT_ZERO(uk_streambuf_istruncated(&sb));

	/* Another memcpy */
	ret = uk_streambuf_memcpy(&sb, str_minus, 2);
	UK_TEST_EXPECT_SNUM_EQ(ret, 2);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_seek(&sb), 8);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_len(&sb), 8);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_left(&sb), 2);
	UK_TEST_EXPECT_PTR_EQ(uk_streambuf_wptr(&sb), &buf[8]);
	UK_TEST_EXPECT_ZERO(uk_streambuf_istruncated(&sb));

	/* Running out of space */
	ret = uk_streambuf_memcpy(&sb, str_test2, 6);
	UK_TEST_EXPECT_SNUM_EQ(ret, 2);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_seek(&sb), 10);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_len(&sb), 10);
	UK_TEST_EXPECT_ZERO(uk_streambuf_left(&sb));
	UK_TEST_EXPECT_PTR_EQ(uk_streambuf_wptr(&sb), &buf[10]);
	UK_TEST_EXPECT_NOT_ZERO(uk_streambuf_istruncated(&sb));

	/* No space left */
	ret = uk_streambuf_memcpy(&sb, str_minus, 2);
	UK_TEST_EXPECT_SNUM_EQ(ret, 0);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_seek(&sb), 10);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_len(&sb), 10);
	UK_TEST_EXPECT_ZERO(uk_streambuf_left(&sb));
	UK_TEST_EXPECT_PTR_EQ(uk_streambuf_wptr(&sb), &buf[10]);
	UK_TEST_EXPECT_NOT_ZERO(uk_streambuf_istruncated(&sb));

	/* Result on buf as expected? */
	UK_TEST_EXPECT_ZERO(memcmp(buf, result, ARRAY_SIZE(buf)));
}

UK_TESTCASE(ukstreambuf, streambuf_memcpy_termshift)
{
	struct uk_streambuf sb;
	char buf[10];
	const char str_test1[] = "test1";
	const char str_minus[] = "-";
	const char str_test2[] = "test2";
	const char result[10] = { 't', 'e', 's', 't', '1', '-',
				  't', 'e', 's', '-' };
	__sz ret;

	/*
	 * NOTE: With TERMSHIFT, the wptr should move one position backwards
	 *       for successive writes (resulting in C-string concatenation).
	 */
	uk_streambuf_init(&sb, buf, ARRAY_SIZE(buf), UK_STREAMBUF_C_TERMSHIFT);

	/* First string */
	ret = uk_streambuf_memcpy(&sb, str_test1, 6);
	UK_TEST_EXPECT_SNUM_EQ(ret, 6);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_seek(&sb), 5);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_len(&sb), 6);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_left(&sb), 5);
	UK_TEST_EXPECT_PTR_EQ(uk_streambuf_wptr(&sb), &buf[5]);
	UK_TEST_EXPECT_SNUM_EQ(buf[5], '\0');
	UK_TEST_EXPECT_ZERO(uk_streambuf_istruncated(&sb));

	/* Append another string */
	ret = uk_streambuf_memcpy(&sb, str_minus, 2);
	UK_TEST_EXPECT_SNUM_EQ(ret, 2);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_seek(&sb), 6);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_len(&sb), 7);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_left(&sb), 4);
	UK_TEST_EXPECT_PTR_EQ(uk_streambuf_wptr(&sb), &buf[6]);
	UK_TEST_EXPECT_SNUM_EQ(buf[6], '\0');
	UK_TEST_EXPECT_ZERO(uk_streambuf_istruncated(&sb));

	/* Running out of space */
	ret = uk_streambuf_memcpy(&sb, str_test2, 6);
	UK_TEST_EXPECT_SNUM_EQ(ret, 4);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_seek(&sb), 9);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_len(&sb), 10);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_left(&sb), 1);
	UK_TEST_EXPECT_PTR_EQ(uk_streambuf_wptr(&sb), &buf[9]);
	UK_TEST_EXPECT_SNUM_NQ(buf[9], '\0');
	UK_TEST_EXPECT_NOT_ZERO(uk_streambuf_istruncated(&sb));

	/*
	 * No space left but the last character (due to TERMSHIFT) should
	 * be over-writable
	 */
	ret = uk_streambuf_memcpy(&sb, str_minus, 2);
	UK_TEST_EXPECT_SNUM_EQ(ret, 1); /* '-' written */
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_seek(&sb), 9);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_len(&sb), 10);
	/* Last '\0' stays overwritable (but only with '\0')*/
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_left(&sb), 1);
	UK_TEST_EXPECT_PTR_EQ(uk_streambuf_wptr(&sb), &buf[9]);
	UK_TEST_EXPECT_SNUM_NQ(buf[9], '\0');
	UK_TEST_EXPECT_NOT_ZERO(uk_streambuf_istruncated(&sb));

	/* Result on buf as expected? */
	UK_TEST_EXPECT_ZERO(memcmp(buf, result, ARRAY_SIZE(buf)));
}

UK_TESTCASE(ukstreambuf, streambuf_reserve)
{
	struct uk_streambuf sb;
	char buf[10];
	void *ret;

	uk_streambuf_init(&sb, buf, ARRAY_SIZE(buf), 0x0);

	/* First blob */
	ret = uk_streambuf_reserve(&sb, 6);
	UK_TEST_EXPECT_PTR_EQ(ret, &buf[0]);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_seek(&sb), 6);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_len(&sb), 6);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_left(&sb), 4);
	UK_TEST_EXPECT_PTR_EQ(uk_streambuf_wptr(&sb), &buf[6]);
	UK_TEST_EXPECT_ZERO(uk_streambuf_istruncated(&sb));

	/* Another blob */
	ret = uk_streambuf_reserve(&sb, 2);
	UK_TEST_EXPECT_PTR_EQ(ret, &buf[6]);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_seek(&sb), 8);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_len(&sb), 8);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_left(&sb), 2);
	UK_TEST_EXPECT_PTR_EQ(uk_streambuf_wptr(&sb), &buf[8]);
	UK_TEST_EXPECT_ZERO(uk_streambuf_istruncated(&sb));

	/* Out of space */
	ret = uk_streambuf_reserve(&sb, 6);
	UK_TEST_EXPECT_NULL(ret);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_seek(&sb), 8);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_len(&sb), 8);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_left(&sb), 2);
	UK_TEST_EXPECT_PTR_EQ(uk_streambuf_wptr(&sb), &buf[8]);
	UK_TEST_EXPECT_ZERO(uk_streambuf_istruncated(&sb));
}

UK_TESTCASE(ukstreambuf, streambuf_reserve_termshift)
{
	struct uk_streambuf sb;
	char buf[10];
	void *ret;

	uk_streambuf_init(&sb, buf, ARRAY_SIZE(buf), UK_STREAMBUF_C_TERMSHIFT);

	/* First blob */
	ret = uk_streambuf_reserve(&sb, 6);
	UK_TEST_EXPECT_PTR_EQ(ret, &buf[0]);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_seek(&sb), 5);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_len(&sb), 6);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_left(&sb), 5);
	UK_TEST_EXPECT_PTR_EQ(uk_streambuf_wptr(&sb), &buf[5]);
	UK_TEST_EXPECT_ZERO(uk_streambuf_istruncated(&sb));

	/* Another blob */
	ret = uk_streambuf_reserve(&sb, 2);
	UK_TEST_EXPECT_PTR_EQ(ret, &buf[5]);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_seek(&sb), 6);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_len(&sb), 7);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_left(&sb), 4);
	UK_TEST_EXPECT_PTR_EQ(uk_streambuf_wptr(&sb), &buf[6]);
	UK_TEST_EXPECT_ZERO(uk_streambuf_istruncated(&sb));

	/* Out of space */
	ret = uk_streambuf_reserve(&sb, 6);
	UK_TEST_EXPECT_NULL(ret);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_seek(&sb), 6);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_len(&sb), 7);
	UK_TEST_EXPECT_SNUM_EQ(uk_streambuf_left(&sb), 4);
	UK_TEST_EXPECT_PTR_EQ(uk_streambuf_wptr(&sb), &buf[6]);
	UK_TEST_EXPECT_ZERO(uk_streambuf_istruncated(&sb));
}

/*
 * NOTE: `uk_streambuf_printf()` relies on `snprintf()` which may require a
 *       TLS (depending on selected libc):
 *       We should run the unit tests after a TLS is set up. This means that
 *       the tests should not be invoked from the ctortab.
 */
uk_testsuite_register(ukstreambuf, NULL);
