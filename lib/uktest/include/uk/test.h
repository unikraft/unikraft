/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Alexander Jung <a.jung@lancs.ac.uk>
 *          Marc Rittinghaus <marc.rittinghaus@kit.edu>
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
#include <errno.h>
#include <string.h>

#include <uk/init.h>
#include <uk/print.h>
#include <uk/ctors.h>
#include <uk/essentials.h>
#include <uk/plat/console.h>

#define UK_TEST_ASSERT_NOT_RUN  0
#define UK_TEST_ASSERT_SUCCESS  1
#define UK_TEST_ASSERT_FAIL     2


#define UKT_PADDING		"............................................" \
				"............................................"
#define UKT_FILENAME_LEN        255
#if CONFIG_LIBUKDEBUG_ANSI_COLOR
#define UKT_COLWIDTH		80
#define UKT_CLR_RESET		UK_ANSI_MOD_RESET
#define UKT_CLR_PASSED		UK_ANSI_MOD_BOLD \
				UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_WHITE) \
				UK_ANSI_MOD_COLORBG(UK_ANSI_COLOR_GREEN)
#define UKT_CLR_FAILED		UK_ANSI_MOD_BOLD \
				UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_WHITE) \
				UK_ANSI_MOD_COLORBG(UK_ANSI_COLOR_RED)
#define UKT_CLR_FILE            UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_CYAN)
#define UKT_CLR_LINE            UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_MAGENTA)
#define LVLC_TESTNAME		UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_CYAN)
#else /* CONFIG_LIBUKDEBUG_ANSI_COLOR */
#define UKT_COLWIDTH		80
#define UKT_CLR_RESET		"]"
#define UKT_CLR_PASSED		"["
#define UKT_CLR_FAILED		"["
#define UKT_CLR_FILE
#define UKT_CLR_LINE
#define LVLC_TESTNAME
#endif /* !CONFIG_LIBUKDEBUG_ANSI_COLOR */

#define UKT_PASSED		UKT_CLR_PASSED " PASSED " UKT_CLR_RESET
#define UKT_FAILED		UKT_CLR_FAILED " FAILED " UKT_CLR_RESET

