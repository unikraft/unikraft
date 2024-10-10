/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <stddef.h>
#include <errno.h>

#include "vmem.h"

#include <uk/essentials.h>
#include <uk/arch/limits.h>
#include <uk/arch/paging.h>
#ifdef CONFIG_HAVE_PAGING
#include <uk/plat/paging.h>
#endif /* CONFIG_HAVE_PAGING */
#include <uk/alloc.h>
#include <uk/assert.h>
#include <uk/list.h>
#include <uk/config.h>

/*
 * Pointer to currently active virtual address space.
 * TODO: This should move to a CPU-local variable
 */
static struct uk_vas *vmem_active_vas;

/* Forward declarations */
static void vmem_vma_unmap(struct uk_vma *vma, __vaddr_t vaddr, __sz len);
static void vmem_vma_unlink_and_free(struct uk_vma *vma);

struct uk_vas *uk_vas_get_active(void)
{
	return vmem_active_vas;
}

int uk_vas_set_active(struct uk_vas *vas)
{
#ifdef CONFIG_HAVE_PAGING
	int rc;

	rc = ukplat_pt_set_active(vas->pt);
	if (unlikely(rc))
		return rc;
#endif /* CONFIG_HAVE_PAGING */

	vmem_active_vas = vas;
	return 0;
}

int uk_vas_init(struct uk_vas *vas, struct uk_pagetable *pt __maybe_unused,
		struct uk_alloc *a)
{
	UK_ASSERT(vas);
	UK_ASSERT(pt);
	UK_ASSERT(a);

	vas->a = a;

#ifdef CONFIG_HAVE_PAGING
	vas->pt = pt;

	vas->vma_base = PAGE_ALIGN_UP(CONFIG_LIBUKVMEM_DEFAULT_BASE);
	UK_ASSERT(ukarch_vaddr_isvalid(vas->vma_base));
#endif /* CONFIG_HAVE_PAGING */

	vas->flags = 0;

	UK_INIT_LIST_HEAD(&vas->vma_list);

	uk_spin_init(&vas->map_lock);
	uk_rwlock_init(&vas->vma_list_lock);

	return 0;
}

void uk_vas_destroy(struct uk_vas *vas)
{
	struct uk_vma *vma, *next;

	uk_list_for_each_entry_safe(vma, next, &vas->vma_list, vma_list) {
		vmem_vma_unmap(vma, vma->start, vmem_vma_len(vma));
		vmem_vma_unlink_and_free(vma);
	}

	UK_ASSERT(uk_list_empty(&vas->vma_list));

	if (vmem_active_vas == vas)
		vmem_active_vas = __NULL;
}

static void vmem_vma_destroy(struct uk_vma *vma)
{
	UK_ASSERT(vma);

	if (vma->ops->destroy)
		vma->ops->destroy(vma);

	uk_free(vma->vas->a, vma);
}

static void vmem_vma_unlink_and_free(struct uk_vma *vma)
{
	UK_ASSERT(vma);
	UK_ASSERT(!uk_list_empty(&vma->vma_list));

	uk_rwlock_wlock(&vma->vas->vma_list_lock);
	uk_list_del(&vma->vma_list);
	uk_rwlock_wunlock(&vma->vas->vma_list_lock);
	vmem_vma_destroy(vma);
}

static struct uk_vma *vmem_vma_find(struct uk_vas *vas, __vaddr_t vaddr,
				    __sz len)
{
	struct uk_vma *vma;
	__vaddr_t vstart = vaddr;
	__vaddr_t vend = vaddr + MAX(len, (__sz)1);

	UK_ASSERT(vas);
	UK_ASSERT(vaddr <= __VADDR_MAX - len);

	uk_rwlock_rlock(&vas->vma_list_lock);
	uk_list_for_each_entry(vma, &vas->vma_list, vma_list) {
		if ((vend > vma->start) && (vstart < vma->end)) {
			uk_rwlock_runlock(&vas->vma_list_lock);
			return vma;
		}
	}
	uk_rwlock_runlock(&vas->vma_list_lock);

	return __NULL;
}

const struct uk_vma *uk_vma_find(struct uk_vas *vas, __vaddr_t vaddr)
{
	return vmem_vma_find(vas, vaddr, 0);
}

