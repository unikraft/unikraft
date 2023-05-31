/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Alexander Jung <a.jung@lancs.ac.uk>
 *          Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *
 * Copyright (c) 2021, Lancaster University.  All rights reserved.
 * Copyright (c) 2021, Karlsruhe Institute of Technology (KIT).
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

#include <stdio.h>
#include <stdint.h>
#include <limits.h>

#include <uk/test.h>
#include <uk/list.h>
#include <uk/config.h>
#include <uk/essentials.h>

#ifdef CONFIG_LIBUKTEST_LOG_STATS
/**
 * Test suite, case and assertion statistics
 */
struct uk_test_stats {
	unsigned int total;
	unsigned int fail;
	unsigned int success;
};

static struct uk_test_stats testsuite_stats = {0};
static struct uk_test_stats testcase_stats = {0};
static struct uk_test_stats testassert_stats = {0};

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

	/*
	 * Test suites
	 */

	uk_pr_info("uktest:suites:     ");
	failed = testsuite_stats.fail;
	if (failed > 0)
		uk_pr_info(UK_TEST_STAT_FAILED
			   "%d failed"
			   UK_ANSI_MOD_RESET
			   ", ", failed);
	uk_pr_info("%d total\n", testsuite_stats.total);

	/*
	 * Test cases
	 */

	uk_pr_info("uktest:cases:      ");
	failed = testcase_stats.fail;
	if (failed > 0)
		uk_pr_info(UK_TEST_STAT_FAILED
			   "%d failed"
			   UK_ANSI_MOD_RESET
			   ", ", failed);
	uk_pr_info("%d total\n", testcase_stats.total);

	/*
	 * Assertions
	 */

	uk_pr_info("uktest:assertions: ");
	failed = testassert_stats.fail;
	if (failed > 0)
		uk_pr_info(UK_TEST_STAT_FAILED
			   "%d failed"
			   UK_ANSI_MOD_RESET
			   ", ", failed);
	uk_pr_info("%d total\n", testassert_stats.total);

	return 0;
}

uk_late_initcall(uk_test_print_stats);
#endif /* CONFIG_LIBUKTEST_LOG_STATS */

static void
_uk_test_compute_assert_stats(struct uk_testcase *esac,
			      struct uk_test_stats *out)
{
	struct uk_assert *assert;

	UK_ASSERT(esac);
	UK_ASSERT(out);

	out->total = 0;
	out->fail = 0;
	out->success = 0;

	uk_test_assert_foreach(esac, assert) {
		out->total++;

		switch (assert->status) {
		case UK_TEST_ASSERT_SUCCESS:
			out->success++;
			break;
		case UK_TEST_ASSERT_FAIL:
			out->fail++;
			break;
		}
	}
}

int
uk_testsuite_run(struct uk_testsuite *suite)
{
	int ret = 0;
	struct uk_testcase *esac;
#ifdef CONFIG_LIBUKTEST_LOG_STATS
	struct uk_test_stats stats;
#endif /* CONFIG_LIBUKTEST_LOG_STATS */

	UK_ASSERT(suite);

#ifdef CONFIG_LIBUKTEST_LOG_STATS
	/* Increase the number of recognized test suites. */
	testsuite_stats.total++;
#endif /* CONFIG_LIBUKTEST_LOG_STATS */

	if (suite->init) {
		ret = suite->init(suite);

		if (ret != 0) {
			uk_pr_err("Could not initialize test suite: %s\n",
				  suite->name);
			goto ERR_EXIT;
		}
	}

	uk_testsuite_case_foreach(suite, esac) {
#ifdef CONFIG_LIBUKTEST_LOG_STATS
		/* Increase the number of recognized test cases. */
		testcase_stats.total++;
#endif /* CONFIG_LIBUKTEST_LOG_STATS */

#ifdef CONFIG_LIBUKTEST_LOG_TESTS
		_uk_printk(KLVL_INFO, UKLIBID_NONE, __NULL, 0x0,
			   (LVLC_TESTNAME "test:" UK_ANSI_MOD_RESET
			   " %s->%s"), suite->name, esac->name);
		if (esac->desc != NULL)
			_uk_printk(KLVL_INFO, UKLIBID_NONE, __NULL, 0x0,
				   ": %s\n", esac->desc);
		else
			_uk_printk(KLVL_INFO, UKLIBID_NONE, __NULL, 0x0, "\n");
#endif /* CONFIG_LIBUKTEST_LOG_TESTS */

		esac->func(esac);

#ifdef CONFIG_LIBUKTEST_LOG_STATS
		_uk_test_compute_assert_stats(esac, &stats);

		testassert_stats.total += stats.total;
		if (stats.fail > 0) {
			suite->failed_cases++;
			testcase_stats.fail++;
			testassert_stats.fail += stats.fail;
		}
		if (stats.success > 0) {
			testcase_stats.success++;
			testassert_stats.success += stats.success;
		}
#endif /* CONFIG_LIBUKTEST_LOG_STATS */
	}

#ifdef CONFIG_LIBUKTEST_LOG_STATS
	if (suite->failed_cases > 0)
		testsuite_stats.fail++;
	else
		testsuite_stats.success++;
#endif /* CONFIG_LIBUKTEST_LOG_STATS */

EXIT:
	return ret;

ERR_EXIT:
#ifdef CONFIG_LIBUKTEST_LOG_STATS
	testsuite_stats.fail++;
#endif /* CONFIG_LIBUKTEST_LOG_STATS */
	goto EXIT;
}
