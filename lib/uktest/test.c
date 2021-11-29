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
#include <stdint.h>
#include <limits.h>

#include <uk/test.h>
#include <uk/list.h>
#include <uk/config.h>
#include <uk/essentials.h>

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
	struct uk_test_stats stats;

	UK_ASSERT(suite);

	/* Increase the number of recognized test suites. */
	testsuite_stats.total++;

	if (suite->init) {
		ret = suite->init(suite);

		if (ret != 0) {
			uk_pr_err("Could not initialize test suite: %s\n",
				  suite->name);
			goto ERR_EXIT;
		}
	}

	uk_testsuite_case_foreach(suite, esac) {
		/* Increase the number of recognized test cases. */
		testcase_stats.total++;

#ifdef CONFIG_LIBUKTEST_LOG_TESTS
		printf(LVLC_TESTNAME "test:" UK_ANSI_MOD_RESET
		       " %s->%s",
		       suite->name,
		       esac->name);
		if (esac->desc != NULL)
			printf(": %s\n", esac->desc);
		else
			printf("\n");
#endif /* CONFIG_LIBUKTEST_LOG_TESTS */

		esac->func(esac);

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
	}

	if (suite->failed_cases > 0)
		testsuite_stats.fail++;
	else
		testsuite_stats.success++;

EXIT:
	return ret;

ERR_EXIT:
	testsuite_stats.fail++;
	goto EXIT;
}