/* Custom print method. */
#define uk_test_printf(fmt, ...)		\
	printf("    "				\
		  LVLC_TESTNAME ":"		\
		  UK_ANSI_MOD_RESET "\t"	\
		  fmt, ##__VA_ARGS__)


/**
 * Standard naming convention macro wrappers.
 */
#define _UK_TESTSUITE_NAME(suite)					\
	testsuite_ ## suite
#define _UK_TESTSUITE_RUN_NAME(suite)					\
	testsuite_run_ ## suite
#define _UK_TESTSUITE_RUN_NAME_END(suite)				\
	testsuite_run_ ## suite ## __end
#define _UK_TESTCASE_NAME(suite, esac)					\
	_uk_testsuite_ ## suite ## _case_ ## esac
#define _UK_TESTCASE_LABEL(suite, esac)					\
	_uk_testtab_ ## suite ## _ ## esac
#define _UK_TESTCASE_ASSERTTAB_START(suite, esac)			\
	_uk_testsuite_ ## suite ## _ ## esac ## __asserts_begin


/**
 * struct uk_assert - A reference to the individual assertion.
 */
struct uk_assert {
	unsigned short status;
	unsigned short line;
};


/**
 * References to the start and end of the assertion table.
 */
extern struct uk_assert uk_asserttab_start[];
extern struct uk_assert uk_asserttab_end[];


/**
 * struct uk_testcase - An individual test case.
 */
struct uk_testcase;
struct uk_testcase {
	/* The name of the test case. */
	const char *name;
	/* An optional short description */
	const char *desc;
	/* Pointer to the method  */
	void (*func)(struct uk_testcase *suite __unused);
	/* The case's assert table */
	struct uk_assert *asserts;
	/* Name of the file where the test case exists. */
	const char file[UKT_FILENAME_LEN];
};


/**
 * struct uk_testsuite - A series of test cases.
 */
struct uk_testsuite;
struct uk_testsuite {
	/* The name of the test suite. */
	const char *name;
	/* An optional initialization method for the suite. */
	int (*init)(struct uk_testsuite *suite __unused);
	/* List of test cases. */
	struct uk_testcase *cases;
	/* The number of failed cases in this suite. */
	unsigned int failed_cases;
};


/**
 * Macro to create a new entry in the assertion table.
 *
 * @param status
 *   The initial status of the assertion.
 */
#define _UK_TEST_ASSERTTAB_ENTRY(status)				\
	_UK_TEST_SECTION_DATA(						\
		".uk_asserttab_" __FILE__ "~" STRINGIFY(__LINE__),	\
		".word " STRINGIFY(status) "\n"				\
		".word " STRINGIFY(__LINE__)				\
	)

/**
 * Push a new test section label.
 *
 * @param section
 *   The name of the section
 * @param label
 *   The label for the section.
 */
#define __UK_TEST_SECTION_LABEL(section, label)				\
	__asm__ (							\
		".pushsection \"" section "\", \"a\"\n"			\
		#label ":\n"						\
		".popsection\n"						\
	)

#define _UK_TEST_SECTION_LABEL(section, label)				\
	__UK_TEST_SECTION_LABEL(section, label)

/**
 * Append data to a section label.
 *
 * @param section
 *   The name of the section
 * @param data
 *   The data for the section.
 */
#define __UK_TEST_SECTION_DATA(section, data)				\
	__asm__ (							\
		".pushsection \"" section "\", \"a\"\n"			\
		data "\n"						\
		".popsection\n"						\
	)

#define _UK_TEST_SECTION_DATA(section, data)				\
	__UK_TEST_SECTION_DATA(section, data)

/**
 * Macros for registering new test suite entries.
 *
 * @param suite
 *   Reference to the test suite.
 * @param initfn
 *   The initialization function for the suite.
 */
#define _UK_TEST_SECTION_HEADER(suite, initfn)				\
	extern struct uk_testcase _UK_TESTSUITE_RUN_NAME_END(suite)[];	\
	struct uk_testsuite						\
	__used __section(".uk_testtab_" #suite "~")			\
	_UK_TESTSUITE_NAME(suite) = {					\
		.name = #suite,						\
		.init = initfn,						\
		.cases = _UK_TESTSUITE_RUN_NAME_END(suite),		\
		.failed_cases = 0					\
	}

#define __UK_TESTSUITE(name, initfn)					\
	_UK_TEST_SECTION_HEADER(name, initfn);				\
	_UK_TEST_SECTION_LABEL(".uk_testtab_" #name "~~",		\
		_UK_TESTSUITE_RUN_NAME_END(name))

#define _UK_TESTSUITE(name, initfn)					\
	__UK_TESTSUITE(name, initfn)


/**
 * Create a new test case based on a function with a description.
 *
 * @param suite
 *   The test suite of the case.
 * @param fn
 *   The function the case invokes.
 * @param desc
 *   A short description of the test case.
 */
#define UK_TESTCASE_DESC(suite, fn, dsc)				\
	void _UK_TESTCASE_NAME(suite, fn)(struct uk_testcase *esac __unused);\
	_UK_TEST_SECTION_LABEL(						\
		".uk_asserttab_" __FILE__ "~" STRINGIFY(__LINE__),	\
		_UK_TESTCASE_ASSERTTAB_START(suite, fn)			\
	);								\
	_UK_TEST_ASSERTTAB_ENTRY(0xffff);				\
	extern struct uk_assert _UK_TESTCASE_ASSERTTAB_START(suite, fn)[];\
	struct uk_testcase						\
	__used __section(".uk_testtab_" #suite "~" #fn)			\
	_UK_TESTCASE_LABEL(suite, fn) = {				\
		.name = #fn,						\
		.desc = dsc,						\
		.func = _UK_TESTCASE_NAME(suite, fn),			\
		.file = __FILE__,					\
		.asserts = _UK_TESTCASE_ASSERTTAB_START(suite, fn)	\
	};								\
	void _UK_TESTCASE_NAME(suite, fn)(struct uk_testcase *esac __unused)


/**
 * Create a new test case based on a function.
 *
 * @param suite
 *   The test suite of the case.
 * @param fn
 *   The function the case invokes.
 */
#define UK_TESTCASE(suite, fn)						\
	UK_TESTCASE_DESC(suite, fn, NULL)


/**
 * Assertion iterator for a case.
 *
 * @param esac
 *   A reference to the case with the list of assertions.
 * @param assert
 *   The currently iterated assertion.
 */
#define uk_test_assert_foreach(esac, assert)				\
	for ((assert) = ((esac)->asserts + 1);				\
	     ((assert) < uk_asserttab_end) && ((assert)->status != 0xffff);\
	     (assert)++)


/**
 * Helper macro which points to the start of the test cases for a test suite.
 *
 * @param suite
 *   A statically initialized `struct uk_testsuite`.
 */
#define _UK_TESTCASE_START(suite)					\
	((struct uk_testsuite *)((suite) + 1))


/**
 * Helper macro which iterates each case in a test suite.
 *
 * @param suite
 *   A statically initialized `struct uk_testsuite`.
 * @param esac
 *   A reference pointer to a case which will be available on each iteration.
 */
#define uk_testsuite_case_foreach(suite, esac)				\
	for ((esac) = DECONST(struct uk_testcase*, _UK_TESTCASE_START(suite));\
	     (esac) < ((suite)->cases);					\
	     (esac)++)


int
uk_testsuite_run(struct uk_testsuite *suite);

/**
 * Add a test suite to constructor table at a specific priority level.
 *
 * @param suite
 *   A statically initialized `struct uk_testsuite`.
 * @param initfn
 *   Reference to the initialization function for the suite.
 * @param prio
 *   The priority level in the constuctor table.
 */
#define UK_TESTSUITE_AT_CTORCALL_PRIO(suite, initfn, prio)		\
	_UK_TESTSUITE(suite, initfn);					\
	static void _UK_TESTSUITE_RUN_NAME(suite)(void)			\
	{								\
		uk_testsuite_run(&_UK_TESTSUITE_NAME(suite));		\
	}								\
	UK_CTOR_PRIO(_UK_TESTSUITE_RUN_NAME(suite), prio)


/**
 * Add a test suite at to inittab at a specific class and priority level.
 *
 * @param suite
 *   A reference to a `struct uk_testsuite`.
 * @param initfn
 *   Reference to the initialization function for the suite.
 * @param class
 *   The class at which this suite should be inserted within the inittab.
 * @param prio
 *   THe priority of this test suite.
 */
#define UK_TESTSUITE_AT_INITCALL_PRIO(suite, initfn, class, prio)	\
	_UK_TESTSUITE(suite, initfn);					\
	static int _UK_TESTSUITE_RUN_NAME(suite)(void)			\
	{								\
		return uk_testsuite_run(&_UK_TESTSUITE_NAME(suite));	\
	}								\
	uk_initcall_class_prio(_UK_TESTSUITE_RUN_NAME(suite), class, prio)


/**
 * Add a test suite to be run in the "early" stage of the inittab.
 *
 * @param suite
 *   The pointer to the suite to add this stage.
 * @param initfn
 *   The initialization function for the suite.
 * @param prio
 *   The priority of this suite at this stage.
 */
#define uk_testsuite_early_prio(suite, initfn, prio)			\
	UK_TESTSUITE_AT_INITCALL_PRIO(suite, initfn, UK_INIT_CLASS_EARLY, prio)


/**
 * Add a test suite to be run in the "plat" stage of the inittab.
 *
 * @param suite
 *   The pointer to the suite to add this stage.
 * @param initfn
 *   The initialization function for the suite.
 * @param prio
 *   The priority of this suite at this stage.
 */
#define uk_testsuite_plat_prio(suite, initfn, prio) \
	UK_TESTSUITE_AT_INITCALL_PRIO(suite, initfn, UK_INIT_CLASS_PLAT, prio)


/**
 * Add a test suite to be run in the "lib" stage of the inittab.
 *
 * @param suite
 *   The pointer to the suite to add this stage.
 * @param initfn
 *   The initialization function for the suite.
 * @param prio
 *   The priority of this suite at this stage.
 */
#define uk_testsuite_lib_prio(suite, initfn, prio) \
	UK_TESTSUITE_AT_INITCALL_PRIO(suite, initfn, UK_INIT_CLASS_LIB, prio)


/**
 * Add a test suite to be run in the "rootfs" stage of the inittab.
 *
 * @param suite
 *   The pointer to the suite to add this stage.
 * @param initfn
 *   The initialization function for the suite.
 * @param prio
 *   The priority of this suite at this stage.
 */
#define uk_testsuite_rootfs_prio(suite, initfn, prio) \
	UK_TESTSUITE_AT_INITCALL_PRIO(suite, initfn, UK_INIT_CLASS_ROOTFS, prio)


/**
 * Add a test suite to be run in the "sys" stage of the inittab.
 *
 * @param suite
 *   The pointer to the suite to add this stage.
 * @param initfn
 *   The initialization function for the suite.
 * @param prio
 *   The priority of this suite at this stage.
 */
#define uk_testsuite_sys_prio(suite, initfn, prio) \
	UK_TESTSUITE_AT_INITCALL_PRIO(suite, initfn, UK_INIT_CLASS_SYS, prio)


/**
 * Add a test suite to be run in the "late" stage of the inittab.
 *
 * @param suite
 *   The pointer to the suite to add this stage.
 * @param initfn
 *   The initialization function for the suite.
 * @param prio
 *   The priority of this suite at this stage.
 */
#define uk_testsuite_late_prio(suite, initfn, prio) \
	UK_TESTSUITE_AT_INITCALL_PRIO(suite, initfn, UK_INIT_CLASS_LATE, prio)


/**
 * The default registration for a test suite with a desired priority level.
 *
 * @param suite
 *   The pointer to the suite to add this stage.
 * @param initfn
 *   The initialization function for the suite.
 * @param prio
 *   The priority of this suite at this stage.
 */
#define uk_testsuite_prio(suite, initfn, prio) \
	uk_testsuite_late_prio(suite, initfn, prio)


/**
 * The default registration for a test suite.
 *
 * @param suite
 *   The pointer to the suite to add this stage.
 * @param initfn
 *   The initialization function for the suite.
 */
#define uk_testsuite_register(suite, initfn) \
	uk_testsuite_prio(suite, initfn, UK_PRIO_LATEST)


/**
 * Find an assertion based on line number.
 *
 * @param esac
 *   The case with list of assertions to iterate over.
 * @param line
 *   The line of the assertion to find.
 */
static struct uk_assert*
_uk_test_find_assert(struct uk_testcase *esac, unsigned short line)
{
	struct uk_assert* assert;

	uk_test_assert_foreach(esac, assert) {
		if (assert->line == line)
			return assert;
	}

	return NULL;
}


/**
 * Perform an assertion; saving and logging its result.
 *
 * @param esac
 *   The case where the assertion is registered to.
 * @param line
 *   The line of the assertion.
 * @param cond
 *   The boolean result of the assertion.
 * @param fmt...
 *   The formatted string to print about the assertion.
 */
static void
_uk_test_do_assert(struct uk_testcase *esac, unsigned short line, int cond,
		   const char *fmt, ...)
{
#ifdef CONFIG_LIBUKTEST_LOG_TESTS
	va_list ap;
	int pad_len;
	char msg[UKT_COLWIDTH];

	/* Save formatted message to buffer. */
	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
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
#endif

	struct uk_assert* assert = _uk_test_find_assert(esac, line);
	assert->status = (cond) ? UK_TEST_ASSERT_SUCCESS : UK_TEST_ASSERT_FAIL;

#if defined(CONFIG_LIBUKTEST_LOG_TESTS) || defined(CONFIG_LIBUKTEST_FAILFAST)
	if (!cond) {
#ifdef CONFIG_LIBUKTEST_LOG_TESTS
		/* Output the location where the expectation failed. */
		uk_test_printf("  in "
			       UKT_CLR_FILE __STR_BASENAME__ UKT_CLR_RESET
			       ":" UKT_CLR_FILE "%d" UKT_CLR_RESET "\n", line);
#endif /* CONFIG_LIBUKTEST_LOG_TESTS */
#if CONFIG_LIBUKTEST_FAILFAST
		UK_CRASH("Crashing on first failure!");
#endif
	}
#endif /* CONFIG_LIBUKTEST_LOG_TESTS || CONFIG_LIBUKTEST_FAILFAST */
}


/**
 * Assert a boolean condition with formatted string.
 *
 * @param cond
 *   The boolean condition to be evaluated for this assertion.
 * @param fmt
 *   Formatted string to print as help text to explain the assertion.
 */
#define UK_TEST_ASSERTF(cond, fmt, ...)					\
	do {								\
		_UK_TEST_ASSERTTAB_ENTRY(UK_TEST_ASSERT_NOT_RUN);	\
		_uk_test_do_assert(esac, __LINE__, cond, fmt, ##__VA_ARGS__);\
	} while (0)


/**
 * Expect a condition to be true.
 *
 * @param cond
 *   The boolean condition which will render at runtime.
 */
#define UK_TEST_EXPECT(cond)						\
	UK_TEST_ASSERTF(						\
		cond,							\
		"expected `" STRINGIFY(cond) "` to be true"		\
	)

#define UK_TEST_ASSERT(cond) UK_TEST_EXPECT(cond)


/**
 * Expect an expression to be NULL.
 *
 * @param exp
 *   The expression under consideration.
 */
#define UK_TEST_EXPECT_NULL(exp)					\
	UK_TEST_ASSERTF(esac,						\
		exp == NULL,						\
		"expected `" STRINGIFY(exp) "` to be NULL"		\
	)


/**
 * Expect an expression to not be NULL.
 *
 * @param exp
 *   The expression under consideration.
 */
#define UK_TEST_EXPECT_NOT_NULL(exp)					\
	UK_TEST_ASSERTF(						\
		exp != NULL,						\
		"expected `" STRINGIFY(exp) "` to not be NULL"		\
	)


/**
 * Expect an expression to be evaluate to zero.
 *
 * @param exp
 *   The expression under consideration.
 */
#define UK_TEST_EXPECT_ZERO(exp)					\
	UK_TEST_ASSERTF(						\
		exp == 0,						\
		"expected `" STRINGIFY(exp) "` to be zero"		\
	)


/**
 * Expect an expression to not evaluate to zero.
 *
 * @param exp
 *   The expression under consideration.
 */
#define UK_TEST_EXPECT_NOT_ZERO(exp)					\
	UK_TEST_ASSERTF(						\
		exp != 0,						\
		"expected `" STRINGIFY(exp) "` to not be zero"		\
	)


/**
 * Expect two pointers to be equal to each other.
 *
 * @param a
 *   The left-hand operand.
 * @param b
 *   The right-hand operand.
 */
#define UK_TEST_EXPECT_PTR_EQ(a, b)					\
	UK_TEST_ASSERTF(						\
		(a) == (b),						\
		"expected `" #a "` and `" #b "` to be %p "		\
		"but was `" #b "` was %p",				\
		(void *)(uintptr_t)(a), (void *)(uintptr_t)(b)		\
	)


/**
 * Expect the contents of two buffers to be equal.
 *
 * @param a
 *   The left-hand operand.
 * @param b
 *   The right-hand operand.
 */
#define UK_TEST_EXPECT_BYTES_EQ(a, b, size)				\
	UK_TEST_ASSERTF(						\
		memcmp(a, b, size) == 0,				\
		"expected `" #a "` at %p to equal `" #b "` at %p",	\
		(void *)(uintptr_t)(b), (void *)(uintptr_t)(a)		\
	)


/**
 * Expect two long integers to be equal to each other.
 *
 * @param a
 *   The left-hand operand.
 * @param b
 *   The right-hand operand.
 */
#define UK_TEST_EXPECT_SNUM_EQ(a, b)					\
	UK_TEST_ASSERTF(						\
		(a) == (b),						\
		"expected `" #a "` to be %ld "				\
		"but was %ld",						\
		(long)(b), (long)(a)					\
	)


/**
 * Expect two long integers to not be equal to each other.
 *
 * @param a
 *   The left-hand operand.
 * @param b
 *   The right-hand operand.
 */
#define UK_TEST_EXPECT_SNUM_NQ(a, b)					\
	UK_TEST_ASSERTF(						\
		(a) != (b),						\
		"expected `" #a "` to not be %ld "			\
		"but was %ld",						\
		(long)(b), (long)(a)					\
	)


/**
 * Expect the left-hand long integersto be greater than the right.
 *
 * @param a
 *   The left-hand operand.
 * @param b
 *   The right-hand operand.
 */
#define UK_TEST_EXPECT_SNUM_GT(a, b)					\
	UK_TEST_ASSERTF(						\
		(a) > (b),						\
		"expected `" #a "` to be greater than %ld "		\
		"but was %ld",						\
		(long)(b), (long)(a)					\
	)


/**
 * Expect the left-hand long integer to be greater or equal to the right.
 *
 * @param a
 *   The left-hand operand.
 * @param b
 *   The right-hand operand.
 */
#define UK_TEST_EXPECT_SNUM_GE(a, b)					\
	UK_TEST_ASSERTF(						\
		(a) >= (b),						\
		"expected `" #a "` to be greater than %ld "		\
		"but was %ld",						\
		(long)(b), (long)(a)					\
	)


/**
 * Expect the left-hand long integer to be less than the right.
 *
 * @param a
 *   The left-hand operand.
 * @param b
 *   The right-hand operand.
 */
#define UK_TEST_EXPECT_SNUM_LT(a, b)					\
	UK_TEST_ASSERTF(						\
		(a) < (b),						\
		"expected `" #a "` to be less than %ld "		\
		"but was %ld",						\
		(long)(b), (long)(a)					\
	)


/**
 * Expect the left-hand long integer to be less than or equal the right.
 *
 * @param a
 *   The left-hand operand.
 * @param b
 *   The right-hand operand.
 */
#define UK_TEST_EXPECT_SNUM_LE(a, b)					\
	UK_TEST_ASSERTF(						\
		(a) <= (b),						\
		"expected `" #a "` to be less than or equal to %ld "	\
		"but was %ld",						\
		(long)(b), (long)(a)					\
	)


#endif /* __UK_TEST_H__ */
