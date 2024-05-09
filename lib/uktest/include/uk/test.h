/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Alexander Jung <a.jung@lancs.ac.uk>
 *          Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *
 * Copyright (c) 2021, Lancaster University.
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

#ifndef __UK_TEST_H__
#define __UK_TEST_H__

/**
 * ## Overview
 *
 * In `uktest`, tests are organised hierarchically powered by the the lowest
 * common denominator: the assertion.  This organisation is inspired by KUnit,
 * the Linux Kernel's in-house testing system.  For licensing reasons the
 * Unikraft project cannot use this source code.  However, by inspiration, we
 * can organise the `uktest` library following a similar pattern:
 *
 * 1. The assertion: repersents the lowest common denominator of a test: some
 *    boolean operation which, when true, a single test passes.  Assertions are
 *    often used in-line and their usage should be no different to the
 *    traditional use of the `ASSERT` macro.  In `uktest`, we introduce a new
 *    definition: `UK_TEST_EXPECT` which has one parameters: the same boolean
 *    opeation which is true-to-form of the traditional `ASSERT` macro.  With
 *    `uktest`, however, the macro is intelligently placed in context within a
 *    case (see 2.).  Additional text or descriptive explanation of the text can
 *    also be provided with the auxiliary and similar macro `UK_TEST_ASSERTF`.
 *
 * 2. The test case: often, assertions are not alone in their means to check the
 *    legitimacy of operation in some function.  We find that a "case" is best
 *    way to organise a group of assertions in which one particular function of
 *    some system is under-going testing.  A case is independent of other cases,
 *    but related in the same sub-system.  For this reason we register them
 *    together in the same test suite.
 *
 * 3. The test suite: represents a group of test cases.  This is the final and
 *    upper-most heirarchical repesentation of tests and their groupings.  With
 *    assertions grouped into test cases and test cases grouped into a test
 *    suite, we end this organisation in a fashion which allows us to follow a
 *    common design pattern within Unikraft: the registration model.  The syntax
 *    follows similar to other registation models within Unikraft, e.g. `ukbus`.
 *    However, `uktest`'s registation model is more powerful.
 *
 *
 *
 * ## Creating tests
 *
 * To register a test suite with `uktest`, we simply invoke
 * `uk_testsuite_register` with a unique symbol name.  This symbol is used along
 * with test cases in order to create the references to one-another.  Each test
 * case has only two input parameters: a reference to the suite is part, as well
 * as a canonical name for the case itself.  Generally, the following pattern is
 * used for test suites:
 *
 *  $LIBNAME_$TESTSUITENAME_testsuite
 *
 * An the following for test cases:
 *
 *  $LIBNAME_test_$TESTCASENAME
 *
 * To create a case, simply invoke the `UK_TESTCASE` macro with the two
 * parameters describe previously, and use in the context of a function, for
 * example:
 *
 * ```c
 * UK_TESTCASE(uktest_mycase_testsuite, uktest_test_case)
 * {
 *         int x = 1;
 *         UK_TEST_EXPECT(x > 0);
 * }
 * ```
 *
 * Finally, to register the case with a suite (see next section), call one of
 * the possible registration functions:
 *
 * ```c
 * uk_testsuite_register(uktest_mycase_testsuite, NULL);
 * ```
 *
 * The above snippet can be organised within a library in a number of ways such
 * as in-line or as an individual file representing the suite.  There are a
 * number of test suite registration handles which are elaborated on in next
 * section.  It should be noted that multiple test suites can exist within a
 * library in order to test multiple features or components of said library.
 *
 * In order to achieve consistency in the use of `uktest` across the Unikraft
 * code base, the following recommendation is made regarding the registration of
 * test suites:
 *
 * 1. A single test suite should be organised into its own file, prefixed with
 *    `test_`, e.g. `test_feature.c`. All tests suites of some library within
 *    Unikraft should be stored within a new folder located at the root of the
 *    library named `tests/`.
 * 2. All tests suites should have a corresponding KConfig option, prefixed with
 *    the library name and then the word "TEST", e.g. `LIBNAME_TEST_`.
 * 3. Every library implementing one or more suite of tests must have a new
 *    menuconfig housing all test suite options under the name `LIBNAME_TEST`.
 *    This menuconfig option must invoke all the suites if `LIBUKTEST_ALL` is
 *    set to true.
 *
 *
 * ## Registering tests
 *
 * `uktest`'s registation model allows for the execution of tests at different
 * levels of the boot process.  All tests occur before the invocation of the
 * application's `main` method.  This is done such that the validity of the
 * kernel-space functions can be legitimised before actual application code is
 * invoked.  A fail-fast option is provided in order to crash the kernel in case
 * of failures for earlier error diagnosis.
 *
 * When registering a test suite, one can hook into either the constructor
 * "`ctor`" table or initialisation table "`inittab`". This allows for running
 * tests before or after certain libraries or sub-systems are invoked during the
 * boot process.
 *
 * The following registation methods are available:
 *
 *  `UK_TESTSUITE_AT_CTORCALL_PRIO`,
 *  `uk_testsuite_early_prio`,
 *  `uk_testsuite_plat_prio`,
 *  `uk_testsuite_lib_prio`,
 *  `uk_testsuite_rootfs_prio`,
 *  `uk_testsuite_sys_prio`,
 *  `uk_testsuite_late_prio`,
 *  `uk_testsuite_prio` and,
 *  `uk_testsuite_register`.
 */

