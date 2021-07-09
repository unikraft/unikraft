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
#ifndef __UK_TEST_H__
#define __UK_TEST_H__

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

#include <uk/init.h>
#include <uk/list.h>
#include <uk/prio.h>
#include <uk/print.h>
#include <uk/config.h>
#include <uk/essentials.h>
#include <uk/arch/atomic.h>
#include <uk/plat/console.h>
#include <uk/plat/spinlock.h>

#include "log.h"
#include "assert.h"

#define UKT_PADDING		"............................................" \
				"............................................"
#if CONFIG_LIBUKDEBUG_ANSI_COLOR
#define UKT_COLWIDTH		80
#define UKT_CLR_RESET		UK_ANSI_MOD_RESET
#define UKT_CLR_PASSED		UK_ANSI_MOD_BOLD \
				UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_WHITE) \
				UK_ANSI_MOD_COLORBG(UK_ANSI_COLOR_GREEN)
#define UKT_CLR_FAILED		UK_ANSI_MOD_BOLD \
				UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_WHITE) \
				UK_ANSI_MOD_COLORBG(UK_ANSI_COLOR_RED)
#define LVLC_TESTNAME		UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_CYAN)
#else /* CONFIG_LIBUKDEBUG_ANSI_COLOR */
#define UKT_COLWIDTH		80
#define UKT_CLR_RESET		"]"
#define UKT_CLR_PASSED		"["
#define UKT_CLR_FAILED		"["
#define LVLC_TESTNAME
#endif /* !CONFIG_LIBUKDEBUG_ANSI_COLOR */

#define UKT_PASSED		UKT_CLR_PASSED " PASSED " UKT_CLR_RESET
#define UKT_FAILED		UKT_CLR_FAILED " FAILED " UKT_CLR_RESET

/**
 * struct uk_testcase - An individual test case.
 */
struct uk_testcase;
struct uk_testcase {
	/* The name of the test case. */
	const char *name;
	/* Pointer to the method  */
	void (*func)(struct uk_testcase *self);
	/* The number of failed assertions in this case. */
	unsigned int failed_asserts;
	/* The number of assertions in this case. */
	unsigned int total_asserts;
};

/**
 * Create a new test case based on a function.
 *
 * @param fn
 *   The function the case invokes.
 */
#define UK_TESTCASE(fn) {	\
	.name = STRINGIFY(fn),	\
	.func = &fn,		\
	.failed_asserts = 0	\
}

/**
 * struct uk_testsuite - A series of test cases.
 */
struct uk_testsuite;
struct uk_testsuite {
	/* Entry for list of block devices */
	UK_TAILQ_ENTRY(struct uk_testsuite) next;
	/* The name of the test suite. */
	const char *name;
	/* An optional initialization method for the suite. */
	int (*init)(struct uk_testsuite *self);
	/* The number of failed cases in this suite. */
	unsigned int failed_cases;
	/* The number of cases in this suite. */
	unsigned int total_cases;
	/* List of test cases. */
	struct uk_testcase *cases;
};

/**
 * List with test suites
 */
UK_TAILQ_HEAD(uk_testsuite_list, struct uk_testsuite);

/**
 * For each case in suite helper iterator.
 *
 * @param suite
 * @param esac
 */
#define uk_testsuite_for_each_case(suite, esac) \
	for (esac = suite->cases; esac->func; esac++)

/**
 * The total number of test suites.
 */
unsigned int
uk_testsuite_count(void);

/**
 * The total number of failed test suites.
 */
unsigned int
uk_testsuite_failed_count(void);

/**
 * The total number of test cases in all test suites.
 */
unsigned int
uk_testcase_count(void);

/**
 * The total number of failed test cases in all test suites.
 */
unsigned int
uk_testcase_failed_count(void);

/**
 * The total number of assertions in all test cases.
 */
unsigned int
uk_test_assert_count(void);

/**
 * The total number of failed assertions in all test cases.
 */
unsigned int
uk_test_assert_failed_count(void);

/**
 * Add a test suite to ther global list of available suites.
 *
 * @param suite
 *   A pointer to the test suite to add to the global list of suites.
 */
int
uk_testsuite_add(struct uk_testsuite *suite);