static int vmem_vma_find_range(struct uk_vas *vas, __vaddr_t *vaddr, __sz *len,
			       struct uk_vma **start, struct uk_vma **end,
			       int strict)
{
	struct uk_vma *vma_start, *vma_end, *next;
	__vaddr_t vstart, vend;
	__sz vl;
	int algn_lvl;

	UK_ASSERT(vas);
	UK_ASSERT(start);
	UK_ASSERT(end);

	UK_ASSERT(vaddr);
	vstart = *vaddr;

	UK_ASSERT(len);
	vl = *len;

	UK_ASSERT(vstart <= __VADDR_MAX - vl);
	vend = vstart + vl;

	if (!*start) {
		vma_start = vmem_vma_find(vas, vstart, vl);
		if (unlikely(!vma_start))
			return -ENOENT;
	} else {
		vma_start = *start;
	}

	/* Adjust vstart to make sure it is within the VMA. We expect
	 * vstart to be properly aligned by the caller.
	 */
	UK_ASSERT(vstart < vma_start->end);
	if (vstart < vma_start->start) {
		if (strict)
			return -ENOENT;

		vstart = vma_start->start;
	} else if (vstart > vma_start->start) {
		algn_lvl = MAX(vma_start->page_lvl, PAGE_LEVEL);
		if (!PAGE_Lx_ALIGNED(vstart, algn_lvl))
			return -EINVAL;
	}

	UK_ASSERT(PAGE_Lx_ALIGNED(vstart,
		  MAX(vma_start->page_lvl, PAGE_LEVEL)));

	/* Find the end VMA */
	vma_end = vma_start;
	if (likely(vl > 0)) {
		uk_rwlock_rlock(&vas->vma_list_lock);
		while (vma_end->vma_list.next != &vas->vma_list) {
			if (vend > vma_end->start && vend <= vma_end->end)
				break;

			/* In strict mode, we do not allow the specified
			 * address range to contain holes between VMAs
			 */
			next = uk_list_next_entry(vma_end, vma_list);
			if (strict && vma_end->end != next->start) {
				uk_rwlock_runlock(&vas->vma_list_lock);
				return -ENOENT;
			}

			if (vend <= next->start)
				break;

			vma_end = next;
		}
		uk_rwlock_runlock(&vas->vma_list_lock);

		/* Adjust vend to make sure it is within the VMA. We expect
		 * vend to be properly aligned by the caller (via len).
		 */
		if (vend > vma_end->end) {
			if (strict)
				return -ENOENT;

			vend = vma_end->end;
		} else if (vend < vma_end->end) {
			algn_lvl = MAX(vma_end->page_lvl, PAGE_LEVEL);
			if (!PAGE_Lx_ALIGNED(vend, algn_lvl))
				return -EINVAL;
		}
	}

	UK_ASSERT(PAGE_Lx_ALIGNED(vend,
		  MAX(vma_end->page_lvl, PAGE_LEVEL)));

	UK_ASSERT(vend > vstart);
	UK_ASSERT(vstart >= vma_start->start && vstart < vma_start->end);
	UK_ASSERT(vend > vma_end->start && vend <= vma_end->end);
	UK_ASSERT(ukarch_vaddr_range_isvalid(vstart, vl));

	*vaddr = vstart;
	*len   = vend - vstart;
	*start = vma_start;
	*end   = vma_end;

	return 0;
}

static void vmem_vma_insert(struct uk_vas *vas, struct uk_vma *vma)
{
	struct uk_list_head *prev;
	struct uk_vma *cur;

	UK_ASSERT(vas);
	UK_ASSERT(uk_list_empty(&vma->vma_list));
	UK_ASSERT(!vmem_vma_find(vas, vma->start, vma->end - vma->start));

	prev = &vas->vma_list;
	uk_rwlock_rlock(&vas->vma_list_lock);
	uk_list_for_each_entry(cur, &vas->vma_list, vma_list) {
		if (vma->start < cur->end) {
			UK_ASSERT(vma->end <= cur->start);

			uk_rwlock_upgrade(&vas->vma_list_lock);
			uk_list_add(&vma->vma_list, prev);
			uk_rwlock_downgrade(&vas->vma_list_lock);
			uk_rwlock_runlock(&vas->vma_list_lock);
			return;
		}

		prev = &cur->vma_list;
	}

	uk_rwlock_upgrade(&vas->vma_list_lock);
	uk_list_add_tail(&vma->vma_list, &vas->vma_list);
	uk_rwlock_downgrade(&vas->vma_list_lock);
	uk_rwlock_runlock(&vas->vma_list_lock);
}

static inline int vmem_vma_can_merge(struct uk_vma *vma, struct uk_vma *next)
{
	UK_ASSERT(vma);
	UK_ASSERT(next);
	UK_ASSERT(vma->vas == next->vas);

	return ((vma->end == next->start) &&
		(vma->ops == next->ops) &&
		(vma->attr == next->attr) &&
		(vma->flags == next->flags) &&
		(vma->page_lvl == next->page_lvl) &&
		(vma->name == next->name));
}

static int vmem_vma_do_try_merge_with_next(struct uk_vma *vma)
{
	struct uk_vma *next;
	int rc;

	UK_ASSERT(vma);

	if (vma->vma_list.next == &vma->vas->vma_list)
		return -ENOENT;

	next = uk_list_next_entry(vma, vma_list);

	if (!vmem_vma_can_merge(vma, next))
		return -EPERM;

	rc = VMA_MERGE(vma, next);
	if (unlikely(rc))
		return rc;

	/* Expand the VMA to include the next VMA */
	vma->end = next->end;

	/* Remove and destroy the next VMA. However, we keep the mapping! */
	vmem_vma_unlink_and_free(next);

	return 0;
}

