/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>
#include <uk/essentials.h>
#include <uk/libparam.h>
#include <uk/test.h>

#define TEST_PREFIX STRINGIFY(UK_LIBPARAM_LIBPREFIX)

static __s8 test_s8;
static __s16 test_s16;
static __s32 test_s32;
static __s64 test_s64;
static __u8 test_u8;

UK_LIBPARAM_PARAM(test_s8, __s8, "uklibparam test parameter");
UK_LIBPARAM_PARAM(test_s16, __s16, "uklibparam test parameter");
UK_LIBPARAM_PARAM(test_s32, __s32, "uklibparam test parameter");
UK_LIBPARAM_PARAM(test_s64, __s64, "uklibparam test parameter");
UK_LIBPARAM_PARAM(test_u8, __u8, "uklibparam test parameter");

static int parse_one(char *arg)
{
	return uk_libparam_parse(1, &arg, UK_LIBPARAM_F_STRICT);
}

UK_TESTCASE(uklibparam, parse_signed_min)
{
	char s8_min[] = TEST_PREFIX ".test_s8=-128";
	char s16_min[] = TEST_PREFIX ".test_s16=-32768";
	char s32_min[] = TEST_PREFIX ".test_s32=-2147483648";
	char s64_min[] = TEST_PREFIX ".test_s64=-9223372036854775808";

	UK_TEST_EXPECT_SNUM_EQ(parse_one(s8_min), 1);
	UK_TEST_EXPECT_SNUM_EQ(test_s8, __S8_MIN);
	UK_TEST_EXPECT_SNUM_EQ(parse_one(s16_min), 1);
	UK_TEST_EXPECT_SNUM_EQ(test_s16, __S16_MIN);
	UK_TEST_EXPECT_SNUM_EQ(parse_one(s32_min), 1);
	UK_TEST_EXPECT_SNUM_EQ(test_s32, __S32_MIN);
	UK_TEST_EXPECT_SNUM_EQ(parse_one(s64_min), 1);
	UK_TEST_EXPECT_SNUM_EQ(test_s64, __S64_MIN);
}

UK_TESTCASE(uklibparam, parse_signed_max)
{
	char s8_max[] = TEST_PREFIX ".test_s8=127";
	char s64_max[] = TEST_PREFIX ".test_s64=9223372036854775807";

	UK_TEST_EXPECT_SNUM_EQ(parse_one(s8_max), 1);
	UK_TEST_EXPECT_SNUM_EQ(test_s8, __S8_MAX);
	UK_TEST_EXPECT_SNUM_EQ(parse_one(s64_max), 1);
	UK_TEST_EXPECT_SNUM_EQ(test_s64, __S64_MAX);
}

UK_TESTCASE(uklibparam, parse_signed_out_of_range)
{
	char s8_below[] = TEST_PREFIX ".test_s8=-129";
	char s8_above[] = TEST_PREFIX ".test_s8=128";
	char s64_below[] = TEST_PREFIX ".test_s64=-9223372036854775809";
	char s64_above[] = TEST_PREFIX ".test_s64=9223372036854775808";

	UK_TEST_EXPECT_SNUM_EQ(parse_one(s8_below), -ERANGE);
	UK_TEST_EXPECT_SNUM_EQ(parse_one(s8_above), -ERANGE);
	UK_TEST_EXPECT_SNUM_EQ(parse_one(s64_below), -ERANGE);
	UK_TEST_EXPECT_SNUM_EQ(parse_one(s64_above), -ERANGE);
}

UK_TESTCASE(uklibparam, parse_unsigned_negative)
{
	char u8_zero[] = TEST_PREFIX ".test_u8=-0";
	char u8_neg[] = TEST_PREFIX ".test_u8=-1";

	UK_TEST_EXPECT_SNUM_EQ(parse_one(u8_zero), 1);
	UK_TEST_EXPECT_SNUM_EQ(test_u8, 0);
	UK_TEST_EXPECT_SNUM_EQ(parse_one(u8_neg), -ERANGE);
}

uk_testsuite_register(uklibparam, NULL);
