/* SPDX-License-Identifier: BSD-3-Clause */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <uk/alloc.h>
#include <uk/essentials.h>
#include <uk/falloc.h>
#include <uk/isr/string.h>
#include <uk/lcpu.h>
#include <uk/print.h>
#include <uk/test.h>
#include <uk/vmem.h>

#include "../cow.h"

struct cow_allocator {
	struct uk_falloc fa;
	struct uk_falloc *real;
	unsigned int allocated;
	unsigned int freed;
	bool fail;
};

struct cow_release_probe {
	unsigned int calls;
	bool irqs_enabled;
};

struct cow_metadata_probe {
	struct uk_alloc *real;
	unsigned int allocated;
	unsigned int freed;
	bool fail;
};

static void *cow_metadata_alloc(struct uk_alloc *a, __sz size)
{
	struct cow_metadata_probe *probe = (void *)a->priv;
	void *ptr;

	if (probe->fail)
		return NULL;
	ptr = uk_malloc(probe->real, size);
	if (ptr)
		probe->allocated++;
	return ptr;
}

static void cow_metadata_free(struct uk_alloc *a, void *ptr)
{
	struct cow_metadata_probe *probe = (void *)a->priv;

	probe->freed++;
	uk_free(probe->real, ptr);
}

static int cow_test_alloc(struct uk_falloc *fa, __paddr_t *paddr,
			  unsigned long frames, unsigned long flags)
{
	struct cow_allocator *a = __containerof(fa, struct cow_allocator, fa);
	int rc;

	if (a->fail) {
		*paddr = UK_PAGING_PADDR_INV;
		return -ENOMEM;
	}
	rc = a->real->falloc(a->real, paddr, frames, flags);
	if (!rc)
		a->allocated += frames;
	return rc;
}

static int cow_test_free(struct uk_falloc *fa, __paddr_t paddr,
			 unsigned long frames)
{
	struct cow_allocator *a = __containerof(fa, struct cow_allocator, fa);
	int rc;

	rc = a->real->ffree(a->real, paddr, frames);
	if (!rc)
		a->freed += frames;
	return rc;
}

static void cow_test_allocator(struct cow_allocator *a, struct uk_falloc *real)
{
	memset_isr(a, 0, sizeof(*a));
	a->real = real;
	a->fa.falloc = cow_test_alloc;
	a->fa.ffree = cow_test_free;
}

static void cow_test_released(void *arg)
{
	struct cow_release_probe *probe = arg;

	probe->calls++;
	probe->irqs_enabled = !uk_lcpu_irqs_disabled();
}

UK_TESTCASE(cow_frames, reverse_retirement_order)
{
	struct uk_vas *kernel = uk_vas_get_active();
	struct vmem_cow_ref first = { 0 }, second = { 0 };
	struct uk_paging_free_batch first_batch, second_batch;
	struct cow_release_probe probe = { 0 };
	struct cow_allocator a;
	int rc;

	cow_test_allocator(&a, kernel->pt->fa);
	rc = vmem_cow_ref_alloc(&first, kernel->a, &a.fa,
				cow_test_released, &probe);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		return;
	UK_TEST_EXPECT(vmem_cow_ref_exclusive(&first));
	vmem_cow_ref_clone(&second, &first, cow_test_released, &probe);
	UK_TEST_EXPECT(!vmem_cow_ref_exclusive(&first));
	UK_TEST_EXPECT_SNUM_EQ(vmem_cow_ref_paddr(&first),
			       vmem_cow_ref_paddr(&second));

	uk_paging_free_batch_init(&first_batch, kernel->pt);
	uk_paging_free_batch_init(&second_batch, kernel->pt);
	vmem_cow_ref_release(&second, &second_batch);
	/* A pending stale translation still prevents exclusive reuse. */
	UK_TEST_EXPECT(!vmem_cow_ref_exclusive(&first));
	vmem_cow_ref_release(&first, &first_batch);
	UK_TEST_EXPECT_ZERO(probe.calls);
	UK_TEST_EXPECT_ZERO(a.freed);

	/* Finish the last queued reference first, retaining the other batch. */
	uk_paging_free_batch_finish(&first_batch);
	UK_TEST_EXPECT_SNUM_EQ(probe.calls, 1);
	UK_TEST_EXPECT_ZERO(a.freed);
	UK_TEST_EXPECT_NULL(first.page);
	uk_paging_free_batch_finish(&second_batch);
	UK_TEST_EXPECT_SNUM_EQ(probe.calls, 2);
	UK_TEST_EXPECT_SNUM_EQ(a.freed, 1);
	UK_TEST_EXPECT_NULL(second.page);
	UK_TEST_EXPECT(probe.irqs_enabled);
	uk_paging_free_batch_finish(&first_batch);
	uk_paging_free_batch_finish(&second_batch);
	UK_TEST_EXPECT_SNUM_EQ(probe.calls, 2);
	UK_TEST_EXPECT_SNUM_EQ(a.allocated, a.freed);
}

