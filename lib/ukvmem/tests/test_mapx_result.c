/* SPDX-License-Identifier: BSD-3-Clause */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <uk/lcpu.h>
#include <uk/paging.h>
#include <uk/test.h>
#include <uk/vmem.h>

struct mapx_result_probe {
	unsigned int maps;
	unsigned int results;
	unsigned int successes;
	unsigned int skips;
	unsigned int retries;
	unsigned int errors;
	unsigned int legacy_maps;
	int action;
	__pte_t before;
	__pte_t proposed;
	bool pending;
	bool order_ok;
	bool pte_matches;
	bool irqs_masked;
	bool batch_recorded;
};

static int mapx_result_prepare(struct uk_pagetable *pt __unused,
			       __vaddr_t vaddr, __vaddr_t pt_vaddr,
			       unsigned int level, __pte_t *pte, void *arg)
{
	struct mapx_result_probe *p = arg;
	int rc;

	if (p->pending)
		p->order_ok = false;
	p->pending = true;
	p->maps++;
	p->proposed = *pte;
	rc = uk_paging_pte_read(pt_vaddr, level,
				UK_PAGING_PT_Lx_IDX(vaddr, level), &p->before);
	if (rc)
		p->pte_matches = false;
	return p->action;
}

static void mapx_result_observe(struct uk_pagetable *pt, __vaddr_t vaddr,
				__vaddr_t pt_vaddr, unsigned int level,
				int result, void *arg)
{
	struct mapx_result_probe *p = arg;
	__pte_t actual;
	int rc;

	if (!p->pending || p->results + 1 != p->maps)
		p->order_ok = false;
	p->pending = false;
	p->results++;
	p->irqs_masked &= uk_lcpu_irqs_disabled();
	if (!pt->defer_batch)
		p->batch_recorded = false;
	else if (!result && UK_PAGING_PT_Lx_PTE_PRESENT(p->before, level))
		p->batch_recorded &= pt->defer_batch->has_tlb_range;
	rc = uk_paging_pte_read(pt_vaddr, level,
				UK_PAGING_PT_Lx_IDX(vaddr, level), &actual);
	if (rc || actual != (result ? p->before : p->proposed))
		p->pte_matches = false;
	if (!result)
		p->successes++;
	else if (result == UK_PAGING_PAGE_MAPX_ESKIP)
		p->skips++;
	else if (result == UK_PAGING_PAGE_MAPX_ETOOBIG)
		p->retries++;
	else
		p->errors++;
}

static int mapx_result_legacy(struct uk_pagetable *pt __unused,
			      __vaddr_t vaddr __unused,
			      __vaddr_t pt_vaddr __unused,
			      unsigned int level __unused,
			      __pte_t *pte __unused, void *arg)
{
	struct mapx_result_probe *p = arg;

	p->legacy_maps++;
	return UK_PAGING_PAGE_MAPX_ESKIP;
}

UK_TESTCASE(paging_results, applied_skipped_and_failed)
{
	struct uk_vas *kernel = uk_vas_get_kernel();
	const __sz span = UK_PAL_PAGE_Lx_SIZE(UK_PAL_PT_LEVELS - 1);
	const unsigned long flags = UK_PAGING_PAGE_FLAG_FORCE_SIZE |
		UK_PAGING_PAGE_FLAG_SIZE(UK_PAGING_PAGE_LEVEL);
	struct uk_pagetable pt;
	struct mapx_result_probe probe = {
		.order_ok = true, .pte_matches = true,
		.irqs_masked = true, .batch_recorded = true
	};
	struct uk_paging_page_mapx mapper = {
		.map = mapx_result_prepare, .ctx = &probe
	};
	int rc;

	rc = uk_paging_pt_clone_shared(&pt, kernel->pt, 0, span);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		return;
	rc = uk_paging_page_mapx_with_result(&pt, span, UK_PAGING_PADDR_ANY,
					     2,
					     UK_PAGING_PAGE_ATTR_PROT_RW,
					     flags, &mapper,
					     mapx_result_observe);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto free_pt;
	UK_TEST_EXPECT_SNUM_EQ(probe.maps, 2);
	UK_TEST_EXPECT_SNUM_EQ(probe.results, 2);
	probe.action = UK_PAGING_PAGE_MAPX_ESKIP;
	rc = uk_paging_page_mapx_with_result(&pt, span, 0, 2,
					     UK_PAGING_PAGE_ATTR_PROT_READ,
					     flags, &mapper,
					     mapx_result_observe);
	UK_TEST_EXPECT_ZERO(rc);
	probe.action = -EIO;
	rc = uk_paging_page_mapx_with_result(&pt, span, 0, 1,
					     UK_PAGING_PAGE_ATTR_PROT_READ,
					     flags, &mapper,
					     mapx_result_observe);
	UK_TEST_EXPECT_SNUM_EQ(rc, -EIO);
	probe.action = UK_PAGING_PAGE_MAPX_ETOOBIG;
	rc = uk_paging_page_mapx_with_result(&pt, span, 0, 1,
					     UK_PAGING_PAGE_ATTR_PROT_READ,
					     flags, &mapper,
					     mapx_result_observe);
	UK_TEST_EXPECT_SNUM_EQ(rc, -ENOMEM);
	probe.action = 0;
	rc = uk_paging_page_mapx_with_result(&pt, span, 0, 1,
					     UK_PAGING_PAGE_ATTR_PROT_READ,
					     flags, &mapper,
					     mapx_result_observe);
	UK_TEST_EXPECT_ZERO(rc);
	UK_TEST_EXPECT_SNUM_EQ(probe.maps, 7);
	UK_TEST_EXPECT_SNUM_EQ(probe.results, 7);
	UK_TEST_EXPECT_SNUM_EQ(probe.successes, 3);
	UK_TEST_EXPECT_SNUM_EQ(probe.skips, 2);
	UK_TEST_EXPECT_SNUM_EQ(probe.errors, 1);
	UK_TEST_EXPECT_SNUM_EQ(probe.retries, 1);
	UK_TEST_EXPECT(!probe.pending);
	UK_TEST_EXPECT(probe.order_ok);
	UK_TEST_EXPECT(probe.pte_matches);
	UK_TEST_EXPECT(probe.irqs_masked);
	UK_TEST_EXPECT(probe.batch_recorded);
	rc = uk_paging_page_mapx_with_result(&pt, span, 0, 1,
					     UK_PAGING_PAGE_ATTR_PROT_RW,
					     flags, NULL,
					     mapx_result_observe);
	UK_TEST_EXPECT_SNUM_EQ(rc, -EINVAL);
	mapper.map = mapx_result_legacy;
	rc = uk_paging_page_mapx(&pt, span, 0, 1,
				 UK_PAGING_PAGE_ATTR_PROT_RW, flags, &mapper);
	UK_TEST_EXPECT_ZERO(rc);
	UK_TEST_EXPECT_SNUM_EQ(probe.legacy_maps, 1);
	UK_TEST_EXPECT_SNUM_EQ(probe.results, 7);
free_pt:
	UK_TEST_EXPECT_ZERO(uk_paging_pt_free(&pt, 0));
}

_UK_TESTSUITE(paging_results, NULL);

static void __constructor run_paging_results(void)
{
	uk_testsuite_run(&_UK_TESTSUITE_NAME(paging_results));
}
