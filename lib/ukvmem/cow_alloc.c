/* SPDX-License-Identifier: BSD-3-Clause */

#include <errno.h>
#include <stddef.h>
#include <uk/alloc.h>
#include <uk/alloc_impl.h>
#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/isr/string.h>
#include <uk/list.h>
#include <uk/paging.h>
#include <uk/plat/spinlock.h>

#include "cow_alloc.h"

#define COW_ALLOC_ALIGN 16UL

struct cow_alloc {
	struct uk_alloc *parent;
	struct __spinlock lock;
	struct uk_list_head partial;
	__sz size;
	__sz stride;
	__sz objects;
	__sz slabs;
};

struct cow_alloc_slab {
	struct uk_alloc *owner;
	struct uk_list_head partial;
	void *free;
	__sz capacity;
	__sz available;
};

#define COW_ALLOC_HEADER \
	ALIGN_UP(sizeof(struct cow_alloc_slab), COW_ALLOC_ALIGN)

static struct cow_alloc *cow_alloc_state(struct uk_alloc *a)
{
	UK_ASSERT(a);
	return (void *)a->priv;
}

static struct cow_alloc_slab *cow_alloc_slab_new(struct uk_alloc *a)
{
	struct cow_alloc *c = cow_alloc_state(a);
	struct cow_alloc_slab *slab;
	__uptr first;
	__sz i;

	slab = uk_palloc(c->parent, 1);
	if (!slab)
		return NULL;
	UK_ASSERT(UK_PAGING_PAGE_ALIGNED((__uptr)slab));
	slab->owner = a;
	slab->free = NULL;
	slab->capacity = (UK_PAGING_PAGE_SIZE - COW_ALLOC_HEADER) / c->stride;
	slab->available = slab->capacity;
	UK_ASSERT(slab->capacity);
	first = (__uptr)slab + COW_ALLOC_HEADER;
	for (i = slab->capacity; i > 0; i--) {
		void **slot = (void **)(first + (i - 1) * c->stride);

		*slot = slab->free;
		slab->free = slot;
	}
	uk_list_add(&slab->partial, &c->partial);
	c->slabs++;
	return slab;
}

static void *cow_alloc_malloc(struct uk_alloc *a, __sz size)
{
	struct cow_alloc *c = cow_alloc_state(a);
	struct cow_alloc_slab *slab;
	unsigned long irqf;
	void *p = NULL;

	if (!size || size > c->size)
		return NULL;
	ukplat_spin_lock_irqsave(&c->lock, irqf);
	slab = uk_list_first_entry_or_null(&c->partial,
					   struct cow_alloc_slab, partial);
	if (!slab)
		slab = cow_alloc_slab_new(a);
	if (slab) {
		UK_ASSERT(slab->free && slab->available);
		p = slab->free;
		slab->free = *(void **)p;
		if (--slab->available == 0)
			uk_list_del_init(&slab->partial);
		c->objects++;
	}
	ukplat_spin_unlock_irqrestore(&c->lock, irqf);
	return p;
}

static void cow_alloc_free(struct uk_alloc *a, void *p)
{
	struct cow_alloc *c = cow_alloc_state(a);
	struct uk_alloc *parent = c->parent;
	struct cow_alloc_slab *slab;
	unsigned long irqf;
	__uptr first;
	int reclaim;

	if (!p)
		return;
	slab = (void *)UK_PAGING_PAGE_ALIGN_DOWN((__uptr)p);
	UK_ASSERT(slab->owner == a);
	first = (__uptr)slab + COW_ALLOC_HEADER;
	UK_ASSERT((__uptr)p >= first);
	UK_ASSERT((__uptr)p < first + slab->capacity * c->stride);
	UK_ASSERT(((__uptr)p - first) % c->stride == 0);
	ukplat_spin_lock_irqsave(&c->lock, irqf);
	UK_ASSERT(c->objects && slab->available < slab->capacity);
	if (!slab->available)
		uk_list_add(&slab->partial, &c->partial);
	*(void **)p = slab->free;
	slab->free = p;
	slab->available++;
	c->objects--;
	reclaim = slab->available == slab->capacity;
	if (reclaim) {
		uk_list_del_init(&slab->partial);
		c->slabs--;
	}
	ukplat_spin_unlock_irqrestore(&c->lock, irqf);
	if (reclaim)
		uk_pfree(parent, slab, 1);
}

struct uk_alloc *vmem_cow_alloc_create(struct uk_alloc *parent, __sz obj_size)
{
	struct cow_alloc *c;
	struct uk_alloc *a;

	if (!parent || !parent->palloc || !parent->pfree || !obj_size ||
	    obj_size > UK_PAGING_PAGE_SIZE - COW_ALLOC_HEADER ||
	    sizeof(*a) + sizeof(*c) > UK_PAGING_PAGE_SIZE)
		return NULL;
	a = uk_palloc(parent, 1);
	if (!a)
		return NULL;
	UK_ASSERT(UK_PAGING_PAGE_ALIGNED((__uptr)a));
	memset_isr(a, 0, sizeof(*a) + sizeof(*c));
	a->malloc = cow_alloc_malloc;
	a->calloc = uk_calloc_compat;
	a->free = cow_alloc_free;
	c = cow_alloc_state(a);
	c->parent = parent;
	c->size = obj_size;
	c->stride = ALIGN_UP(MAX(obj_size, sizeof(void *)), COW_ALLOC_ALIGN);
	ukarch_spin_init(&c->lock);
	UK_INIT_LIST_HEAD(&c->partial);
	return a;
}

void vmem_cow_alloc_stats(struct uk_alloc *a, struct vmem_cow_alloc_stats *out)
{
	struct cow_alloc *c = cow_alloc_state(a);
	unsigned long irqf;

	ukplat_spin_lock_irqsave(&c->lock, irqf);
	out->objects = c->objects;
	out->slabs = c->slabs;
	ukplat_spin_unlock_irqrestore(&c->lock, irqf);
}

int vmem_cow_alloc_destroy(struct uk_alloc *a)
{
	struct cow_alloc *c = cow_alloc_state(a);
	struct uk_alloc *parent = c->parent;
	unsigned long irqf;
	int rc = 0;

	ukplat_spin_lock_irqsave(&c->lock, irqf);
	if (c->objects) {
		rc = -EBUSY;
	} else {
		UK_ASSERT(!c->slabs);
		UK_ASSERT(uk_list_empty(&c->partial));
	}
	ukplat_spin_unlock_irqrestore(&c->lock, irqf);
	if (!rc)
		uk_pfree(parent, a, 1);
	return rc;
}
