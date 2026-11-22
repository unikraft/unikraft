/* SPDX-License-Identifier: BSD-3-Clause */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <uk/alloc.h>
#include <uk/alloc_impl.h>
#include <uk/falloc.h>
#include <uk/isr/string.h>
#include <uk/print.h>
#include <uk/test.h>
#include <uk/vmem/process.h>
#include <uk/vmem/vma_types.h>

struct process_test_alloc {
	struct uk_alloc *real;
	unsigned long live;
	unsigned int calls;
	unsigned int left;
	bool fail;
};

static void *process_test_palloc(struct uk_alloc *a, unsigned long pages)
{
	struct process_test_alloc *p = (void *)a->priv;
	void *ptr;

	p->calls++;
	if (p->fail) {
		if (!p->left)
			return NULL;
		p->left--;
	}
	ptr = uk_palloc(p->real, pages);
	if (ptr)
		p->live += pages;
	return ptr;
}

static void process_test_pfree(struct uk_alloc *a, void *ptr,
			       unsigned long pages)
{
	struct process_test_alloc *p = (void *)a->priv;

	p->live -= pages;
	uk_pfree(p->real, ptr, pages);
}

static struct uk_alloc *process_test_allocator(void)
{
	struct uk_alloc *real = uk_vas_get_kernel()->a;
	struct process_test_alloc *p;
	struct uk_alloc *a;

	a = uk_malloc(real, sizeof(*a) + sizeof(*p));
	if (!a)
		return NULL;
	memset_isr(a, 0, sizeof(*a) + sizeof(*p));
	p = (void *)a->priv;
	p->real = real;
	a->malloc = uk_malloc_ifpages;
	a->free = uk_free_ifpages;
	a->palloc = process_test_palloc;
	a->pfree = process_test_pfree;
	return a;
}

static int process_test_read(struct uk_vas *vas, __vaddr_t address,
			     __u32 *value)
{
	unsigned int level = UK_PAGING_PAGE_LEVEL;
	__vaddr_t mapped;
	__paddr_t paddr;
	__pte_t pte;
	int rc;

	rc = uk_vas_fault(vas, address, UK_VMA_FAULT_READ);
	if (rc)
		return rc;
	rc = uk_paging_pt_walk(vas->pt, address, &level, NULL, &pte);
	if (rc)
		return rc;
	if (!UK_PAGING_PT_Lx_PTE_PRESENT(pte, level) ||
	    level != UK_PAGING_PAGE_LEVEL)
		return -EIO;
	paddr = UK_PAGING_PT_Lx_PTE_PADDR(pte, level);
	mapped = uk_paging_page_kmap(vas->pt, paddr, 1, 0);
	if (mapped == UK_PAGING_VADDR_INV)
		return -ENOMEM;
	*value = *(__u32 *)(mapped + address % UK_PAGING_PAGE_SIZE);
	uk_paging_page_kunmap(vas->pt, mapped, 1, 0);
	return 0;
}

static unsigned long process_test_aliases(void)
{
	struct uk_pagetable *pt = uk_vas_get_kernel()->pt;
	unsigned long aliases = 0;
	unsigned int level;

	for (level = 0; level < UK_PAGING_PT_LEVELS; level++)
		aliases += pt->nr_lx_pages[level] *
			(UK_PAGING_PAGE_Lx_SIZE(level) / UK_PAGING_PAGE_SIZE);
	return aliases;
}

