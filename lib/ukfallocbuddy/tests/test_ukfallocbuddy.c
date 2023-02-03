#include <uk/test.h>
#include <uk/fallocbuddy.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct test_ukfallocbuddy {

	struct uk_falloc *fa;
	__paddr_t start;
	__sz len;
	unsigned long pages;
	__sz fa_meta_size;
	void *start_zone;
	void *fa_meta;
};


static struct uk_falloc *init_new_fb_alloc(void *fa_meta, void *start_zone,
	unsigned long len, __paddr_t *new_start, __sz fa_meta_size,
	unsigned long *pages)
{
	__vaddr_t dm_off;
	__paddr_t start = (__paddr_t)start_zone;
	struct uk_falloc *fa;

	UK_ASSERT(fa = malloc(uk_fallocbuddy_size()));

	UK_ASSERT(!uk_fallocbuddy_init(fa));
	start = PAGE_ALIGN_UP(start + fa_meta_size);
	*pages = (len - fa_meta_size) >> PAGE_SHIFT;
	dm_off = start;
	UK_ASSERT(!fa->addmem(fa, fa_meta, start, *pages, dm_off));
	*new_start = start;
	return fa;
}

static struct test_ukfallocbuddy *initialize_parameters(__sz len)
{
	struct test_ukfallocbuddy *test_attributes =
(struct test_ukfallocbuddy *) malloc(sizeof(struct test_ukfallocbuddy));

	UK_ASSERT(test_attributes);

	test_attributes->len = len;
	test_attributes->pages = len >> PAGE_SHIFT;
	test_attributes->fa_meta_size =
		uk_fallocbuddy_metadata_size(test_attributes->pages);
	test_attributes->start_zone = malloc(len);
	UK_ASSERT(test_attributes->start_zone);
	test_attributes->fa_meta = malloc(test_attributes->fa_meta_size);
	UK_ASSERT(test_attributes->fa_meta_size);
	test_attributes->fa = init_new_fb_alloc(test_attributes->fa_meta,
						test_attributes->start_zone,
						test_attributes->len,
						&(test_attributes->start),
						test_attributes->fa_meta_size,
						&(test_attributes->pages));
	UK_ASSERT(test_attributes->fa);

	return test_attributes;

}

static void free_memory(struct test_ukfallocbuddy *test_attr)
{
	free(test_attr->start_zone);
	free(test_attr->fa_meta);
	free(test_attr->fa);
	free(test_attr);
}

/**
 * Test if the init function works
 */
UK_TESTCASE(ukfallocbuddy, test_init)
{
	struct uk_falloc *fa;

	UK_ASSERT(fa = malloc(uk_fallocbuddy_size()));
	UK_TEST_EXPECT_SNUM_EQ(uk_fallocbuddy_init(fa), 0);

	free(fa);
}

/**
 * Test if the addmem functionality works
 */
UK_TESTCASE(ukfallocbuddy, test_add_memory)
{
	__sz len = 10000;
	void *start_zone = malloc(len);
	__vaddr_t dm_off;
	__paddr_t start = (__paddr_t)start_zone;
	unsigned long pages = len >> PAGE_SHIFT;
	__sz fa_meta_size = uk_fallocbuddy_metadata_size(pages);
	void *fa_meta = malloc(fa_meta_size);
	struct uk_falloc *fa = malloc(uk_fallocbuddy_size());
	int addmem_result;

	uk_fallocbuddy_init(fa);

	start = PAGE_ALIGN_UP(start + fa_meta_size);
	pages = (len - fa_meta_size) >> PAGE_SHIFT;
	dm_off = start;
	addmem_result = fa->addmem(fa, fa_meta, start, pages, dm_off);
	UK_TEST_EXPECT_SNUM_EQ(addmem_result, 0);

	free(start_zone);
	free(fa_meta);
	free(fa);
}

/**
 * Test variables have correct values
 */
UK_TESTCASE(ukfallocbuddy, test_check_variables)
{
	struct test_ukfallocbuddy *test = initialize_parameters(10000);

	UK_TEST_EXPECT_SNUM_EQ(test->fa->free_memory, 8192);
	UK_TEST_EXPECT_SNUM_EQ(test->fa->total_memory, 8192);

	free_memory(test);
}

/**
 * Test a simple allocation
 */
UK_TESTCASE(ukfallocbuddy, test_simple_falloc)
{
	struct test_ukfallocbuddy *test = initialize_parameters(10000);
	int falloc_result;

	falloc_result = test->fa->falloc(test->fa, (__paddr_t *)test->start,
						1, 0);
	UK_TEST_EXPECT_SNUM_EQ(falloc_result, 0);

	free_memory(test);
}

/**
 * Test if alloc_from_range functionality works
 */
UK_TESTCASE(ukfallocbuddy, test_alloc_from_range)
{
	struct test_ukfallocbuddy *test = initialize_parameters(50000);
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

	free_memory(test);
}

/**
 * Test allocation with too many pages
 */
UK_TESTCASE(ukfallocbuddy, test_alloc_large)
{
	struct test_ukfallocbuddy *test = initialize_parameters(50000);
	int falloc_result;

	falloc_result = test->fa->falloc(test->fa, (__paddr_t *)test->start,
						600, 0);
	UK_TEST_EXPECT_SNUM_EQ(falloc_result, -EFAULT);

	free_memory(test);
}

/**
 * Test allocation over an already allocated zone
 */
UK_TESTCASE(ukfallocbuddy, test_alloc_over)
{
	struct test_ukfallocbuddy *test = initialize_parameters(50000);
	int falloc_from_range_result;
	__paddr_t end;

	end = test->start + 8192;
	test->fa->falloc(test->fa, (__paddr_t *)test->start, 2, 0);
	falloc_from_range_result = test->fa->falloc_from_range(test->fa,
					(__paddr_t *)test->start, 2, 0,
					test->start, end);

	UK_TEST_EXPECT_SNUM_EQ(falloc_from_range_result, -ENOMEM);

	free_memory(test);
}

/**
 * Test allocation of two frames between an already allocated one
 */
UK_TESTCASE(ukfallocbuddy, test_alloc_over_2)
{
	struct test_ukfallocbuddy *test = initialize_parameters(50000);
	int falloc_from_range_result;

	test->fa->falloc_from_range(test->fa, (__paddr_t *)test->start, 1, 0,
					test->start + 4097,
					test->start + 8192);
	falloc_from_range_result = test->fa->falloc_from_range(test->fa,
			(__paddr_t *)test->start, 2, 0, test->start,
			test->start + 12288);
	UK_TEST_EXPECT_SNUM_EQ(falloc_from_range_result, 0);

	free_memory(test);
}

uk_testsuite_register(ukfallocbuddy, NULL);
