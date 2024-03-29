/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/test.h>
#include <uk/isr/string.h>

/*  FIXME: There is no type checking inside memset_isr
 *  the following two comments cause crash for accessing nullptr
 */

/* char *ptr = NULL; */
/* memset_isr(ptr, 'A', 1); */

UK_TESTCASE(isrlib_testsuite, test_memset_isr)
{
	char array_1[5];

	memset_isr(array_1, 'U', 5);
	UK_TEST_EXPECT_BYTES_EQ(array_1, "UUUUU", 5);

	memset_isr(array_1, 0, 5);
	UK_TEST_EXPECT_BYTES_EQ(array_1, "\0\0\0\0\0", 5);

	char array_2[5] = "01234";

	memset_isr(array_2, 'U', 3);
	UK_TEST_EXPECT_BYTES_EQ(array_2, "UUU34", 5);

	memset_isr(array_2, 'A', 0);
	UK_TEST_EXPECT_BYTES_EQ(array_2, "UUU34", 5);

	char array_3[30];
	char result_3[30] = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";

	memset_isr(array_3, 'A', 30);
	UK_TEST_EXPECT_BYTES_EQ(array_3, result_3, 30);
}

/*  FIXME: There is no type checking inside memcpy_isr
 *  the following two comments cause crash for accessing nullptr
 */

/* memcpy_isr(NULL, src_1, 14); */
/* memcpy_isr(result_2, NULL, 14); */

UK_TESTCASE(isrlib_testsuite, test_memcpy_isr)
{
	char src_0[10] = "0123456789";
	char dest_0[10];

	memcpy_isr(dest_0, src_0, 10);
	UK_TEST_EXPECT_BYTES_EQ(src_0, dest_0, 10);

	char src_1[14] = "Hello, World!";
	char dest_1[14];
	char result_1[5] = "Hello";

	memcpy_isr(dest_1, src_1, 5);
	UK_TEST_EXPECT_BYTES_EQ(result_1, dest_1, 5);

	char result_2[15] = "HelloHellold!";

	memcpy_isr(src_1 + 5, src_1, 5);
	UK_TEST_EXPECT_BYTES_EQ(src_1, result_2, 14);

	memcpy_isr(result_2, src_1, 0);
	UK_TEST_EXPECT_BYTES_EQ(src_1, result_2, 14);

	char src_2[1024] = {'A'};
	char dest_2[1024];

	memcpy_isr(dest_2, src_2, 1024);
	UK_TEST_EXPECT_BYTES_EQ(src_2, dest_2, 1024);
}

/*  FIXME: There is no type checking inside strlen_isr
 *  the following comment causes crash for accessing nullptr
 */

/* ret = strnlen_isr(NULL, 1); */

UK_TESTCASE(isrlib_testsuite, test_strnlen_isr)
{
	char str[] = "\0";
	size_t len = 1;

	size_t ret = strnlen_isr(str, len);

	UK_TEST_EXPECT_SNUM_EQ(ret, 0);

	char str1[] = "Unikraft ";

	len = 10;

	ret = strnlen_isr(str1, len);

	UK_TEST_EXPECT_SNUM_EQ(ret, 9);

	len = 5;
	ret = strnlen_isr(str1, len);

	UK_TEST_EXPECT_SNUM_EQ(ret, 5);
}

/*
 *  FIXME: There is no type checking inside strlen_isr
 *	the following comment causes crash for accessing nullptr
 */

/* ret = strlen_isr(NULL); */

UK_TESTCASE(isrlib_testsuite, test_strlen_isr)
{
	char str[] = "\0";
	size_t ret = strlen_isr(str);

	UK_TEST_EXPECT_SNUM_EQ(ret, 0);

	char str1[] = "Unikraft is evolution";

	ret = strlen_isr(str1);

	UK_TEST_EXPECT_SNUM_EQ(ret, 21);

	char str2[] = "\n\n";

	ret = strlen_isr(str2);

	UK_TEST_EXPECT_SNUM_EQ(ret, 2);
}

/*
 *  FIXME: There is no type checking inside strcmp_isr
 *  the following comment causes crash for accessing nullptr
 */

/* ret = strcmp_isr(NULL, NULL); */

