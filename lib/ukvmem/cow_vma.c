/* SPDX-License-Identifier: BSD-3-Clause */

#include <errno.h>
#include <stddef.h>
#include <uk/alloc.h>
#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/falloc.h>
#include <uk/isr/string.h>
#include <uk/lcpu.h>
#include <uk/plat/spinlock.h>
#include <uk/refcount.h>
#include <uk/tree.h>
#include <uk/vmem/vma_ops.h>
#include <uk/vmem/vma_types.h>

#include "cow.h"
#include "cow_alloc.h"
#include "cow_vma.h"
#include "vmem.h"

#define COW_PAGE_FLAGS UK_PAGING_PAGE_FLAG_SIZE(UK_PAGING_PAGE_LEVEL)

struct cow_slot;
UK_RB_HEAD(cow_map, cow_slot);

struct cow_pool {
	struct uk_alloc *parent;
	struct uk_alloc *frames;
	struct uk_alloc *objects;
	__atomic refs;
};

struct uk_vma_cow_source {
	struct cow_pool *pool;
	__atomic refs;
	__sz count;
	struct vmem_cow_ref pages[];
};

struct vmem_cow_vas {
	struct cow_pool *pool;
	struct cow_map slots;
};

struct cow_slot {
	UK_RB_ENTRY(cow_slot) entry;
	__vaddr_t vaddr;
	struct vmem_cow_ref frame;
	struct cow_pool *pool;
};

struct cow_fault {
	struct uk_vm_fault_op operation;
	struct uk_paging_deferred_call after;
	struct vmem_vas_transition transition;
	struct cow_pool *pool;
	struct uk_vma *vma;
	struct cow_slot *old;
	struct cow_slot *next;
	bool copied;
	bool committed;
};

static __vaddr_t cow_slot_key(struct cow_slot *slot)
{
	return slot->vaddr;
}

static int cow_slot_cmp(__vaddr_t a, __vaddr_t b)
{
	return (a > b) - (a < b);
}

UK_RB_KEY_GENERATE_STATIC(cow_map, cow_slot, entry,
			  cow_slot_cmp, cow_slot_key);

static void cow_pool_put(struct cow_pool *pool)
{
	int rc;

	if (!uk_refcount_release(&pool->refs))
		return;
	rc = vmem_cow_alloc_destroy(pool->frames);
	UK_ASSERT(!rc);
	if (rc)
		UK_CRASH("COW frame metadata remains at final release\n");
	rc = vmem_cow_alloc_destroy(pool->objects);
	UK_ASSERT(!rc);
	if (rc)
		UK_CRASH("COW mapping metadata remains at final release\n");
	uk_free(pool->parent, pool);
}

static struct cow_pool *cow_pool_new(struct uk_alloc *a)
{
	struct cow_pool *pool;
	__sz object_size;

	object_size = MAX(sizeof(struct cow_slot), sizeof(struct cow_fault));
	pool = uk_malloc(a, sizeof(*pool));
	if (!pool)
		return NULL;
	pool->parent = a;
	pool->frames = vmem_cow_alloc_create(a, vmem_cow_page_metadata_size());
	if (!pool->frames)
		goto fail;
	pool->objects = vmem_cow_alloc_create(a, object_size);
	if (!pool->objects) {
		vmem_cow_alloc_destroy(pool->frames);
		goto fail;
	}
	uk_refcount_init(&pool->refs, 1);
	return pool;
fail:
	uk_free(a, pool);
	return NULL;
}

struct uk_vma_cow_source *uk_vma_cow_source_get(struct uk_vma_cow_source *s)
{
	if (s)
		uk_refcount_acquire(&s->refs);
	return s;
}

void uk_vma_cow_source_put(struct uk_vma_cow_source *s)
{
	struct cow_pool *pool;
	__sz i;

	if (!s || !uk_refcount_release(&s->refs))
		return;
	UK_ASSERT(!uk_lcpu_irqs_disabled());
	pool = s->pool;
	for (i = 0; i < s->count; i++)
		vmem_cow_ref_release(&s->pages[i], NULL);
	uk_free(pool->parent, s);
	cow_pool_put(pool);
}

