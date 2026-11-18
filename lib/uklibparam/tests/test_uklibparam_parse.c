/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: B Srinivas Reddy <sriniv4sreddy@gmail.com>
 *
 * Copyright (c) 2026, B Srinivas Reddy <sriniv4sreddy@gmail.com>
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
#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Register the test parameters under their own library prefix so they
 * cannot clash with parameters of any other library linked into the image.
 */
#ifndef UK_LIBPARAM_LIBPREFIX
#define UK_LIBPARAM_LIBPREFIX uktestlibparam
#endif /* !UK_LIBPARAM_LIBPREFIX */

#include <uk/libparam.h>

static int test_bool;
static __s32 test_s32;
static __u8 test_u8;
static __s8 test_s8;
static char *test_str;
static __u32 test_arr[2];

/* Depending on the libc, `bool` may be a macro from <stdbool.h> by this
 * point; the registration macros need the raw token for their type name
 * concatenation.
 */
#pragma push_macro("bool")
#undef bool

UK_LIBPARAM_PARAM(test_bool, bool, "Boolean test parameter");
UK_LIBPARAM_PARAM(test_s32, __s32, "Signed 32-bit test parameter");
UK_LIBPARAM_PARAM(test_u8, __u8, "Unsigned 8-bit test parameter");
UK_LIBPARAM_PARAM(test_s8, __s8, "Signed 8-bit test parameter");
UK_LIBPARAM_PARAM(test_str, charp, "String test parameter");
UK_LIBPARAM_PARAM_ARR(test_arr, __u32, 2, "Array test parameter");

#pragma pop_macro("bool")

UK_TESTCASE(uklibparam, bool_without_value)
{
	char arg[] = "uktestlibparam.test_bool";
	int rc;

	test_bool = 0;
	rc = uk_libparam_parse(1, (char *[]) { arg }, 0);
	UK_TEST_EXPECT_SNUM_EQ(rc, 1);
	UK_TEST_EXPECT_SNUM_EQ(test_bool, 1);
}

UK_TESTCASE(uklibparam, bool_values)
{
	/* Non-zero numbers are "true", zero is "false" */
	static const char * const vals_true[] = {"on", "yes", "true", "7"};
	static const char * const vals_false[] = {"off", "no", "false", "0"};
	char arg[64];
	int rc;
	__sz i;

	for (i = 0; i < ARRAY_SIZE(vals_true); i++) {
		snprintf(arg, sizeof(arg), "uktestlibparam.test_bool=%s",
			 vals_true[i]);
		test_bool = 0;
		rc = uk_libparam_parse(1, (char *[]) { arg }, 0);
		UK_TEST_EXPECT_SNUM_EQ(rc, 1);
		UK_TEST_EXPECT_SNUM_EQ(test_bool, 1);
	}

	for (i = 0; i < ARRAY_SIZE(vals_false); i++) {
		snprintf(arg, sizeof(arg), "uktestlibparam.test_bool=%s",
			 vals_false[i]);
		test_bool = 1;
		rc = uk_libparam_parse(1, (char *[]) { arg }, 0);
		UK_TEST_EXPECT_SNUM_EQ(rc, 1);
		UK_TEST_EXPECT_SNUM_EQ(test_bool, 0);
	}
}

UK_TESTCASE(uklibparam, int_number_bases)
{
	char arg_dec[] = "uktestlibparam.test_s32=42";
	char arg_hex[] = "uktestlibparam.test_s32=0x2A";
	char arg_oct[] = "uktestlibparam.test_s32=052";
	char arg_neg[] = "uktestlibparam.test_s32=-42";
	int rc;

	rc = uk_libparam_parse(1, (char *[]) { arg_dec }, 0);
	UK_TEST_EXPECT_SNUM_EQ(rc, 1);
	UK_TEST_EXPECT_SNUM_EQ(test_s32, 42);

	test_s32 = -1;
	rc = uk_libparam_parse(1, (char *[]) { arg_hex }, 0);
	UK_TEST_EXPECT_SNUM_EQ(rc, 1);
	UK_TEST_EXPECT_SNUM_EQ(test_s32, 42);

	test_s32 = -1;
	rc = uk_libparam_parse(1, (char *[]) { arg_oct }, 0);
	UK_TEST_EXPECT_SNUM_EQ(rc, 1);
	UK_TEST_EXPECT_SNUM_EQ(test_s32, 42);

	rc = uk_libparam_parse(1, (char *[]) { arg_neg }, 0);
	UK_TEST_EXPECT_SNUM_EQ(rc, 1);
	UK_TEST_EXPECT_SNUM_EQ(test_s32, -42);
}

