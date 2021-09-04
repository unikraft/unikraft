/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Rico Muench <muench.rico@gmail.com>
 *
 *
 * Copyright (c) 2020, NEC Laboratories Europe GmbH, NEC Corporation,
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
#include <stdint.h>

#include "../include/uk/argparse.h"

#if CONFIG_LIBUKARGPARSE_TESTS_PARSE
UK_TESTCASE(ukargparse, parse_space_separated)
{
	char *arg_str = {"some/path/file -test 1 --test-this"};
	static const char * const arg_ex[] = {"some/path/file",
		"-test", "1", "--test-this"};

	// null-terminated array of parsed arguments
	char *arg_out[0x10] = {NULL};
	int argc = uk_argparse(arg_str, arg_out, ARRAY_SIZE(arg_out) - 1);

	UK_TEST_EXPECT_SNUM_EQ(argc, ARRAY_SIZE(arg_ex));
	if (argc != ARRAY_SIZE(arg_ex))
		return;

	for (size_t i = 0; i < ARRAY_SIZE(arg_ex); ++i)
		UK_TEST_EXPECT_SNUM_EQ(strcmp(arg_ex[i], arg_out[i]), 0);
}
#endif

#if CONFIG_LIBUKARGPARSE_TESTS_NULL
UK_TESTCASE(ukargparse, parse_null)
{
	char arg_str[] = "";

	char *arg_out = NULL;
	int argc = uk_argparse(arg_str, &arg_out, 0);

	UK_TEST_EXPECT_SNUM_EQ(argc, 0);
	UK_TEST_EXPECT_PTR_EQ(arg_out, NULL);
}
#endif

#if CONFIG_LIBUKARGPARSE_TESTS_EXTRA
UK_TESTCASE(ukargparse, parse_extra_whitespaces)
{
	char *arg_str = {"some\t\t\n\t\tseparated\n\t\t  \n  string"};
	static const char * const arg_ex[] = {"some", "separated", "string"};

	// null-terminated array of parsed arguments
	char *arg_out[0x10] = {NULL};
	int argc = uk_argparse(arg_str, arg_out, ARRAY_SIZE(arg_out) - 1);

	UK_TEST_EXPECT_SNUM_EQ(argc, ARRAY_SIZE(arg_ex));
	if (argc != ARRAY_SIZE(arg_ex))
		return;

	for (size_t i = 0; i < ARRAY_SIZE(arg_ex); ++i)
		UK_TEST_EXPECT_SNUM_EQ(strcmp(arg_ex[i], arg_out[i]), 0);
}
#endif

uk_testsuite_register(ukargparse, NULL);