static struct uk_vma *vmem_vma_try_merge_with_next(struct uk_vma *vma)
{
	vmem_vma_do_try_merge_with_next(vma);
	return vma;
}

static struct uk_vma *vmem_vma_try_merge_with_prev(struct uk_vma *vma)
{
	struct uk_vma *prev;

	if (vma->vma_list.prev == &vma->vas->vma_list)
		return vma;

	prev = uk_list_prev_entry(vma, vma_list);
	return (vmem_vma_do_try_merge_with_next(prev) == 0) ? prev : vma;
}

static struct uk_vma *vmem_vma_try_merge(struct uk_vma *vma)
{
	vmem_vma_try_merge_with_next(vma);
	return vmem_vma_try_merge_with_prev(vma);
}

static int vmem_vma_split(struct uk_vma *vma, __vaddr_t vaddr,
			  struct uk_vma **new_vma)
{
	struct uk_vma *v = __NULL;
	int rc;

	UK_ASSERT(vma);
	UK_ASSERT(new_vma);

	UK_ASSERT(vaddr >= vma->start && vaddr < vma->end);
	UK_ASSERT(PAGE_Lx_ALIGNED(vaddr, MAX(vma->page_lvl, PAGE_LEVEL)));
	UK_ASSERT(PAGE_Lx_ALIGNED(vma->end - vaddr,
		  MAX(vma->page_lvl, PAGE_LEVEL)));

	rc = VMA_SPLIT(vma, vaddr, &v);
	if (unlikely(rc))
		return rc;

	if (!v) {
		v = uk_malloc(vma->vas->a, sizeof(struct uk_vma));
		if (unlikely(!v))
			return -ENOMEM;
	}

	UK_ASSERT(v);

	UK_INIT_LIST_HEAD(&v->vma_list);
	v->start	= vaddr;
	v->end		= vma->end;
	v->vas		= vma->vas;
	v->ops		= vma->ops;
	v->attr		= vma->attr;
	v->flags	= vma->flags;
	v->page_lvl	= vma->page_lvl;
	v->name		= vma->name;

	vma->end	= vaddr;

	uk_rwlock_wlock(&vma->vas->vma_list_lock);
	uk_list_add(&v->vma_list, &vma->vma_list);
	uk_rwlock_wunlock(&vma->vas->vma_list_lock);

	*new_vma = v;
	return 0;
}

static inline int vmem_vma_need_split(struct uk_vma *vma, __sz len,
				      unsigned long *attr)
{
	UK_ASSERT(vma);

	return (len > 0) && (!attr || (vma->attr != *attr));
}

static int vmem_vma_split_vmas(struct uk_vas *vas, __vaddr_t vaddr, __sz len,
			       unsigned long *attr, struct uk_vma **start,
			       struct uk_vma **end, int strict)
{
	struct uk_vma *vma_start = __NULL, *vma_end, *vma;
	__vaddr_t vend;
	__sz left, right;
	int rc;

	UK_ASSERT(vas);
	UK_ASSERT(start);
	UK_ASSERT(end);

	/*
	 * We can have the following cases:
	 *
	 * AAAAAAAAA        AAAAAAAAA         AAAAAAAAA         AAAAAAAAA
	 * ^-------^        ^---^                 ^---^           ^---^
	 *
	 * AAA C BBB        AAA C BBB         AAA C BBB         AAA C BBB
	 * ^-------^         ^------^         ^------^           ^-----^
	 */
	rc = vmem_vma_find_range(vas, &vaddr, &len, &vma_start, &vma_end,
				 strict);
	if (unlikely(rc))
		return rc;

	vend  = vaddr + len;
	left  = vaddr - vma_start->start;
	right = vma_end->end - vend;

	if (vmem_vma_need_split(vma_start, left, attr)) {
		rc = vmem_vma_split(vma_start, vaddr, &vma);
		if (unlikely(rc))
			return rc;

		UK_ASSERT(vma);

		if (vma_start == vma_end)
			vma_end = vma;

		vma_start = vma;
	}

	if (likely(len > 0)) {
		if (vmem_vma_need_split(vma_end, right, attr)) {
			rc = vmem_vma_split(vma_end, vend, &vma);
			if (unlikely(rc)) {
				vmem_vma_try_merge(vma_start);
				return rc;
			}

			UK_ASSERT(vma);
		}
	} else {
		UK_ASSERT(vma_start == vma_end);
	}

	*start = vma_start;
	*end   = vma_end;

	return 0;
}

int vma_op_deny()
{
	return -EPERM;
}