UK_TESTCASE(isrlib_testsuite, test_strcmp_isr)
{
	int ret = strcmp_isr("unikraft", "unikraft");

	UK_TEST_EXPECT_SNUM_EQ(ret, 0);

	ret = strcmp_isr("unikraft", "unikrafT");
	UK_TEST_EXPECT_SNUM_GT(ret, 0);

	ret = strcmp_isr("unikrafT", "unikraft");
	UK_TEST_EXPECT_SNUM_LT(ret, 0);

	ret = strcmp_isr("unikrafT", "Unikraft");
	UK_TEST_EXPECT_SNUM_EQ(ret, 1);

	ret = strcmp_isr("", "");
	UK_TEST_EXPECT_SNUM_EQ(ret, 0);
}

/*
 *  FIXME: There is no type checking inside strncmp_isr
 *	the following comment causes crash for accessing nullptr
 */

/* ret = strncmp_isr(NULL, NULL, 1); */

UK_TESTCASE(isrlib_testsuite, test_strncmp_isr)
{
	int ret = strncmp_isr("unikraft", "unikraft", 8);

	UK_TEST_EXPECT_SNUM_EQ(ret, 0);

	ret = strncmp_isr("unikraft", "unikraftfft", 8);
	UK_TEST_EXPECT_SNUM_EQ(ret, 0);

	ret = strncmp_isr("unikraft", "unikraftfft", 9);
	UK_TEST_EXPECT_SNUM_LT(ret, 0);

	ret = strncmp_isr("\0", "", 1);
	UK_TEST_EXPECT_SNUM_EQ(ret, 0);
}

/*
 *  FIXME: There is no type checking inside memrchr_isr
 *  the following comment causes crash for accessing nullptr
 */

/* memrchr_isr(NULL, '\0', 2); */

UK_TESTCASE(isrlib_testsuite, test_memrchr_isr)
{
	char array_0[12] = "Hello World";
	char *ptr = (char *)memrchr_isr(array_0, 'o', 11);

	UK_TEST_EXPECT_PTR_EQ(ptr, array_0 + 7);

	ptr = (char *)memrchr_isr(array_0, 'H', 11);
	UK_TEST_EXPECT_PTR_EQ(ptr, array_0);

	ptr = (char *)memrchr_isr(array_0, 'l', 11);

	UK_TEST_EXPECT_PTR_EQ(ptr, array_0 + 9);

	ptr = (char *)memrchr_isr(array_0, 'd', 11);

	UK_TEST_EXPECT_PTR_EQ(ptr, array_0 + 10);

	ptr = (char *)memrchr_isr(array_0, 'z', 11);

	UK_TEST_EXPECT_PTR_EQ(ptr, NULL);

	char array_1[10] = {'H', 'e', '\0', 'l', 'o', 'W', '\0', 'r', 'l', 'd'};

	ptr = (char *)memrchr_isr(array_1, '\0', 10);
	UK_TEST_EXPECT_PTR_EQ(ptr, array_1 + 6);

	ptr = (char *)memrchr_isr(array_1, '\0', 0);

	UK_TEST_EXPECT_PTR_EQ(ptr, NULL);

	char array_2[40] = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXYX";

	ptr = (char *)memrchr_isr(array_2, 'X', 40);

	UK_TEST_EXPECT_PTR_EQ(ptr, array_2 + 39);
}

/*
 *  FIXME: There is no type checking inside memmove_isr
 *  the following two comments cause crash for accessing nullptr
 */

/* memmove_isr(NULL, src_0, 2); */
/* memmove_isr(src_0, NULL, 2); */

UK_TESTCASE(isrlib_testsuite, test_memmove_isr)
{
	char src_0[11] = "0123456789";
	char expect_0[11] = "0123456789";

	memmove_isr(src_0, src_0, 10);
	UK_TEST_EXPECT_BYTES_EQ(expect_0, src_0, 10);

	char src_1[15] = "Hello, World!";
	char expect_1[15] = "World, World!";

	memmove_isr(src_1, src_1 + 7, 5);
	UK_TEST_EXPECT_BYTES_EQ(expect_1, src_1, 15);

	/* in case of forward overlapping the method */
	/*	 does not behave as expected */
	char src_2[15] = "Hello, World!";
	char expect_2[15] = "Hello, Wello,";

	memmove_isr(src_2 + 7, src_2, 5);
	UK_TEST_EXPECT_BYTES_EQ(expect_2, src_2, 15);

	/* here is the correct output i think */
	char expect_3[15] = "Hello, Hello!";

	UK_TEST_EXPECT_BYTES_EQ(expect_3, src_2, 15);
}

