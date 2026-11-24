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
struct uk_vma_shared_anon {
	struct uk_alloc *a;
	struct uk_falloc *fa;
	__atomic refs;
	__sz count;
	__paddr_t pages[];
};

struct shared_vma {
	struct uk_vma base;
	struct uk_vma_shared_anon *backing;
	__sz first;
};

void uk_vma_shared_anon_release(struct uk_vma_shared_anon *b)
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
	struct uk_vma_shared_anon *b = (struct uk_vma_shared_anon *)object;

	uk_refcount_acquire(&b->refs);
}

static void shared_ref_put(const void *object)
{
	uk_vma_shared_anon_release((struct uk_vma_shared_anon *)object);
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

int uk_vma_shared_anon_create(struct uk_vas *vas, __sz len,
			      struct uk_vma_shared_anon **out)
{
	const __sz count = len / UK_PAGING_PAGE_SIZE;
	struct uk_pagetable *pt = vas->pt;
	struct uk_vma_shared_anon *b;
	__vaddr_t alias;
	__paddr_t page;
	int rc;

	if (!len || !UK_PAGING_PAGE_ALIGNED(len))
		return -EINVAL;
	if (count > (__SZ_MAX - sizeof(*b)) / sizeof(b->pages[0]))
		return -ENOMEM;
	b = uk_malloc(vas->a, sizeof(*b) + count * sizeof(b->pages[0]));
	if (!b)
		return -ENOMEM;
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
	*out = b;
	return 0;
fail:
	uk_vma_shared_anon_release(b);
	return rc;
}

static int shared_new(struct uk_vas *vas, __vaddr_t vaddr __unused,
		      __sz len, void *data __unused,
		      unsigned long attr __unused, unsigned long *flags,
		      struct uk_vma **out)
{
	unsigned int order = UK_VMA_MAP_SIZE_TO_ORDER(*flags);
	struct shared_vma *v;
	int rc;

	if (order && order != UK_PAGING_PAGE_SHIFT)
		return -ENOTSUP;
	v = uk_malloc(vas->a, sizeof(*v));
	if (!v)
		return -ENOMEM;
	rc = uk_vma_shared_anon_create(vas, len, &v->backing);
	if (rc) {
		uk_free(vas->a, v);
		return rc;
	}
	v->first = 0;
	v->base.name = NULL;
	*out = &v->base;
	return 0;
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
	uk_vma_shared_anon_release(((struct shared_vma *)vma)->backing);
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

struct shared_attachment_vma {
	struct shared_vma shared;
	struct uk_vma_shared_anon_attachment attachment;
};

static int attachment_new(struct uk_vas *vas, __vaddr_t vaddr __unused,
			  __sz len, void *data, unsigned long attr __unused,
			  unsigned long *flags, struct uk_vma **out)
{
	const struct uk_vma_shared_anon_attachment *attachment = data;
	unsigned int order = UK_VMA_MAP_SIZE_TO_ORDER(*flags);
	struct shared_attachment_vma *v;

	if (order && order != UK_PAGING_PAGE_SHIFT)
		return -ENOTSUP;
	if (len / UK_PAGING_PAGE_SIZE != attachment->backing->count)
		return -EINVAL;
	v = uk_malloc(vas->a, sizeof(*v));
	if (!v)
		return -ENOMEM;
	v->shared.backing = attachment->backing;
	v->shared.first = 0;
	v->shared.base.name = NULL;
	v->attachment = *attachment;
	shared_ref_get(v->shared.backing);
	v->attachment.attach(v->attachment.arg);
	*out = &v->shared.base;
	return 0;
}

static int attachment_clone(struct uk_vma *vma, struct uk_alloc *a,
			    struct uk_vma **out)
{
	struct shared_attachment_vma *old = (struct shared_attachment_vma *)vma;
	struct shared_attachment_vma *v = uk_malloc(a, sizeof(*v));

	if (!v)
		return -ENOMEM;
	v->shared.backing = old->shared.backing;
	v->shared.first = 0;
	v->attachment = old->attachment;
	shared_ref_get(v->shared.backing);
	v->attachment.attach(v->attachment.arg);
	*out = &v->shared.base;
	return 0;
}

static void attachment_destroy(struct uk_vma *vma)
{
	struct shared_attachment_vma *v = (struct shared_attachment_vma *)vma;

	v->attachment.detach(v->attachment.arg);
	shared_destroy(vma);
}

static int attachment_split(struct uk_vma *vma __unused,
			    __vaddr_t addr __unused,
			    struct uk_vma **out __unused)
{
	return -ENOTSUP;
}

static int attachment_merge(struct uk_vma *vma __unused,
			    struct uk_vma *next __unused)
{
	return -ENOTSUP;
}

static int attachment_set_attr(struct uk_vma *vma, unsigned long attr)
{
	struct shared_attachment_vma *v = (struct shared_attachment_vma *)vma;

	if (v->attachment.readonly && (attr & UK_PAGING_PAGE_ATTR_PROT_WRITE))
		return -EACCES;
	return uk_vma_op_set_attr(vma, attr);
}

const struct uk_vma_ops uk_vma_shared_anon_attachment_ops = {
	.new = attachment_new,
	.destroy = attachment_destroy,
	.clone_cow = attachment_clone,
	.shared_ref = shared_ref,
	.fault = shared_fault,
	.unmap = shared_unmap,
	.split = attachment_split,
	.merge = attachment_merge,
	.set_attr = attachment_set_attr,
	.advise = shared_advise,
};
