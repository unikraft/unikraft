/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <stdio.h>
#include <errno.h>

#include <uk/vmem.h>
#include <uk/vmem/vma_test.h>
#include <uk/test.h>
#include <uk/list.h>
#include <uk/print.h>
#include <uk/alloc.h>
#include <uk/falloc.h>
#include <uk/nofault.h>
#include <uk/paging.h>
#include <uk/arch/limits.h>

#define MAPPING_BASE CONFIG_LIBUKVMEM_DEFAULT_BASE

/* Just define some shorter aliases */
#undef PROT_R
#define PROT_R UK_PAGING_PAGE_ATTR_PROT_READ

#undef PROT_RW
#define PROT_RW UK_PAGING_PAGE_ATTR_PROT_RW

#undef PROT_RWX
#define PROT_RWX UK_PAGING_PAGE_ATTR_PROT_RWX

static void vmem_print(struct uk_vas *vas)
{
	struct uk_vma *vma;
	__sz i = 0;

	uk_pr_info("   VAS layout:\n");
	uk_list_for_each_entry(vma, &vas->vma_list, vma_list) {
		if (vma->name && strcmp("heap", vma->name) == 0)
			continue;

		uk_pr_info("     [%zu] 0x%lx-0x%lx %c%c%c %s\n", i++,
			   vma->start, vma->end,
			   (vma->attr & UK_PAGING_PAGE_ATTR_PROT_READ) ? 'r' : '-',
			   (vma->attr & UK_PAGING_PAGE_ATTR_PROT_WRITE) ? 'w' : '-',
			   (vma->attr & UK_PAGING_PAGE_ATTR_PROT_EXEC) ? 'x' : '-',
			   (vma->name) ? vma->name : "");
	}
}

int uk_vmem_test_check_vas(struct uk_vas *vas,
			   struct uk_vmem_test_vma *vmas, __sz num)
{
	struct uk_vma *vma;
	__sz i = 0;

	vmem_print(vas);

	uk_list_for_each_entry(vma, &vas->vma_list, vma_list) {
		if (vma->name && strcmp("heap", vma->name) == 0)
			continue;

		if (i >= num)
			return -1;

		if (vma->start != vmas[i].start ||
		    vma->end   != vmas[i].end ||
		    vma->attr  != vmas[i].attr ||
		    vma->vas   != vas)
			return -1;

		i++;
	}

	return (i == num) ? 0 : -1;
}

struct uk_vas *uk_vmem_test_vas_init(void)
{
	struct uk_vas *vas = uk_vas_get_active();

	uk_vmem_test_bug_on(vas == __NULL);
	return vas;
}

void uk_vmem_test_vas_clean(struct uk_vas *vas)
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
			uk_vmem_test_bug_on(rc != 0);

			restart = 1;
			break;
		}
	} while (restart);
}

/**
 * Tests if the virtual address space layout is as expected after creating and
 * removing address reservations. This includes tests if VMAs properly merge
 * and split and if VMAs with different attributes will not merge. Since we are
 * only working with address reservations the page table is not modified and we
 * are solely testing VMA management.
 */