UK_TESTCASE(process_spaces, private_clone_and_parent_exit)
{
	struct uk_vas *parent = NULL, *child = NULL;
	struct uk_vas *saved = uk_vas_get_active();
	struct uk_vas *kernel = uk_vas_get_kernel();
	struct uk_pagetable *owner;
	struct uk_alloc *a = process_test_allocator();
	struct process_test_alloc *p;
	unsigned int users = kernel->pt->shared_users;
	__vaddr_t address = UK_PAGING_VADDR_ANY;
	__vaddr_t forbidden = CONFIG_LIBUKVMEM_DEFAULT_BASE;
	__u32 value = 0;
	int rc;

	UK_TEST_EXPECT_NOT_NULL(a);
	if (!a)
		return;
	p = (void *)a->priv;
	rc = uk_vas_create_process(a, &parent);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	UK_TEST_EXPECT_SNUM_EQ(parent->range_start, UK_VAS_PROCESS_BASE);
	UK_TEST_EXPECT_SNUM_EQ(parent->range_end, UK_VAS_PROCESS_END);
	UK_TEST_EXPECT_SNUM_EQ(uk_refcount_read(&parent->refs), 1);
	owner = uk_paging_pt_get_owner(parent->pt, (__vaddr_t)&rc);
	UK_TEST_EXPECT_PTR_EQ(owner, kernel->pt);
	rc = uk_vma_map_anon(parent, &forbidden, UK_PAGING_PAGE_SIZE,
			     UK_PAGING_PAGE_ATTR_PROT_RW, 0, NULL);
	UK_TEST_EXPECT_SNUM_EQ(rc, -ENOMEM);
	rc = uk_vma_map_anon(parent, &address, UK_PAGING_PAGE_SIZE,
			     UK_PAGING_PAGE_ATTR_PROT_RW, 0, NULL);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	UK_TEST_EXPECT_SNUM_EQ(address, UK_VAS_PROCESS_BASE);
	UK_TEST_EXPECT_PTR_EQ(uk_paging_pt_get_owner(parent->pt, address),
			      parent->pt);
	rc = uk_vas_set_active(parent);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	__atomic_store_n((__u32 *)address, 0x13579bdf, __ATOMIC_RELAXED);
	rc = uk_vas_clone_process(parent, &child);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	UK_TEST_EXPECT_PTR_EQ(uk_vas_get_active(), parent);
	rc = uk_vas_set_active(child);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	value = __atomic_load_n((__u32 *)address, __ATOMIC_RELAXED);
	UK_TEST_EXPECT_SNUM_EQ(value, 0x13579bdf);
	__atomic_store_n((__u32 *)address, 0x2468ace0, __ATOMIC_RELAXED);
	UK_TEST_EXPECT_ZERO(process_test_read(parent, address, &value));
	UK_TEST_EXPECT_SNUM_EQ(value, 0x13579bdf);
	UK_TEST_EXPECT_ZERO(uk_vas_set_active(saved));
	uk_vas_put(parent);
	parent = NULL;
	UK_TEST_EXPECT_ZERO(process_test_read(child, address, &value));
	UK_TEST_EXPECT_SNUM_EQ(value, 0x2468ace0);
out:
	UK_TEST_EXPECT_ZERO(uk_vas_set_active(saved));
	uk_vas_put(child);
	uk_vas_put(parent);
	UK_TEST_EXPECT_ZERO(p->live);
	UK_TEST_EXPECT_SNUM_EQ(kernel->pt->shared_users, users);
	uk_free(kernel->a, a);
	uk_pr_info("PROCESS_VAS_PASS isolation\n");
}

UK_TESTCASE(process_spaces, resident_allocation_failure_rollback)
{
	struct uk_vas *kernel = uk_vas_get_kernel();
	struct uk_vas *parent = NULL, *child = NULL;
	struct uk_alloc *a = process_test_allocator();
	struct process_test_alloc *p;
	unsigned int create_calls, clone_calls, fail;
	unsigned int users = kernel->pt->shared_users;
	unsigned long free_bytes, live;
	__vaddr_t address = UK_PAGING_VADDR_ANY;
	__u32 value = 0;
	int rc;

	UK_TEST_EXPECT_NOT_NULL(a);
	if (!a)
		return;
	p = (void *)a->priv;
	UK_TEST_EXPECT_SNUM_EQ(uk_vas_create_process(NULL, &child), -EINVAL);
	UK_TEST_EXPECT_NULL(child);
	UK_TEST_EXPECT_SNUM_EQ(uk_vas_clone_process(kernel, &child), -ENOTSUP);
	UK_TEST_EXPECT_NULL(child);
	rc = uk_vas_create_process(a, &parent);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	create_calls = p->calls;
	uk_vas_put(parent);
	parent = NULL;
	free_bytes = kernel->pt->fa->free_memory;
	for (fail = 0; fail < create_calls; fail++) {
		p->fail = true;
		p->left = fail;
		rc = uk_vas_create_process(a, &child);
		p->fail = false;
		UK_TEST_EXPECT_SNUM_EQ(rc, -ENOMEM);
		UK_TEST_EXPECT_NULL(child);
		uk_vas_put(child);
		child = NULL;
		UK_TEST_EXPECT_ZERO(p->live);
		UK_TEST_EXPECT_SNUM_EQ(kernel->pt->shared_users, users);
		UK_TEST_EXPECT_SNUM_EQ(kernel->pt->fa->free_memory, free_bytes);
	}
	rc = uk_vas_create_process(a, &parent);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	rc = uk_vma_map_anon(parent, &address, UK_PAGING_PAGE_SIZE,
			     UK_PAGING_PAGE_ATTR_PROT_RW, 0, NULL);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	rc = uk_vas_store_u32(parent, address, 0x10203040);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	p->calls = 0;
	rc = uk_vas_clone_process(parent, &child);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	clone_calls = p->calls;
	uk_vas_put(child);
	child = NULL;
	live = p->live;
	free_bytes = kernel->pt->fa->free_memory;
	for (fail = 0; fail < clone_calls; fail++) {
		p->fail = true;
		p->left = fail;
		rc = uk_vas_clone_process(parent, &child);
		p->fail = false;
		UK_TEST_EXPECT_SNUM_EQ(rc, -ENOMEM);
		UK_TEST_EXPECT_NULL(child);
		uk_vas_put(child);
		child = NULL;
		UK_TEST_EXPECT_ZERO(process_test_read(parent, address, &value));
		UK_TEST_EXPECT_SNUM_EQ(value, 0x10203040);
		UK_TEST_EXPECT_SNUM_EQ(p->live, live);
		UK_TEST_EXPECT_SNUM_EQ(kernel->pt->shared_users, users + 1);
		UK_TEST_EXPECT_SNUM_EQ(kernel->pt->fa->free_memory, free_bytes);
	}
	uk_pr_info("PROCESS_VAS_PASS rollback create=%u clone=%u\n",
		   create_calls, clone_calls);
out:
	uk_vas_put(child);
	uk_vas_put(parent);
	UK_TEST_EXPECT_ZERO(p->live);
	UK_TEST_EXPECT_SNUM_EQ(kernel->pt->shared_users, users);
	uk_free(kernel->a, a);
}