int uk_vma_cow_source_take(struct uk_vas *vas, __paddr_t *pages, __sz count,
			   struct uk_vma_cow_source **out)
{
	struct uk_vma_cow_source *s;
	struct cow_pool *pool;
	__sz i, bytes;
	int rc;

	if (!out)
		return -EINVAL;
	*out = NULL;
	if (!vas || !vas->cow || !pages || !count)
		return -EINVAL;
	if (uk_lcpu_irqs_disabled())
		return -EWOULDBLOCK;
	if (count > (__SZ_MAX - sizeof(*s)) / sizeof(s->pages[0]))
		return -EOVERFLOW;
	for (i = 0; i < count; i++)
		if (pages[i] == UK_PAGING_PADDR_INV ||
		    !UK_PAGING_PAGE_ALIGNED(pages[i]))
			return -EINVAL;
	pool = vas->cow->pool;
	bytes = sizeof(*s) + count * sizeof(s->pages[0]);
	s = uk_malloc(pool->parent, bytes);
	if (!s) {
		rc = -ENOMEM;
		goto free_pages;
	}
	memset_isr(s, 0, bytes);
	s->pool = pool;
	uk_refcount_init(&s->refs, 1);
	uk_refcount_acquire(&pool->refs);
	for (i = 0; i < count; i++) {
		rc = vmem_cow_ref_adopt(&s->pages[i], pool->frames,
					vas->pt->fa, pages[i], NULL, NULL);
		if (rc) {
			uk_vma_cow_source_put(s);
			goto free_pages;
		}
		pages[i] = UK_PAGING_PADDR_INV;
		s->count++;
	}
	*out = s;
	return 0;
free_pages:
	for (i = 0; i < count; i++) {
		if (pages[i] == UK_PAGING_PADDR_INV)
			continue;
		uk_ffree(vas->pt->fa, pages[i], 1);
		pages[i] = UK_PAGING_PADDR_INV;
	}
	return rc;
}

static struct vmem_cow_vas *cow_vas_new(struct cow_pool *pool)
{
	struct vmem_cow_vas *cow = uk_malloc(pool->parent, sizeof(*cow));

	if (!cow)
		return NULL;
	cow->pool = pool;
	UK_RB_INIT(&cow->slots);
	return cow;
}

void vmem_cow_vas_destroy(struct uk_vas *vas)
{
	struct vmem_cow_vas *cow = vas->cow;
	struct cow_pool *pool;

	if (!cow)
		return;
	UK_ASSERT(UK_RB_EMPTY(&cow->slots));
	pool = cow->pool;
	vas->cow = NULL;
	uk_free(pool->parent, cow);
	cow_pool_put(pool);
}

int uk_vas_enable_cow(struct uk_vas *vas, struct uk_alloc *resident)
{
	struct vmem_cow_vas *cow;
	struct cow_pool *pool;
	unsigned long irqf;
	int rc = 0;

	if (!vas || !resident || !uk_vas_is_owned(vas) ||
	    vas == uk_vas_get_kernel() || vas == uk_vas_get_active())
		return -EINVAL;
	if (uk_lcpu_irqs_disabled())
		return -EWOULDBLOCK;
	pool = cow_pool_new(resident);
	if (!pool)
		return -ENOMEM;
	cow = cow_vas_new(pool);
	if (!cow) {
		cow_pool_put(pool);
		return -ENOMEM;
	}
	ukplat_spin_lock_irqsave(&vas->lock, irqf);
	if (vas->cow || vas->transition || !uk_list_empty(&vas->vma_list))
		rc = -EBUSY;
	else
		vas->cow = cow;
	ukplat_spin_unlock_irqrestore(&vas->lock, irqf);
	if (rc) {
		uk_free(pool->parent, cow);
		cow_pool_put(pool);
	}
	return rc;
}

static void cow_slot_free(void *arg)
{
	struct cow_slot *slot = arg;

	uk_free(slot->pool->objects, slot);
}

static struct cow_slot *cow_slot_new(struct cow_pool *pool, __vaddr_t vaddr)
{
	struct cow_slot *slot = uk_malloc(pool->objects, sizeof(*slot));