UK_TESTCASE(uklibparam, int_out_of_range)
{
	char arg_u8_ok[] = "uktestlibparam.test_u8=255";
	char arg_u8_over[] = "uktestlibparam.test_u8=256";
	char arg_s8_max[] = "uktestlibparam.test_s8=127";
	char arg_s8_over[] = "uktestlibparam.test_s8=128";
	char arg_s8_under[] = "uktestlibparam.test_s8=-129";
	int rc;

	rc = uk_libparam_parse(1, (char *[]) { arg_u8_ok }, 0);
	UK_TEST_EXPECT_SNUM_EQ(rc, 1);
	UK_TEST_EXPECT_SNUM_EQ(test_u8, 255);

	rc = uk_libparam_parse(1, (char *[]) { arg_s8_max }, 0);
	UK_TEST_EXPECT_SNUM_EQ(rc, 1);
	UK_TEST_EXPECT_SNUM_EQ(test_s8, 127);

	/* Values outside the type's range are rejected */
	test_u8 = 1;
	rc = uk_libparam_parse(1, (char *[]) { arg_u8_over },
			       UK_LIBPARAM_F_STRICT);
	UK_TEST_EXPECT_SNUM_EQ(rc, -ERANGE);
	UK_TEST_EXPECT_SNUM_EQ(test_u8, 1);

	test_s8 = 1;
	rc = uk_libparam_parse(1, (char *[]) { arg_s8_over },
			       UK_LIBPARAM_F_STRICT);
	UK_TEST_EXPECT_SNUM_EQ(rc, -ERANGE);
	UK_TEST_EXPECT_SNUM_EQ(test_s8, 1);

	test_s8 = 1;
	rc = uk_libparam_parse(1, (char *[]) { arg_s8_under },
			       UK_LIBPARAM_F_STRICT);
	UK_TEST_EXPECT_SNUM_EQ(rc, -ERANGE);
	UK_TEST_EXPECT_SNUM_EQ(test_s8, 1);
}

UK_TESTCASE(uklibparam, int_malformed)
{
	char arg_alpha[] = "uktestlibparam.test_s32=abc";
	char arg_trail[] = "uktestlibparam.test_s32=12x";
	char arg_empty[] = "uktestlibparam.test_s32=";
	int rc;

	test_s32 = 1;
	rc = uk_libparam_parse(1, (char *[]) { arg_alpha },
			       UK_LIBPARAM_F_STRICT);
	UK_TEST_EXPECT_SNUM_EQ(rc, -EINVAL);

	rc = uk_libparam_parse(1, (char *[]) { arg_trail },
			       UK_LIBPARAM_F_STRICT);
	UK_TEST_EXPECT_SNUM_EQ(rc, -EINVAL);

	rc = uk_libparam_parse(1, (char *[]) { arg_empty },
			       UK_LIBPARAM_F_STRICT);
	UK_TEST_EXPECT_SNUM_EQ(rc, -EINVAL);
	UK_TEST_EXPECT_SNUM_EQ(test_s32, 1);
}

UK_TESTCASE(uklibparam, string_value)
{
	char arg[] = "uktestlibparam.test_str=hello";
	char arg_noval[] = "uktestlibparam.test_str";
	int rc;

	test_str = __NULL;
	rc = uk_libparam_parse(1, (char *[]) { arg }, 0);
	UK_TEST_EXPECT_SNUM_EQ(rc, 1);
	UK_TEST_EXPECT_NOT_NULL(test_str);
	if (test_str)
		UK_TEST_EXPECT_SNUM_EQ(strcmp(test_str, "hello"), 0);

	/* A string parameter without a value is rejected */
	rc = uk_libparam_parse(1, (char *[]) { arg_noval },
			       UK_LIBPARAM_F_STRICT);
	UK_TEST_EXPECT_SNUM_EQ(rc, -EINVAL);

	/* The rejected parse must not clobber the previous value */
	UK_TEST_EXPECT_NOT_NULL(test_str);
	if (test_str)
		UK_TEST_EXPECT_SNUM_EQ(strcmp(test_str, "hello"), 0);
}

UK_TESTCASE(uklibparam, unknown_param)
{
	char arg[] = "uktestlibparam.does_not_exist=1";
	char arg_prefix[] = "uktestlibparam.test_boo=1";
	char arg_otherlib[] = "otherlib.test_s32=1";
	int rc;

	/* Without strict mode, unknown parameters are skipped */
	rc = uk_libparam_parse(1, (char *[]) { arg }, 0);
	UK_TEST_EXPECT_SNUM_EQ(rc, 1);

	rc = uk_libparam_parse(1, (char *[]) { arg }, UK_LIBPARAM_F_STRICT);
	UK_TEST_EXPECT_SNUM_EQ(rc, -ENOENT);

	/* Only exact parameter names match */
	rc = uk_libparam_parse(1, (char *[]) { arg_prefix },
			       UK_LIBPARAM_F_STRICT);
	UK_TEST_EXPECT_SNUM_EQ(rc, -ENOENT);

	/* Arguments under another library's prefix are unknown too */
	test_s32 = 7;
	rc = uk_libparam_parse(1, (char *[]) { arg_otherlib },
			       UK_LIBPARAM_F_STRICT);
	UK_TEST_EXPECT_SNUM_EQ(rc, -ENOENT);
	UK_TEST_EXPECT_SNUM_EQ(test_s32, 7);
}