UK_TESTCASE(cow_frames, regain_exclusive_after_completion)
{
	struct uk_vas *kernel = uk_vas_get_active();
	struct vmem_cow_ref first = { 0 }, second = { 0 };
	struct uk_paging_free_batch batch;
	struct cow_allocator a;
	int rc;

	cow_test_allocator(&a, kernel->pt->fa);
	rc = vmem_cow_ref_alloc(&first, kernel->a, &a.fa, NULL, NULL);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		return;
	vmem_cow_ref_clone(&second, &first, NULL, NULL);
	uk_paging_free_batch_init(&batch, kernel->pt);
	vmem_cow_ref_release(&second, &batch);
	UK_TEST_EXPECT(!vmem_cow_ref_exclusive(&first));
	uk_paging_free_batch_finish(&batch);
	UK_TEST_EXPECT(vmem_cow_ref_exclusive(&first));
	UK_TEST_EXPECT_ZERO(a.freed);
	vmem_cow_ref_release(&first, NULL);
	UK_TEST_EXPECT_SNUM_EQ(a.allocated, a.freed);
}

UK_TESTCASE(cow_frames, copy_contents_and_allocation_failure)
{
	struct uk_vas *kernel = uk_vas_get_active();
	struct vmem_cow_ref source = { 0 }, copy = { 0 }, failed = { 0 };
	struct cow_release_probe probe = { 0 };
	struct cow_allocator a;
	__vaddr_t from, to;
	unsigned char *bytes;
	unsigned int i;
	bool contents = true;
	int rc;

	cow_test_allocator(&a, kernel->pt->fa);
	rc = vmem_cow_ref_alloc(&source, kernel->a, &a.fa,
				cow_test_released, &probe);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		return;
	from = uk_paging_page_kmap(kernel->pt,
				   vmem_cow_ref_paddr(&source), 1, 0);
	UK_TEST_EXPECT(from != UK_PAGING_VADDR_INV);
	if (from == UK_PAGING_VADDR_INV)
		goto out_source;
	bytes = (unsigned char *)from;
	for (i = 0; i < UK_PAGING_PAGE_SIZE; i++)
		bytes[i] = (unsigned char)(i ^ (i >> 8));
	uk_paging_page_kunmap(kernel->pt, from, 1, 0);

	a.fail = true;
	rc = vmem_cow_ref_copy(&failed, &source, kernel->pt,
			       cow_test_released, &probe);
	UK_TEST_EXPECT_SNUM_EQ(rc, -ENOMEM);
	UK_TEST_EXPECT_NULL(failed.page);
	UK_TEST_EXPECT_ZERO(probe.calls);
	UK_TEST_EXPECT_SNUM_EQ(a.allocated, 1);
	UK_TEST_EXPECT_ZERO(a.freed);
	a.fail = false;
	rc = vmem_cow_ref_copy(&copy, &source, kernel->pt,
			       cow_test_released, &probe);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out_source;
	UK_TEST_EXPECT(vmem_cow_ref_paddr(&source) !=
		       vmem_cow_ref_paddr(&copy));
	UK_TEST_EXPECT(vmem_cow_ref_exclusive(&source));
	UK_TEST_EXPECT(vmem_cow_ref_exclusive(&copy));
	from = uk_paging_page_kmap(kernel->pt,
				   vmem_cow_ref_paddr(&source), 1, 0);
	to = uk_paging_page_kmap(kernel->pt, vmem_cow_ref_paddr(&copy), 1, 0);
	UK_TEST_EXPECT(from != UK_PAGING_VADDR_INV);
	UK_TEST_EXPECT(to != UK_PAGING_VADDR_INV);
	if (from != UK_PAGING_VADDR_INV && to != UK_PAGING_VADDR_INV) {
		UK_TEST_EXPECT_BYTES_EQ((void *)from, (void *)to,
					UK_PAGING_PAGE_SIZE);
		memset_isr((void *)to, 0xa5, UK_PAGING_PAGE_SIZE);
		bytes = (unsigned char *)from;
		for (i = 0; i < UK_PAGING_PAGE_SIZE; i++)
			if (bytes[i] != (unsigned char)(i ^ (i >> 8)))
				contents = false;
		UK_TEST_EXPECT(contents);
	}
	if (to != UK_PAGING_VADDR_INV)
		uk_paging_page_kunmap(kernel->pt, to, 1, 0);
	if (from != UK_PAGING_VADDR_INV)
		uk_paging_page_kunmap(kernel->pt, from, 1, 0);
	vmem_cow_ref_release(&copy, NULL);
out_source:
	vmem_cow_ref_release(&source, NULL);
	UK_TEST_EXPECT_SNUM_EQ(a.allocated, a.freed);
	UK_TEST_EXPECT_SNUM_EQ(probe.calls, a.allocated);
}

