/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/test.h>
#include <uk/fallocbuddy.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define fab_bug_on(cond)						\
	do {								\
		if (unlikely(cond))					\
			UK_CRASH("'%s' during test execution.\n",	\
				 STRINGIFY(cond));			\
	} while (0)

struct test_ukfallocbuddy {
	struct uk_falloc *fa;
	__paddr_t start;
	__sz len;
	unsigned long pages;
	__sz fa_meta_size;
	void *start_zone;
	void *fa_meta;
};

static struct uk_falloc
*init_new_fb_alloc(void *fa_meta, void *start_zone, unsigned long len,
		__paddr_t *new_start, __sz fa_meta_size, unsigned long *pages)
{
	__paddr_t start = (__paddr_t)start_zone;
	struct uk_falloc *fa;
	int rc;

	fa = malloc(uk_fallocbuddy_size());
	fab_bug_on(!fa);

	rc = uk_fallocbuddy_init(fa);
	fab_bug_on(rc != 0);

	start = PAGE_ALIGN_UP(start + fa_meta_size);
	*pages = (len - fa_meta_size) >> PAGE_SHIFT;

	rc = fa->addmem(fa, fa_meta, start, *pages, start);
	fab_bug_on(rc != 0);

	*new_start = start;
	return fa;
}

static struct test_ukfallocbuddy *new_allocator(__sz len)
{
	struct test_ukfallocbuddy *attr;

	attr = (struct test_ukfallocbuddy *)malloc(sizeof(*attr));
	fab_bug_on(!attr);

	attr->len = len;
	attr->pages = len >> PAGE_SHIFT;
	attr->start_zone = malloc(len);
	fab_bug_on(!attr->start_zone);

	attr->fa_meta_size = uk_fallocbuddy_metadata_size(attr->pages);
	attr->fa_meta = malloc(attr->fa_meta_size);
	fab_bug_on(!attr->fa_meta);

	attr->fa = init_new_fb_alloc(attr->fa_meta, attr->start_zone,
					attr->len, &attr->start,
					attr->fa_meta_size, &attr->pages);
	fab_bug_on(!attr->fa);

	return attr;
}

static void free_allocator(struct test_ukfallocbuddy *attr)
{
	free(attr->start_zone);
	free(attr->fa_meta);
	free(attr->fa);
	free(attr);
}

/**
 * Tests if the init function works
 */
UK_TESTCASE(ukfallocbuddy, test_init)
{
	struct uk_falloc *fa = NULL;

	fa = malloc(uk_fallocbuddy_size());
	UK_TEST_ASSERT(fa != NULL);
	UK_TEST_EXPECT_SNUM_EQ(uk_fallocbuddy_init(fa), 0);

	free(fa);
}

/**
 * Tests if the addmem functionality works
 */
UK_TESTCASE(ukfallocbuddy, test_add_memory)
{
	__sz len;
	unsigned long pages;
	void *start_zone;
	__sz fa_meta_size;
	void *fa_meta;
	struct uk_falloc *fa;
	__paddr_t start;
	int rc;

	len = 10 * 1024 * 1024; /* 10 MiB */
	pages = len >> PAGE_SHIFT;
	start_zone = malloc(len);
	fab_bug_on(!start_zone);

	fa_meta_size = uk_fallocbuddy_metadata_size(pages);
	fa_meta = malloc(fa_meta_size);
	fab_bug_on(!fa_meta);

	fa = malloc(uk_fallocbuddy_size());
	fab_bug_on(!fa);

	rc = uk_fallocbuddy_init(fa);
	fab_bug_on(rc != 0);

	start = PAGE_ALIGN_UP((__paddr_t)start_zone + fa_meta_size);
	pages = (len - fa_meta_size) >> PAGE_SHIFT;

	rc = fa->addmem(fa, fa_meta, start, pages, start);
	UK_TEST_EXPECT_ZERO(rc);

	free(start_zone);
	free(fa_meta);
	free(fa);
}

/**
 * Tests variables have correct values
 */
UK_TESTCASE(ukfallocbuddy, test_check_variables)
{
	struct test_ukfallocbuddy *test = new_allocator(10000);

	UK_TEST_EXPECT_SNUM_EQ(test->fa->free_memory, 8192);
	UK_TEST_EXPECT_SNUM_EQ(test->fa->total_memory, 8192);

	free_allocator(test);
}

/**
 * Tests a simple allocation
 */
UK_TESTCASE(ukfallocbuddy, test_simple_falloc)
{
	struct test_ukfallocbuddy *test = new_allocator(10000);
	int falloc_result;

	falloc_result = test->fa->falloc(test->fa, (__paddr_t *)test->start,
					1, 0);
	UK_TEST_EXPECT_SNUM_EQ(falloc_result, 0);

	free_allocator(test);
}

/**
 * Tests if alloc_from_range functionality works
 */
UK_TESTCASE(ukfallocbuddy, test_alloc_from_range)
{
	struct test_ukfallocbuddy *test = new_allocator(50000);
	__paddr_t min, max;
	int falloc_from_range_result;

	min = test->start + 4096;
	max = test->start + 20000;
	test->fa->falloc(test->fa, (__paddr_t *)test->start, 1,
			FALLOC_FLAG_ALIGNED);
	falloc_from_range_result = test->fa->falloc_from_range(test->fa,
					(__paddr_t *)test->start, 2, 0, min,
					max);
	UK_TEST_EXPECT_SNUM_EQ(falloc_from_range_result, 0);

	free_allocator(test);
}

/**
 * Tests allocation with too many pages
 */
UK_TESTCASE(ukfallocbuddy, test_alloc_large)
{
	struct test_ukfallocbuddy *test = new_allocator(50000);
	int falloc_result;

	falloc_result = test->fa->falloc(test->fa, (__paddr_t *)test->start,
					600, 0);
	UK_TEST_EXPECT_SNUM_EQ(falloc_result, -EFAULT);

	free_allocator(test);
}

/**
 * Tests allocation over an already allocated zone
 */
UK_TESTCASE(ukfallocbuddy, test_alloc_over)
{
	struct test_ukfallocbuddy *test = new_allocator(50000);
	int falloc_from_range_result;
	__paddr_t end;

	end = test->start + 8192;
	test->fa->falloc(test->fa, (__paddr_t *)test->start, 2, 0);
	falloc_from_range_result = test->fa->falloc_from_range(test->fa,
					(__paddr_t *)test->start, 2, 0,
					test->start, end);

	UK_TEST_EXPECT_SNUM_EQ(falloc_from_range_result, -ENOMEM);

	free_allocator(test);
}

/**
 * Tests allocation of two frames between an already allocated one
 */
UK_TESTCASE(ukfallocbuddy, test_alloc_over_2)
{
	struct test_ukfallocbuddy *test = new_allocator(50000);
	int falloc_from_range_result;

	test->fa->falloc_from_range(test->fa, (__paddr_t *)test->start, 1, 0,
					test->start + 4097, test->start + 8192);
	falloc_from_range_result = test->fa->falloc_from_range(test->fa,
			(__paddr_t *)test->start, 2, 0, test->start,
			test->start + 12288);
	UK_TEST_EXPECT_SNUM_EQ(falloc_from_range_result, 0);

	free_allocator(test);
}

uk_testsuite_register(ukfallocbuddy, NULL);
