/* SPDX-License-Identifier: BSD-3-Clause */

#include <errno.h>
#include <stddef.h>
#include <uk/alloc.h>
#include <uk/assert.h>
#include <uk/falloc.h>
#include <uk/isr/string.h>
#include <uk/refcount.h>

#include "cow.h"

struct vmem_cow_page {
	__atomic refs;
	struct uk_alloc *a;
	struct uk_falloc *fa;
	__paddr_t paddr;
};

static void cow_page_put(struct vmem_cow_page *page)
{
	if (!uk_refcount_release(&page->refs))
		return;

	uk_ffree(page->fa, page->paddr, 1);
	uk_free(page->a, page);
}

static void cow_ref_init(struct vmem_cow_ref *ref, struct vmem_cow_page *page,
			 void (*released)(void *), void *arg)
{
	UK_ASSERT(ref && !ref->page);
	ref->page = page;
	ref->released = released;
	ref->arg = arg;
	ref->queued = false;
}

int vmem_cow_ref_alloc(struct vmem_cow_ref *ref, struct uk_alloc *a,
		       struct uk_falloc *fa,
		       void (*released)(void *), void *arg)
{
	struct vmem_cow_page *page;
	int rc;

	UK_ASSERT(ref && !ref->page);
	UK_ASSERT(a && fa);
	page = uk_malloc(a, sizeof(*page));
	if (!page)
		return -ENOMEM;

	page->paddr = UK_PAGING_PADDR_ANY;
	rc = fa->falloc(fa, &page->paddr, 1, 0);
	if (rc) {
		uk_free(a, page);
		return rc;
	}
	page->a = a;
	page->fa = fa;
	uk_refcount_init(&page->refs, 1);
	cow_ref_init(ref, page, released, arg);
	return 0;
}

void vmem_cow_ref_clone(struct vmem_cow_ref *dst,
			const struct vmem_cow_ref *src,
			void (*released)(void *), void *arg)
{
	UK_ASSERT(src && src->page && !src->queued);
	UK_ASSERT(dst && !dst->page);
	uk_refcount_acquire(&src->page->refs);
	cow_ref_init(dst, src->page, released, arg);
}

int vmem_cow_ref_copy(struct vmem_cow_ref *dst,
		      const struct vmem_cow_ref *src, struct uk_pagetable *pt,
		      void (*released)(void *), void *arg)
{
	struct vmem_cow_ref copy = { 0 };
	__vaddr_t from, to;
	int rc;

	UK_ASSERT(src && src->page && !src->queued);
	UK_ASSERT(dst && !dst->page);
	UK_ASSERT(pt);
	rc = vmem_cow_ref_alloc(&copy, src->page->a, src->page->fa,
				NULL, NULL);
	if (rc)
		return rc;

	from = uk_paging_page_kmap(pt, src->page->paddr, 1, 0);
	if (from == UK_PAGING_VADDR_INV)
		goto err_copy;
	to = uk_paging_page_kmap(pt, copy.page->paddr, 1, 0);
	if (to == UK_PAGING_VADDR_INV) {
		uk_paging_page_kunmap(pt, from, 1, 0);
		goto err_copy;
	}
	memcpy_isr((void *)to, (const void *)from, UK_PAGING_PAGE_SIZE);
	uk_paging_page_kunmap(pt, to, 1, 0);
	uk_paging_page_kunmap(pt, from, 1, 0);
	cow_ref_init(dst, copy.page, released, arg);
	return 0;

err_copy:
	cow_page_put(copy.page);
	return -ENOMEM;
}

__paddr_t vmem_cow_ref_paddr(const struct vmem_cow_ref *ref)
{
	UK_ASSERT(ref && ref->page && !ref->queued);
	return ref->page->paddr;
}

bool vmem_cow_ref_exclusive(const struct vmem_cow_ref *ref)
{
	UK_ASSERT(ref && ref->page && !ref->queued);
	return uk_refcount_read(&ref->page->refs) == 1;
}

static void cow_ref_retired(void *arg)
{
	struct vmem_cow_ref *ref = arg;
	struct vmem_cow_page *page = ref->page;
	void (*released)(void *) = ref->released;
	void *released_arg = ref->arg;

	UK_ASSERT(page && ref->queued);
	ref->page = NULL;
	ref->queued = false;
	cow_page_put(page);
	/* released may free the object containing ref. */
	if (released)
		released(released_arg);
}

void vmem_cow_ref_release(struct vmem_cow_ref *ref,
			  struct uk_paging_free_batch *batch)
{
	UK_ASSERT(ref && ref->page && !ref->queued);
	ref->queued = true;
	if (batch)
		uk_paging_batch_defer(batch, &ref->retire,
				      cow_ref_retired, ref);
	else
		cow_ref_retired(ref);
}