/*
 *  FIXME: There is no type checking inside memcmp_isr
 *	the following comment causes crash for accessing nullptr
 */

/* ret = memcmp_isr(NULL, NULL, 5); */

UK_TESTCASE(isrlib_testsuite, test_memcmp_isr)
{
	char arr1[5] = {'A', 'B', 'C', 'D', 'E'};
	char arr2[5] = {'A', 'B', 'C', 'D', 'E'};
	int ret = memcmp_isr(arr1, arr2, 5);

	UK_TEST_EXPECT_SNUM_EQ(ret, 0);

	char arr3[5] = {'A', 'B', 'D', 'D', 'E'};

	ret = memcmp_isr(arr1, arr3, 5);

	UK_TEST_EXPECT_SNUM_LT(ret, 0);

	ret = memcmp_isr(arr3, arr1, 5);

	UK_TEST_EXPECT_SNUM_GT(ret, 0);

	ret = memcmp_isr(arr3, arr1, 0);

	UK_TEST_EXPECT_SNUM_EQ(ret, 0);
}

UK_TESTCASE(isrlib_testsuite, test_strncpy_isr)
{
	char src_0[] = "Hello, World!";
	char dest_0[20];
	char expected_0[] = "Hello";

	strncpy_isr(dest_0, src_0, 5);
	UK_TEST_EXPECT_BYTES_EQ(expected_0, src_0, 5);

	char dest_1[20];

	strncpy_isr(dest_1, src_0, 13);
	UK_TEST_EXPECT_BYTES_EQ(dest_1, src_0, 13);
}

UK_TESTCASE(isrlib_testsuite, test_strcpy_isr)
{
	char src_0[] = "Hello, World!";
	char dest_0[20];

	strcpy_isr(dest_0, src_0);
	UK_TEST_EXPECT_BYTES_EQ(dest_0, src_0, 13);

	char src_1[] = "";
	char dest_1[5];

	strcpy_isr(dest_1, src_1);
	UK_TEST_EXPECT_BYTES_EQ(dest_1, src_1, 1);

	char src_2[] = "Hello\0World";
	char dest_2[7];

	strcpy_isr(dest_2, src_2);
	UK_TEST_EXPECT_BYTES_EQ(dest_2, src_2, 6);
}

/*
 *  FIXME: There is no type checking inside strchrnul_isr
 *  the following comment causes crash for accessing nullptr
 */

/* ptr = strchrnul_isr(NULL, ch); */

UK_TESTCASE(isrlib_testsuite, test_strchrnul_isr)
{
	char str_0[] = "hello world", ch = 'o';
	char *ptr = strchrnul_isr(str_0, ch);

	UK_TEST_EXPECT_PTR_EQ(ptr, str_0 + 4);

	ch = 'Z';
	ptr = strchrnul_isr(str_0, ch);
	UK_TEST_EXPECT_PTR_EQ(ptr, NULL);

	ch = 'h';
	ptr = strchrnul_isr(str_0, ch);
	UK_TEST_EXPECT_PTR_EQ(ptr, str_0);

	ch = 'd';
	ptr = strchrnul_isr(str_0, ch);
	UK_TEST_EXPECT_PTR_EQ(ptr, str_0 + 10);

	char str_1[] = "";

	ptr = strchrnul_isr(str_1, ch);
	UK_TEST_EXPECT_PTR_EQ(ptr, NULL);

	ch = '\0';
	ptr = strchrnul_isr(str_0, ch);
	UK_TEST_EXPECT_PTR_EQ(ptr, str_0 + 11);

	ch = 'w';
	char str_2[] = "hello\0world";

	ptr = strchrnul_isr(str_2, ch);

	UK_TEST_EXPECT_PTR_EQ(ptr, NULL);
}

