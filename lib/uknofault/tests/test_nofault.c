/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/nofault.h>
#include <uk/plat/paging.h>
#include <uk/assert.h>
#include <uk/test.h>
#include <uk/essentials.h>

#define TEST_MAP_BASE	0x20000000000 /* 2 TiB - must not be used otherwise */
#define TEST_MAP_END	(TEST_MAP_BASE + PAGE_SIZE)

#define nf_bug_on(cond)							\
	do {								\
		if (unlikely(cond))					\
			UK_CRASH("'%s' during test execution.\n",	\
				 STRINGIFY(cond));			\
	} while (0)

UK_TESTCASE(uknofault, test_nofault_probe_noaccess)
{
	__sz len;

	len = uk_nofault_probe_r(TEST_MAP_BASE, 1, 0);
	UK_TEST_EXPECT_SNUM_EQ(len, 0);

	len = uk_nofault_probe_r(TEST_MAP_BASE, PAGE_SIZE, 0);
	UK_TEST_EXPECT_SNUM_EQ(len, 0);
}

/**
 * Tests if we can successfully probe for read access
 */
UK_TESTCASE(uknofault, test_nofault_probe_r)
{
	struct uk_pagetable *pt = ukplat_pt_get_active();
	__sz len;
	int rc;

	rc = ukplat_page_map(pt, TEST_MAP_BASE, __PADDR_ANY, 1,
			     PAGE_ATTR_PROT_READ, 0);
	nf_bug_on(rc != 0);

	len = uk_nofault_probe_r(TEST_MAP_BASE, 1, 0);
	UK_TEST_EXPECT_SNUM_EQ(len, 1);

	len = uk_nofault_probe_r(TEST_MAP_BASE, PAGE_SIZE, 0);
	UK_TEST_EXPECT_SNUM_EQ(len, PAGE_SIZE);

	len = uk_nofault_probe_r(TEST_MAP_BASE - 1, 2, 0);
	UK_TEST_EXPECT_SNUM_EQ(len, 0);

	len = uk_nofault_probe_r(TEST_MAP_BASE - 6, 8, 0);
	UK_TEST_EXPECT_SNUM_EQ(len, 0);

	len = uk_nofault_probe_r(TEST_MAP_END - 1, 8, 0);
	UK_TEST_EXPECT_SNUM_EQ(len, 1);

	len = uk_nofault_probe_r(TEST_MAP_END - 1244, PAGE_SIZE, 0);
	UK_TEST_EXPECT_SNUM_EQ(len, 1244);

	/* Map another page and leave a hole of one page to check if
	 * continuation works
	 */
	rc = ukplat_page_map(pt, TEST_MAP_BASE + 2 * PAGE_SIZE, __PADDR_ANY, 1,
			     PAGE_ATTR_PROT_READ, 0);
	nf_bug_on(rc != 0);

	len = uk_nofault_probe_r(TEST_MAP_BASE, 3 * PAGE_SIZE,
				 UK_NOFAULTF_CONTINUE);
	UK_TEST_EXPECT_SNUM_EQ(len, 2 * PAGE_SIZE);

	/* Clean up */
	rc = ukplat_page_unmap(pt, TEST_MAP_BASE, 3, 0);
	nf_bug_on(rc != 0);
}

/**
 * Tests if we can successfully probe for read/write access. This will also test
 * the uk_nofault_memcpy() function as the read/write probe functions use the
 * memcpy function.
 */
UK_TESTCASE(uknofault, test_nofault_probe_rw)
{
	struct uk_pagetable *pt = ukplat_pt_get_active();
	__sz len;
	int rc;

	rc = ukplat_page_map(pt, TEST_MAP_BASE, __PADDR_ANY, 1,
			     PAGE_ATTR_PROT_READ, 0);
	nf_bug_on(rc != 0);

	len = uk_nofault_probe_r(TEST_MAP_BASE, 1, 0);
	UK_TEST_EXPECT_SNUM_EQ(len, 1);

	len = uk_nofault_probe_rw(TEST_MAP_BASE, 1, 0);
	UK_TEST_EXPECT_SNUM_EQ(len, 0);

	/* Make the page writeable. Set a sanity value so we can verify that
	 * the rw probe does not change memory contents
	 */
	rc = ukplat_page_set_attr(pt, TEST_MAP_BASE, 1,
				  PAGE_ATTR_PROT_RW, 0);
	nf_bug_on(rc != 0);

	*((unsigned long *)TEST_MAP_BASE) = 0xdeadb0b0;

	len = uk_nofault_probe_rw(TEST_MAP_BASE, 1, 0);
	UK_TEST_EXPECT_SNUM_EQ(len, 1);

	len = uk_nofault_probe_rw(TEST_MAP_END - 1244, PAGE_SIZE, 0);
	UK_TEST_EXPECT_SNUM_EQ(len, 1244);

	/* Map another page and leave a hole of one page to check if
	 * continuation works
	 */
	rc = ukplat_page_map(pt, TEST_MAP_BASE + 2 * PAGE_SIZE, __PADDR_ANY, 1,
			     PAGE_ATTR_PROT_RW, 0);
	nf_bug_on(rc != 0);

	len = uk_nofault_probe_rw(TEST_MAP_BASE, 3 * PAGE_SIZE,
				  UK_NOFAULTF_CONTINUE);
	UK_TEST_EXPECT_SNUM_EQ(len, 2 * PAGE_SIZE);

	/* Check sanity value */
	UK_TEST_EXPECT_SNUM_EQ(*((unsigned long *)TEST_MAP_BASE), 0xdeadb0b0);

	/* Clean up */
	rc = ukplat_page_unmap(pt, TEST_MAP_BASE, 3, 0);
	nf_bug_on(rc != 0);
}

/**
 * Tests the uk_nofault_try_read function
 */
UK_TESTCASE(uknofault, test_nofault_try_read)
{
	unsigned long v = 0x42;
	unsigned long v2;
	int rc;

	rc = uk_nofault_try_read(v2, __VADDR_INV);
	UK_TEST_EXPECT_SNUM_EQ(rc, 0);

	rc = uk_nofault_try_read(v2, &v);
	UK_TEST_EXPECT_SNUM_EQ(rc, 1);
	UK_TEST_EXPECT_SNUM_EQ(v2, v);
}

/**
 * Tests the uk_nofault_try_write function
 */
UK_TESTCASE(uknofault, test_nofault_try_write)
{
	unsigned long v = 0x42;
	unsigned long v2;
	int rc;

	rc = uk_nofault_try_write(v, __VADDR_INV);
	UK_TEST_EXPECT_SNUM_EQ(rc, 0);

	rc = uk_nofault_try_write(v, &v2);
	UK_TEST_EXPECT_SNUM_EQ(rc, 1);
	UK_TEST_EXPECT_SNUM_EQ(v2, v);
}

uk_testsuite_register(uknofault, NULL);