UK_TESTCASE(ukvmem, test_basic_vas_layout)
{
	struct uk_vas *vas = uk_vmem_test_vas_init();
	__vaddr_t va;
	int rc;

	/* Create VMA at mapping base */
	va = UK_PAGING_VADDR_ANY;
	rc = uk_vma_reserve(vas, &va, 0x10000);
	UK_TEST_EXPECT_ZERO(rc);

	/* Create VMA at user-defined address */
	va = MAPPING_BASE + 0x20000;
	rc = uk_vma_reserve(vas, &va, 0x1000);
	UK_TEST_EXPECT_ZERO(rc);

	UK_TEST_EXPECT_ZERO(uk_vmem_test_check_vas(vas,
		(struct uk_vmem_test_vma[]){
			{MAPPING_BASE + 0x000000, MAPPING_BASE + 0x010000, 0x0},
			{MAPPING_BASE + 0x020000, MAPPING_BASE + 0x021000, 0x0},
		}, 2));

	/* Create VMA after first one, should merge */
	va = UK_PAGING_VADDR_ANY;
	rc = uk_vma_reserve(vas, &va, 0x1000);
	UK_TEST_EXPECT_ZERO(rc);

	/* Try to create overlapping VMAs (should fail) */
	va = MAPPING_BASE;
	rc = uk_vma_reserve(vas, &va, 0x1000);
	UK_TEST_EXPECT_SNUM_EQ(rc, -EEXIST);

	va = MAPPING_BASE - 0x1000;
	rc = uk_vma_reserve(vas, &va, 0x2000);
	UK_TEST_EXPECT_SNUM_EQ(rc, -EEXIST);

	va = MAPPING_BASE + 0x20000 - 0x1000;
	rc = uk_vma_reserve(vas, &va, 0x2000);
	UK_TEST_EXPECT_SNUM_EQ(rc, -EEXIST);

	va = MAPPING_BASE + 0x20000;
	rc = uk_vma_reserve(vas, &va, 0x1000);
	UK_TEST_EXPECT_SNUM_EQ(rc, -EEXIST);

	va = MAPPING_BASE - 0x1000;
	rc = uk_vma_reserve(vas, &va, 0x100000);
	UK_TEST_EXPECT_SNUM_EQ(rc, -EEXIST);

	/* Create a VMA in front of second VMA, should merge */
	va = MAPPING_BASE + 0x20000 - 0x1000;
	rc = uk_vma_reserve(vas, &va, 0x1000);
	UK_TEST_EXPECT_ZERO(rc);

	UK_TEST_EXPECT_ZERO(uk_vmem_test_check_vas(vas,
		(struct uk_vmem_test_vma[]){
			{MAPPING_BASE + 0x000000, MAPPING_BASE + 0x011000, 0x0},
			{MAPPING_BASE + 0x01f000, MAPPING_BASE + 0x021000, 0x0},
		}, 2));

	/* Create VMA that is too large for the hole, should merge with last */
	va = UK_PAGING_VADDR_ANY;
	rc = uk_vma_reserve(vas, &va, 0x100000);
	UK_TEST_EXPECT_ZERO(rc);

	UK_TEST_EXPECT_ZERO(uk_vmem_test_check_vas(vas,
		(struct uk_vmem_test_vma[]){
			{MAPPING_BASE + 0x000000, MAPPING_BASE + 0x011000, 0x0},
			{MAPPING_BASE + 0x01f000, MAPPING_BASE + 0x121000, 0x0},
		}, 2));

	/* Create VMA that fits the hole, should merge everything */
	va = UK_PAGING_VADDR_ANY;
	rc = uk_vma_reserve(vas, &va, 0xe000);
	UK_TEST_EXPECT_ZERO(rc);

	UK_TEST_EXPECT_ZERO(uk_vmem_test_check_vas(vas,
		(struct uk_vmem_test_vma[]){
			{MAPPING_BASE + 0x000000, MAPPING_BASE + 0x121000, 0x0},
		}, 1));

	/* Punch a few holes into the VMA */
	rc = uk_vma_unmap(vas, MAPPING_BASE + 0x4000, 0x1000, 0);
	UK_TEST_EXPECT_ZERO(rc);

	rc = uk_vma_unmap(vas, MAPPING_BASE + 0x6000, 0x8000, 0);
	UK_TEST_EXPECT_ZERO(rc);

	rc = uk_vma_unmap(vas, MAPPING_BASE + 0x100000, 0x100000, 0);
	UK_TEST_EXPECT_ZERO(rc);

	rc = uk_vma_unmap(vas, MAPPING_BASE - 0x1000, 0x2000, 0);
	UK_TEST_EXPECT_ZERO(rc);

	UK_TEST_EXPECT_ZERO(uk_vmem_test_check_vas(vas,
		(struct uk_vmem_test_vma[]){
			{MAPPING_BASE + 0x001000, MAPPING_BASE + 0x004000, 0x0},
			{MAPPING_BASE + 0x005000, MAPPING_BASE + 0x006000, 0x0},
			{MAPPING_BASE + 0x00e000, MAPPING_BASE + 0x100000, 0x0},
		}, 3));

	/* Unmap a region where nothing is - ok without strict checking */
	rc = uk_vma_unmap(vas, MAPPING_BASE + 0x200000, 0x1000, 0);
	UK_TEST_EXPECT_ZERO(rc);

	rc = uk_vma_unmap(vas, MAPPING_BASE + 0x200000, 0x1000,
			  UK_VMA_FLAG_STRICT_VMA_CHECK);
	UK_TEST_EXPECT_SNUM_EQ(rc, -ENOENT);

	/* Unmap a region that has holes with strict checking */
	rc = uk_vma_unmap(vas, MAPPING_BASE + 0x2000, 0xd000,
			  UK_VMA_FLAG_STRICT_VMA_CHECK);
	UK_TEST_EXPECT_SNUM_EQ(rc, -ENOENT);

	/* Unmap a region that covers multiple VMAs */
	rc = uk_vma_unmap(vas, MAPPING_BASE + 0x2000, 0xd000, 0);
	UK_TEST_EXPECT_ZERO(rc);

	UK_TEST_EXPECT_ZERO(uk_vmem_test_check_vas(vas,
		(struct uk_vmem_test_vma[]){
			{MAPPING_BASE + 0x001000, MAPPING_BASE + 0x002000, 0x0},
			{MAPPING_BASE + 0x00f000, MAPPING_BASE + 0x100000, 0x0},
		}, 2));

	/* Check that length is page aligned */
	rc = uk_vma_unmap(vas, MAPPING_BASE + 0xf000, 0x0100, 0);
	UK_TEST_EXPECT_SNUM_EQ(rc, -EINVAL);

	/* Unmap the rest */
	rc = uk_vma_unmap(vas, MAPPING_BASE, 0x200000, 0);
	UK_TEST_EXPECT_ZERO(rc);

	UK_TEST_EXPECT_ZERO(uk_vmem_test_check_vas(vas, NULL, 0));

	/* Create a couple of more complex VMAs, some of which should merge,
	 * and some should not
	 */
	va = UK_PAGING_VADDR_ANY;
	rc = uk_vma_reserve_ex(vas, &va, 0x2000, PROT_RW, 0, NULL);
	UK_TEST_EXPECT_ZERO(rc);

	va = UK_PAGING_VADDR_ANY;
	rc = uk_vma_reserve_ex(vas, &va, 0x2000, PROT_RWX, 0, NULL);
	UK_TEST_EXPECT_ZERO(rc);

	va = UK_PAGING_VADDR_ANY;
	rc = uk_vma_reserve_ex(vas, &va, 0x2000, PROT_RWX, 0, "tst");
	UK_TEST_EXPECT_ZERO(rc);

	va = UK_PAGING_VADDR_ANY;
	rc = uk_vma_reserve_ex(vas, &va, 0x2000, PROT_RWX, 0, "tst");
	UK_TEST_EXPECT_ZERO(rc);

	va = UK_PAGING_VADDR_ANY;
	rc = uk_vma_reserve_ex(vas, &va, 0x2000, PROT_RW, 0, "tst");
	UK_TEST_EXPECT_ZERO(rc);

	UK_TEST_EXPECT_ZERO(uk_vmem_test_check_vas(vas,
		(struct uk_vmem_test_vma[]){
			{MAPPING_BASE + 0x000000, MAPPING_BASE + 0x002000, PROT_RW},
			{MAPPING_BASE + 0x002000, MAPPING_BASE + 0x004000, PROT_RWX},
			{MAPPING_BASE + 0x004000, MAPPING_BASE + 0x008000, PROT_RWX},
			{MAPPING_BASE + 0x008000, MAPPING_BASE + 0x00a000, PROT_RW}
		}, 4));

	/* Set attributes so that some areas are merged */
	va = MAPPING_BASE + 0x1000;
	rc = uk_vma_set_attr(vas, va, 0x7000, UK_PAGING_PAGE_ATTR_PROT_RW, 0);
	UK_TEST_EXPECT_ZERO(rc);

	UK_TEST_EXPECT_ZERO(uk_vmem_test_check_vas(vas,
		(struct uk_vmem_test_vma[]){
			{MAPPING_BASE + 0x000000, MAPPING_BASE + 0x004000, PROT_RW},
			{MAPPING_BASE + 0x004000, MAPPING_BASE + 0x00a000, PROT_RW},
		}, 2));

	uk_vmem_test_vas_clean(vas);
}