/*
 *  FIXME: There is no type checking inside strchr_isr
 *  the following comment causes crash for accessing nullptr
 */

/* ptr = strchr_isr(NULL, ch); */

UK_TESTCASE(isrlib_testsuite, test_strchr_isr)
{
	char str_0[] = "hello world", ch = 'o';

	char *ptr = strchr_isr(str_0, ch);

	UK_TEST_EXPECT_PTR_EQ(ptr, str_0 + 4);

	ch = 'X';

	ptr = strchr_isr(str_0, ch);
	UK_TEST_EXPECT_PTR_EQ(ptr, NULL);

	ch = 'h';

	ptr = strchr_isr(str_0, ch);
	UK_TEST_EXPECT_PTR_EQ(ptr, str_0);

	ch = 'd';

	ptr = strchr_isr(str_0, ch);
	UK_TEST_EXPECT_PTR_EQ(ptr, str_0 + 10);

	char str_1[] = "";

	ptr = strchr_isr(str_1, ch);
	UK_TEST_EXPECT_PTR_EQ(ptr, NULL);

	ch = '\0';
	ptr = strchr_isr(str_0, ch);
	UK_TEST_EXPECT_PTR_EQ(ptr, str_0 + 11);

	ch = 'w';
	char str_2[] = "hello\0world";

	ptr = strchr_isr(str_2, ch);

	UK_TEST_EXPECT_PTR_EQ(ptr, NULL);
}

/*
 *  FIXME: There is no type checking inside strrchr_isr
 *  the following comment causes crash for accessing nullptr
 */

/* ptr = strrchr_isr(NULL, ch); */

UK_TESTCASE(isrlib_testsuite, test_strrchr_isr)
{
	char str_0[] = "hello world", ch = 'o';
	char *ptr = strrchr_isr(str_0, ch);

	UK_TEST_EXPECT_PTR_EQ(ptr, str_0 + 7);

	ch = 'x';
	ptr = strrchr_isr(str_0, ch);
	UK_TEST_EXPECT_PTR_EQ(ptr, NULL);

	ptr = strrchr_isr("", ch);
	UK_TEST_EXPECT_PTR_EQ(ptr, NULL);

	ch = 'l';
	ptr = strrchr_isr(str_0, ch);
	UK_TEST_EXPECT_PTR_EQ(ptr, str_0 + 9);

	ch = 'h';
	ptr = strrchr_isr(str_0, ch);
	UK_TEST_EXPECT_PTR_EQ(ptr, str_0);

	ch = 'd';
	ptr = strrchr_isr(str_0, ch);
	UK_TEST_EXPECT_PTR_EQ(ptr, str_0 + 10);
}

/*
 *  FIXME: There is no type checking inside strcspn_isr
 *  the following comment causes crash for accessing nullptr
 */

/* ret = strcspn_isr(NULL, NULL); */

UK_TESTCASE(isrlib_testsuite, test_strcspn_isr)
{
	char str_0[] = "abcdef";
	char str_1[] = "xyz";
	size_t ret = strcspn_isr(str_0, str_1);

	UK_TEST_EXPECT_SNUM_EQ(ret, 6);

	char str_2[] = "az";

	ret = strcspn_isr(str_0, str_2);
	UK_TEST_EXPECT_SNUM_EQ(ret, 0);

	ret = strcspn_isr(str_0, "");
	UK_TEST_EXPECT_SNUM_EQ(ret, 6);

	ret = strcspn_isr("", str_0);
	UK_TEST_EXPECT_SNUM_EQ(ret, 0);

	ret = strcspn_isr("abcdef\0xyz", "\0");
	UK_TEST_EXPECT_SNUM_EQ(ret, 6);

	ret = strcspn_isr("", "");
	UK_TEST_EXPECT_SNUM_EQ(ret, 0);
}

/*
 *  FIXME: There is no type checking inside strspn_isr
 *  the following comment causes crash for accessing nullptr
 */

/* ret = strspn_isr(NULL, NULL); */

