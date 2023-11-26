/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Rico Muench <muench.rico@gmail.com>
 *          Stefan Jumarea <stefanjumarea02@gmail.com>
 *
 *
 * Copyright (c) 2021 Rico Muench <muench.rico@gmail.com>,
 *               2022 Stefan Jumarea <stefanjumarea02@gmail.com>
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

#include <uk/argparse.h>

UK_TESTCASE(ukargparse, parse_space_separated)
{
	char arg_str[] = "some/path/file -test 1 --test-this";
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

UK_TESTCASE(ukargparse, parse_null)
{
	char arg_str[] = "";

	char *arg_out = NULL;
	int argc = uk_argparse(arg_str, &arg_out, 0);

	UK_TEST_EXPECT_SNUM_EQ(argc, 0);
	UK_TEST_EXPECT_PTR_EQ(arg_out, NULL);
}

UK_TESTCASE(ukargparse, parse_extra_whitespaces)
{
	char arg_str[] = "some\t\t\n\t\tseparated\n\t\t  \n  string";
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

UK_TESTCASE(ukargparse, parse_quotes)
{
	char arg_str[] = "\"\targ_a\" ' arg_b' \"arg'c\" arg_d'' arg\"_\"e\"'\"";
	static const char * const arg_ex[] = {"\targ_a", " arg_b", "arg'c", "arg_d", "arg_e'"};

	// null-terminated array of parsed arguments
	char *arg_out[0x10] = {NULL};
	int argc = uk_argparse(arg_str, arg_out, ARRAY_SIZE(arg_out) - 1);

	UK_TEST_EXPECT_SNUM_EQ(argc, ARRAY_SIZE(arg_ex));
	if (argc != ARRAY_SIZE(arg_ex))
		return;

	for (size_t i = 0; i < ARRAY_SIZE(arg_ex); ++i)
		UK_TEST_EXPECT_SNUM_EQ(strcmp(arg_ex[i], arg_out[i]), 0);
}

UK_TESTCASE(ukargparse, parse_quotes_escaped)
{
	int argc, i;
	char *arg_vec[0x10] = { NULL };
	char arg_str[] =
		"\\'"
		" \\\""
		" \"arg0\\\"\""
		" \"\\\"arg1 '-'\\\"\"-\" \\\\ arg2\\\"\""
		" '\" \\\\ \\\" \\''\"'"
		" \\a\\b\\\"\\c\\\""
		" \"\\a\\b\\\"\\c\\\"\""
		" '\\a\\b\\\"\\c\\\"'"
		" '\\'a\\'"
		" a\\ b"
		" \\";
	static const char * const arg_exp[] = {
		"'",
		"\"",
		"arg0\"",
		"\"arg1 '-'\"- \\ arg2\"",
		"\" \\\\ \\\" \\\"",
		"ab\"c\"",
		"\\a\\b\"\\c\"",
		"\\a\\b\\\"\\c\\\"",
		"\\a'",
		"a b",
		"\\"
	};

	argc = uk_argparse(arg_str, arg_vec, ARRAY_SIZE(arg_vec) - 1);
	UK_TEST_EXPECT_SNUM_EQ(argc, ARRAY_SIZE(arg_exp));
	if (argc != ARRAY_SIZE(arg_exp))
		return;

	for (i = 0; i < (int) ARRAY_SIZE(arg_exp); ++i)
		UK_TEST_EXPECT_SNUM_EQ(strcmp(arg_exp[i], arg_vec[i]), 0);
}

uk_testsuite_register(ukargparse, NULL);