int vma_op_unmap(struct uk_vma *vma, __vaddr_t vaddr, __sz len)
{
	UK_ASSERT(vaddr >= vma->start);
	UK_ASSERT(vaddr + len <= vma->end);
	UK_ASSERT(PAGE_ALIGNED(len));

	return ukplat_page_unmap(vma->vas->pt, vaddr, len / PAGE_SIZE, 0);
}

static void vmem_vma_unmap(struct uk_vma *vma, __vaddr_t vaddr, __sz len)
{
	int rc;

	UK_ASSERT(vma);

	UK_ASSERT(vaddr >= vma->start && vaddr < vma->end);
	UK_ASSERT(PAGE_Lx_ALIGNED(vaddr, MAX(vma->page_lvl, PAGE_LEVEL)));
	UK_ASSERT(vaddr <= __VADDR_MAX - len);
	UK_ASSERT((vaddr + len) > vma->start && (vaddr + len) <= vma->end);
	UK_ASSERT(PAGE_Lx_ALIGNED(len, MAX(vma->page_lvl, PAGE_LEVEL)));
	UK_ASSERT(len > 0);

	rc = VMA_UNMAP(vma, vaddr, len);
	if (unlikely(rc)) {
		/* We consider a failure to unmap an address range as fatal
		 * to simplify error handling.
		 */
		UK_CRASH("Failed to unmap address range 0x%" __PRIvaddr
			 "-0x%" __PRIvaddr ": %d", vaddr, vaddr + len, rc);
	}
}

static void vmem_vma_unmap_and_free(struct uk_vma *vma)
{
	vmem_vma_unmap(vma, vma->start, vmem_vma_len(vma));
	vmem_vma_destroy(vma);
}

static void vmem_vma_unmap_and_free_vmas(struct uk_vma *start,
					 struct uk_vma *end)
{
	struct uk_vma *vma = start, *next;

	UK_ASSERT(start);
	UK_ASSERT(end);

	/* It is safe now to unmap and free the VMAs */
	while (vma != end) {
		next = uk_list_next_entry(vma, vma_list);
		vmem_vma_unmap_and_free(vma);

		vma = next;
	}

	vmem_vma_unmap_and_free(end);
}

int uk_vma_unmap(struct uk_vas *vas, __vaddr_t vaddr, __sz len,
		 unsigned long flags)
{
	struct uk_vma *vma_start = __NULL, *vma_end;
	int strict = (flags & UK_VMA_FLAG_STRICT_VMA_CHECK);
	int rc;

	if (unlikely(len == 0))
		return 0;

	rc = vmem_vma_split_vmas(vas, vaddr, len, __NULL,
				 &vma_start, &vma_end, strict);

	if (unlikely(rc)) {
		if (rc == -ENOENT && !strict)
			return 0;

		return rc;
	}

	/* Unlink all VMAs starting from vma_start to vma_end */
	uk_rwlock_wlock(&vas->vma_list_lock);
	vma_start->vma_list.prev->next = vma_end->vma_list.next;
	vma_end->vma_list.next->prev   = vma_start->vma_list.prev;
	uk_rwlock_wunlock(&vas->vma_list_lock);

	vmem_vma_unmap_and_free_vmas(vma_start, vma_end);

	return 0;
}

static __vaddr_t vmem_first_fit(struct uk_vas *vas, __vaddr_t base, __sz align,
				__sz len)
{
	__vaddr_t vaddr = base;
	struct uk_vma *cur;

	UK_ASSERT(vas);

	/* Since we are scanning the VAS for an empty address range, we need
	 * to be careful not to overflow. Checks are thus always active and not
	 * just asserts.
	 */
	uk_rwlock_rlock(&vas->vma_list_lock);
	uk_list_for_each_entry(cur, &vas->vma_list, vma_list) {
		if (unlikely(vaddr > __VADDR_MAX - align)) {
			uk_rwlock_runlock(&vas->vma_list_lock);
			return __VADDR_INV;
		}

		vaddr = ALIGN_UP(vaddr, align);

		if (unlikely(vaddr > __VADDR_MAX - len)) {
			uk_rwlock_runlock(&vas->vma_list_lock);
			return __VADDR_INV;
		}

		if (vaddr + len <= cur->start) {
			uk_rwlock_runlock(&vas->vma_list_lock);
			return vaddr;
		}

		vaddr = MAX(cur->end, base);
	}
	uk_rwlock_runlock(&vas->vma_list_lock);

	if (unlikely(vaddr > __VADDR_MAX - align))
		return __VADDR_INV;

	return ALIGN_UP(vaddr, align);
}

