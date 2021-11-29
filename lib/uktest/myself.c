/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Alexander Jung <a.jung@lancs.ac.uk>
 *
 * Copyright (c) 2021, Lancaster University.  All rights reserved.
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

UK_TESTCASE(uktest_myself_testsuite, uktest_test_sanity)
{
	unsigned int x = 0xdeadb0b0;
	unsigned int *a = &x;
	unsigned int *b = &x;

	// Expect an expression to be NULL
	UK_TEST_EXPECT_NULL(NULL);

	// Expect an expression to not be NULL
	UK_TEST_EXPECT_NOT_NULL(1);

	// Expect an expression to evaluate to zero
	UK_TEST_EXPECT_ZERO(0);

	// Expect an expression to not evaluate to zero
	UK_TEST_EXPECT_NOT_ZERO(1);

	// Expect two pointers to be equal to each other
	UK_TEST_EXPECT_PTR_EQ(a, b);

	// Expect the contents of two buffers to be equal
	UK_TEST_EXPECT_BYTES_EQ(a, b, sizeof(x));

	// Expect two long integers to be equal to each other
	UK_TEST_EXPECT_SNUM_EQ(1, 1);

	// Expect two long integers to not be equal to each other
	UK_TEST_EXPECT_SNUM_NQ(0, 1);

	// Expect the left-hand long integer to be greater than the right
	UK_TEST_EXPECT_SNUM_GT(1, 0);

	// Expect the left-hand long integer to be greater or equal to the right
	UK_TEST_EXPECT_SNUM_GE(1, 1);
	UK_TEST_EXPECT_SNUM_GE(2, 1);

	// Expect the left-hand long integer to be less than the right
	UK_TEST_EXPECT_SNUM_LT(0, 1);

	// Expect the left-hand long integer to be less than or equal the right
	UK_TEST_EXPECT_SNUM_LE(1, 1);
	UK_TEST_EXPECT_SNUM_LE(1, 2);
}

UK_TESTSUITE_AT_CTORCALL_PRIO(uktest_myself_testsuite, NULL, 0);