static inline int is_zero(__vaddr_t va, __sz len)
{
	unsigned long *p = (unsigned long *)va;

	UK_ASSERT(ALIGN_UP(len, sizeof(*p)) == len);

	while (len > 0) {
		if (*p != 0)
			return 0;

		len -= sizeof(*p);
	}

	return 1;
}

/**
 * Tests if reserved memory regions behave as expected. Since we test all
 * things related to the VAS layout in a different test using reserved regions,
 * this only checks that accessing a reserved region will be handled properly
 * and just cause a fault.
 */
UK_TESTCASE(ukvmem, test_vma_rsvd)
{
	struct uk_vas *vas = uk_vmem_test_vas_init();
	__vaddr_t vaddr;
	int rc;
	__sz len;

	vaddr = UK_PAGING_VADDR_ANY;
	rc = uk_vma_reserve(vas, &vaddr, UK_PAGING_PAGE_SIZE);
	UK_TEST_EXPECT_ZERO(rc);

	len = uk_vmem_test_probe_r(vaddr, UK_PAGING_PAGE_SIZE);
	UK_TEST_EXPECT_SNUM_EQ(len, 0);

	uk_vmem_test_vas_clean(vas);
}

/**
 * Tests if anonymous memory regions behave as expected. The test performs
 * various checks including memory probes to verify on-demand paging operation
 * and page permission checks.
 */
