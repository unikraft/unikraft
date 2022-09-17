# uktest: Unikraft’s Testing Framework

`uktest` is Unikraft's internal testing framework.
It provides a flexible API for performing unit testing of internal libraries, methods and more in order to safeguard changes which affect existing functionality.

## Overview

In `uktest`, tests are organized hierarchically powered by the lowest common denominator: the assertion.
This organization is inspired by [KUnit](https://kunit.dev/), the Linux Kernel's in-house testing system.
For licensing reasons, the Unikraft project cannot use this source code.
However, by inspiration, we can organize the `uktest` library following a similar pattern:

1. The assertion: it represents the lowest common denominator of a test: some boolean operation which, when true, a single test passes.

1. The test case: it represents a group of assertions in which one particular function or some system is undergoing testing.

1. The test suite: represents a group of test cases.

### Assertions

Assertions are often used in-line and their usage should be no different from the traditional use of the `ASSERT` macro.
In `uktest`, we introduce a new definition: `UK_TEST_EXPECT` which has one parameter: the same boolean operation which is true-to-form of the traditional `ASSERT` macro.
With `uktest`, however, the macro is intelligently placed in context within a case (see the next point).
Additional text or descriptive explanation of the text can also be provided with the auxiliary and similar macro `UK_TEST_ASSERTF`.

### Test Cases

The test case: often, assertions are not alone in their means to check the legitimacy of operation in some function.
We find that a "case" is the best way to organize a group of assertions in which one particular function of some system is undergoing testing.
A case is independent of other cases but related in the same sub-system.
For this reason, we register them together in the same test suite.

### Test Suites

The test suite represents a group of test cases.
This is the final and upper-most hierarchical representation of tests and their groupings.
With assertions grouped into test cases and test cases grouped into a test suite, we end this organization in a fashion which allows us to follow a common design pattern within Unikraft: the registration model.
The syntax is similar to other registration models within Unikraft, e.g. `ukbus`.
However, `uktest`'s registration model is more powerful.

## Creating Tests

To register a test suite with `uktest`, we simply invoke `uk_testsuite_register` with a unique symbol name.
This symbol is used along with test cases in order to create references to one another.
Each test case has two input parameters: a reference to the suite is part, as well as a canonical name for the case itself.
Generally, the following pattern is used for test suites:

```
$LIBNAME_$TESTSUITENAME_testsuite
```

And the following for test cases:

```
$LIBNAME_test_$TESTCASENAME
```

To create a case, invoke the `UK_TESTCASE` macro with the two parameters describe previously, and use it in the context of a function, for example:

```c
UK_TESTCASE(uktest_mycase_testsuite, uktest_test_case)
{
        int x = 1;
        UK_TEST_EXPECT(x > 0);
}
```

Finally, to register the case with a suite (see next section), call one of the possible registration functions:

```c
uk_testsuite_register(uktest_mycase_testsuite, NULL);
```

The above snippet can be organized within a library in a number of ways, such as in-line or as an individual file representing the suite.
There are a number of test suite registration handles which are elaborated on in the next section.
It should be noted that multiple test suites can exist within a library in order to test multiple features or components of said library.

In order to achieve consistency in the use of `uktest` across the Unikraft code base, the following recommendation is made regarding the registration of test suites:

1. A single test suite should be organized into its own file, prefixed with `test_` (e.g. `test_feature.c`).
   All test suites of some libraries within Unikraft should be stored within a new folder located at the root of the library named `tests/`.

1. All test suites should have a corresponding KConfig option, prefixed with the library name and then the word "TEST" (e.g. `LIBNAME_TEST_`).

1. Every library implementing one or more suites of tests must have a new menuconfig housing all test suite options under the name `LIBNAME_TEST`.
   This menuconfig option must invoke all the suites if `LIBUKTEST_ALL` is set to true.

## Key Functions and Data Structures

The key structure used is `uk_testcase` defined as:

```c
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
};
```

## Registering Tests

`uktest`'s registration model allows for the execution of tests at different levels of the boot process.
All tests occur before the invocation of the application's `main` method.
This is done such that the validity of the kernel-space functions can be legitimized before the actual application code is invoked.
A fail-fast option is provided in order to crash the kernel in case of failures for earlier error diagnosis.

When registering a test suite, one can hook into either the constructor "`ctor`" table or the initialization table "`inittab`".
This allows for running tests before or after certain libraries or sub-systems are invoked during the boot process.

The following registration methods are available:

```text
UK_TESTSUITE_AT_CTORCALL_PRIO,
uk_testsuite_early_prio,
uk_testsuite_plat_prio,
uk_testsuite_lib_prio,
uk_testsuite_rootfs_prio,
uk_testsuite_sys_prio,
uk_testsuite_late_prio,
uk_testsuite_prio and,
uk_testsuite_register.
```

You can learn more about the Unikraft boot sequence in the [`booting` documentation section](https://unikraft.org/docs/develop/booting/).