static int vmem_mapx_populate(struct uk_pagetable *pt __unused,
			      __vaddr_t vaddr, __vaddr_t pt_vaddr __unused,
			      unsigned int level, __pte_t *pte, void *user)
{
	struct uk_vma *vma = (struct uk_vma *)user;
	struct uk_vm_fault fault = {
		.vaddr = vaddr,
		.vbase = vaddr,
		.len   = PAGE_Lx_SIZE(level),
		.paddr = PT_Lx_PTE_PADDR(*pte, level),
		.type  = UK_VMA_FAULT_SOFT | UK_VMA_FAULT_NONPRESENT,
		.pte   = *pte,
		.level = level,
		.regs  = __NULL,
	};
	int rc;

	UK_ASSERT(vma->ops);
	UK_ASSERT(vma->ops->fault);

	rc = vma->ops->fault(vma, &fault);
	if (unlikely(rc)) {
		if (rc == -ENOMEM)
			return UKPLAT_PAGE_MAPX_ETOOBIG;

		return rc;
	}

	UK_ASSERT(PAGE_Lx_ALIGNED(fault.paddr, fault.level));

	*pte = PT_Lx_PTE_SET_PADDR(fault.pte, fault.level, fault.paddr);

	return 0;
}

int uk_vma_map(struct uk_vas *vas, __vaddr_t *vaddr, __sz len,
	       unsigned long attr, unsigned long flags, const char *name,
	       const struct uk_vma_ops *ops, void *args)
{
	unsigned int order = UK_VMA_MAP_SIZE_TO_ORDER(flags);
	int rc, to_lvl, algn_lvl, strict;
	struct uk_vma *vma_start = __NULL;
	struct uk_vma *vma_end = __NULL;
	struct uk_vma *vma = __NULL;
	unsigned long extf;
	unsigned long flgs;
	__vaddr_t va, base;

	UK_ASSERT(vas);
	UK_ASSERT(vaddr);
	UK_ASSERT(ops);
	UK_ASSERT(len > 0);

	if (order > 0) {
		UK_ASSERT(order >= 12);

		to_lvl   = PAGE_SHIFT_Lx(order);
		algn_lvl = to_lvl;

		UK_ASSERT(PAGE_Lx_HAS(algn_lvl));
	} else {
		to_lvl   = -1;
		algn_lvl = PAGE_LEVEL;
	}

	if (unlikely(!PAGE_Lx_ALIGNED(len, algn_lvl)))
		return -EINVAL;

	va = *vaddr;
	uk_spin_lock(&vas->map_lock);
	if (va == __VADDR_ANY) {
		/* Select the first virtual address range starting at the
		 * mapping base that can accommodate the requested VMA.
		 */
		base = (ops->get_base) ? ops->get_base(vas, args, flags) :
					 vas->vma_base;

		va = vmem_first_fit(vas, base, PAGE_Lx_SIZE(algn_lvl), len);
		if (unlikely(va == __VADDR_INV)) {
			uk_spin_unlock(&vas->map_lock);
			return -ENOMEM;
		}
	} else {
		if (unlikely(!PAGE_Lx_ALIGNED(va, algn_lvl))) {
			uk_spin_unlock(&vas->map_lock);
			return -EINVAL;
		}

		/* The caller specified an exact address to map to. We only
		 * allow this if the mapping does not collide with other
		 * mappings. However, we enforce the new mapping if the replace
		 * flag is provided by unmapping everything in the range first.
		 */
		if ((vma_start = vmem_vma_find(vas, va, len))) {
			if (unlikely(!(flags & UK_VMA_MAP_REPLACE))) {
				uk_spin_unlock(&vas->map_lock);
				return -EEXIST;
			}

			strict = (flags & UK_VMA_FLAG_STRICT_VMA_CHECK);
			rc = vmem_vma_split_vmas(vas, va, len, __NULL,
						 &vma_start, &vma_end, strict);
			if (unlikely(rc)) {
				uk_spin_unlock(&vas->map_lock);
				return rc;
			}

			/* It could be that during the split operation we
			 * recognized that len must be aligned to a larger
			 * page size to allow the split.
			 */
			UK_ASSERT(vma_end->end > vma_start->start);
			len = vma_end->end - vma_start->start;
		}
	}

	UK_ASSERT(PAGE_Lx_ALIGNED(va, algn_lvl));
	UK_ASSERT(va <= __VADDR_MAX - len);
	UK_ASSERT(ukarch_vaddr_range_isvalid(va, len));

	/* Create a new VMA for the requested range. */
	if (ops->new) {
		/* Split also needs to allocate a new VMA */
		UK_ASSERT(ops->split);

		rc = ops->new(vas, va, len, args, attr, &flags, &vma);
		if (unlikely(rc)) {
			uk_spin_unlock(&vas->map_lock);
			return rc;
		}
	} else {
		vma = uk_malloc(vas->a, sizeof(struct uk_vma));
		if (unlikely(!vma)) {
			uk_spin_unlock(&vas->map_lock);
			return -ENOMEM;
		}

		vma->name = __NULL;
	}

	UK_ASSERT(vma);

	/* If this mapping replaces existing VMAs we now have to unmap them
	 * before we attempt to map the new VMA because this entail changes to
	 * the page table.
	 */
	if (vma_start) {
		UK_ASSERT(vma_end);

		/* Unlink all VMAs starting from vma_start to vma_end */

		uk_rwlock_wlock(&vas->vma_list_lock);
		vma_start->vma_list.prev->next = vma_end->vma_list.next;
		vma_end->vma_list.next->prev   = vma_start->vma_list.prev;
		uk_rwlock_wunlock(&vas->vma_list_lock);

		vmem_vma_unmap_and_free_vmas(vma_start, vma_end);
	}

	UK_ASSERT(vmem_vma_find(vas, va, len) == __NULL);

	/* Extract extended flags */
	extf = flags & (UK_VMA_MAP_EXTF_MASK << UK_VMA_MAP_EXTF_SHIFT);

	UK_INIT_LIST_HEAD(&vma->vma_list);
	vma->start	= va;
	vma->end	= va + len;
	vma->vas	= vas;
	vma->ops	= ops;
	vma->attr	= attr;
	vma->flags	= extf;
	vma->page_lvl	= to_lvl;
	if (name)
		vma->name = name;

	if (flags & UK_VMA_MAP_UNINITIALIZED)
		vma->flags |= UK_VMA_FLAG_UNINITIALIZED;

	if (flags & UK_VMA_MAP_POPULATE) {
		UK_ASSERT(vma->ops->fault);

		if (vma->page_lvl >= 0) {
			/* Enforce desired page size when populating the VMA */
			flgs = PAGE_FLAG_SIZE(vma->page_lvl) |
				PAGE_FLAG_FORCE_SIZE;
		} else {
			flgs = 0;
		}

		rc = ukplat_page_mapx(vas->pt, vma->start, 0,
				      len >> PAGE_Lx_SHIFT(algn_lvl),
				      vma->attr, flgs,
				      &(struct ukplat_page_mapx){
						.map = vmem_mapx_populate,
						.ctx = vma,
				      });
		if (unlikely(rc)) {
			/* If the address range replaces existing mappings, we
			 * have an unrecoverable error and the address range
			 * will be empty!
			 */
			vmem_vma_destroy(vma);
			uk_spin_unlock(&vas->map_lock);
			return rc;
		}
	}

	vmem_vma_insert(vas, vma);
	vmem_vma_try_merge(vma);

	uk_spin_unlock(&vas->map_lock);

	*vaddr = va;
	return 0;
}

