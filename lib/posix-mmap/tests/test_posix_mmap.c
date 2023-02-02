/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>

#include <uk/test.h>
#include <uk/list.h>
#include <uk/arch/paging.h>
#include <uk/vmem.h>

#define pr_info(fmt, ...)						\
	_uk_printk(KLVL_INFO, __NULL, __NULL, 0x0, fmt, ##__VA_ARGS__)

#define pm_bug_on(cond)							\
	do {								\
		if (unlikely(cond))					\
			UK_CRASH("'%s' during test execution.\n",	\
				 STRINGIFY(cond));			\
	} while (0)

static struct uk_vas *vas_init(void)
{
	struct uk_vas *vas = uk_vas_get_active();

	pm_bug_on(vas == __NULL);
	return vas;
}

static void vas_clean(struct uk_vas *vas)
{
	struct uk_vma *vma;
	int restart, rc;

	do {
		restart = 0;
		uk_list_for_each_entry(vma, &vas->vma_list, vma_list) {
			if (vma->name && strcmp("heap", vma->name) == 0)
				continue;

			rc = uk_vma_unmap(vas, vma->start,
					  vma->end - vma->start, 0);
			pm_bug_on(rc != 0);

			restart = 1;
			break;
		}
	} while (restart);
}

UK_TESTCASE(posix_mmap, test_mmap)
{
	struct uk_vas *vas = vas_init();
	void *addr;
	int rc;

	addr = mmap(NULL, 2 * PAGE_SIZE, PROT_READ | PROT_WRITE,
		    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	UK_TEST_EXPECT(addr != MAP_FAILED);

	rc = munmap(addr, 2 * PAGE_SIZE);
	UK_TEST_EXPECT_ZERO(rc);

	vas_clean(vas);
}

uk_testsuite_register(posix_mmap, NULL);
