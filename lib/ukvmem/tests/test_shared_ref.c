/* SPDX-License-Identifier: BSD-3-Clause */

#include <errno.h>
#include <stddef.h>
#include <uk/falloc.h>
#include <uk/lcpu.h>
#include <uk/print.h>
#include <uk/test.h>
#include <uk/vmem/process.h>
#include <uk/vmem/vma_types.h>

#include "../vmem.h"

UK_TESTCASE(shared_vas_refs, word_load_is_fault_free)
{
	struct uk_vas *parent = NULL, *child = NULL;
	struct vmem_vas_transition transition = { 0 };
	struct uk_vma_shared_ref ref = { 0 };
	__vaddr_t addr = UK_VAS_PROCESS_BASE;
	__u32 value = 0x7777;
	unsigned long irqf;
	int rc;

	rc = uk_vas_create_process(uk_vas_get_kernel()->a, &parent);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		return;
	rc = uk_vma_map_anon(parent, &addr, UK_PAGING_PAGE_SIZE,
			     UK_PAGING_PAGE_ATTR_PROT_RW, 0, NULL);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	irqf = uk_lcpu_save_irqf();
	rc = uk_vas_load_u32_try(parent, addr, &value);
	uk_lcpu_restore_irqf(irqf);
	UK_TEST_EXPECT_SNUM_EQ(rc, -EAGAIN);
	UK_TEST_EXPECT_SNUM_EQ(value, 0x7777);
	rc = uk_vas_get_shared_ref(parent, addr, sizeof(value), &ref);
	UK_TEST_EXPECT_SNUM_EQ(rc, -ENOTSUP);
	UK_TEST_EXPECT_NULL(ref.object);
	rc = uk_vas_fault(parent, addr, UK_VMA_FAULT_READ);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	irqf = uk_lcpu_save_irqf();
	rc = uk_vas_load_u32_try(parent, addr, &value);
	uk_lcpu_restore_irqf(irqf);
	UK_TEST_EXPECT_ZERO(rc);
	UK_TEST_EXPECT_ZERO(value);
	irqf = uk_lcpu_save_irqf();
	rc = uk_vas_load_u32_shared_try(parent, addr, &value, &ref);
	uk_lcpu_restore_irqf(irqf);
	UK_TEST_EXPECT_ZERO(rc);
	UK_TEST_EXPECT_ZERO(value);
	UK_TEST_EXPECT_NULL(ref.object);
	UK_TEST_EXPECT_ZERO(uk_vas_store_u32(parent, addr, 0x1234));
	rc = uk_vas_clone_process(parent, &child);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	UK_TEST_EXPECT_ZERO(uk_vas_store_u32(child, addr, 0x5678));
	UK_TEST_EXPECT_ZERO(uk_vas_load_u32_try(parent, addr, &value));
	UK_TEST_EXPECT_SNUM_EQ(value, 0x1234);
	UK_TEST_EXPECT_ZERO(uk_vas_load_u32_try(child, addr, &value));
	UK_TEST_EXPECT_SNUM_EQ(value, 0x5678);
	rc = vmem_vas_transition_begin(parent, &transition);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	value = 0x7777;
	irqf = uk_lcpu_save_irqf();
	rc = uk_vas_load_u32_try(parent, addr, &value);
	uk_lcpu_restore_irqf(irqf);
	vmem_vas_transition_end(&transition);
	UK_TEST_EXPECT_SNUM_EQ(rc, -EAGAIN);
	UK_TEST_EXPECT_SNUM_EQ(value, 0x7777);
	rc = uk_vma_set_attr(parent, addr, UK_PAGING_PAGE_SIZE, 0,
			     UK_VMA_FLAG_STRICT_VMA_CHECK);
	UK_TEST_EXPECT_ZERO(rc);
	UK_TEST_EXPECT_SNUM_EQ(uk_vas_load_u32_try(parent, addr, &value),
			       -EFAULT);
	UK_TEST_EXPECT_SNUM_EQ(value, 0x7777);
	UK_TEST_EXPECT_SNUM_EQ(uk_vas_load_u32_try(child, addr + 1, &value),
			       -EINVAL);
	uk_pr_info("SHARED_VAS_REF_PASS word_load absent=1 transition=1 cow=1\n");
out:
	uk_vas_put(child);
	uk_vas_put(parent);
}