	if (!slot)
		return NULL;
	memset_isr(slot, 0, sizeof(*slot));
	slot->pool = pool;
	slot->vaddr = vaddr;
	return slot;
}

static void cow_slot_insert(struct vmem_cow_vas *cow, struct cow_slot *slot)
{
	struct cow_slot *old __maybe_unused;

	old = UK_RB_INSERT(cow_map, &cow->slots, slot);
	UK_ASSERT(!old);
}

static __pte_t cow_fault_pte(struct uk_vm_fault *fault,
			     unsigned long attr, __paddr_t paddr)
{
	return uk_pal_pte_create(paddr, attr, fault->level,
				 fault->pte, fault->level);
}

static void cow_fault_after(void *arg)
{
	struct cow_fault *op = arg;
	struct uk_vas *vas = op->transition.vas;
	struct cow_pool *pool = op->pool;
	struct uk_paging_free_batch batch;
	unsigned long irqf;
	int rc;

	if (op->committed && op->copied) {
		/* The first batch has acknowledged the old frame everywhere.
		 * The transition keeps this VMA and its new slot unchanged.
		 */
		ukplat_spin_lock_irqsave(&vas->lock, irqf);
		UK_ASSERT(vas->transition == &op->transition);
		UK_ASSERT(UK_RB_FIND(cow_map, &vas->cow->slots,
				     op->next->vaddr) == op->next);
		uk_paging_batch_begin(&batch, vas->pt);
		rc = uk_paging_page_set_attr(vas->pt, op->next->vaddr, 1,
					     op->vma->attr,
					     COW_PAGE_FLAGS);
		uk_paging_batch_end(&batch);
		ukplat_spin_unlock_irqrestore(&vas->lock, irqf);
		if (unlikely(rc))
			UK_CRASH("Cannot promote committed COW page: %d\n", rc);
		uk_paging_free_batch_finish(&batch);
	}
	if (op->transition.vas)
		vmem_vas_transition_end(&op->transition);
	uk_free(pool->objects, op);
}

static void cow_fault_result(struct uk_vm_fault_op *base, int status,
			     struct uk_paging_free_batch *batch)
{
	struct cow_fault *op = __containerof(base, struct cow_fault, operation);
	struct vmem_cow_vas *cow = op->vma->vas->cow;

	if (!status) {
		if (op->next != op->old) {
			if (op->old) {
				UK_RB_REMOVE(cow_map, &cow->slots, op->old);
				vmem_cow_ref_release(&op->old->frame, batch);
			}
			cow_slot_insert(cow, op->next);
		}
		op->committed = true;
	} else if (op->next != op->old) {
		vmem_cow_ref_release(&op->next->frame, NULL);
	}
	/* Even a rejected mapper must end a transition after dropping locks. */
	uk_paging_batch_defer(batch, &op->after, cow_fault_after, op);
}

static int cow_fault_from(struct uk_vma *vma, struct uk_vm_fault *fault,
			  struct uk_vma_cow_source *source, __sz index)
{
	struct vmem_cow_vas *cow = vma->vas->cow;
	struct cow_pool *pool = cow->pool;
	struct uk_pagetable *pt = vma->vas->pt;
	struct cow_slot *old, *next;
	struct cow_fault *op;
	const struct vmem_cow_ref *initial;
	unsigned long attr = vma->attr;
	__paddr_t paddr;
	__vaddr_t mapped;
	bool write, copied;
	int rc;