#include <uk/essentials.h>
#include <uk/assert.h>
#include <uk/config.h>
#include <uk/init.h>
#include <uk/print.h>
#include <uk/ctors.h>
#include <uk/plat/console.h>

#include <stdio.h>
#include <string.h>

#define UK_TEST_ASSERT_NOT_RUN	0
#define UK_TEST_ASSERT_SUCCESS	1
#define UK_TEST_ASSERT_FAIL	2


#define UKT_PADDING		"............................................"\
				"............................................"
#if CONFIG_LIBUKDEBUG_ANSI_COLOR
#define UKT_COLWIDTH		80
#define UKT_CLR_RESET		UK_ANSI_MOD_RESET
#define UKT_CLR_PASSED		UK_ANSI_MOD_BOLD			\
				UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_WHITE)\
				UK_ANSI_MOD_COLORBG(UK_ANSI_COLOR_GREEN)
#define UKT_CLR_FAILED		UK_ANSI_MOD_BOLD			\
				UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_WHITE)\
				UK_ANSI_MOD_COLORBG(UK_ANSI_COLOR_RED)
#define UKT_CLR_FILE		UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_CYAN)
#define UKT_CLR_LINE		UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_MAGENTA)
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
#define uk_test_printf(fmt, ...)					\
	_uk_printk(KLVL_INFO, UKLIBID_NONE, __NULL, 0x0,		\
	"\t" fmt, ##__VA_ARGS__)


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
 * struct uk_assert - A reference to an individual assertion.
 */
struct uk_assert {
	/* The status of the assertion (one of UK_TEST_ASSERT_*) */
	unsigned short status;
	/* The line of the assertion in the source code file */
	unsigned short line;
} __packed;


/**
 * References to the start and end of the assertion table.
 */
extern struct uk_assert uk_asserttab_start[];
extern struct uk_assert uk_asserttab_end[];


/**
 * struct uk_testcase - An individual test case.
 */
struct uk_testcase {
	/* The name of the test case. */
	const char *name;
	/* An optional short description. */
	const char *desc;
	/* Pointer to the test method. */
	void (*func)(struct uk_testcase *esac __maybe_unused);
	/* Pointer into the assertion table. */
	struct uk_assert *asserts;
	/* Name of the file where the test case exists. */
	const char *file;
} __packed;


/**
 * struct uk_testsuite - A series of test cases.
 */
struct uk_testsuite {
	/* The name of the test suite. */
	const char *name;
	/* An optional initialization method for the suite. */
	int (*init)(struct uk_testsuite *suite __maybe_unused);
	/* Cases are stored directly after the suite until this end marker. */
	struct uk_testcase *cases_end;
	/* The number of failed cases in this suite. */
	unsigned int failed_cases;
} __packed;


/**
 * Push a new test section label.
 *
 * @param section
 *   The name of the section.
 * @param label
 *   The label for the section.
 */
#define __UK_TEST_SECTION_LABEL(section, label)				\
	__asm__ (							\
		".pushsection \"" section "\","				\
		STRINGIFY(__LINE__) ", \"a\"\n"				\
		#label ":\n"						\
		".popsection\n"						\
	)

#define _UK_TEST_SECTION_LABEL(section, label)				\
	__UK_TEST_SECTION_LABEL(section, label)

/**
 * Append data to a section label.
 *
 * @param section
 *   The name of the section.
 * @param data
 *   The data for the section.
 */
#define __UK_TEST_SECTION_DATA(section, data)				\
	__asm__ (							\
		".pushsection \"" section "\","				\
		STRINGIFY(__LINE__) ", \"a\"\n"				\
		data "\n"						\
		".popsection\n"						\
	)

#define _UK_TEST_SECTION_DATA(section, data)				\
	__UK_TEST_SECTION_DATA(section, data)

/**
 * Macro to create a new entry in the assertion table.
 *
 * @param status
 *   The initial status of the assertion.
 */
#define _UK_TEST_ASSERTTAB_ENTRY(status)				\
	_UK_TEST_SECTION_DATA(						\
		".uk_asserttab",					\
		".hword " STRINGIFY(status) "\n"			\
		".hword " STRINGIFY(__LINE__)				\
	)

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
	__used __section(".uk_testtab_" #suite "~") __align(1)		\
	_UK_TESTSUITE_NAME(suite) = {					\
		.name = #suite,						\
		.init = initfn,						\
		.cases_end = _UK_TESTSUITE_RUN_NAME_END(suite),		\
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
	void _UK_TESTCASE_NAME(suite, fn)(				\
		struct uk_testcase *esac __maybe_unused);		\
	_UK_TEST_SECTION_LABEL(						\
		".uk_asserttab",					\
		_UK_TESTCASE_ASSERTTAB_START(suite, fn)			\
	);								\
	_UK_TEST_ASSERTTAB_ENTRY(0xffff);				\
	extern struct uk_assert _UK_TESTCASE_ASSERTTAB_START(suite, fn)[];\
	struct uk_testcase						\
	__used __section(".uk_testtab_" #suite "~" #fn) __align(1)	\
	_UK_TESTCASE_LABEL(suite, fn) = {				\
		.name = #fn,						\
		.desc = dsc,						\
		.func = _UK_TESTCASE_NAME(suite, fn),			\
		.asserts = _UK_TESTCASE_ASSERTTAB_START(suite, fn),	\
		.file = __FILE__					\
	};								\
	void _UK_TESTCASE_NAME(suite, fn)(				\
		struct uk_testcase *esac __maybe_unused)


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
	((struct uk_testcase *)((suite) + 1))


/**
 * Helper macro which iterates each case in a test suite.
 *
 * @param suite
 *   A statically initialized `struct uk_testsuite`.
 * @param esac
 *   A reference pointer to a case which will be available on each iteration.
 */
#define uk_testsuite_case_foreach(suite, esac)				\
	for ((esac) = _UK_TESTCASE_START(suite);			\
	     (esac) < ((suite)->cases_end);				\
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
 * Add a test suite to inittab at a specific class and priority level.
 *
 * @param suite
 *   A reference to a `struct uk_testsuite`.
 * @param initfn
 *   The initialization function for the suite.
 * @param class
 *   The class at which this suite should be inserted within the inittab.
 * @param prio
 *   The priority of this test suite.
 */
#define UK_TESTSUITE_AT_INITCALL_PRIO(suite, initfn, class, prio)	\
	_UK_TESTSUITE(suite, initfn);					\
	static int _UK_TESTSUITE_RUN_NAME(suite)(struct uk_init_ctx	\
						      *__ictx __unused)	\
	{								\
		return uk_testsuite_run(&_UK_TESTSUITE_NAME(suite));	\
	}								\
	uk_initcall_class_prio(_UK_TESTSUITE_RUN_NAME(suite), 0x0, class, prio)


/**
 * Add a test suite to be run in the "early" stage of the inittab.
 *
 * @param suite
 *   The pointer to the suite to add to this stage.
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
 *   The pointer to the suite to add to this stage.
 * @param initfn
 *   The initialization function for the suite.
 * @param prio
 *   The priority of this suite at this stage.
 */
#define uk_testsuite_plat_prio(suite, initfn, prio)			\
	UK_TESTSUITE_AT_INITCALL_PRIO(suite, initfn, UK_INIT_CLASS_PLAT, prio)


/**
 * Add a test suite to be run in the "lib" stage of the inittab.
 *
 * @param suite
 *   The pointer to the suite to add to this stage.
 * @param initfn
 *   The initialization function for the suite.
 * @param prio
 *   The priority of this suite at this stage.
 */
#define uk_testsuite_lib_prio(suite, initfn, prio)			\
	UK_TESTSUITE_AT_INITCALL_PRIO(suite, initfn, UK_INIT_CLASS_LIB, prio)


/**
 * Add a test suite to be run in the "rootfs" stage of the inittab.
 *
 * @param suite
 *   The pointer to the suite to add to this stage.
 * @param initfn
 *   The initialization function for the suite.
 * @param prio
 *   The priority of this suite at this stage.
 */
#define uk_testsuite_rootfs_prio(suite, initfn, prio)			\
	UK_TESTSUITE_AT_INITCALL_PRIO(suite, initfn, UK_INIT_CLASS_ROOTFS, prio)


/**
 * Add a test suite to be run in the "sys" stage of the inittab.
 *
 * @param suite
 *   The pointer to the suite to add to this stage.
 * @param initfn
 *   The initialization function for the suite.
 * @param prio
 *   The priority of this suite at this stage.
 */
#define uk_testsuite_sys_prio(suite, initfn, prio)			\
	UK_TESTSUITE_AT_INITCALL_PRIO(suite, initfn, UK_INIT_CLASS_SYS, prio)


/**
 * Add a test suite to be run in the "late" stage of the inittab.
 *
 * @param suite
 *   The pointer to the suite to add to this stage.
 * @param initfn
 *   The initialization function for the suite.
 * @param prio
 *   The priority of this suite at this stage.
 */
#define uk_testsuite_late_prio(suite, initfn, prio)			\
	UK_TESTSUITE_AT_INITCALL_PRIO(suite, initfn, UK_INIT_CLASS_LATE, prio)


/**
 * The default registration for a test suite with a desired priority level.
 *
 * @param suite
 *   The pointer to the suite to add.
 * @param initfn
 *   The initialization function for the suite.
 * @param prio
 *   The priority of this suite.
 */
#define uk_testsuite_prio(suite, initfn, prio)				\
	uk_testsuite_late_prio(suite, initfn, prio)


/**
 * The default registration for a test suite.
 *
 * @param suite
 *   The pointer to the suite to add.
 * @param initfn
 *   The initialization function for the suite.
 */
#define uk_testsuite_register(suite, initfn)				\
	uk_testsuite_prio(suite, initfn, UK_PRIO_LATEST)


/**
 * Find an assertion based on a line number.
 *
 * @param esac
 *   The case with list of assertions to iterate over.
 * @param line
 *   The line of the assertion to find.
 */
static struct uk_assert*
_uk_test_find_assert(struct uk_testcase *esac, unsigned short line)
{
	struct uk_assert *assert;

	UK_ASSERT(esac);

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
static void __maybe_unused
_uk_test_do_assert(struct uk_testcase *esac, unsigned short line, int cond,
		   const char *fmt, ...)
{
	struct uk_assert *assert;
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
	UK_ASSERT(pad_len >= 0);

	uk_test_printf("%s %*.*s %s\n",
		       msg,
		       pad_len, pad_len,
		       UKT_PADDING,
		       cond ? UKT_PASSED : UKT_FAILED);
#endif
	assert = _uk_test_find_assert(esac, line);

	UK_ASSERT(assert);

	if (!cond) {
		assert->status = UK_TEST_ASSERT_FAIL;
#ifdef CONFIG_LIBUKTEST_LOG_TESTS
		/* Output the location where the expectation failed. */
		uk_test_printf("  in "
			       UKT_CLR_FILE __STR_BASENAME__ UKT_CLR_RESET
			       ":" UKT_CLR_FILE "%d" UKT_CLR_RESET "\n", line);
#endif /* CONFIG_LIBUKTEST_LOG_TESTS */
#if CONFIG_LIBUKTEST_FAILFAST
		UK_CRASH("Crashing on first failure!");
#endif
	} else if (assert->status != UK_TEST_ASSERT_FAIL) {
		/* If the test failed once, we do not reset it to success */
		assert->status = UK_TEST_ASSERT_SUCCESS;
	}
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
		(cond),							\
		"expected `%s` to be true",				\
		STRINGIFY(cond)						\
	)

#define UK_TEST_ASSERT(cond) UK_TEST_EXPECT(cond)


/**
 * Helper macro to compare expression a with expression b of same type.
 *
 * @param a
 *   The left-hand operand.
 * @param cond
 *   The comparison operator (e.g., ==).
 * @param b
 *   The right-hand operand.
 * @param desc
 *   A textual description of the comparison.
 * @param type
 *   The type of the evaluated expression.
 * @param fmt
 *   A printf format specifier to print the expression values.
 */
#define _UK_TEST_EXPECT_A_COND_B(a, cond, b, desc, type, fmt)		\
	do {								\
		type a_v = (type)(a);					\
		type b_v = (type)(b);					\
		int _cond = (a_v cond b_v);				\
		UK_TEST_ASSERTF(					\
			_cond,						\
			"expected `%s` to " desc			\
			" " fmt " %s was " fmt,				\
			STRINGIFY(a), b_v,				\
			_cond ? "and" : "but", a_v			\
		);							\
	} while (0)


/**
 * Expect an expression to be NULL.
 *
 * @param exp
 *   The expression under consideration.
 */
#define UK_TEST_EXPECT_NULL(exp)					\
	_UK_TEST_EXPECT_A_COND_B(exp, ==, NULL, "be", void *, "%p")


/**
 * Expect an expression to not be NULL.
 *
 * @param exp
 *   The expression under consideration.
 */
#define UK_TEST_EXPECT_NOT_NULL(exp)					\
	_UK_TEST_EXPECT_A_COND_B(exp, !=, NULL, "not be", void *, "%p")


/**
 * Expect an expression to evaluate to zero.
 *
 * @param exp
 *   The expression under consideration.
 */
#define UK_TEST_EXPECT_ZERO(exp)					\
	_UK_TEST_EXPECT_A_COND_B(exp, ==, 0, "be", long, "%ld")


/**
 * Expect an expression to not evaluate to zero.
 *
 * @param exp
 *   The expression under consideration.
 */
#define UK_TEST_EXPECT_NOT_ZERO(exp)					\
	_UK_TEST_EXPECT_A_COND_B(exp, !=, 0, "not be", long, "%ld")


/**
 * Expect two pointers to be equal to each other.
 *
 * @param a
 *   The left-hand operand.
 * @param b
 *   The right-hand operand.
 */
#define UK_TEST_EXPECT_PTR_EQ(a, b)					\
	_UK_TEST_EXPECT_A_COND_B(a, ==, b, "be", void *, "%p")


/**
 * Expect the contents of two buffers to be equal.
 *
 * @param a
 *   The left-hand operand.
 * @param b
 *   The right-hand operand.
 */
#define UK_TEST_EXPECT_BYTES_EQ(a, b, size)				\
	do {								\
		void *a_p = (void *)(a);				\
		void *b_p = (void *)(b);				\
		UK_TEST_ASSERTF(					\
			memcmp(a_p, b_p, size) == 0,			\
			"expected `%s` at %p "				\
			"to equal `%s` at %p",				\
			STRINGIFY(a), b_p, STRINGIFY(b), a_p		\
		);							\
	} while (0)


/**
 * Expect two long integers to be equal to each other.
 *
 * @param a
 *   The left-hand operand.
 * @param b
 *   The right-hand operand.
 */
#define UK_TEST_EXPECT_SNUM_EQ(a, b)					\
	_UK_TEST_EXPECT_A_COND_B(a, ==, b, "be", long, "%ld")


/**
 * Expect two long integers to not be equal to each other.
 *
 * @param a
 *   The left-hand operand.
 * @param b
 *   The right-hand operand.
 */
#define UK_TEST_EXPECT_SNUM_NQ(a, b)					\
	_UK_TEST_EXPECT_A_COND_B(a, !=, b, "not be", long, "%ld")


/**
 * Expect the left-hand long integer to be greater than the right.
 *
 * @param a
 *   The left-hand operand.
 * @param b
 *   The right-hand operand.
 */
#define UK_TEST_EXPECT_SNUM_GT(a, b)					\
	_UK_TEST_EXPECT_A_COND_B(a, >, b, "be greater than", long, "%ld")


/**
 * Expect the left-hand long integer to be greater or equal to the right.
 *
 * @param a
 *   The left-hand operand.
 * @param b
 *   The right-hand operand.
 */
#define UK_TEST_EXPECT_SNUM_GE(a, b)					\
	_UK_TEST_EXPECT_A_COND_B(a, >=, b, "be greater than or equal to",\
				 long, "%ld")


/**
 * Expect the left-hand long integer to be less than the right.
 *
 * @param a
 *   The left-hand operand.
 * @param b
 *   The right-hand operand.
 */
#define UK_TEST_EXPECT_SNUM_LT(a, b)					\
	_UK_TEST_EXPECT_A_COND_B(a, <, b, "be less than", long, "%ld")


/**
 * Expect the left-hand long integer to be less than or equal the right.
 *
 * @param a
 *   The left-hand operand.
 * @param b
 *   The right-hand operand.
 */
#define UK_TEST_EXPECT_SNUM_LE(a, b)					\
	_UK_TEST_EXPECT_A_COND_B(a, <=, b, "be less than or equal to",	\
				 long, "%ld")


#endif /* __UK_TEST_H__ */