/**
 * Run a partcular suite, including all cases.
 *
 * @param suite
 *   A pointer to the suite to run.
 */
int
uk_testsuite_run(struct uk_testsuite *suite);

#define UK_TESTSUITE_FN(suite) \
	testsuite_run_ ## suite


/**
 * Add a test suite to constructor table at a specific prority level.
 *
 * @param suite
 *   A statically initialized `struct uk_testsuite`.
 * @param prio
 *   The priority level in the constuctor table.
 */
#define uk_test_at_ctorcall_prio(suite, prio)				\
	static void UK_TESTSUITE_FN(suite)(void)			\
	{								\
		uk_testsuite_add(&suite);				\
		uk_testsuite_run(&suite);				\
	}								\
	UK_CTOR_PRIO(UK_TESTSUITE_FN(suite), prio)

/**
 * Add a test suite at to inittab at a specific class and prority level.
 *
 * @param suite
 *   A reference to a `struct uk_testsuite`.
 * @param class
 *   The class at which this suite should be inserted within the inittab.
 * @param prio
 *   THe priority of this test suite.
 */
#define uk_test_at_initcall_prio(suite, class, prio)			\
	static int UK_TESTSUITE_FN(suite)(void)				\
	{								\
		uk_testsuite_add(&suite);				\
		uk_testsuite_run(&suite);				\
		return 0;						\
	}								\
	uk_initcall_class_prio(UK_TESTSUITE_FN(suite), class, prio)

/**
 * Add a test suite to be run in the "early" stage of the inittab.
 *
 * @param suite
 *   The pointer to the suite to add this stage.
 * @param prio
 *   The priority of this suite at this stage.
 */
#define uk_early_testsuite_prio(suite, prio) \
	uk_test_at_initcall_prio(suite, UK_INIT_CLASS_EARLY,   prio)

/**
 * Add a test suite to be run in the "plat" stage of the inittab.
 *
 * @param suite
 *   The pointer to the suite to add this stage.
 * @param prio
 *   The priority of this suite at this stage.
 */
#define uk_plat_testsuite_prio(suite, prio) \
	uk_test_at_initcall_prio(suite, UK_INIT_CLASS_PLAT,   prio)

/**
 * Add a test suite to be run in the "lib" stage of the inittab.
 *
 * @param suite
 *   The pointer to the suite to add this stage.
 * @param prio
 *   The priority of this suite at this stage.
 */
#define uk_lib_testsuite_prio(suite, prio) \
	uk_test_at_initcall_prio(suite, UK_INIT_CLASS_LIB,    prio)

/**
 * Add a test suite to be run in the "rootfs" stage of the inittab.
 *
 * @param suite
 *   The pointer to the suite to add this stage.
 * @param prio
 *   The priority of this suite at this stage.
 */
#define uk_rootfs_testsuite_prio(suite, prio) \
	uk_test_at_initcall_prio(suite, UK_INIT_CLASS_ROOTFS, prio)

/**
 * Add a test suite to be run in the "sys" stage of the inittab.
 *
 * @param suite
 *   The pointer to the suite to add this stage.
 * @param prio
 *   The priority of this suite at this stage.
 */
#define uk_sys_testsuite_prio(suite, prio) \
	uk_test_at_initcall_prio(suite, UK_INIT_CLASS_SYS,    prio)

/**
 * Add a test suite to be run in the "late" stage of the inittab.
 *
 * @param suite
 *   The pointer to the suite to add this stage.
 * @param prio
 *   The priority of this suite at this stage.
 */
#define uk_late_testsuite_prio(suite, prio) \
	uk_test_at_initcall_prio(suite, UK_INIT_CLASS_LATE,   prio)

/**
 * The default registration for a test suite with a desird priority level.
 *
 * @param suite
 *   The pointer to the suite to add this stage.
 * @param prio
 *   The priority of this suite at this stage.
 */
#define uk_testsuite_prio(suite, prio) \
	uk_late_testsuite_prio(suite, prio)

/**
 * The default registration for a test suite.
 *
 * @param suite
 *   The pointer to the suite to add this stage.
 */