	if (fault->level != UK_PAGING_PAGE_LEVEL)
		return -ENOMEM;
	UK_ASSERT(fault->len == UK_PAGING_PAGE_SIZE);
	UK_ASSERT(fault->operation && !*fault->operation);
	initial = source ? &source->pages[index] : NULL;
	old = UK_RB_FIND(cow_map, &cow->slots, fault->vbase);
	write = (fault->type & UK_VMA_FAULT_ACCESSTYPE) == UK_VMA_FAULT_WRITE;
	copied = old && write && !vmem_cow_ref_exclusive(&old->frame);
	if (old && !copied && (fault->type & UK_VMA_FAULT_NONPRESENT)) {
		if (!vmem_cow_ref_exclusive(&old->frame))
			attr &= ~UK_PAGING_PAGE_ATTR_PROT_WRITE;
		fault->paddr = vmem_cow_ref_paddr(&old->frame);
		fault->pte = cow_fault_pte(fault, attr, fault->paddr);
		return 0;
	}
	if (!old && !(fault->type & UK_VMA_FAULT_NONPRESENT))
		return -EFAULT;
	if (old && !(fault->type & UK_VMA_FAULT_IRQS_ENABLED))
		return -EWOULDBLOCK;
	op = uk_malloc(pool->objects, sizeof(*op));
	if (!op)
		return -ENOMEM;
	memset_isr(op, 0, sizeof(*op));
	op->operation.result = cow_fault_result;
	op->pool = pool;
	op->vma = vma;
	op->old = old;
	op->copied = copied;
	next = old;
	if (!old || copied) {
		next = cow_slot_new(pool, fault->vbase);
		if (!next) {
			rc = -ENOMEM;
			goto fail_op;
		}
		if (copied) {
			rc = vmem_cow_ref_copy(&next->frame, &old->frame,
					       pt, cow_slot_free, next);
		} else if (source && write) {
			rc = vmem_cow_ref_copy(&next->frame, initial,
					       pt, cow_slot_free, next);
		} else if (source) {
			vmem_cow_ref_clone(&next->frame, initial,
					   cow_slot_free, next);
			rc = 0;
		} else {
			rc = vmem_cow_ref_alloc(&next->frame, pool->frames,
						pt->fa, cow_slot_free, next);
		}
		if (rc)
			goto fail_slot;
		if (!old && !source &&
		    !(vma->flags & UK_VMA_FLAG_UNINITIALIZED)) {
			paddr = vmem_cow_ref_paddr(&next->frame);
			mapped = uk_paging_page_kmap(pt, paddr, 1, 0);
			if (mapped == UK_PAGING_VADDR_INV) {
				rc = -ENOMEM;
				goto fail_frame;
			}
			memset_isr((void *)mapped, 0, UK_PAGING_PAGE_SIZE);
			uk_paging_page_kunmap(pt, mapped, 1, 0);
		}
	}
	op->next = next;
	if (old) {
		rc = vmem_vas_transition_mark_locked(vma->vas, &op->transition);
		if (rc)
			goto fail_frame;
	}
	if (copied || !vmem_cow_ref_exclusive(&next->frame))
		attr &= ~UK_PAGING_PAGE_ATTR_PROT_WRITE;
	fault->paddr = vmem_cow_ref_paddr(&next->frame);
	fault->pte = cow_fault_pte(fault, attr, fault->paddr);
	*fault->operation = &op->operation;
	return 0;
fail_frame:
	if (next != old)
		vmem_cow_ref_release(&next->frame, NULL);
	goto fail_op;
fail_slot:
	uk_free(pool->objects, next);
fail_op:
	uk_free(pool->objects, op);
	return rc;
}

static int cow_anon_fault(struct uk_vma *vma, struct uk_vm_fault *fault)
{
	return cow_fault_from(vma, fault, NULL, 0);
}

int uk_vma_cow_source_fault(struct uk_vma *vma, struct uk_vm_fault *fault,
			    struct uk_vma_cow_source *s, __sz index)
{
	if (!vma || !vma->vas || !vma->vas->cow || !fault || !s ||
	    vma->vas->cow->pool != s->pool || index >= s->count)
		return -EINVAL;
	return cow_fault_from(vma, fault, s, index);
}

static int cow_anon_unmap(struct uk_vma *vma, __vaddr_t vaddr, __sz len)
{
	struct vmem_cow_vas *cow = vma->vas->cow;
	struct uk_paging_free_batch *batch;
	struct cow_slot *slot, *next;
	int rc;

	batch = uk_paging_batch_current(vma->vas->pt);
	UK_ASSERT(batch);
	rc = uk_paging_page_unmap(vma->vas->pt, vaddr,
				  len / UK_PAGING_PAGE_SIZE,
				  UK_PAGING_PAGE_FLAG_KEEP_FRAMES);
	if (rc)
		return rc;
	next = UK_RB_NFIND(cow_map, &cow->slots, vaddr);
	UK_RB_FOREACH_FROM(slot, cow_map, next) {
		if (slot->vaddr - vaddr >= len)
			break;
		UK_RB_REMOVE(cow_map, &cow->slots, slot);
		vmem_cow_ref_release(&slot->frame, batch);
	}
	return 0;
}