UK_TESTCASE(uklibparam, stop_sequence)
{
	char arg_bool[] = "uktestlibparam.test_bool";
	char arg_stop[] = "--";
	char arg_s32[] = "uktestlibparam.test_s32=5";
	int rc;

	test_bool = 0;
	test_s32 = 0;
	rc = uk_libparam_parse(3, (char *[]) { arg_bool, arg_stop, arg_s32 },
			       0);
	UK_TEST_EXPECT_SNUM_EQ(rc, 1);
	UK_TEST_EXPECT_SNUM_EQ(test_bool, 1);
	UK_TEST_EXPECT_SNUM_EQ(test_s32, 0);
}

UK_TESTCASE(uklibparam, scan_mode)
{
	char arg[] = "uktestlibparam.test_s32=9";
	int rc;

	test_s32 = 0;
	rc = uk_libparam_parse(1, (char *[]) { arg }, UK_LIBPARAM_F_SCAN);
	UK_TEST_EXPECT_SNUM_EQ(rc, 1);
	UK_TEST_EXPECT_SNUM_EQ(test_s32, 0);
}

UK_TESTCASE(uklibparam, usage_request)
{
	char arg[] = "help";
	int rc;

	/* Without UK_LIBPARAM_F_USAGE, "help" is only flagged */
	rc = uk_libparam_parse(1, (char *[]) { arg }, 0);
	UK_TEST_EXPECT_SNUM_EQ(rc, 1);

	rc = uk_libparam_parse(1, (char *[]) { arg }, UK_LIBPARAM_F_USAGE);
	UK_TEST_EXPECT_SNUM_EQ(rc, -EINTR);
}

UK_TESTCASE(uklibparam, array_values)
{
	char arg_a[] = "uktestlibparam.test_arr=1";
	char arg_b[] = "uktestlibparam.test_arr=2";
	char arg_list1[] = "uktestlibparam.test_arr=[3]";
	char arg_c[] = "uktestlibparam.test_arr=4";
	char arg_open[] = "uktestlibparam.test_arr=[5";
	char arg_close[] = "6]";
	int rc;

	/* Repeated assignments fill the array in order */
	test_arr[0] = 0;
	test_arr[1] = 0;
	rc = uk_libparam_parse(2, (char *[]) { arg_a, arg_b }, 0);
	UK_TEST_EXPECT_SNUM_EQ(rc, 2);
	UK_TEST_EXPECT_SNUM_EQ(test_arr[0], 1);
	UK_TEST_EXPECT_SNUM_EQ(test_arr[1], 2);

	/* List form, single token, combined with a plain assignment */
	test_arr[0] = 0;
	test_arr[1] = 0;
	rc = uk_libparam_parse(2, (char *[]) { arg_list1, arg_c }, 0);
	UK_TEST_EXPECT_SNUM_EQ(rc, 2);
	UK_TEST_EXPECT_SNUM_EQ(test_arr[0], 3);
	UK_TEST_EXPECT_SNUM_EQ(test_arr[1], 4);

	/* List form, spread over two tokens */
	test_arr[0] = 0;
	test_arr[1] = 0;
	rc = uk_libparam_parse(2, (char *[]) { arg_open, arg_close }, 0);
	UK_TEST_EXPECT_SNUM_EQ(rc, 2);
	UK_TEST_EXPECT_SNUM_EQ(test_arr[0], 5);
	UK_TEST_EXPECT_SNUM_EQ(test_arr[1], 6);
}

UK_TESTCASE(uklibparam, array_overflow)
{
	char arg_a[] = "uktestlibparam.test_arr=7";
	char arg_b[] = "uktestlibparam.test_arr=8";
	char arg_c[] = "uktestlibparam.test_arr=9";
	int rc;

	/* The third assignment exceeds the array size of two */
	test_arr[0] = 0;
	test_arr[1] = 0;
	rc = uk_libparam_parse(3, (char *[]) { arg_a, arg_b, arg_c },
			       UK_LIBPARAM_F_STRICT);
	UK_TEST_EXPECT_SNUM_EQ(rc, -ENOSPC);
	UK_TEST_EXPECT_SNUM_EQ(test_arr[0], 7);
	UK_TEST_EXPECT_SNUM_EQ(test_arr[1], 8);

	/* Without strict mode the error is skipped */
	test_arr[0] = 0;
	test_arr[1] = 0;
	rc = uk_libparam_parse(3, (char *[]) { arg_a, arg_b, arg_c }, 0);
	UK_TEST_EXPECT_SNUM_EQ(rc, 3);
	UK_TEST_EXPECT_SNUM_EQ(test_arr[0], 7);
	UK_TEST_EXPECT_SNUM_EQ(test_arr[1], 8);
}

uk_testsuite_register(uklibparam, NULL);
