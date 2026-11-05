/* SPDX-License-Identifier: BSD-3-Clause */

#include <errno.h>
#include <uk/list.h>
#include <uk/paging.h>
#include <uk/test.h>
#include <uk/vmem.h>

struct sync_probe {
	struct uk_vma vma;
	__vaddr_t address;
	__sz length;
	unsigned int calls;
	int result;
};

static int probe_sync(struct uk_vma *vma, __vaddr_t address, __sz length)
{
	struct sync_probe *probe = (struct sync_probe *)vma;

	probe->address = address;
	probe->length = length;
	probe->calls++;
	return probe->result;
}

static const struct uk_vma_ops sync_ops = { .sync = probe_sync };
static const struct uk_vma_ops anonymous_ops;

UK_TESTCASE(vma_sync, range_callbacks_and_failures)
{
	const __sz page = UK_PAGING_PAGE_SIZE;
	const __vaddr_t base = CONFIG_LIBUKVMEM_DEFAULT_BASE;
	struct uk_vas vas = { 0 };
	struct sync_probe first = {
		.vma = { .start = base, .end = base + 2 * page,
			 .ops = &sync_ops, .vas = &vas }
	};
	struct sync_probe second = {
		.vma = { .start = base + 2 * page, .end = base + 4 * page,
			 .ops = &sync_ops, .vas = &vas }
	};

	/* Metadata only: do not change the active address space. */
	UK_INIT_LIST_HEAD(&vas.vma_list);
	uk_list_add_tail(&first.vma.vma_list, &vas.vma_list);
	uk_list_add_tail(&second.vma.vma_list, &vas.vma_list);
	UK_TEST_EXPECT_ZERO(uk_vma_sync(&vas, base + page, 2 * page));
	UK_TEST_EXPECT_SNUM_EQ(first.calls, 1);
	UK_TEST_EXPECT_SNUM_EQ(first.address, base + page);
	UK_TEST_EXPECT_SNUM_EQ(first.length, page);
	UK_TEST_EXPECT_SNUM_EQ(second.calls, 1);
	UK_TEST_EXPECT_SNUM_EQ(second.address, base + 2 * page);
	UK_TEST_EXPECT_SNUM_EQ(second.length, page);

	/* Reject a hole before calling either backing store. */
	second.vma.start += page;
	first.calls = 0;
	second.calls = 0;
	UK_TEST_EXPECT_SNUM_EQ(uk_vma_sync(&vas, base, 4 * page), -ENOENT);
	UK_TEST_EXPECT_ZERO(first.calls);
	UK_TEST_EXPECT_ZERO(second.calls);
	second.vma.start -= page;

	/* Propagate the original backing-store failure and stop traversal. */
	first.result = -ENOSPC;
	UK_TEST_EXPECT_SNUM_EQ(uk_vma_sync(&vas, base, 4 * page), -ENOSPC);
	UK_TEST_EXPECT_SNUM_EQ(first.calls, 1);
	UK_TEST_EXPECT_ZERO(second.calls);

	first.vma.ops = &anonymous_ops;
	second.vma.ops = &anonymous_ops;
	UK_TEST_EXPECT_ZERO(uk_vma_sync(&vas, base, 4 * page));
	UK_TEST_EXPECT_SNUM_EQ(uk_vma_sync(&vas, base + 1, page), -EINVAL);
	UK_TEST_EXPECT_SNUM_EQ(uk_vma_sync(&vas, base, page - 1), -EINVAL);
	const __vaddr_t last_page = __VADDR_MAX & ~(page - 1);

	UK_TEST_EXPECT_SNUM_EQ(uk_vma_sync(&vas, last_page, page), -ENOMEM);
	UK_TEST_EXPECT_ZERO(uk_vma_sync(&vas, base, 0));
}

/* Run before the global statistics initcall at late priority nine. */
uk_testsuite_late_prio(vma_sync, NULL, 8);