static int cow_anon_set_attr(struct uk_vma *vma, unsigned long attr)
{
	struct vmem_cow_vas *cow = vma->vas->cow;
	struct cow_slot *slot;
	unsigned int level;
	unsigned long effective;
	__pte_t pte;
	int rc;

	for (slot = UK_RB_NFIND(cow_map, &cow->slots, vma->start);
	     slot && slot->vaddr < vma->end;
	     slot = UK_RB_NEXT(cow_map, &cow->slots, slot)) {
		level = UK_PAGING_PAGE_LEVEL;
		rc = uk_paging_pt_walk(vma->vas->pt, slot->vaddr,
				       &level, NULL, &pte);
		if (rc)
			return rc;
		if (!UK_PAGING_PT_Lx_PTE_PRESENT(pte, level))
			continue;
		UK_ASSERT(level == UK_PAGING_PAGE_LEVEL);
		effective = attr;
		if (!vmem_cow_ref_exclusive(&slot->frame))
			effective &= ~UK_PAGING_PAGE_ATTR_PROT_WRITE;
		rc = uk_paging_page_set_attr(vma->vas->pt, slot->vaddr, 1,
					     effective,
					     COW_PAGE_FLAGS);
		if (rc)
			return rc;
	}
	return 0;
}

static int cow_anon_advise(struct uk_vma *vma, __vaddr_t vaddr, __sz len,
			   unsigned long advice)
{
	if (!(advice & UK_VMA_ADV_WILLNEED) && (advice & UK_VMA_ADV_DONTNEED))
		return cow_anon_unmap(vma, vaddr, len);
	return uk_vma_op_advise(vma, vaddr, len, advice);
}

static int cow_anon_new(struct uk_vas *vas, __vaddr_t vaddr __unused,
			__sz len __unused, void *data __unused,
			unsigned long attr __unused, unsigned long *flags,
			struct uk_vma **out)
{
	unsigned int order = UK_VMA_MAP_SIZE_TO_ORDER(*flags);
	struct uk_vma *vma;

	if (!vas->cow)
		return -EINVAL;
	if (order && order != UK_PAGING_PAGE_SHIFT)
		return -ENOTSUP;
	vma = uk_malloc(vas->a, sizeof(*vma));
	if (!vma)
		return -ENOMEM;
	vma->name = NULL;
	*out = vma;
	return 0;
}

static int cow_anon_split(struct uk_vma *vma __unused,
			  __vaddr_t vaddr __unused, struct uk_vma **out)
{
	/* All split ranges keep using the VAS's address-indexed slots. */
	*out = NULL;
	return 0;
}

static int cow_anon_clone(struct uk_vma *vma __unused,
			  struct uk_alloc *a __unused, struct uk_vma **out)
{
	*out = NULL;
	return 0;
}

const struct uk_vma_ops uk_vma_cow_anon_ops = {
	.new = cow_anon_new,
	.clone_cow = cow_anon_clone,
	.fault = cow_anon_fault,
	.unmap = cow_anon_unmap,
	.split = cow_anon_split,
	.set_attr = cow_anon_set_attr,
	.advise = cow_anon_advise
};

