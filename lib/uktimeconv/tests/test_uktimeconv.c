/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/test.h>
#include <uk/timeutil.h>

UK_TESTCASE(uktimeconv, spec_sum_no_carry)
{
	struct timespec a = { .tv_sec = 1, .tv_nsec = 100 };
	struct timespec b = { .tv_sec = 2, .tv_nsec = 200 };
	struct timespec r = uk_time_spec_sum(&a, &b);

	UK_TEST_EXPECT_SNUM_EQ(r.tv_sec, 3);
	UK_TEST_EXPECT_SNUM_EQ(r.tv_nsec, 300);
}

UK_TESTCASE(uktimeconv, spec_sum_carry_exact)
{
	struct timespec a = { .tv_sec = 0, .tv_nsec = 500000000 };
	struct timespec b = { .tv_sec = 0, .tv_nsec = 500000000 };
	struct timespec r = uk_time_spec_sum(&a, &b);

	UK_TEST_EXPECT_SNUM_EQ(r.tv_sec, 1);
	UK_TEST_EXPECT_SNUM_EQ(r.tv_nsec, 0);
	UK_TEST_EXPECT(uk_time_spec_canonical(&r));
}

UK_TESTCASE(uktimeconv, spec_sum_carry)
{
	struct timespec a = { .tv_sec = 2, .tv_nsec = 999999999 };
	struct timespec b = { .tv_sec = 3, .tv_nsec = 999999999 };
	struct timespec r = uk_time_spec_sum(&a, &b);

	UK_TEST_EXPECT_SNUM_EQ(r.tv_sec, 6);
	UK_TEST_EXPECT_SNUM_EQ(r.tv_nsec, 999999998);
}

uk_testsuite_register(uktimeconv, NULL);