static int process_test_cycle(struct uk_alloc *a, unsigned int cycle)
{
	struct uk_vas *parent = NULL, *child = NULL;
	__vaddr_t address = UK_PAGING_VADDR_ANY;
	__u32 value = 0;
	int rc;

	rc = uk_vas_create_process(a, &parent);
	if (rc)
		goto out;
	rc = uk_vma_map_anon(parent, &address, UK_PAGING_PAGE_SIZE,
			     UK_PAGING_PAGE_ATTR_PROT_RW, 0, NULL);
	if (rc)
		goto out;
	rc = uk_vas_store_u32(parent, address, cycle);
	if (rc)
		goto out;
	rc = uk_vas_clone_process(parent, &child);
	if (rc)
		goto out;
	rc = uk_vas_store_u32(child, address, cycle + 64);
	if (rc)
		goto out;
	rc = process_test_read(parent, address, &value);
	if (rc)
		goto out;
	if (value != cycle) {
		rc = -EIO;
		goto out;
	}
	uk_vas_put(parent);
	parent = NULL;
	rc = process_test_read(child, address, &value);
	if (!rc && value != cycle + 64)
		rc = -EIO;
out:
	uk_vas_put(child);
	uk_vas_put(parent);
	return rc;
}

UK_TESTCASE(process_spaces, repeated_create_clone_resource_balance)
{
	struct uk_vas *kernel = uk_vas_get_kernel();
	struct uk_alloc *a = process_test_allocator();
	struct process_test_alloc *p;
	unsigned int users = kernel->pt->shared_users;
	unsigned long first_free = 0, first_aliases = 0, free_bytes, aliases;
	unsigned int cycle;
	int rc;

	UK_TEST_EXPECT_NOT_NULL(a);
	if (!a)
		return;
	p = (void *)a->priv;
	for (cycle = 1; cycle <= 64; cycle++) {
		rc = process_test_cycle(a, cycle);
		UK_TEST_EXPECT_ZERO(rc);
		UK_TEST_EXPECT_ZERO(p->live);
		UK_TEST_EXPECT_SNUM_EQ(kernel->pt->shared_users, users);
		free_bytes = kernel->pt->fa->free_memory;
		aliases = process_test_aliases();
		if (cycle == 1) {
			first_free = free_bytes;
			first_aliases = aliases;
		}
		UK_TEST_EXPECT_SNUM_EQ(free_bytes, first_free);
		UK_TEST_EXPECT_SNUM_EQ(aliases, first_aliases);
		uk_pr_info("PROCESS_VAS_CYCLE cycle=%u free_bytes=%lu alias_frames=%lu metadata_live=%lu shared_users=%u\n",
			   cycle, free_bytes, aliases, p->live, users);
		if (rc)
			break;
	}
	uk_free(kernel->a, a);
}

_UK_TESTSUITE(process_spaces, NULL);

static void __constructor run_process_spaces(void)
{
	uk_testsuite_run(&_UK_TESTSUITE_NAME(process_spaces));
}