int vma_op_set_attr(struct uk_vma *vma, unsigned long attr)
{
	unsigned long pgs = vmem_vma_len(vma) / PAGE_SIZE;

	return ukplat_page_set_attr(vma->vas->pt, vma->start, pgs, attr, 0);
}

static void vmem_vma_set_attr(struct uk_vma *vma, unsigned long attr)
{
	int rc;

	UK_ASSERT(vma);

	if (vma->attr == attr)
		return;

	/* Do not change attributes if there is no VMA handler for this */
	if (!vma->ops->set_attr)
		return;

	rc = VMA_SETATTR(vma, attr);
	if (unlikely(rc)) {
		/* We consider a failure to set attributes in an address range
		 * as fatal to simplify error handling. In contrast to unmap(),
		 * this can be changed if necessary.
		 */
		UK_CRASH("Failed to set attributes in address range "
			 "0x%" __PRIvaddr "-0x%" __PRIvaddr " to 0x%lx: %d",
			 vma->start, vma->end, attr, rc);
	}

	vma->attr = attr;
}

static void vmem_vma_set_attr_vmas(struct uk_vma *start, struct uk_vma *end,
				   unsigned long attr)
{
	struct uk_vma *vma = start;

	UK_ASSERT(start);
	UK_ASSERT(end);

	while (vma != end) {
		vmem_vma_set_attr(vma, attr);
		vma = uk_list_next_entry(vma, vma_list);
	}

	vmem_vma_set_attr(end, attr);

	/* Do a second pass and try to merge VMAs */
	vma = vmem_vma_try_merge_with_next(end);
	UK_ASSERT(vma == end);

	vma = start;
	while (vma != end) {
		vma = vmem_vma_try_merge_with_prev(vma);
		vma = uk_list_next_entry(vma, vma_list);
	}

	vmem_vma_try_merge_with_prev(end);
}

int uk_vma_set_attr(struct uk_vas *vas, __vaddr_t vaddr, __sz len,
		    unsigned long attr, unsigned long flags)
{
	struct uk_vma *vma_start = __NULL, *vma_end;
	int strict = (flags & UK_VMA_FLAG_STRICT_VMA_CHECK);
	int rc;

	if (unlikely(len == 0))
		return 0;

	rc = vmem_vma_split_vmas(vas, vaddr, len, &attr,
				 &vma_start, &vma_end, strict);
	if (unlikely(rc)) {
		if (rc == -ENOENT && !strict)
			return 0;

		return rc;
	}

	vmem_vma_set_attr_vmas(vma_start, vma_end, attr);

	return 0;
}

