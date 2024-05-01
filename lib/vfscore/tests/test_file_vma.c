/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <uk/test.h>
#include <uk/vmem/vma_test.h>
#include <vfscore/vma.h>

/**
 * Tests file mappings
 */
#define VMEM_TEST_FILENAME "/test_vma_file"
UK_TESTCASE(vfscore, test_vma_file)
{
	struct uk_vas *vas = uk_vmem_test_vas_init();
	unsigned char *buf;
	int fd, i, rc;
	__vaddr_t va1, va2;
	__ssz len;

	/* First create a dummy file which we can map. We fill the file with
	 * pages that start with an incrementing number. This way, we can check
	 * if we are mapping the correct offset. We also append an incomplete
	 * page of data at the end, to check if the mapping returns zeroed
	 * memory for the remainder of the page.
	 */
	fd = creat(VMEM_TEST_FILENAME, 0700);
	uk_vmem_test_bug_on(fd < 0);

	buf = malloc(PAGE_SIZE);
	uk_vmem_test_bug_on(!buf);

	memset(buf, 0xdd, PAGE_SIZE);

	for (i = 0; i < 3; i++) {
		buf[0] = (char)i;

		len = write(fd, buf, PAGE_SIZE);
		uk_vmem_test_bug_on(len != PAGE_SIZE);
	}

	buf[0] = (char)i;
	len = write(fd, buf, PAGE_SIZE / 2);
	uk_vmem_test_bug_on(len != PAGE_SIZE / 2);

	free(buf);

	/* Create the mapping */
	va1 = __VADDR_ANY;

	rc = uk_vma_map_file(vas, &va1, 3 * PAGE_SIZE, PAGE_ATTR_PROT_READ,
			     0, fd, 0);
	UK_TEST_EXPECT_ZERO(rc);

	len = uk_vmem_test_probe_r(va1, 3 * PAGE_SIZE);
	uk_vmem_test_bug_on(len != 3 * PAGE_SIZE);

	UK_TEST_EXPECT_SNUM_EQ(*((unsigned char *)(va1)), 0);
	UK_TEST_EXPECT_SNUM_EQ(*((unsigned char *)(va1 + 1)), 0xdd);
	UK_TEST_EXPECT_SNUM_EQ(*((unsigned char *)(va1 + PAGE_SIZE)), 1);
	UK_TEST_EXPECT_SNUM_EQ(*((unsigned char *)(va1 + 2 * PAGE_SIZE)), 2);

	rc = uk_vma_set_attr(vas, va1 + 2 * PAGE_SIZE, PAGE_SIZE,
			     PAGE_ATTR_PROT_RW, 0);
	UK_TEST_EXPECT_ZERO(rc);

	UK_TEST_EXPECT_ZERO(uk_vmem_test_check_vas(vas,
		(struct uk_vmem_test_vma[]){
			{va1, va1 + 2 * PAGE_SIZE, PAGE_ATTR_PROT_READ},
			{va1 + 2 * PAGE_SIZE, va1 + 3 * PAGE_SIZE, PAGE_ATTR_PROT_RW},
		}, 2));

	/* Unmap the first two pages of the file. Then map the last half page
	 * of the file directly after the last VMA. They should merge.
	 */
	rc = uk_vma_unmap(vas, va1, 2 * PAGE_SIZE, 0);
	UK_TEST_EXPECT_ZERO(rc);

	va2 = va1 + 3 * PAGE_SIZE;
	rc = uk_vma_map_file(vas, &va2, PAGE_SIZE, PAGE_ATTR_PROT_RW, 0, fd,
			     3 * PAGE_SIZE);
	UK_TEST_EXPECT_ZERO(rc);

	UK_TEST_EXPECT_ZERO(uk_vmem_test_check_vas(vas,
		(struct uk_vmem_test_vma[]){
			{va1 + 2 * PAGE_SIZE, va1 + 4 * PAGE_SIZE, PAGE_ATTR_PROT_RW},
		}, 1));

	/* Check if we correctly read the last bits and if the remainder of the
	 * page is zeroed (we just check two bytes here).
	 */
	UK_TEST_EXPECT_SNUM_EQ(*((unsigned char *)(va2)), 3);

	va1 = va2 + PAGE_SIZE / 2 - 1;
	UK_TEST_EXPECT_SNUM_EQ(*((unsigned char *)(va1)), 0xdd);
	UK_TEST_EXPECT_SNUM_EQ(*((unsigned char *)(va1 + 1)), 0x00);

	/* Clean up */
	close(fd);
	unlink(VMEM_TEST_FILENAME);

	uk_vmem_test_vas_clean(vas);
}

uk_testsuite_register(vfscore, NULL);