UK_TESTCASE(ukvmem, test_vma_anon)
{
	struct uk_vas *vas = uk_vmem_test_vas_init();
	__vaddr_t va1, va2;
	int rc;
	__sz len;

	/* Check if we can map and populate a simple anonymous memory.
	 * We disable demand-paging during the test access to make sure the
	 * memory is actually pre-faulted.
	 */
	va1 = UK_PAGING_VADDR_ANY;
	rc = uk_vma_map_anon(vas, &va1, 0x10000, PROT_R,
			     UK_VMA_MAP_POPULATE, NULL);
	UK_TEST_EXPECT_ZERO(rc);

	len = uk_vmem_test_probe_r_nopage(va1, 0x10000);
	UK_TEST_EXPECT_SNUM_EQ(len, 0x10000);
	UK_TEST_EXPECT(is_zero(va1, 0x10000));

	/* Check protections are set correctly */
	len = uk_vmem_test_probe_rw_nopage(va1, 0x10000);
	UK_TEST_EXPECT_ZERO(len);

	/* Perform another mapping with demand-paging. We test if we can
	 * access the memory with disabled/enabled paging. The VMA should merge
	 * with the first one.
	 */
	va2 = UK_PAGING_VADDR_ANY;
	rc = uk_vma_map_anon(vas, &va2, 0x2000, PROT_R, 0, NULL);
	UK_TEST_EXPECT_ZERO(rc);

	UK_TEST_EXPECT_ZERO(uk_vmem_test_check_vas(vas,
		(struct uk_vmem_test_vma[]){
			{va1, va1 + 0x10000 + 0x2000, PROT_R},
		}, 1));

	len = uk_vmem_test_probe_r_nopage(va2, 0x2000);
	UK_TEST_EXPECT_ZERO(len);

	len = uk_vmem_test_probe_r(va2 + 0x1000, 0x1000);
	UK_TEST_EXPECT_SNUM_EQ(len, 0x1000);
	UK_TEST_EXPECT(is_zero(va2 + 0x1000, 0x1000));

	/* Check protections are set correctly */
	len = uk_vmem_test_probe_rw(va2 + 0x1000, 0x1000);
	UK_TEST_EXPECT_ZERO(len);

	/* First page should still be inaccessible */
	len = uk_vmem_test_probe_r_nopage(va2, 0x1000);
	UK_TEST_EXPECT_ZERO(len);

	/* We change protections for the second VMA range */
	rc = uk_vma_set_attr(vas, va2, 0x2000, PROT_RW, 0);
	UK_TEST_EXPECT_ZERO(rc);

	UK_TEST_EXPECT_ZERO(uk_vmem_test_check_vas(vas,
		(struct uk_vmem_test_vma[]){
			{va1, va1 + 0x10000, PROT_R},
			{va2, va2 + 0x2000, PROT_RW},
		}, 2));

	/* This should create a new mapping based on the updated attr for the
	 * first page and use the updated mapping for the second page
	 */
	len = uk_vmem_test_probe_rw(va2, 0x2000);
	UK_TEST_EXPECT_SNUM_EQ(len, 0x2000);

	/* Unmap some part of the area */
	rc = uk_vma_unmap(vas, va1 + 0x2000, 0x3000, 0);
	UK_TEST_EXPECT_ZERO(rc);

	UK_TEST_EXPECT_ZERO(uk_vmem_test_check_vas(vas,
		(struct uk_vmem_test_vma[]){
			{va1 + 0x0000, va1 + 0x2000, PROT_R},
			{va1 + 0x5000, va1 + 0x10000, PROT_R},
			{va2 + 0x0000, va2 + 0x2000, PROT_RW},
		}, 3));

	len = uk_vmem_test_probe_r(va1 + 0x2000, 0x3000);
	UK_TEST_EXPECT_ZERO(len);

	uk_vmem_test_vas_clean(vas);
}

