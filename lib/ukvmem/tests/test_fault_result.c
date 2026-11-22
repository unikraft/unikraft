/* SPDX-License-Identifier: BSD-3-Clause */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <uk/alloc.h>
#include <uk/essentials.h>
#include <uk/falloc.h>
#include <uk/lcpu.h>
#include <uk/paging.h>
#include <uk/test.h>
#include <uk/vmem.h>
#include <uk/vmem/vma_ops.h>

struct fault_result_probe {
	unsigned int prepared;
	unsigned int committed;
	unsigned int discarded;
	unsigned int after;
	unsigned int freed;
	unsigned int destroyed;
	unsigned int fail_at;
	int last_abort;
	int free_rc;
	bool result_irqs_masked;
	bool after_irqs_enabled;
	bool after_pte_matches;
	bool batch_matches;
	bool cleanup_order;
};

struct result_vma {
	struct uk_vma base;
	struct fault_result_probe *probe;
};

struct fault_result_op {
	struct uk_vm_fault_op op;
	struct uk_paging_deferred_call after;
	struct fault_result_probe *probe;
	struct uk_vas *vas;
	struct uk_alloc *a;
	__vaddr_t address;
	__paddr_t paddr;
};

static void fault_result_after(void *arg)
{
	struct fault_result_op *op = arg;
	struct fault_result_probe *p = op->probe;
	struct uk_alloc *a = op->a;
	unsigned int level = UK_PAGING_PAGE_LEVEL;
	__pte_t pte;
	bool present;
	int rc;

	p->after++;
	p->after_irqs_enabled &= !uk_lcpu_irqs_disabled();
	rc = uk_paging_pt_walk(op->vas->pt, op->address, &level, NULL, &pte);
	present = !rc && UK_PAGING_PT_Lx_PTE_PRESENT(pte, level);
	if (p->fail_at)
		p->after_pte_matches &= !rc && !present;
	else
		p->after_pte_matches &= present &&
			UK_PAGING_PT_Lx_PTE_PADDR(pte, level) == op->paddr;
	p->freed++;
	uk_free(a, op);
}

static void fault_result_apply(struct uk_vm_fault_op *base, int status,
			       struct uk_paging_free_batch *batch)
{
	struct fault_result_op *op = __containerof(base,
						   struct fault_result_op, op);
	struct fault_result_probe *p = op->probe;
	struct uk_alloc *a = op->a;

	p->result_irqs_masked &= uk_lcpu_irqs_disabled();
	p->batch_matches &= batch->pt == op->vas->pt;
	if (!status) {
		p->committed++;
		uk_paging_batch_defer(batch, &op->after,
				      fault_result_after, op);
	} else {
		p->discarded++;
		p->last_abort = status;
		if (op->paddr != UK_PAGING_PADDR_INV)
			p->free_rc = op->vas->pt->fa->ffree(op->vas->pt->fa,
							 op->paddr, 1);
		p->freed++;
		uk_free(a, op);
	}
}

static int fault_result_fault(struct uk_vma *base, struct uk_vm_fault *fault)
{
	struct result_vma *vma = __containerof(base,
					       struct result_vma, base);
	struct fault_result_op *op;
	int rc;

	if (!fault->operation)
		return -ENOTSUP;
	op = uk_malloc(base->vas->a, sizeof(*op));
	if (!op)
		return -ENOMEM;
	op->op.result = fault_result_apply;
	op->probe = vma->probe;
	op->vas = base->vas;
	op->a = base->vas->a;
	op->address = fault->vbase;
	op->paddr = UK_PAGING_PADDR_INV;
	*fault->operation = &op->op;
	vma->probe->prepared++;
	rc = uk_vma_anon_ops.fault(base, fault);
	if (!rc) {
		op->paddr = fault->paddr;
		if (vma->probe->prepared == vma->probe->fail_at)
			rc = -EIO;
	}
	return rc;
}

static int fault_result_new(struct uk_vas *vas,
			    __vaddr_t addr __unused, __sz len __unused,
			    void *data, unsigned long attr __unused,
			    unsigned long *flags __unused,
			    struct uk_vma **out)
{
	struct result_vma *vma = uk_malloc(vas->a, sizeof(*vma));

	if (!vma)
		return -ENOMEM;
	vma->probe = data;
	vma->base.name = NULL;
	*out = &vma->base;
	return 0;
}

static void fault_result_destroy(struct uk_vma *base)
{
	struct result_vma *vma = __containerof(base,
					       struct result_vma, base);

	vma->probe->cleanup_order &=
		vma->probe->prepared == vma->probe->freed;
	vma->probe->destroyed++;
}

static const struct uk_vma_ops fault_result_ops = {
	.new = fault_result_new,
	.destroy = fault_result_destroy,
	.fault = fault_result_fault,
	.unmap = uk_vma_op_unmap,
	.split = uk_vma_op_deny,
	.merge = uk_vma_op_deny,
	.set_attr = uk_vma_op_set_attr,
	.advise = uk_vma_op_advise
};