int uk_vas_clone_cow(struct uk_vas *dst, struct uk_vas *src)
{
	struct vmem_vas_transition transition = { 0 };
	struct uk_paging_free_batch batch;
	struct vmem_cow_vas *cow;
	struct cow_slot *slot, *copy;
	struct uk_vma *vma, *v;
	unsigned long irqf, attr;
	unsigned int level;
	__pte_t pte;
	__paddr_t paddr;
	int rc;

	if (!src || !dst || src == dst || !src->cow || dst->cow ||
	    !uk_vas_is_owned(dst) || dst == uk_vas_get_active() ||
	    !uk_list_empty(&dst->vma_list))
		return -EINVAL;
	if (uk_lcpu_irqs_disabled())
		return -EWOULDBLOCK;
	cow = cow_vas_new(src->cow->pool);
	if (!cow)
		return -ENOMEM;
	uk_refcount_acquire(&cow->pool->refs);
	dst->cow = cow;
	rc = vmem_vas_transition_begin(src, &transition);
	if (rc)
		goto fail_dst;
	ukplat_spin_lock_irqsave(&src->lock, irqf);
	/* Validate every class before changing source permissions. */
	uk_list_for_each_entry(vma, &src->vma_list, vma_list) {
		if (!vma->ops->clone_cow ||
		    !vmem_vas_range_allowed(dst, vma->start,
					vma->end - vma->start) ||
		    vma->page_lvl > UK_PAGING_PAGE_LEVEL ||
		    uk_paging_pt_get_owner(src->pt, vma->start) != src->pt ||
		    uk_paging_pt_get_owner(src->pt, vma->end - 1) != src->pt ||
		    uk_paging_pt_get_owner(dst->pt, vma->start) != dst->pt ||
		    uk_paging_pt_get_owner(dst->pt, vma->end - 1) != dst->pt) {
			rc = -ENOTSUP;
			goto unlock;
		}
	}
	uk_list_for_each_entry(vma, &src->vma_list, vma_list) {
		v = NULL;
		rc = vma->ops->clone_cow(vma, dst->a, &v);
		if (rc)
			goto unlock;
		if (!v) {
			v = uk_malloc(dst->a, sizeof(*v));
			if (!v) {
				rc = -ENOMEM;
				goto unlock;
			}
		}
		memcpy_isr(v, vma, sizeof(*v));
		v->vas = dst;
		v->sync_inflight = 0;
		UK_INIT_LIST_HEAD(&v->vma_list);
		uk_list_add_tail(&v->vma_list, &dst->vma_list);
	}
	UK_RB_FOREACH(slot, cow_map, &src->cow->slots) {
		copy = cow_slot_new(cow->pool, slot->vaddr);
		if (!copy) {
			rc = -ENOMEM;
			goto unlock;
		}
		vmem_cow_ref_clone(&copy->frame, &slot->frame,
				   cow_slot_free, copy);
		cow_slot_insert(cow, copy);
	}
	uk_paging_batch_begin(&batch, src->pt);
	UK_RB_FOREACH(slot, cow_map, &src->cow->slots) {
		level = UK_PAGING_PAGE_LEVEL;
		rc = uk_paging_pt_walk(src->pt, slot->vaddr,
				       &level, NULL, &pte);
		if (rc)
			break;
		if (!UK_PAGING_PT_Lx_PTE_PRESENT(pte, level))
			continue;
		UK_ASSERT(level == UK_PAGING_PAGE_LEVEL);
		paddr = vmem_cow_ref_paddr(&slot->frame);
		UK_ASSERT(UK_PAGING_PT_Lx_PTE_PADDR(pte, level) ==
			  paddr);
		attr = uk_paging_attr_from_pte(pte, level) &
			~UK_PAGING_PAGE_ATTR_PROT_WRITE;
		rc = uk_paging_page_map(dst->pt, slot->vaddr, paddr,
					1, attr, COW_PAGE_FLAGS);
		if (rc)
			break;
		rc = uk_paging_page_set_attr(src->pt, slot->vaddr, 1, attr,
					     COW_PAGE_FLAGS);
		if (rc)
			break;
	}
	uk_paging_batch_end(&batch);
	ukplat_spin_unlock_irqrestore(&src->lock, irqf);
	uk_paging_free_batch_finish(&batch);
	vmem_vas_transition_end(&transition);
	if (rc)
		goto fail_dst;
	return 0;
unlock:
	ukplat_spin_unlock_irqrestore(&src->lock, irqf);
	vmem_vas_transition_end(&transition);
fail_dst:
	uk_vas_destroy(dst);
	return rc;
}