static int vmem_mapx_advise(struct uk_pagetable *pt,
			    __vaddr_t vaddr, __vaddr_t pt_vaddr,
			    unsigned int level, __pte_t *pte, void *user)
{
	__pte_t opte;
	int rc;

	/* With advise it can happen that we get calls for pages already
	 * present. Just ignore these.
	 */
	rc = ukarch_pte_read(pt_vaddr, level, PT_Lx_IDX(vaddr, level), &opte);
	if (unlikely(rc))
		return rc;

	if (PT_Lx_PTE_PRESENT(opte, level))
		return UKPLAT_PAGE_MAPX_ESKIP;

	return vmem_mapx_populate(pt, vaddr, pt_vaddr, level, pte, user);
}

int vma_op_advise(struct uk_vma *vma, __vaddr_t vaddr, __sz len,
		  unsigned long advice)
{
	unsigned int lvl;
	unsigned long flgs;
	int rc;

	/* WILLNEED takes precedence over DONTNEED */
	if (advice & UK_VMA_ADV_WILLNEED) {
		if (vma->page_lvl >= 0) {
			flgs = PAGE_FLAG_SIZE(vma->page_lvl) |
				PAGE_FLAG_FORCE_SIZE;
			lvl  = vma->page_lvl;
		} else {
			flgs = 0;
			lvl  = PAGE_LEVEL;
		}

		rc = ukplat_page_mapx(vma->vas->pt, vaddr, 0,
				      len >> PAGE_Lx_SHIFT(lvl),
				      vma->attr, flgs,
				      &(struct ukplat_page_mapx){
						.map = vmem_mapx_advise,
						.ctx = vma,
				      });
		if (unlikely(rc))
			return rc;
	} else if (advice & UK_VMA_ADV_DONTNEED) {
		/* Note: We are using the same semantic as Linux here, where
		 * DONTNEED means we will actually free the physical memory and
		 * not only swap it out.
		 */
		rc = vma_op_unmap(vma, vaddr, len);
		if (unlikely(rc))
			return rc;
	}

	/* Add further advices here */

	return 0;
}

static int vmem_vma_advise(struct uk_vma *vma, __vaddr_t vaddr, __sz len,
			   unsigned long advice)
{
	UK_ASSERT(vma);
	UK_ASSERT(vaddr <= __VADDR_MAX - len);
	UK_ASSERT(vaddr >= vma->start && vaddr + len <= vma->end);
	UK_ASSERT(PAGE_Lx_ALIGNED(vaddr, MAX(vma->page_lvl, PAGE_LEVEL)));
	UK_ASSERT(PAGE_Lx_ALIGNED(len, MAX(vma->page_lvl, PAGE_LEVEL)));

	return VMA_ADVISE(vma, vaddr, len, advice);
}

int uk_vma_advise(struct uk_vas *vas, __vaddr_t vaddr, __sz len,
		  unsigned long advice, unsigned long flags)
{
	struct uk_vma *vma_start = __NULL, *vma_end, *vma;
	int strict = (flags & UK_VMA_FLAG_STRICT_VMA_CHECK);
	__vaddr_t vend;
	int rc;

	if (unlikely(len == 0))
		return 0;

	rc = vmem_vma_find_range(vas, &vaddr, &len,
				 &vma_start, &vma_end, strict);
	if (unlikely(rc)) {
		if (rc == -ENOENT && !strict)
			return 0;

		return rc;
	}

	vend = vaddr + len;
	vma  = vma_start;
	while (vma != vma_end) {
		rc = vmem_vma_advise(vma, vaddr, vma->end - vaddr, advice);
		if (unlikely(rc))
			return rc;

		vma   = uk_list_next_entry(vma, vma_list);
		vaddr = vma->start;
	}

	return vmem_vma_advise(vma, vaddr, vend - vaddr, advice);
}

#ifdef CONFIG_HAVE_PAGING
static inline int vmem_largest_level(__vaddr_t vaddr, __sz len,
				   unsigned int max_lvl)
{
	unsigned int lvl = max_lvl;

	while (lvl > PAGE_LEVEL) {
		if (PAGE_Lx_HAS(lvl) &&
		    PAGE_Lx_ALIGNED(vaddr, lvl) &&
		    PAGE_Lx_SIZE(lvl) <= len)
			return lvl;

		lvl--;
	}

	return lvl;
}

static inline int vmem_access_allowed(unsigned long attr,
				      unsigned int faulttype)
{
	switch (faulttype & UK_VMA_FAULT_ACCESSTYPE) {
	case UK_VMA_FAULT_READ:
		return (attr & PAGE_ATTR_PROT_READ);
	case UK_VMA_FAULT_WRITE:
		return (attr & PAGE_ATTR_PROT_WRITE);
	case UK_VMA_FAULT_EXEC:
		return (attr & PAGE_ATTR_PROT_EXEC);
	}

	return 0;
}