#ifdef UK_PAGING_PAGE_LARGE_SHIFT
/**
 * Tests if we can create anonymous mappings with large pages.
 */
UK_TESTCASE(ukvmem, test_vma_anon_large)
{
	struct uk_vas *vas = uk_vmem_test_vas_init();
	__vaddr_t va;
	unsigned int lvl;
	int rc;
	__sz len;

	/* First try with an invalid length and address */
	va = UK_PAGING_VADDR_ANY;
	rc = uk_vma_map_anon(vas, &va, UK_PAGING_PAGE_SIZE, PROT_RW,
			     UK_VMA_MAP_SIZE(UK_PAGING_PAGE_LARGE_SHIFT), NULL);
	UK_TEST_EXPECT_SNUM_EQ(rc, -EINVAL);

	va = UK_PAGING_PAGE_LARGE_ALIGN_DOWN(MAPPING_BASE) - UK_PAGING_PAGE_SIZE;
	rc = uk_vma_map_anon(vas, &va, UK_PAGING_PAGE_LARGE_SIZE * 2, PROT_RW,
			     UK_VMA_MAP_SIZE(UK_PAGING_PAGE_LARGE_SHIFT), NULL);
	UK_TEST_EXPECT_SNUM_EQ(rc, -EINVAL);

	va = UK_PAGING_VADDR_ANY;
	rc = uk_vma_map_anon(vas, &va, UK_PAGING_PAGE_LARGE_SIZE * 2, PROT_RW,
			     UK_VMA_MAP_SIZE(UK_PAGING_PAGE_LARGE_SHIFT), NULL);
	UK_TEST_EXPECT_ZERO(rc);

	len = uk_vmem_test_probe_rw(va, UK_PAGING_PAGE_LARGE_SIZE * 2);
	UK_TEST_EXPECT_SNUM_EQ(len, UK_PAGING_PAGE_LARGE_SIZE * 2);

	/* Check if this is really a large page */
	lvl = UK_PAGING_PAGE_LEVEL;
	rc = uk_paging_pt_walk(vas->pt, va, &lvl, NULL, NULL);
	uk_vmem_test_bug_on(rc != 0);

	UK_TEST_EXPECT_SNUM_EQ(lvl, UK_PAGING_PAGE_LARGE_LEVEL);

	/* Make sure that we cannot split the region with an invalid address
	 * and length
	 */
	rc = uk_vma_set_attr(vas, va + UK_PAGING_PAGE_LARGE_SIZE,
			     UK_PAGING_PAGE_SIZE, PROT_R, 0);
	UK_TEST_EXPECT_SNUM_EQ(rc, -EINVAL);

	rc = uk_vma_set_attr(vas, va + UK_PAGING_PAGE_SIZE,
			     UK_PAGING_PAGE_LARGE_SIZE, PROT_R, 0);
	UK_TEST_EXPECT_SNUM_EQ(rc, -EINVAL);

	rc = uk_vma_set_attr(vas, va + UK_PAGING_PAGE_LARGE_SIZE,
			     UK_PAGING_PAGE_LARGE_SIZE,
			     PROT_R, 0);
	UK_TEST_EXPECT_ZERO(0);

	UK_TEST_EXPECT_ZERO(uk_vmem_test_check_vas(vas,
		(struct uk_vmem_test_vma[]){
			{va, va + UK_PAGING_PAGE_LARGE_SIZE, PROT_RW},
			{va + UK_PAGING_PAGE_LARGE_SIZE,
			 va + 2 * UK_PAGING_PAGE_LARGE_SIZE, PROT_R},
		}, 2));

	uk_vmem_test_vas_clean(vas);
}
#endif /* UK_PAGING_PAGE_LARGE_SHIFT */