UK_TESTCASE(fault_results, partial_population_and_retry)
{
	struct uk_vas *kernel = uk_vas_get_kernel();
	const __sz span = UK_PAL_PAGE_Lx_SIZE(UK_PAL_PT_LEVELS - 1);
	struct uk_pagetable pt;
	struct uk_vas vas;
	struct fault_result_probe probe = {
		.result_irqs_masked = true, .after_irqs_enabled = true,
		.after_pte_matches = true, .batch_matches = true,
		.cleanup_order = true, .fail_at = 2
	};
	__vaddr_t address = span;
	int rc;

	rc = uk_paging_pt_clone_shared(&pt, kernel->pt, 0, span);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		return;
	rc = uk_vas_init(&vas, &pt, kernel->a);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto free_pt;
	/* Commit page one, then fail page two after allocating its frame. */
	rc = uk_vma_map(&vas, &address, 2 * UK_PAGING_PAGE_SIZE,
			UK_PAGING_PAGE_ATTR_PROT_RW, UK_VMA_MAP_POPULATE |
			UK_VMA_MAP_SIZE_4KB, NULL, &fault_result_ops, &probe);
	UK_TEST_EXPECT_SNUM_EQ(rc, -EIO);
	UK_TEST_EXPECT_SNUM_EQ(probe.prepared, 2);
	UK_TEST_EXPECT_SNUM_EQ(probe.committed, 1);
	UK_TEST_EXPECT_SNUM_EQ(probe.discarded, 1);
	UK_TEST_EXPECT_SNUM_EQ(probe.freed, 2);
	UK_TEST_EXPECT_SNUM_EQ(probe.after, 1);
	UK_TEST_EXPECT_SNUM_EQ(probe.last_abort, -EIO);
	UK_TEST_EXPECT_ZERO(probe.free_rc);
	UK_TEST_EXPECT_NULL(uk_vma_find(&vas, span));
	probe.fail_at = 0;
	address = span;
	rc = uk_vma_map(&vas, &address, 2 * UK_PAGING_PAGE_SIZE,
			UK_PAGING_PAGE_ATTR_PROT_RW, UK_VMA_MAP_POPULATE |
			UK_VMA_MAP_SIZE_4KB, NULL, &fault_result_ops, &probe);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto destroy_vas;
	UK_TEST_EXPECT_SNUM_EQ(probe.prepared, 4);
	UK_TEST_EXPECT_SNUM_EQ(probe.committed, 3);
	UK_TEST_EXPECT_SNUM_EQ(probe.freed, 4);
	rc = uk_vma_unmap(&vas, span, 2 * UK_PAGING_PAGE_SIZE, 0);
	UK_TEST_EXPECT_ZERO(rc);
	address = span;
	rc = uk_vma_map(&vas, &address, UK_PAGING_PAGE_SIZE,
			UK_PAGING_PAGE_ATTR_PROT_RW, UK_VMA_MAP_SIZE_4KB,
			NULL, &fault_result_ops, &probe);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto destroy_vas;
	rc = uk_vma_advise(&vas, span, UK_PAGING_PAGE_SIZE,
			   UK_VMA_ADV_WILLNEED, 0);
	UK_TEST_EXPECT_ZERO(rc);
	UK_TEST_EXPECT_SNUM_EQ(probe.prepared, 5);
	UK_TEST_EXPECT_SNUM_EQ(probe.committed, 4);
	rc = uk_vma_advise(&vas, span, UK_PAGING_PAGE_SIZE,
			   UK_VMA_ADV_WILLNEED, 0);
	UK_TEST_EXPECT_ZERO(rc);
	UK_TEST_EXPECT_SNUM_EQ(probe.prepared, 5);
	UK_TEST_EXPECT_ZERO(uk_vma_unmap(&vas, span, UK_PAGING_PAGE_SIZE, 0));
	address = span;
	rc = uk_vma_map(&vas, &address, UK_PAGING_PAGE_SIZE,
			UK_PAGING_PAGE_ATTR_PROT_RW, UK_VMA_MAP_SIZE_4KB,
			NULL, &fault_result_ops, &probe);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto destroy_vas;
	UK_TEST_EXPECT_ZERO(uk_vas_fault(&vas, span, UK_VMA_FAULT_WRITE));
	UK_TEST_EXPECT_ZERO(uk_vas_store_u32(&vas, span, 42));
	UK_TEST_EXPECT_SNUM_EQ(probe.prepared, 6);
	UK_TEST_EXPECT_SNUM_EQ(probe.committed, 5);
	UK_TEST_EXPECT_SNUM_EQ(probe.discarded, 1);
	UK_TEST_EXPECT_SNUM_EQ(probe.after, 5);
	UK_TEST_EXPECT_SNUM_EQ(probe.freed, 6);
	UK_TEST_EXPECT_ZERO(uk_vma_unmap(&vas, span, UK_PAGING_PAGE_SIZE, 0));
	UK_TEST_EXPECT_SNUM_EQ(probe.destroyed, 4);
	UK_TEST_EXPECT(probe.result_irqs_masked);
	UK_TEST_EXPECT(probe.after_irqs_enabled);
	UK_TEST_EXPECT(probe.after_pte_matches);
	UK_TEST_EXPECT(probe.batch_matches);
	UK_TEST_EXPECT(probe.cleanup_order);
destroy_vas:
	uk_vas_destroy(&vas);
free_pt:
	UK_TEST_EXPECT_ZERO(uk_paging_pt_free(&pt, 0));
}

_UK_TESTSUITE(fault_results, NULL);

static void __constructor run_fault_results(void)
{
	uk_testsuite_run(&_UK_TESTSUITE_NAME(fault_results));
}