struct mapx_pagefault_ctx {
	/** Faulting virtual address */
	__vaddr_t vaddr;
	/** Type of the fault */
	unsigned int type;
	/** Trap frame */
	struct __regs *regs;
	/** VMA in which the fault happens */
	struct uk_vma *vma;
};

static int vmem_mapx_pagefault(struct uk_pagetable *pt __unused,
			       __vaddr_t vaddr, __vaddr_t pt_vaddr __unused,
			       unsigned int level, __pte_t *pte, void *user)
{
	struct mapx_pagefault_ctx *ctx = (struct mapx_pagefault_ctx *)user;
	struct uk_vm_fault fault = {
		.vaddr = ctx->vaddr,
		.vbase = vaddr,
		.len   = PAGE_Lx_SIZE(level),
		.paddr = PT_Lx_PTE_PADDR(*pte, level),
		.type  = ctx->type,
		.pte   = *pte,
		.level = level,
		.regs  = ctx->regs,
	};
	int rc;

	UK_ASSERT(ctx->vma->ops);
	UK_ASSERT(ctx->vma->ops->fault);

	rc = ctx->vma->ops->fault(ctx->vma, &fault);
	if (unlikely(rc)) {
		if (rc == -ENOMEM)
			return UKPLAT_PAGE_MAPX_ETOOBIG;

		return rc;
	}

	UK_ASSERT(PAGE_Lx_ALIGNED(fault.paddr, fault.level));

	*pte = PT_Lx_PTE_SET_PADDR(fault.pte, fault.level, fault.paddr);

	return 0;
}

int vmem_pagefault(__vaddr_t vaddr, unsigned int type, struct __regs *regs)
{
	const unsigned int demand_lvl =
		PAGE_SHIFT_Lx(CONFIG_LIBUKVMEM_DEMAND_PAGE_IN_SIZE);
	struct uk_vas *vas;
	struct uk_pagetable *pt;
	struct mapx_pagefault_ctx ctx = {
		.vaddr = vaddr,
		.type  = type,
		.regs  = regs,
	};
	struct ukplat_page_mapx mapx = {
		.map = vmem_mapx_pagefault,
		.ctx = &ctx,
	};
	__vaddr_t vbase;
	unsigned int lvl = PAGE_LEVEL;
	unsigned long flags;
	int rc;

	/* Check if a virtual address space is set */
	vas = uk_vas_get_active();
	if (unlikely(!vas || vas->flags & UK_VAS_FLAG_NO_PAGING))
		return -EFAULT;

	/* If the page fault was caused by an access to a region not covered by
	 * a virtual memory area, fail early.
	 */
	ctx.vma = vmem_vma_find(vas, vaddr, 0);
	if (unlikely(!ctx.vma))
		return -EFAULT;

	UK_ASSERT(vaddr >= ctx.vma->start && vaddr < ctx.vma->end);

	/* Fail if the access is not compatible with the VMA's settings */
	if (unlikely(!vmem_access_allowed(ctx.vma->attr, type)))
		return -EFAULT;

	/* Fail early if the VMA does not have a fault handler */
	if (unlikely(!ctx.vma->ops->fault))
		return -EFAULT;

	UK_ASSERT(ctx.vma->vas);
	UK_ASSERT(ctx.vma->vas->pt);
	pt = ctx.vma->vas->pt;

	/* Find the page level at which we want to page-in. If the VMA does not
	 * enforce a specific page size and the configuration allows to page-in
	 * large pages, we first check up to which level we find page tables.
	 * We cannot create pages larger than that. Afterwards, we adjust
	 * according to alignment and VMA boundaries.
	 */
	if (ctx.vma->page_lvl < 0 && demand_lvl > PAGE_LEVEL) {
		rc = ukplat_pt_walk(pt, vaddr, &lvl, __NULL, __NULL);
		if (unlikely(rc))
			return rc;

		vbase = MAX(PAGE_Lx_ALIGN_DOWN(vaddr, lvl), ctx.vma->start);

		lvl = vmem_largest_level(vbase, ctx.vma->end - vbase,
					 MIN(lvl, demand_lvl));

		flags = PAGE_FLAG_FORCE_SIZE;
	} else {
		lvl   = MAX(ctx.vma->page_lvl, PAGE_LEVEL);
		flags = 0;
	}

	vbase = PAGE_Lx_ALIGN_DOWN(vaddr, lvl);

	UK_ASSERT(vbase >= ctx.vma->start &&
		  vbase < ctx.vma->end);
	UK_ASSERT(vbase <= __VADDR_MAX - PAGE_Lx_SIZE(lvl));
	UK_ASSERT(vbase + PAGE_Lx_SIZE(lvl) >= ctx.vma->start &&
		  vbase + PAGE_Lx_SIZE(lvl) <= ctx.vma->end);

	return ukplat_page_mapx(pt, vbase, 0, 1, ctx.vma->attr,
				PAGE_FLAG_SIZE(lvl) | flags, &mapx);
}
#endif /* CONFIG_HAVE_PAGING */
