/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#define _GNU_SOURCE

#include <sys/mman.h>
#include <uk/vmem.h>
#include <uk/test.h>

UK_TESTCASE(ukmmap, test_mmap)
{
	void *addr;
	int rc;

	addr = mmap(NULL, 2 * PAGE_SIZE, PROT_READ | PROT_WRITE,
		    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	UK_TEST_EXPECT(addr != MAP_FAILED);

	rc = munmap(addr, 2 * PAGE_SIZE);
	UK_TEST_EXPECT_ZERO(rc);
}

uk_testsuite_register(ukmmap, NULL);