UK_TESTCASE(shared_vas_refs, identity_survives_all_mappings)
{
	struct uk_vas *kernel = uk_vas_get_kernel();
	struct uk_vas *parent = NULL, *child = NULL;
	struct uk_vma_shared_ref first = { 0 }, second = { 0 }, copy = { 0 };
	struct vmem_vas_transition transition = { 0 };
	__vaddr_t addr = UK_VAS_PROCESS_BASE, word, alias;
	__paddr_t frame;
	__u32 value;
	__pte_t pte;
	unsigned long free_before;
	unsigned int level = UK_PAGING_PAGE_LEVEL;
	int rc;

	rc = uk_vas_create_process(kernel->a, &parent);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		return;
	rc = uk_vma_map(parent, &addr, 2 * UK_PAGING_PAGE_SIZE,
			UK_PAGING_PAGE_ATTR_PROT_RW, 0, NULL,
			&uk_vma_shared_anon_ops, NULL);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	word = addr + UK_PAGING_PAGE_SIZE + sizeof(__u32);
	UK_TEST_EXPECT_ZERO(uk_vas_store_u32(parent, word, 0x5555));
	rc = uk_vas_load_u32_shared_try(parent, word, &value, &first);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	UK_TEST_EXPECT_SNUM_EQ(value, 0x5555);
	UK_TEST_EXPECT_SNUM_EQ(first.offset,
			       UK_PAGING_PAGE_SIZE + sizeof(__u32));
	rc = uk_vma_set_attr(parent, addr, UK_PAGING_PAGE_SIZE,
			     UK_PAGING_PAGE_ATTR_PROT_READ,
			     UK_VMA_FLAG_STRICT_VMA_CHECK);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	rc = uk_vas_clone_process(parent, &child);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	rc = uk_vas_get_shared_ref(child, word, sizeof(__u32), &second);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	UK_TEST_EXPECT(first.object == second.object);
	UK_TEST_EXPECT_SNUM_EQ(first.offset, second.offset);
	value = 0x7777;
	rc = uk_vas_load_u32_shared_try(child, word, &value, &first);
	UK_TEST_EXPECT_SNUM_EQ(rc, -EAGAIN);
	UK_TEST_EXPECT_SNUM_EQ(value, 0x7777);
	UK_TEST_EXPECT(first.object == second.object);
	UK_TEST_EXPECT_SNUM_EQ(first.offset, second.offset);
	rc = uk_vas_fault(child, word, UK_VMA_FAULT_READ);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	rc = uk_vas_load_u32_shared_try(child, word, &value, &copy);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	UK_TEST_EXPECT_SNUM_EQ(value, 0x5555);
	UK_TEST_EXPECT(first.object == copy.object);
	UK_TEST_EXPECT_SNUM_EQ(first.offset, copy.offset);
	uk_vma_shared_ref_put(&copy);
	rc = vmem_vas_transition_begin(child, &transition);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	rc = uk_vas_get_shared_ref(child, word, sizeof(__u32), &second);
	vmem_vas_transition_end(&transition);
	UK_TEST_EXPECT_SNUM_EQ(rc, -EAGAIN);
	UK_TEST_EXPECT(first.object == second.object);
	UK_TEST_EXPECT_SNUM_EQ(first.offset, second.offset);
	rc = uk_paging_pt_walk(parent->pt, word, &level, NULL, &pte);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc || !UK_PAGING_PT_Lx_PTE_PRESENT(pte, level))
		goto out;
	frame = UK_PAGING_PT_Lx_PTE_PADDR(pte, level);
	uk_vma_shared_ref_get(&first);
	copy = first;
	uk_vas_put(child);
	child = NULL;
	uk_vas_put(parent);
	parent = NULL;
	uk_vma_shared_ref_put(&first);
	uk_vma_shared_ref_put(&second);
	alias = uk_paging_page_kmap(kernel->pt, frame, 1, 0);
	UK_TEST_EXPECT(alias != UK_PAGING_VADDR_INV);
	if (alias == UK_PAGING_VADDR_INV)
		goto out;
	value = __atomic_load_n((__u32 *)(alias + sizeof(__u32)),
				__ATOMIC_RELAXED);
	UK_TEST_EXPECT_SNUM_EQ(value, 0x5555);
	uk_paging_page_kunmap(kernel->pt, alias, 1, 0);
	free_before = kernel->pt->fa->free_memory;
	uk_vma_shared_ref_put(&copy);
	UK_TEST_EXPECT_NULL(copy.object);
	UK_TEST_EXPECT(kernel->pt->fa->free_memory >=
		       free_before + 2 * UK_PAGING_PAGE_SIZE);
	uk_pr_info("SHARED_VAS_REF_PASS identity split=1 unmapped=1 copied=1\n");
out:
	uk_vas_put(child);
	uk_vas_put(parent);
	uk_vma_shared_ref_put(&first);
	uk_vma_shared_ref_put(&second);
	uk_vma_shared_ref_put(&copy);
}

_UK_TESTSUITE(shared_vas_refs, NULL);

static void __constructor run_shared_vas_refs(void)
{
	uk_testsuite_run(&_UK_TESTSUITE_NAME(shared_vas_refs));
}
