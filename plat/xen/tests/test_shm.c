/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Titouan Christophe
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <xen-arm/mm.h>
#include <xen-arm/shm.h>

#include <uk/config.h>
#include <uk/essentials.h>
#include <uk/paging.h>
#include <uk/test.h>

#define TEST_SHM_ID	"uktest-shm"
#define TEST_SHM_VAL	0x0123456789abcdefUL

/**
 * Test that a value written to a shm region mapped RW can be read back
 */
UK_TESTCASE(xen_static_shm, test_shm_write_read)
{
	int ret;
	__u64 *p;

	ret = uk_plat_xen_shm_get(TEST_SHM_ID, PAGE_SIZE, false, 0, (void **)&p);
	UK_TEST_EXPECT_ZERO(ret);
	UK_TEST_EXPECT_NOT_NULL(p);
	if (unlikely(!p))
		return;

	p[0] = TEST_SHM_VAL;
	UK_TEST_EXPECT_SNUM_EQ(p[0], TEST_SHM_VAL);
}

/**
 * Test that we can't map an unaligned region
 */
UK_TESTCASE(xen_static_shm, test_shm_not_aligned)
{
	int ret;
	__u64 *p;

	/* Unaligned len */
	ret = uk_plat_xen_shm_get(TEST_SHM_ID, 3, false, 0, (void **)&p);
	UK_TEST_EXPECT_SNUM_EQ(ret, -EINVAL);
	UK_TEST_EXPECT_NULL(p);

	/* Unaligned offset */
	ret = uk_plat_xen_shm_get(TEST_SHM_ID, PAGE_SIZE, false, 3, (void **)&p);
	UK_TEST_EXPECT_SNUM_EQ(ret, -EINVAL);
	UK_TEST_EXPECT_NULL(p);

	/* Unaligned len and offset */
	ret = uk_plat_xen_shm_get(TEST_SHM_ID, 3, false, 3, (void **)&p);
	UK_TEST_EXPECT_SNUM_EQ(ret, -EINVAL);
	UK_TEST_EXPECT_NULL(p);

	/* Unaligned len and offset (but offset+len is aligned) */
	ret = uk_plat_xen_shm_get(TEST_SHM_ID, 1024, false, 3072, (void **)&p);
	UK_TEST_EXPECT_SNUM_EQ(ret, -EINVAL);
	UK_TEST_EXPECT_NULL(p);
}

/**
 * Test that we can't map a region not defined in the devicetree
 */
UK_TESTCASE(xen_static_shm, test_shm_not_found)
{
	int ret;
	__u64 *p;

	ret = uk_plat_xen_shm_get("__NOT_AN_ID__", PAGE_SIZE, false, 0, (void **)&p);
	UK_TEST_EXPECT_SNUM_EQ(ret, -ENOENT);
	UK_TEST_EXPECT_NULL(p);
}

UK_TESTCASE(xen_static_shm, test_shm_null_memptr)
{
	int ret;

	ret = uk_plat_xen_shm_get(TEST_SHM_ID, PAGE_SIZE, false, 0, NULL);
	UK_TEST_EXPECT_SNUM_EQ(ret, -EINVAL);
}

uk_testsuite_register(xen_static_shm, NULL);