/**
 * Tests direct physical memory mappings. We especially make sure that the
 * mapping actually maps the correct physical memory and that this is still
 * true for new mappings after splitting the VMA.
 */
UK_TESTCASE(ukvmem, test_vma_dma)
{
	struct uk_vas *vas = uk_vmem_test_vas_init();
	__vaddr_t kvaddr;
	__vaddr_t vaddr = UK_PAGING_VADDR_ANY;
	__paddr_t paddr = UK_PAGING_PADDR_ANY;
	__vaddr_t va;
	__sz len;
	int rc, i;

	/* Allocate some physical pages and write magic values to them via
	 * kernel mapping. Afterwards, also map them via ukvmem and check if
	 * the contents matches
	 */
	rc = vas->pt->fa->falloc(vas->pt->fa, &paddr, 4, 0);
	uk_vmem_test_bug_on(rc != 0);

	kvaddr = uk_paging_page_kmap(vas->pt, paddr, 4, 0);
	uk_vmem_test_bug_on(kvaddr == UK_PAGING_VADDR_INV);

	for (i = 0, va = kvaddr; i < 4; i++, va += UK_PAGING_PAGE_SIZE)
		*((unsigned long *)va) = 0xDEADB0B0 + i;

	rc = uk_vma_map_dma(vas, &vaddr, 4 * UK_PAGING_PAGE_SIZE, PROT_RW, 0,
			    NULL, paddr);
	UK_TEST_EXPECT_ZERO(rc);

	/* Check if we can read the first 3 values back in */
	for (i = 0, va = vaddr; i < 3; i++, va += UK_PAGING_PAGE_SIZE)
		if (*((unsigned long *)va) != 0xDEADB0B0 + i)
			break;

	UK_TEST_EXPECT_SNUM_EQ(i, 3);

	/* Modify the values via the new mapping */
	for (i = 0, va = vaddr; i < 3; i++, va += UK_PAGING_PAGE_SIZE)
		*((unsigned long *)va) = 0xB0B0CAFE + i;

	/* Read the values back in via the kernel mapping */
	for (i = 0, va = kvaddr; i < 3; i++, va += UK_PAGING_PAGE_SIZE)
		if (*((unsigned long *)va) != 0xB0B0CAFE + i)
			break;

	UK_TEST_EXPECT_SNUM_EQ(i, 3);

	/* Change protections of the first pair of pages */
	rc = uk_vma_set_attr(vas, vaddr, 2 * UK_PAGING_PAGE_SIZE, PROT_R, 0);
	UK_TEST_EXPECT_ZERO(rc);

	UK_TEST_EXPECT_ZERO(uk_vmem_test_check_vas(vas,
		(struct uk_vmem_test_vma[]){
			{vaddr + 0 * UK_PAGING_PAGE_SIZE,
			 vaddr + 2 * UK_PAGING_PAGE_SIZE, PROT_R},
			{vaddr + 2 * UK_PAGING_PAGE_SIZE,
			 vaddr + 4 * UK_PAGING_PAGE_SIZE, PROT_RW},
		}, 2));

	/* Two pages should be read-only now and one of the rw-pages (the 4th)
	 * should not be mapped at this point. We then perform a write, which
	 * causes a write fault on the unmapped rw-page. We then compare the
	 * written value with the direct kernel mapping to verify that the
	 * new physical base address computed during the split and used during
	 * the fault is correct.
	 */
	len = uk_vmem_test_probe_r_nopage(vaddr, 4 * UK_PAGING_PAGE_SIZE);
	UK_TEST_EXPECT_SNUM_EQ(len, 3 * UK_PAGING_PAGE_SIZE);

	len = uk_vmem_test_probe_rw_nopage(vaddr, 4 * UK_PAGING_PAGE_SIZE);
	UK_TEST_EXPECT_SNUM_EQ(len, UK_PAGING_PAGE_SIZE);

	*((unsigned long *)(vaddr + 3 * UK_PAGING_PAGE_SIZE)) = 0xB0B0CAFE + 3;

	UK_TEST_EXPECT_SNUM_EQ(*((unsigned long *)(kvaddr + 3 * UK_PAGING_PAGE_SIZE)),
			       0xB0B0CAFE + 3);

	/* We change the protections back to what they were and see if the
	 * merge was successful.
	 */
	rc = uk_vma_set_attr(vas, vaddr, 2 * UK_PAGING_PAGE_SIZE, PROT_RW, 0);
	UK_TEST_EXPECT_ZERO(rc);

	UK_TEST_EXPECT_ZERO(uk_vmem_test_check_vas(vas,
		(struct uk_vmem_test_vma[]){
			{vaddr, vaddr + 4 * UK_PAGING_PAGE_SIZE, PROT_RW},
		}, 1));

	/* Clean up */
	uk_paging_page_kunmap(vas->pt, kvaddr, 4, 0);

	rc = vas->pt->fa->ffree(vas->pt->fa, paddr, 4);
	uk_vmem_test_bug_on(rc != 0);

	uk_vmem_test_vas_clean(vas);
}