#define uk_testsuite_register(suite) \
	uk_late_testsuite_prio(suite, UK_PRIO_LATEST)

static inline void
_uk_test_do_assert(struct uk_testcase *esac, bool cond, const char *fmt, ...)
{
#ifdef CONFIG_LIBUKTEST_LOG_TESTS
	va_list ap;
	int pad_len;
	char msg[UKT_COLWIDTH];

	/* Save formatted message to buffer. */
	va_start(ap, fmt);
	vsprintf(msg, fmt, ap);
	va_end(ap);

	/* Output the result. */
	pad_len = UKT_COLWIDTH - strlen(msg);
	if (pad_len < 0)
		pad_len = 0;
	uk_test_printf("%s %*.*s %s\n",
		       msg,
		       pad_len, pad_len,
		       UKT_PADDING,
		       cond ? UKT_PASSED : UKT_FAILED);
#endif /* CONFIG_LIBUKTEST_LOG_TESTS */

	esac->total_asserts++;

	if (!cond) {
		esac->failed_asserts++;

#ifdef CONFIG_LIBUKTEST_LOG_TESTS
		/* Output the location where the expectation failed. */
		uk_test_printf("  in " __STR_BASENAME__ ":"
			       STRINGIFY(__LINE__) "\n");
#endif /* CONFIG_LIBUKTEST_LOG_TESTS */

#if CONFIG_LIBUKTEST_FAILFAST
		UK_CRASH("Crashing on first failure!");
#endif
	}
}

/**
 * Assert a boolean condition.
 *
 * @param esac
 *   The test case under consideration.
 * @param cond
 *   The boolean condition of the assertion.
 */
#define UK_TEST_ASSERT(esac, cond) \
	_uk_test_do_assert(esac, cond, STRINGIFY(cond))

/**
 * Assert a boolean condition with formatted string.
 *
 * @param esac
 *   The test case under consideration.
 * @param cond
 *   The boolean condition to be evaluated for this assertion.
 * @param fmt
 *   Formatted string to print as help text to explain the assertion.
 */
