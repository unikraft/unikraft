/* SPDX-License-Identifier: BSD-3-Clause */

#include <errno.h>
#include <stddef.h>
#include <uk/alloc.h>
#include <uk/assert.h>
#include <uk/falloc.h>
#include <uk/isr/string.h>
#include <uk/refcount.h>
#include <uk/vmem/vma_ops.h>
#include <uk/vmem/vma_types.h>

/* Allocate and zero backing in mapping context. Faults only install a
 * resident page; split mappings and cloned VASes retain the same object.
 */
struct shared_anon {
	struct uk_alloc *a;
	struct uk_falloc *fa;
	__atomic refs;
	__sz count;
	__paddr_t pages[];
};

struct shared_vma {
	struct uk_vma base;
	struct shared_anon *backing;
	__sz first;
};

static void shared_anon_put(struct shared_anon *b)
{
	__sz i;

	if (!uk_refcount_release(&b->refs))
		return;
	for (i = 0; i < b->count; i++)
		b->fa->ffree(b->fa, b->pages[i], 1);
	uk_free(b->a, b);
}

static void shared_ref_get(const void *object)
{
	struct shared_anon *b = (struct shared_anon *)object;

	uk_refcount_acquire(&b->refs);
}

static void shared_ref_put(const void *object)
{
	shared_anon_put((struct shared_anon *)object);
}

static void shared_ref(struct uk_vma *vma, __vaddr_t addr,
		       struct uk_vma_shared_ref *out)
{
	struct shared_vma *v = (struct shared_vma *)vma;

	out->object = v->backing;
	out->offset = v->first * UK_PAGING_PAGE_SIZE + addr - vma->start;
	out->get = shared_ref_get;
	out->put = shared_ref_put;
	shared_ref_get(out->object);
}

void uk_vma_shared_ref_get(const struct uk_vma_shared_ref *ref)
{
	if (ref->object)
		ref->get(ref->object);
}

void uk_vma_shared_ref_put(struct uk_vma_shared_ref *ref)
{
	struct uk_vma_shared_ref old = *ref;

	memset_isr(ref, 0, sizeof(*ref));
	if (old.object)
		old.put(old.object);
}

static int shared_new(struct uk_vas *vas, __vaddr_t vaddr __unused,
		      __sz len, void *data __unused,
		      unsigned long attr __unused, unsigned long *flags,
		      struct uk_vma **out)
{
	const __sz count = len / UK_PAGING_PAGE_SIZE;
	unsigned int order = UK_VMA_MAP_SIZE_TO_ORDER(*flags);
	struct uk_pagetable *pt = vas->pt;
	struct shared_anon *b;
	struct shared_vma *v;
	__vaddr_t alias;
	__paddr_t page;
	int rc;

	if (order && order != UK_PAGING_PAGE_SHIFT)
		return -ENOTSUP;
	if (!count || count > (__SZ_MAX - sizeof(*b)) / sizeof(b->pages[0]))
		return -ENOMEM;
	v = uk_malloc(vas->a, sizeof(*v));
	if (!v)
		return -ENOMEM;
	b = uk_malloc(vas->a, sizeof(*b) + count * sizeof(b->pages[0]));
	if (!b) {
		uk_free(vas->a, v);
		return -ENOMEM;
	}
	b->a = vas->a;
	b->fa = pt->fa;
	b->count = 0;
	uk_refcount_init(&b->refs, 1);
	while (b->count < count) {
		page = UK_PAGING_PADDR_ANY;
		rc = pt->fa->falloc(pt->fa, &page, 1, FALLOC_FLAG_ALIGNED);
		if (rc)
			goto fail;
		b->pages[b->count++] = page;
		alias = uk_paging_page_kmap(pt, page, 1, 0);
		if (alias == UK_PAGING_VADDR_INV) {
			rc = -ENOMEM;
			goto fail;
		}
		memset_isr((void *)alias, 0, UK_PAGING_PAGE_SIZE);
		uk_paging_page_kunmap(pt, alias, 1, 0);
	}
	v->backing = b;
	v->first = 0;
	v->base.name = NULL;
	*out = &v->base;
	return 0;
fail:
	shared_anon_put(b);
	uk_free(vas->a, v);
	return rc;
}

static int shared_fault(struct uk_vma *vma, struct uk_vm_fault *fault)
{
	struct shared_vma *v = (struct shared_vma *)vma;
	__sz i;

	UK_ASSERT(fault->type & UK_VMA_FAULT_NONPRESENT);
	if (fault->level != UK_PAGING_PAGE_LEVEL)
		return -ENOMEM;
	i = v->first + (fault->vbase - vma->start) / UK_PAGING_PAGE_SIZE;
	UK_ASSERT(i < v->backing->count);
	fault->paddr = v->backing->pages[i];
	return 0;
}

static int shared_unmap(struct uk_vma *vma, __vaddr_t addr, __sz len)
{
	/* VMA destruction follows the enclosing batch's TLB acknowledgment. */
	return uk_paging_page_unmap(vma->vas->pt, addr,
				    len / UK_PAGING_PAGE_SIZE,
				    UK_PAGING_PAGE_FLAG_KEEP_FRAMES);
}

static void shared_destroy(struct uk_vma *vma)
{
	shared_anon_put(((struct shared_vma *)vma)->backing);
}

static int shared_clone(struct uk_vma *vma, struct uk_alloc *a,
			struct uk_vma **out)
{
	struct shared_vma *old = (struct shared_vma *)vma;
	struct shared_vma *v = uk_malloc(a, sizeof(*v));

	if (!v)
		return -ENOMEM;
	v->backing = old->backing;
	v->first = old->first;
	uk_refcount_acquire(&v->backing->refs);
	*out = &v->base;
	return 0;
}

static int shared_split(struct uk_vma *vma, __vaddr_t addr,
			struct uk_vma **out)
{
	int rc = shared_clone(vma, vma->vas->a, out);

	if (!rc)
		((struct shared_vma *)*out)->first +=
			(addr - vma->start) / UK_PAGING_PAGE_SIZE;
	return rc;
}

static int shared_merge(struct uk_vma *vma, struct uk_vma *next)
{
	struct shared_vma *v = (struct shared_vma *)vma;
	struct shared_vma *n = (struct shared_vma *)next;

	return v->backing == n->backing &&
		v->first + uk_vma_len(vma) / UK_PAGING_PAGE_SIZE == n->first
		? 0 : -EPERM;
}

static int shared_advise(struct uk_vma *vma, __vaddr_t addr, __sz len,
			 unsigned long advice)
{
	if (!(advice & UK_VMA_ADV_WILLNEED) && (advice & UK_VMA_ADV_DONTNEED))
		return shared_unmap(vma, addr, len);
	return uk_vma_op_advise(vma, addr, len, advice);
}

const struct uk_vma_ops uk_vma_shared_anon_ops = {
	.new = shared_new,
	.destroy = shared_destroy,
	.clone_cow = shared_clone,
	.shared_ref = shared_ref,
	.fault = shared_fault,
	.unmap = shared_unmap,
	.split = shared_split,
	.merge = shared_merge,
	.set_attr = uk_vma_op_set_attr,
	.advise = shared_advise
};