/**
 * Tests stack mappings. Focus for the stacks is on if the initial length
 * configuration option works, the guard page is present, and that stacks
 * are not merged or split.
 */
#define VMEM_STACKSIZE (16 * UK_PAGING_PAGE_SIZE)
UK_TESTCASE(ukvmem, test_vma_stack)
{
	struct uk_vas *vas = uk_vmem_test_vas_init();
	__vaddr_t va1 = UK_PAGING_VADDR_ANY;
	__vaddr_t va2 = UK_PAGING_VADDR_ANY;
	__sz len;
	int rc;

	rc = uk_vma_map_stack(vas, &va1, VMEM_STACKSIZE, UK_VMA_STACK_GROWS_UP,
			      NULL, 2 * UK_PAGING_PAGE_SIZE);
	UK_TEST_EXPECT_ZERO(rc);

	/* Verify that only the initial length has been allocated and the
	 * memory is readable/writable
	 */
	len = uk_vmem_test_probe_rw_nopage(va1, VMEM_STACKSIZE);
	UK_TEST_EXPECT_SNUM_EQ(len, 2 * UK_PAGING_PAGE_SIZE);

	/* Probe the top guard pages */
	len = uk_vmem_test_probe_r(va1 + VMEM_STACKSIZE,
				   UK_VMA_STACK_TOP_GUARD_SIZE);
	UK_TEST_EXPECT_ZERO(len);

	/* Probe the bottom guard pages */
	len = uk_vmem_test_probe_r(va1 - UK_VMA_STACK_BOTTOM_GUARD_SIZE,
				   UK_VMA_STACK_BOTTOM_GUARD_SIZE);
	UK_TEST_EXPECT_ZERO(len);

	/* Probe the entire stack */
	len = uk_vmem_test_probe_r(va1 - UK_VMA_STACK_BOTTOM_GUARD_SIZE,
				   VMEM_STACKSIZE + UK_VMA_STACK_GUARDS_SIZE);
	UK_TEST_EXPECT_SNUM_EQ(len, VMEM_STACKSIZE);

	rc = uk_vma_map_stack(vas, &va2, VMEM_STACKSIZE, 0,
			      NULL, VMEM_STACKSIZE);
	UK_TEST_EXPECT_ZERO(rc);

	UK_TEST_EXPECT_ZERO(uk_vmem_test_check_vas(vas,
		(struct uk_vmem_test_vma[]){
			{va1, va1 + VMEM_STACKSIZE, PROT_RW},
			{va2, va2 + VMEM_STACKSIZE, PROT_RW},
		}, 2));

	/* Probe the top guard pages */
	len = uk_vmem_test_probe_r(va2 + VMEM_STACKSIZE,
				   UK_VMA_STACK_TOP_GUARD_SIZE);
	UK_TEST_EXPECT_ZERO(len);

	/* Probe the bottom guard pages */
	len = uk_vmem_test_probe_r(va2 - UK_VMA_STACK_BOTTOM_GUARD_SIZE,
				   UK_VMA_STACK_BOTTOM_GUARD_SIZE);
	UK_TEST_EXPECT_ZERO(len);

	/* Probe the entire stack */
	len = uk_vmem_test_probe_r(va2 - UK_VMA_STACK_BOTTOM_GUARD_SIZE,
				   VMEM_STACKSIZE + UK_VMA_STACK_GUARDS_SIZE);
	UK_TEST_EXPECT_SNUM_EQ(len, VMEM_STACKSIZE);

	/* Try to unmap only some part of the stack */
	rc = uk_vma_unmap(vas, va2, UK_PAGING_PAGE_SIZE * 2, 0);
	UK_TEST_EXPECT_SNUM_EQ(rc, -EPERM);

	/* Try to set attr for a part of the stack */
	rc = uk_vma_set_attr(vas, va2, UK_PAGING_PAGE_SIZE * 2, PROT_R, 0);
	UK_TEST_EXPECT_SNUM_EQ(rc, -EPERM);

	/* But we should be able to change attributes for the whole VMA */
	rc = uk_vma_set_attr(vas, va2 - UK_VMA_STACK_BOTTOM_GUARD_SIZE,
			     VMEM_STACKSIZE + UK_VMA_STACK_GUARDS_SIZE,
			     PROT_R, 0);
	UK_TEST_EXPECT_ZERO(rc);

	uk_vmem_test_vas_clean(vas);
}