UK_TESTCASE(cow_frames, metadata_failure_and_last_release)
{
	struct uk_vas *kernel = uk_vas_get_active();
	struct vmem_cow_ref source = { 0 }, copy = { 0 };
	struct uk_paging_free_batch batch;
	struct cow_metadata_probe *probe;
	struct uk_alloc *metadata;
	struct cow_allocator frames;
	int rc;

	/* Do not register this test allocator globally. */
	metadata = uk_malloc(kernel->a, sizeof(*metadata) + sizeof(*probe));
	UK_TEST_EXPECT_NOT_NULL(metadata);
	if (!metadata)
		return;
	memset_isr(metadata, 0, sizeof(*metadata) + sizeof(*probe));
	probe = (void *)metadata->priv;
	probe->real = kernel->a;
	metadata->malloc = cow_metadata_alloc;
	metadata->free = cow_metadata_free;
	cow_test_allocator(&frames, kernel->pt->fa);

	probe->fail = true;
	rc = vmem_cow_ref_alloc(&source, metadata, &frames.fa, NULL, NULL);
	UK_TEST_EXPECT_SNUM_EQ(rc, -ENOMEM);
	UK_TEST_EXPECT_NULL(source.page);
	UK_TEST_EXPECT_ZERO(probe->allocated);
	UK_TEST_EXPECT_ZERO(frames.allocated);
	probe->fail = false;
	frames.fail = true;
	rc = vmem_cow_ref_alloc(&source, metadata, &frames.fa, NULL, NULL);
	UK_TEST_EXPECT_SNUM_EQ(rc, -ENOMEM);
	UK_TEST_EXPECT_NULL(source.page);
	UK_TEST_EXPECT_SNUM_EQ(probe->allocated, 1);
	UK_TEST_EXPECT_SNUM_EQ(probe->freed, 1);
	UK_TEST_EXPECT_ZERO(frames.allocated);
	frames.fail = false;
	rc = vmem_cow_ref_alloc(&source, metadata, &frames.fa, NULL, NULL);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out_metadata;

	probe->fail = true;
	rc = vmem_cow_ref_copy(&copy, &source, kernel->pt, NULL, NULL);
	UK_TEST_EXPECT_SNUM_EQ(rc, -ENOMEM);
	UK_TEST_EXPECT_NULL(copy.page);
	UK_TEST_EXPECT(vmem_cow_ref_exclusive(&source));
	UK_TEST_EXPECT_SNUM_EQ(frames.allocated, 1);
	probe->fail = false;
	vmem_cow_ref_clone(&copy, &source, NULL, NULL);
	/* Sharing must not allocate a second ownership record. */
	UK_TEST_EXPECT_SNUM_EQ(probe->allocated, 2);
	uk_paging_free_batch_init(&batch, kernel->pt);
	vmem_cow_ref_release(&copy, &batch);
	vmem_cow_ref_release(&source, NULL);
	UK_TEST_EXPECT_SNUM_EQ(probe->freed, 1);
	UK_TEST_EXPECT_ZERO(frames.freed);
	uk_paging_free_batch_finish(&batch);
	UK_TEST_EXPECT_SNUM_EQ(probe->allocated, probe->freed);
	UK_TEST_EXPECT_SNUM_EQ(frames.allocated, frames.freed);
out_metadata:
	uk_free(kernel->a, metadata);
}

_UK_TESTSUITE(cow_frames, NULL);

static void __constructor run_cow_frames(void)
{
	uk_testsuite_run(&_UK_TESTSUITE_NAME(cow_frames));
}