UK_TESTCASE(isrlib_testsuite, test_strspn_isr)
{
	size_t ret = strspn_isr("abcde", "abc");

	UK_TEST_EXPECT_SNUM_EQ(ret, 3);

	ret = strspn_isr("abcdef", "xyz");
	UK_TEST_EXPECT_SNUM_EQ(ret, 0);

	ret = strspn_isr("aaa", "a");
	UK_TEST_EXPECT_SNUM_EQ(ret, 3);

	ret = strspn_isr("abcdef", "");
	UK_TEST_EXPECT_SNUM_EQ(ret, 0);

	ret = strspn_isr("aaabbbccc", "abc");
	UK_TEST_EXPECT_SNUM_EQ(ret, 9);

	ret = strspn_isr("", "abcdef");
	UK_TEST_EXPECT_SNUM_EQ(ret, 0);

	ret = strspn_isr("abc", "abc");
	UK_TEST_EXPECT_SNUM_EQ(ret, 3);

	ret = strspn_isr("xyzabc", "abcxyz");
	UK_TEST_EXPECT_SNUM_EQ(ret, 6);
}

/*
 *  FIXME: There is no type checking inside strlcpy_isr
 *  the following comment causes crash for accessing nullptr
 */

/* ret = strlcpy_isr(NULL, NULL, 5); */

UK_TESTCASE(isrlib_testsuite, test_strlcpy_isr)
{
	char dist_0[14];
	size_t ret = strlcpy_isr(dist_0, "Hello, world!", 13);

	UK_TEST_EXPECT_SNUM_EQ(ret, 13);

	char dist_1[10];

	ret = strlcpy_isr(dist_1, "Short", 5);
	UK_TEST_EXPECT_SNUM_EQ(ret, 5);

	char dist_2[5];

	ret = strlcpy_isr(dist_2, "This is too long", 16);
	UK_TEST_EXPECT_SNUM_EQ(ret, 16);

	char dist_3[1];

	ret = strlcpy_isr(dist_3, "Non-empty", 9);
	UK_TEST_EXPECT_SNUM_EQ(ret, 9);

	char dist_4[0];

	ret = strlcpy_isr(dist_4, "Hello, world!", 13);

	UK_TEST_EXPECT_SNUM_EQ(ret, 13);

	char dist_5[12];

	ret = strlcpy_isr(dist_5, "Exact match", 11);
	UK_TEST_EXPECT_SNUM_EQ(ret, 11);

	char dist_6[5];
	char src[] = "";

	ret = strlcpy_isr(dist_6, src, 5);
	UK_TEST_EXPECT_SNUM_EQ(ret, 0);

	ret = strlcpy_isr(dist_6, "", 5);
	UK_TEST_EXPECT_SNUM_EQ(ret, 0);
}

/*
 *  FIXME: There is no type checking inside strlcat_isr
 *  the following comment causes crash for accessing nullptr
 */

/* ret = strlcat_isr(NULL, NULL, 14); */

UK_TESTCASE(isrlib_testsuite, test_strlcat_isr)
{
	size_t ret = strlcat_isr("Hello, ", "world!", 14);

	UK_TEST_EXPECT_SNUM_EQ(ret, 13);

	ret = strlcat_isr("Hello", ", world!", 13);
	UK_TEST_EXPECT_SNUM_EQ(ret, 13);

	ret = strlcat_isr("Hello, world!", "", 20);
	UK_TEST_EXPECT_SNUM_EQ(ret, 13);

	char dist[] = "Hello";
	char src[] = ", world!";

	ret = strlcat_isr(dist, src, 10);
	UK_TEST_EXPECT_SNUM_EQ(ret, 13);

	/* take a look at this case and the above they */
	/*   are identical with different results */

	ret = strlcat_isr("Hello", ", world!", 10);
	UK_TEST_EXPECT_SNUM_EQ(ret, 13);

	ret = strlcat_isr("Buffer", " Overflow", 0);
	UK_TEST_EXPECT_SNUM_EQ(ret, 6);

	ret = strlcat_isr("", "New string", 15);
	UK_TEST_EXPECT_SNUM_EQ(ret, 10);

	ret = strlcat_isr("Part", "ial", 7);
	UK_TEST_EXPECT_SNUM_EQ(ret, 7);
}

uk_testsuite_register(isrlib_testsuite, NULL);