/**
 * Tests if it is in general possible to replace VMAs with other VMAs using the
 * UK_VMA_MAP_REPLACE flag.
 */
#define VMEM_TEST_HEAPSIZE 0x40000000
UK_TESTCASE(ukvmem, test_vma_replace)
{
	struct uk_vas *vas = uk_vmem_test_vas_init();
	__vaddr_t va1 = UK_PAGING_VADDR_ANY, va2;
	__sz len;
	int rc;

	/* Reserve a large area */
	rc = uk_vma_reserve(uk_vas_get_active(), &va1, VMEM_TEST_HEAPSIZE);
	uk_vmem_test_bug_on(rc != 0);

	UK_TEST_EXPECT_ZERO(uk_vmem_test_check_vas(vas,
		(struct uk_vmem_test_vma[]){
			{va1, va1 + VMEM_TEST_HEAPSIZE, 0},
		}, 1));

	len = uk_vmem_test_probe_r(va1, 1);
	UK_TEST_EXPECT_ZERO(len);

	/* Initial 1MB mapping */
	rc = uk_vma_map_anon(uk_vas_get_active(),
			     &va1, 0x100000, PROT_RW,
			     UK_VMA_MAP_REPLACE, "HEAP");

	va2 = va1 + 0x100000;
	UK_TEST_EXPECT_ZERO(uk_vmem_test_check_vas(vas,
		(struct uk_vmem_test_vma[]){
			{va1, va2, PROT_RW},
			{va2, va1 + VMEM_TEST_HEAPSIZE, 0},
		}, 2));

	len = uk_vmem_test_probe_r(va1, va2 - va1);
	UK_TEST_EXPECT_SNUM_EQ(len, va2 - va1);

	// Grow mapping by 2MB
	rc = uk_vma_map_anon(uk_vas_get_active(),
			     &va2, 0x200000, PROT_RW,
			     UK_VMA_MAP_REPLACE, "HEAP");

	va2 += 0x200000;
	UK_TEST_EXPECT_ZERO(uk_vmem_test_check_vas(vas,
		(struct uk_vmem_test_vma[]){
			{va1, va2, PROT_RW},
			{va2, va1 + VMEM_TEST_HEAPSIZE, 0},
		}, 2));

	len = uk_vmem_test_probe_r(va1, va2 - va1);
	UK_TEST_EXPECT_SNUM_EQ(len, va2 - va1);

	// Shrink mapping by 1MB
	va2 -= 0x100000;
	rc = uk_vma_reserve_ex(uk_vas_get_active(),
			       &va2, 0x100000, 0,
			       UK_VMA_MAP_REPLACE, NULL);

	UK_TEST_EXPECT_ZERO(uk_vmem_test_check_vas(vas,
		(struct uk_vmem_test_vma[]){
			{va1, va2, PROT_RW},
			{va2, va1 + VMEM_TEST_HEAPSIZE, 0},
		}, 2));

	len = uk_vmem_test_probe_r(va1, va2 - va1);
	UK_TEST_EXPECT_SNUM_EQ(len, va2 - va1);

	len = uk_vmem_test_probe_r(va2, 0x100000);
	UK_TEST_EXPECT_SNUM_EQ(len, 0);

	uk_vmem_test_vas_clean(vas);
}

uk_testsuite_register(ukvmem, NULL);