#define UK_TEST_ASSERTF(esac, cond, fmt, ...) \
	_uk_test_do_assert(esac, cond, fmt, ##__VA_ARGS__)

/**
 * Expect a condition to be true.
 *
 * @param esac
 *   The test case under consideration.
 * @param cond
 *   The boolean condition which will render at runtime.
 */
#define UK_TEST_EXPECT(esac, cond)					\
	UK_TEST_ASSERTF(esac,						\
		cond,							\
		"expected `" STRINGIFY(cond) "` to be true"		\
	)

/**
 * Expect an expression to be NULL.
 *
 * @param esac
 *   The test case under consideration.
 * @param exp
 *   The expression under consideration.s
 */
#define UK_TEST_EXPECT_NULL(esac, exp)					\
	UK_TEST_ASSERTF(esac,						\
		exp == NULL,						\
		"expected `" STRINGIFY(exp) "` to be NULL"		\
	)

/**
 * Expect an expression to not be NULL.
 *
 * @param esac
 *   The test case under consideration.
 * @param exp
 *   The expression under consideration.s
 */
#define UK_TEST_EXPECT_NOT_NULL(esac, exp)				\
	UK_TEST_ASSERTF(esac,						\
		exp != NULL,						\
		"expected `" STRINGIFY(exp) "` to not be NULL"		\
	)

/**
 * Expect an expression to be evaluate to zero.
 *
 * @param esac
 *   The test case under consideration.
 * @param exp
 *   The expression under consideration.s
 */
#define UK_TEST_EXPECT_ZERO(esac, exp)					\
	UK_TEST_ASSERTF(esac,						\
		exp == 0,						\
		"expected `" STRINGIFY(exp) "` to be zero"		\
	)

/**
 * Expect an expression to not evaluate to zero.
 *
 * @param esac
 *   The test case under consideration.
 * @param exp
 *   The expression under consideration.s
 */
#define UK_TEST_EXPECT_NOT_ZERO(esac, exp)				\
	UK_TEST_ASSERTF(esac,						\
		exp != 0,						\
		"expected `" STRINGIFY(exp) "` to not be zero"		\
	)

/**
 * Expect two pointers to be equal to each other.
 *
 * @param esac
 *   The test case under consideration.
 * @param a
 *   The left-hand operand.
 * @param b
 *   The right-hand operand.
 */
#define UK_TEST_EXPECT_PTR_EQ(esac, a, b)				\
	UK_TEST_ASSERTF(esac,						\
		(a) == (b),						\
		"expected `" #a "` and `" #b "` to be %p "		\
		"but was %p",						\
		(void *)(uintptr_t)(a), (void *)(uintptr_t)(b)		\
	)

/**
 * Expect two byte values to be equal to eachother.
 *
 * @param esac
 *   The test case under consideration.
 * @param a
 *   The left-hand operand.
 * @param b
 *   The right-hand operand.
 */
#define UK_TEST_EXPECT_BYTES_EQ(esac, a, b, size)			\
	UK_TEST_ASSERTF(esac,						\
		memcmp(a, b, size) == 0,				\
		"expected `" #a "` at %p to equal `" #b "` at %p",	\
		(void *)(uintptr_t)(b), (void *)(uintptr_t)(a)		\
	)

/**
 * Expect two long integers to be equal to each other.
 *
 * @param esac
 *   The test case under consideration.
 * @param a
 *   The left-hand operand.
 * @param b
 *   The right-hand operand.
 */
#define UK_TEST_EXPECT_SNUM_EQ(esac, a, b)				\
	UK_TEST_ASSERTF(esac,						\
		(a) == (b),						\
		"expected `" #a "` to be %ld "				\
		"but was %ld",						\
		(long)(b), (long)(a)					\
	)

/**
 * Expect two long integers to not be equal to each other.
 *
 * @param esac
 *   The test case under consideration.
 * @param a
 *   The left-hand operand.
 * @param b
 *   The right-hand operand.
 */
#define UK_TEST_EXPECT_SNUM_NQ(esac, a, b)				\
	UK_TEST_ASSERTF(esac,						\
		(a) != (b),						\
		"expected `" #a "` to not be %ld "			\
		"but was %ld",						\
		(long)(b), (long)(a)					\
	)

/**
 * Expect the left hand long integersto be greater than the right.
 *
 * @param esac
 *   The test case under consideration.
 * @param a
 *   The left-hand operand.
 * @param b
 *   The right-hand operand.
 */
#define UK_TEST_EXPECT_SNUM_GT(esac, a, b)				\
	UK_TEST_ASSERTF(esac,						\
		(a) > (b),						\
		"expected `" #a "` to be greater than %ld "		\
		"but was %ld",						\
		(long)(b), (long)(a)					\
	)

/**
 * Expect the left hand long integer to be greater or equal to the right.
 *
 * @param esac
 *   The test case under consideration.
 * @param a
 *   The left-hand operand.
 * @param b
 *   The right-hand operand.
 */
#define UK_TEST_EXPECT_SNUM_GE(esac, a, b)				\
	UK_TEST_ASSERTF(esac,						\
		(a) >= (b),						\
		"expected `" #a "` to be greater than %ld "		\
		"but was %ld",						\
		(long)(b), (long)(a)					\
	)

/**
 * Expect the left-hand long integerr to be less than the right.
 *
 * @param esac
 *   The test case under consideration.
 * @param a
 *   The left-hand operand.
 * @param b
 *   The right-hand operand.
 */
#define UK_TEST_EXPECT_SNUM_LT(esac, a, b)				\
	UK_TEST_ASSERTF(esac,						\
		(a) < (b),						\
		"expected `" #a "` to be less than %ld "		\
		"but was %ld",						\
		(long)(b), (long)(a)					\
	)

/**
 * Expect the left-hand long integerr to be less than or equal the right.
 *
 * @param esac
 *   The test case under consideration.
 * @param a
 *   The left-hand operand.
 * @param b
 *   The right-hand operand.
 */
#define UK_TEST_EXPECT_SNUM_LE(esac, a, b)				\
	UK_TEST_ASSERTF(esac,						\
		(a) <= (b),						\
		"expected `" #a "` to be less than or equal to %ld "	\
		"but was %ld",						\
		(long)(b), (long)(a)					\
	)

#endif /* __UK_TEST_H__ */
