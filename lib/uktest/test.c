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

#include <stdio.h>
#include <limits.h>

#include <uk/test.h>
#include <uk/list.h>
#include <uk/assert.h>
#include <uk/config.h>
#include <uk/essentials.h>

struct uk_testsuite_list uk_testsuite_list =
	UK_TAILQ_HEAD_INITIALIZER(uk_testsuite_list);
uint16_t testsuite_count;

unsigned int
uk_testsuite_count(void)
{
	return (unsigned int) testsuite_count;
}

unsigned int
uk_testsuite_failed_count(void)
{
	unsigned int count = 0;
	struct uk_testsuite *suite;

	UK_TAILQ_FOREACH(suite, &uk_testsuite_list, next) {
		if (suite->failed_cases > 0)
			count++;
	}

	return count;
}

unsigned int
uk_testcase_count(void)
{
	unsigned int count = 0;
	struct uk_testsuite *suite;
	struct uk_testcase *testcase;

	UK_TAILQ_FOREACH(suite, &uk_testsuite_list, next) {
		uk_testsuite_for_each_case(suite, testcase) {
			count++;
		}
	}

	return count;
}

unsigned int
uk_testcase_failed_count(void)
{
	unsigned int count = 0;
	struct uk_testsuite *suite;

	UK_TAILQ_FOREACH(suite, &uk_testsuite_list, next) {
		count += suite->failed_cases;
	}

	return count;
}

unsigned int
uk_test_assert_count(void)
{
	unsigned int count = 0;
	struct uk_testsuite *suite;
	struct uk_testcase *testcase;

	UK_TAILQ_FOREACH(suite, &uk_testsuite_list, next) {
		uk_testsuite_for_each_case(suite, testcase) {
			count += testcase->total_asserts;
		}
	}

	return count;
}

unsigned int
uk_test_assert_failed_count(void)
{
	unsigned int count = 0;
	struct uk_testsuite *suite;
	struct uk_testcase *testcase;

	UK_TAILQ_FOREACH(suite, &uk_testsuite_list, next) {
		uk_testsuite_for_each_case(suite, testcase) {
			count += testcase->failed_asserts;
		}
	}

	return count;
}

#ifdef CONFIG_LIBUKTEST_PRINT_STATS
#include <uk/plat/console.h>

#define UK_TEST_STATS_INIT_CLASS UK_INIT_CLASS_LATE
#define UK_TEST_STATS_INIT_PRIO  9 /* As late as possible. */

#if CONFIG_LIBUKDEBUG_ANSI_COLOR
#define UK_TEST_STAT_FAILED	UK_ANSI_MOD_BOLD \
				UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_RED)
#else /* CONFIG_LIBUKDEBUG_ANSI_COLOR */
#define UK_TEST_STAT_FAILED
#endif /* !CONFIG_LIBUKDEBUG_ANSI_COLOR */

static int
uk_test_print_stats(void)
{
	int failed;

	uk_printd("\nTest Summary:\n");

	/*
	 * Test suites
	 */

	uk_printd(" - Suites:     ");
	failed = uk_testsuite_failed_count();
	if (failed > 0)
		uk_printd(UK_TEST_STAT_FAILED
			  "%d failed"
			  UK_ANSI_MOD_RESET
			  ", ", failed);
	uk_printd("%d total\n", uk_testsuite_count());

	/*
	 * Test cases
	 */

	uk_printd(" - Cases:      ");
	failed = uk_testcase_failed_count();
	if (failed > 0)
		uk_printd(UK_TEST_STAT_FAILED
			  "%d failed"
			  UK_ANSI_MOD_RESET
			  ", ", failed);
	uk_printd("%d total\n", uk_testcase_count());

	/*
	 * Assertions
	 */

	uk_printd(" - Assertions: ");
	failed = uk_test_assert_failed_count();
	if (failed > 0)
		uk_printd(UK_TEST_STAT_FAILED
			  "%d failed"
			  UK_ANSI_MOD_RESET
			  ", ", failed);
	uk_printd("%d total\n", uk_test_assert_count());

	return 0;
}
uk_initcall_class_prio(
	uk_test_print_stats,
	UK_TEST_STATS_INIT_CLASS,
	UK_TEST_STATS_INIT_PRIO
);
#endif /* CONFIG_LIBUKTEST_PRINT_STATS */

int
uk_testsuite_add(struct uk_testsuite *suite)
{
	UK_ASSERT(suite);
	UK_TAILQ_INSERT_TAIL(&uk_testsuite_list, suite, next);

	return testsuite_count++;
}

int
uk_testsuite_run(struct uk_testsuite *suite)
{
	int ret = 0;
	struct uk_testcase *testcase;

	if (suite->init) {
		ret = suite->init(suite);

		if (ret != 0) {
			uk_pr_err("Could not initialize test suite: %s",
				  suite->name);
			goto ERR_EXIT;
		}
	}

	/* Reset the number of failed cases before running each case. */
	suite->failed_cases = 0;

	uk_testsuite_for_each_case(suite, testcase) {
#ifdef CONFIG_LIBUKTEST_LOG_TESTS
		uk_printd(LVLC_TESTNAME "test:" UK_ANSI_MOD_RESET
			  " [%s:%s]\n",
			  suite->name,
			  testcase->name
		);
#endif /* CONFIG_LIBUKTEST_LOG_TESTS */

		testcase->func(testcase);

		/* If one case fails, the whole suite fails. */
		if (testcase->failed_asserts > 0)
			suite->failed_cases++;
	}

ERR_EXIT:
	return ret;
}
