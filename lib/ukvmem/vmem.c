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
#ifdef CONFIG_LIBUKPAGING
#include <uk/paging.h>
#endif /* CONFIG_LIBUKPAGING */
#include <uk/alloc.h>
#include <uk/assert.h>
#if CONFIG_LIBUKVMEM_VAS_ACCESS
#include <uk/atomic.h>
#endif
#include <uk/list.h>
#include <uk/pcpuvar.h>
#if CONFIG_LIBUKDEBUG_ENABLE_ASSERT
#include <uk/print.h>
#endif
#include <uk/plat/spinlock.h>
#include <uk/config.h>

static __uk_pcpuvar struct uk_vas *vmem_active_vas;
static __uk_pcpuvar struct uk_vas *vmem_no_paging_vas;

#if CONFIG_LIBUKVMEM_THREAD_VAS
static struct uk_vas *vmem_kernel_vas;

struct uk_vas *uk_vas_get_kernel(void)
{
	return __atomic_load_n(&vmem_kernel_vas, __ATOMIC_ACQUIRE);
}

void uk_vas_set_kernel(struct uk_vas *vas)
{
	UK_ASSERT(vas);
	UK_ASSERT(!uk_vas_get_kernel());
	__atomic_store_n(&vmem_kernel_vas, vas, __ATOMIC_RELEASE);
}
#endif

/* Forward declarations */
static void vmem_vma_unmap(struct uk_vma *vma, __vaddr_t vaddr, __sz len);

static void vmem_vma_destroy_list(struct uk_list_head *head);

struct uk_vas *uk_vas_get_active(void)
{
	return uk_pcpuvar_current_get(vmem_active_vas);
}

unsigned long uk_vas_paging_savef(struct uk_vas *vas)
{
	struct uk_vas *saved = uk_pcpuvar_current_get(vmem_no_paging_vas);

	uk_pcpuvar_current_set(vmem_no_paging_vas, vas);
	return (unsigned long)saved;
}

void uk_vas_paging_restoref(struct uk_vas *vas __maybe_unused,
			   unsigned long state)
{
	UK_ASSERT(uk_pcpuvar_current_get(vmem_no_paging_vas) == vas);
	uk_pcpuvar_current_set(vmem_no_paging_vas, (struct uk_vas *)state);
}

int uk_vas_paging_enabled(struct uk_vas *vas)
{
	return vas && !(UK_READ_ONCE(vas->flags) & UK_VAS_FLAG_NO_PAGING) &&
	       uk_pcpuvar_current_get(vmem_no_paging_vas) != vas;
}

int uk_vas_set_active(struct uk_vas *vas)
{
#ifdef CONFIG_LIBUKPAGING
	int rc;

	rc = uk_paging_pt_set_active(vas->pt);
	if (unlikely(rc))
		return rc;
#endif /* CONFIG_LIBUKPAGING */

	uk_pcpuvar_current_set(vmem_active_vas, vas);
	return 0;
}

int uk_vas_init(struct uk_vas *vas, struct uk_pagetable *pt __maybe_unused,
		struct uk_alloc *a)
{
	UK_ASSERT(vas);
	UK_ASSERT(pt);
	UK_ASSERT(a);

	vas->a = a;
#if CONFIG_LIBUKVMEM_VAS_REFCOUNT
	uk_refcount_init(&vas->refs, 1);
	vas->release = __NULL;
#endif

#ifdef CONFIG_LIBUKPAGING
	vas->pt = pt;

	vas->vma_base = UK_PAGING_PAGE_ALIGN_UP(CONFIG_LIBUKVMEM_DEFAULT_BASE);
	UK_ASSERT(uk_paging_vaddr_isvalid(vas->vma_base));
#endif /* CONFIG_LIBUKPAGING */

	vas->flags = 0;
	ukarch_spin_init(&vas->lock);

	UK_INIT_LIST_HEAD(&vas->vma_list);

	return 0;
}

#if CONFIG_LIBUKVMEM_VAS_REFCOUNT
int uk_vas_init_owned(struct uk_vas *vas, struct uk_pagetable *pt,
		      struct uk_alloc *a, void (*release)(struct uk_vas *))
{
	int rc;

	if (!release)
		return -EINVAL;
	rc = uk_vas_init(vas, pt, a);
	if (!rc)
		vas->release = release;
	return rc;
}

struct uk_vas *uk_vas_get(struct uk_vas *vas)
{
	if (uk_vas_is_owned(vas))
		uk_refcount_acquire(&vas->refs);
	return vas;
}

void uk_vas_put(struct uk_vas *vas)
{
	if (!uk_vas_is_owned(vas) || !uk_refcount_release(&vas->refs))
		return;
	UK_ASSERT(vas != uk_vas_get_active());
	UK_ASSERT(!uk_lcpu_irqs_disabled());
	vas->release(vas);
}
#endif

void uk_vas_destroy(struct uk_vas *vas)
{
	struct uk_paging_free_batch batch;
	UK_LIST_HEAD(dead_vmas);
	struct uk_vma *vma, *next;
	unsigned long irqf;

	ukplat_spin_lock_irqsave(&vas->lock, irqf);
	uk_paging_batch_begin(&batch, vas->pt);

	uk_list_for_each_entry_safe(vma, next, &vas->vma_list, vma_list) {
		vmem_vma_unmap(vma, vma->start, uk_vma_len(vma));
		uk_list_del_init(&vma->vma_list);
		uk_list_add_tail(&vma->vma_list, &dead_vmas);
	}

	UK_ASSERT(uk_list_empty(&vas->vma_list));

	if (uk_vas_get_active() == vas)
		uk_pcpuvar_current_set(vmem_active_vas, __NULL);

	uk_paging_batch_end(&batch);
	ukplat_spin_unlock_irqrestore(&vas->lock, irqf);

	uk_paging_free_batch_finish(&batch);
	vmem_vma_destroy_list(&dead_vmas);
}

static void vmem_vma_destroy(struct uk_vma *vma)
{
	UK_ASSERT(vma);

	while (__atomic_load_n(&vma->sync_inflight, __ATOMIC_ACQUIRE) != 0)
		uk_arch_spinwait();

	if (vma->ops->destroy)
		vma->ops->destroy(vma);

	uk_free(vma->vas->a, vma);
}

static void vmem_vma_destroy_list(struct uk_list_head *head)
{
	struct uk_vma *vma, *next;

	uk_list_for_each_entry_safe(vma, next, head, vma_list) {
		uk_list_del_init(&vma->vma_list);
		vmem_vma_destroy(vma);
	}
}

static inline void vmem_vma_defer_destroy(struct uk_vma *vma,
					  struct uk_list_head *dead_vmas)
{
	UK_ASSERT(vma);
	UK_ASSERT(uk_list_empty(&vma->vma_list));

	uk_list_add_tail(&vma->vma_list, dead_vmas);
}

static struct uk_vma *vmem_vma_find(struct uk_vas *vas, __vaddr_t vaddr,
				    __sz len)
{
	struct uk_vma *vma;
	__vaddr_t vstart = vaddr;
	__vaddr_t vend = vaddr + MAX(len, (__sz)1);

	UK_ASSERT(vas);
	UK_ASSERT(vaddr <= __VADDR_MAX - len);

	uk_list_for_each_entry(vma, &vas->vma_list, vma_list) {
		if ((vend > vma->start) && (vstart < vma->end))
			return vma;
	}

	return __NULL;
}

const struct uk_vma *uk_vma_find(struct uk_vas *vas, __vaddr_t vaddr)
{
	const struct uk_vma *vma;
	unsigned long irqf;

	ukplat_spin_lock_irqsave(&vas->lock, irqf);
	vma = vmem_vma_find(vas, vaddr, 0);
	ukplat_spin_unlock_irqrestore(&vas->lock, irqf);
	return vma;
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
		algn_lvl = MAX(vma_start->page_lvl, UK_PAGING_PAGE_LEVEL);
		if (!UK_PAGING_PAGE_Lx_ALIGNED(vstart, algn_lvl))
			return -EINVAL;
	}

	UK_ASSERT(UK_PAGING_PAGE_Lx_ALIGNED(vstart, MAX(vma_start->page_lvl,
							UK_PAGING_PAGE_LEVEL)));

	/* Find the end VMA */
	vma_end = vma_start;
	if (likely(vl > 0)) {
		while (vma_end->vma_list.next != &vas->vma_list) {
			if (vend > vma_end->start && vend <= vma_end->end)
				break;

			/* In strict mode, we do not allow the specified
			 * address range to contain holes between VMAs
			 */
			next = uk_list_next_entry(vma_end, vma_list);
			if (strict && vma_end->end != next->start)
				return -ENOENT;

			if (vend <= next->start)
				break;

			vma_end = next;
		}

		/* Adjust vend to make sure it is within the VMA. We expect
		 * vend to be properly aligned by the caller (via len).
		 */
		if (vend > vma_end->end) {
			if (strict)
				return -ENOENT;

			vend = vma_end->end;
		} else if (vend < vma_end->end) {
			algn_lvl = MAX(vma_end->page_lvl, UK_PAGING_PAGE_LEVEL);
			if (!UK_PAGING_PAGE_Lx_ALIGNED(vend, algn_lvl))
				return -EINVAL;
		}
	}

	UK_ASSERT(UK_PAGING_PAGE_Lx_ALIGNED(vend, MAX(vma_end->page_lvl,
						      UK_PAGING_PAGE_LEVEL)));

	UK_ASSERT(vend > vstart);
	UK_ASSERT(vstart >= vma_start->start && vstart < vma_start->end);
	UK_ASSERT(vend > vma_end->start && vend <= vma_end->end);
	UK_ASSERT(uk_paging_vaddr_range_isvalid(vstart, vl));

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
	uk_list_for_each_entry(cur, &vas->vma_list, vma_list) {
		if (vma->start < cur->end) {
			UK_ASSERT(vma->end <= cur->start);

			uk_list_add(&vma->vma_list, prev);
			return;
		}

		prev = &cur->vma_list;
	}

	uk_list_add_tail(&vma->vma_list, &vas->vma_list);
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

static int vmem_vma_do_try_merge_with_next(struct uk_vma *vma,
					   struct uk_list_head *dead_vmas)
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

	/* Unlink the merged next VMA; its destructor runs outside vas->lock */
	uk_list_del_init(&next->vma_list);
	vmem_vma_defer_destroy(next, dead_vmas);

	return 0;
}

static struct uk_vma *
vmem_vma_try_merge_with_next(struct uk_vma *vma, struct uk_list_head *dead_vmas)
{
	vmem_vma_do_try_merge_with_next(vma, dead_vmas);
	return vma;
}

static struct uk_vma *
vmem_vma_try_merge_with_prev(struct uk_vma *vma, struct uk_list_head *dead_vmas)
{
	struct uk_vma *prev;

	if (vma->vma_list.prev == &vma->vas->vma_list)
		return vma;

	prev = uk_list_prev_entry(vma, vma_list);
	return (vmem_vma_do_try_merge_with_next(prev, dead_vmas) == 0) ?
	       prev : vma;
}

static struct uk_vma *vmem_vma_try_merge(struct uk_vma *vma,
					 struct uk_list_head *dead_vmas)
{
	vmem_vma_try_merge_with_next(vma, dead_vmas);
	return vmem_vma_try_merge_with_prev(vma, dead_vmas);
}

static int vmem_vma_split(struct uk_vma *vma, __vaddr_t vaddr,
			  struct uk_vma **new_vma)
{
	struct uk_vma *v = __NULL;
	int rc;

	UK_ASSERT(vma);
	UK_ASSERT(new_vma);

	UK_ASSERT(vaddr >= vma->start && vaddr < vma->end);
	UK_ASSERT(UK_PAGING_PAGE_Lx_ALIGNED(vaddr, MAX(vma->page_lvl,
						       UK_PAGING_PAGE_LEVEL)));
	UK_ASSERT(UK_PAGING_PAGE_Lx_ALIGNED(vma->end - vaddr,
					    MAX(vma->page_lvl,
						UK_PAGING_PAGE_LEVEL)));

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
	v->sync_inflight = 0;
	v->name		= vma->name;

	vma->end	= vaddr;

	uk_list_add(&v->vma_list, &vma->vma_list);

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
			       struct uk_vma **end, int strict,
			       struct uk_list_head *dead_vmas)
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
				vmem_vma_try_merge(vma_start, dead_vmas);
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

/* Argument list is empty to accept (and ignore) any number of args.
 *
 * Do not convert to `uk_vma_op_deny(void)` no matter how much your static code
 * checker screams at you.
 */
int uk_vma_op_deny()
{
	return -EPERM;
}

int uk_vma_op_unmap(struct uk_vma *vma, __vaddr_t vaddr, __sz len)
{
	UK_ASSERT(vaddr >= vma->start);
	UK_ASSERT(vaddr + len <= vma->end);
	UK_ASSERT(UK_PAGING_PAGE_ALIGNED(len));

	return uk_paging_page_unmap(vma->vas->pt, vaddr,
				    len / UK_PAGING_PAGE_SIZE, 0);
}

static void vmem_vma_unmap(struct uk_vma *vma, __vaddr_t vaddr, __sz len)
{
	int rc;

	UK_ASSERT(vma);

	UK_ASSERT(vaddr >= vma->start && vaddr < vma->end);
	UK_ASSERT(UK_PAGING_PAGE_Lx_ALIGNED(vaddr, MAX(vma->page_lvl,
						       UK_PAGING_PAGE_LEVEL)));
	UK_ASSERT(vaddr <= __VADDR_MAX - len);
	UK_ASSERT((vaddr + len) > vma->start && (vaddr + len) <= vma->end);
	UK_ASSERT(UK_PAGING_PAGE_Lx_ALIGNED(len, MAX(vma->page_lvl,
						     UK_PAGING_PAGE_LEVEL)));
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

static void vmem_vma_unmap_and_defer_vmas(struct uk_vma *start,
					  struct uk_vma *end,
					  struct uk_list_head *dead_vmas)
{
	struct uk_vma *vma = start, *next;

	UK_ASSERT(start);
	UK_ASSERT(end);

	/* Unmap PTEs under vas->lock; defer VMA destruction outside lock */
	while (1) {
		int is_end = (vma == end);

		next = is_end ? __NULL : uk_list_next_entry(vma, vma_list);
		vmem_vma_unmap(vma, vma->start, uk_vma_len(vma));
		UK_INIT_LIST_HEAD(&vma->vma_list);
		vmem_vma_defer_destroy(vma, dead_vmas);
		if (is_end)
			break;
		vma = next;
	}
}

static int vmem_do_unmap_locked(struct uk_vas *vas, __vaddr_t vaddr, __sz len,
				unsigned long flags,
				struct uk_list_head *dead_vmas)
{
	struct uk_vma *vma_start = __NULL, *vma_end;
	int strict = (flags & UK_VMA_FLAG_STRICT_VMA_CHECK);
	int rc;

	rc = vmem_vma_split_vmas(vas, vaddr, len, __NULL,
				 &vma_start, &vma_end, strict, dead_vmas);
	if (unlikely(rc)) {
		if (rc == -ENOENT && !strict)
			return 0;

		return rc;
	}

	/* Unlink all VMAs starting from vma_start to vma_end */
	vma_start->vma_list.prev->next = vma_end->vma_list.next;
	vma_end->vma_list.next->prev   = vma_start->vma_list.prev;

	vmem_vma_unmap_and_defer_vmas(vma_start, vma_end, dead_vmas);

	return 0;
}

int uk_vma_unmap(struct uk_vas *vas, __vaddr_t vaddr, __sz len,
		 unsigned long flags)
{
	struct uk_paging_free_batch batch;
	UK_LIST_HEAD(dead_vmas);
	unsigned long irqf;
	int rc;

	if (unlikely(len == 0))
		return 0;

	ukplat_spin_lock_irqsave(&vas->lock, irqf);
	uk_paging_batch_begin(&batch, vas->pt);
	rc = vmem_do_unmap_locked(vas, vaddr, len, flags, &dead_vmas);
	uk_paging_batch_end(&batch);
	ukplat_spin_unlock_irqrestore(&vas->lock, irqf);

	uk_paging_free_batch_finish(&batch);
	vmem_vma_destroy_list(&dead_vmas);

	return rc;
}

int uk_vma_unmap_at(struct uk_vas *vas, __vaddr_t vaddr)
{
	struct uk_paging_free_batch batch;
	UK_LIST_HEAD(dead_vmas);
	struct uk_vma *vma;
	unsigned long irqf;
	int rc;

	ukplat_spin_lock_irqsave(&vas->lock, irqf);
	uk_paging_batch_begin(&batch, vas->pt);
	vma = vmem_vma_find(vas, vaddr, 0);
	if (unlikely(!vma))
		rc = -ENOENT;
	else
		rc = vmem_do_unmap_locked(vas, vma->start,
					  vma->end - vma->start, 0,
					  &dead_vmas);
	uk_paging_batch_end(&batch);
	ukplat_spin_unlock_irqrestore(&vas->lock, irqf);

	uk_paging_free_batch_finish(&batch);
	vmem_vma_destroy_list(&dead_vmas);

	return rc;
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
	uk_list_for_each_entry(cur, &vas->vma_list, vma_list) {
		if (unlikely(vaddr > __VADDR_MAX - align))
			return UK_PAGING_VADDR_INV;

		vaddr = ALIGN_UP(vaddr, align);

		if (unlikely(vaddr > __VADDR_MAX - len))
			return UK_PAGING_VADDR_INV;

		if (vaddr + len <= cur->start)
			return vaddr;

		vaddr = MAX(cur->end, base);
	}

	if (unlikely(vaddr > __VADDR_MAX - align))
		return UK_PAGING_VADDR_INV;

	return ALIGN_UP(vaddr, align);
}

struct vmem_mapx_ctx {
	/** Faulting virtual address */
	__vaddr_t vaddr;
	/** Type of the fault */
	unsigned int type;
	/** Trap frame */
	struct uk_lcpu_regs *regs;
	/** VMA in which the fault happens */
	struct uk_vma *vma;
#if CONFIG_LIBUKVMEM_COW
	struct uk_vm_fault_op *operation;
#endif
};

#if CONFIG_LIBUKVMEM_COW
static void vmem_mapx_result(struct uk_pagetable *pt,
			     __vaddr_t vaddr __unused,
			     __vaddr_t pt_vaddr __unused,
			     unsigned int level __unused,
			     int result, void *user)
{
	struct vmem_mapx_ctx *ctx = user;
	struct uk_vm_fault_op *op = ctx->operation;

	ctx->operation = NULL;
	if (op) {
		UK_ASSERT(op->result);
		UK_ASSERT(pt->defer_batch);
		op->result(op, result, pt->defer_batch);
	}
}
#endif

static inline int vmem_page_mapx(struct uk_pagetable *pt, __vaddr_t vaddr,
				 __paddr_t paddr, unsigned long pages,
				 unsigned long attr, unsigned long flags,
				 struct uk_paging_page_mapx *mapx)
{
#if CONFIG_LIBUKVMEM_COW
	return uk_paging_page_mapx_with_result(pt, vaddr, paddr, pages, attr,
					     flags, mapx, vmem_mapx_result);
#else
	return uk_paging_page_mapx(pt, vaddr, paddr, pages, attr, flags, mapx);
#endif
}

static int vmem_mapx_populate(struct uk_pagetable *pt __unused,
			      __vaddr_t vaddr, __vaddr_t pt_vaddr __unused,
			      unsigned int level, __pte_t *pte, void *user)
{
	struct vmem_mapx_ctx *ctx = user;
	struct uk_vma *vma = ctx->vma;
	struct uk_vm_fault fault = {
		.vaddr = vaddr,
		.vbase = vaddr,
		.len   = UK_PAGING_PAGE_Lx_SIZE(level),
		.paddr = UK_PAGING_PT_Lx_PTE_PADDR(*pte, level),
		.type  = UK_VMA_FAULT_SOFT | UK_VMA_FAULT_NONPRESENT,
		.pte   = *pte,
		.level = level,
		.regs  = __NULL,
#if CONFIG_LIBUKVMEM_COW
		.operation = &ctx->operation,
#endif
	};
	int rc;

	UK_ASSERT(vma->ops);
	UK_ASSERT(vma->ops->fault);

	rc = vma->ops->fault(vma, &fault);
	if (unlikely(rc)) {
#if CONFIG_LIBUKDEBUG_ENABLE_ASSERT
		if (rc != -ENOMEM)
			uk_pr_err_isr("VMA population failed at %#lx, level %u: %d\n",
				      (unsigned long)vaddr, level, rc);
#endif
		if (rc == -ENOMEM)
			return UK_PAGING_PAGE_MAPX_ETOOBIG;

		return rc;
	}

	UK_ASSERT(UK_PAGING_PAGE_Lx_ALIGNED(fault.paddr, fault.level));

	*pte = UK_PAGING_PT_Lx_PTE_SET_PADDR(fault.pte, fault.level,
					     fault.paddr);

	return 0;
}

int uk_vma_map(struct uk_vas *vas, __vaddr_t *vaddr, __sz len,
	       unsigned long attr, unsigned long flags, const char *name,
	       const struct uk_vma_ops *ops, void *args)
{
	unsigned int order = UK_VMA_MAP_SIZE_TO_ORDER(flags);
	struct uk_paging_free_batch batch;
	UK_LIST_HEAD(dead_vmas);
	int rc, to_lvl, algn_lvl, strict;
	struct uk_vma *vma_start = __NULL;
	struct uk_vma *vma_end = __NULL;
	struct uk_vma *vma = __NULL;
	unsigned long irqf;
	unsigned long extf;
	unsigned long flgs;
	__vaddr_t va, base;

	UK_ASSERT(vas);
	UK_ASSERT(vaddr);
	UK_ASSERT(ops);
	UK_ASSERT(len > 0);

	if (order > 0) {
		UK_ASSERT(order >= 12);

		to_lvl   = UK_PAGING_PAGE_SHIFT_Lx(order);
		algn_lvl = to_lvl;

		UK_ASSERT(UK_PAGING_PAGE_Lx_HAS(algn_lvl));
	} else {
		to_lvl   = -1;
		algn_lvl = UK_PAGING_PAGE_LEVEL;
	}

	if (unlikely(!UK_PAGING_PAGE_Lx_ALIGNED(len, algn_lvl))) {
#if CONFIG_LIBUKDEBUG_ENABLE_ASSERT
		uk_pr_err("Invalid VMA length %#lx for level %d\n",
			  (unsigned long)len, algn_lvl);
#endif
		return -EINVAL;
	}

	va = *vaddr;
	if (va != UK_PAGING_VADDR_ANY) {
		if (unlikely(!UK_PAGING_PAGE_Lx_ALIGNED(va, algn_lvl))) {
#if CONFIG_LIBUKDEBUG_ENABLE_ASSERT
			uk_pr_err("SMP_VM_IO stage=vma_address value=%#lx level=%d\n",
				  (unsigned long)va, algn_lvl);
#endif
			return -EINVAL;
		}
		if (unlikely(va > __VADDR_MAX - len ||
			     !uk_paging_vaddr_range_isvalid(va, len)))
			return -ENOMEM;
	}

	/* Create a new VMA in sleepable thread context before taking vas->lock
	 * so file-backed ops->new never blocks with interrupts disabled.
	 */
	if (ops->new) {
		/* Split also needs to allocate a new VMA */
		UK_ASSERT(ops->split);

		rc = ops->new(vas, va, len, args, attr, &flags, &vma);
		if (unlikely(rc)) {
#if CONFIG_LIBUKDEBUG_ENABLE_ASSERT
			uk_pr_err("SMP_VM_IO stage=vma_new rc=%d va=%#lx len=%#lx flags=%#lx\n",
				  rc, (unsigned long)va, (unsigned long)len, flags);
#endif
			return rc;
		}
	} else {
		vma = uk_malloc(vas->a, sizeof(struct uk_vma));
		if (unlikely(!vma))
			return -ENOMEM;

		vma->name = __NULL;
	}

	UK_ASSERT(vma);
	UK_INIT_LIST_HEAD(&vma->vma_list);
	vma->vas = vas;
	vma->ops = ops;
	vma->sync_inflight = 0;

	ukplat_spin_lock_irqsave(&vas->lock, irqf);
	uk_paging_batch_begin(&batch, vas->pt);

	if (va == UK_PAGING_VADDR_ANY) {
		/* Select the first virtual address range starting at the
		 * mapping base that can accommodate the requested VMA.
		 */
		base = (ops->get_base) ? ops->get_base(vas, args, flags) :
					 vas->vma_base;

		va = vmem_first_fit(vas, base, UK_PAGING_PAGE_Lx_SIZE(algn_lvl),
				    len);
		if (unlikely(va == UK_PAGING_VADDR_INV ||
			     va > __VADDR_MAX - len ||
			     !uk_paging_vaddr_range_isvalid(va, len))) {
			rc = -ENOMEM;
			goto err_unlock_destroy;
		}
	} else {
		/* The caller specified an exact address to map to. We only
		 * allow this if the mapping does not collide with other
		 * mappings. However, we enforce the new mapping if the replace
		 * flag is provided by unmapping everything in the range first.
		 */
		if ((vma_start = vmem_vma_find(vas, va, len))) {
			if (unlikely(!(flags & UK_VMA_MAP_REPLACE))) {
				rc = -EEXIST;
				goto err_unlock_destroy;
			}

			strict = (flags & UK_VMA_FLAG_STRICT_VMA_CHECK);
			rc = vmem_vma_split_vmas(vas, va, len, __NULL,
						 &vma_start, &vma_end, strict,
						 &dead_vmas);
			if (unlikely(rc))
				goto err_unlock_destroy;

			/* It could be that during the split operation we
			 * recognized that len must be aligned to a larger
			 * page size to allow the split.
			 */
			UK_ASSERT(vma_end->end > vma_start->start);
			if (!ops->new &&
			    vma_end->end - vma_start->start > len)
				len = vma_end->end - vma_start->start;
		}
	}

	UK_ASSERT(UK_PAGING_PAGE_Lx_ALIGNED(va, algn_lvl));
	UK_ASSERT(va <= __VADDR_MAX - len);

	/* Preserve mmap's ENOMEM contract for invalid address ranges. */
	if (unlikely(!uk_paging_vaddr_range_isvalid(va, len))) {
		rc = -ENOMEM;
		goto err_unlock_destroy;
	}

	/* If this mapping replaces existing VMAs we now have to unmap them
	 * before we attempt to map the new VMA because this entail changes to
	 * the page table.
	 */
	if (vma_start) {
		UK_ASSERT(vma_end);

		/* Unlink all VMAs starting from vma_start to vma_end */
		vma_start->vma_list.prev->next = vma_end->vma_list.next;
		vma_end->vma_list.next->prev   = vma_start->vma_list.prev;

		vmem_vma_unmap_and_defer_vmas(vma_start, vma_end, &dead_vmas);
	}

	UK_ASSERT(vmem_vma_find(vas, va, len) == __NULL);

	/* Extract extended flags */
	extf = flags & (UK_VMA_MAP_EXTF_MASK << UK_VMA_MAP_EXTF_SHIFT);

	vma->start	= va;
	vma->end	= va + len;
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
			flgs = UK_PAGING_PAGE_FLAG_SIZE(vma->page_lvl) |
				UK_PAGING_PAGE_FLAG_FORCE_SIZE;
		} else {
			flgs = 0;
		}

		rc = vmem_page_mapx(vas->pt, vma->start, 0,
				    len >> UK_PAGING_PAGE_Lx_SHIFT(algn_lvl),
				    vma->attr, flgs,
				    &(struct uk_paging_page_mapx){
					   .map = vmem_mapx_populate,
					   .ctx = &(struct vmem_mapx_ctx){
						   .vma = vma
					   },
				    });
		if (unlikely(rc)) {
#if CONFIG_LIBUKDEBUG_ENABLE_ASSERT
			uk_pr_err_isr("Cannot map VMA at %#lx, length %#lx: %d\n",
				      (unsigned long)vma->start,
				      (unsigned long)len, rc);
#endif
			/* A partial mapx can leave PTEs. Remove them through
			 * the owner before dropping its backing references;
			 * private and borrowed frames have different
			 * ownership rules.
			 */
			vmem_vma_unmap(vma, vma->start, uk_vma_len(vma));
			goto err_unlock_destroy;
		}
	}

	vmem_vma_insert(vas, vma);
	vmem_vma_try_merge(vma, &dead_vmas);

	uk_paging_batch_end(&batch);
	ukplat_spin_unlock_irqrestore(&vas->lock, irqf);

	uk_paging_free_batch_finish(&batch);
	vmem_vma_destroy_list(&dead_vmas);

	*vaddr = va;
	return 0;

err_unlock_destroy:
	vmem_vma_defer_destroy(vma, &dead_vmas);
	uk_paging_batch_end(&batch);
	ukplat_spin_unlock_irqrestore(&vas->lock, irqf);

	uk_paging_free_batch_finish(&batch);
	vmem_vma_destroy_list(&dead_vmas);
	return rc;
}

int uk_vma_op_set_attr(struct uk_vma *vma, unsigned long attr)
{
	unsigned long pgs = uk_vma_len(vma) / UK_PAGING_PAGE_SIZE;

	return uk_paging_page_set_attr(vma->vas->pt, vma->start, pgs, attr, 0);
}

static int vmem_vma_set_attr(struct uk_vma *vma, unsigned long attr)
{
	int rc;

	if (vma->attr == attr || !vma->ops->set_attr)
		return 0;
	rc = VMA_SETATTR(vma, attr);
	if (unlikely(rc))
		return rc;
	vma->attr = attr;
	return 0;
}

static int vmem_vma_set_attr_vmas(struct uk_vma *start, struct uk_vma *end,
				  unsigned long attr,
				  struct uk_list_head *dead_vmas)
{
	struct uk_vma *vma = start;
	int rc;

	for (;;) {
		rc = vmem_vma_set_attr(vma, attr);
		if (unlikely(rc))
			return rc;
		if (vma == end)
			break;
		vma = uk_list_next_entry(vma, vma_list);
	}

	/* Merge only after every VMA accepted its new permissions. */
	vmem_vma_try_merge_with_next(end, dead_vmas);
	vma = start;
	while (vma != end) {
		vma = vmem_vma_try_merge_with_prev(vma, dead_vmas);
		vma = uk_list_next_entry(vma, vma_list);
	}
	vmem_vma_try_merge_with_prev(end, dead_vmas);
	return 0;
}

int uk_vma_set_attr(struct uk_vas *vas, __vaddr_t vaddr, __sz len,
		    unsigned long attr, unsigned long flags)
{
	struct uk_paging_free_batch batch;
	UK_LIST_HEAD(dead_vmas);
	struct uk_vma *vma_start = __NULL, *vma_end;
	int strict = (flags & UK_VMA_FLAG_STRICT_VMA_CHECK);
	unsigned long irqf;
	int rc;

	if (unlikely(len == 0))
		return 0;

	ukplat_spin_lock_irqsave(&vas->lock, irqf);
	uk_paging_batch_begin(&batch, vas->pt);

	rc = vmem_vma_split_vmas(vas, vaddr, len, &attr,
				 &vma_start, &vma_end, strict, &dead_vmas);
	if (unlikely(rc)) {
		if (rc == -ENOENT && !strict)
			rc = 0;
		goto out;
	}

	rc = vmem_vma_set_attr_vmas(vma_start, vma_end, attr, &dead_vmas);

out:
	uk_paging_batch_end(&batch);
	ukplat_spin_unlock_irqrestore(&vas->lock, irqf);

	uk_paging_free_batch_finish(&batch);
	vmem_vma_destroy_list(&dead_vmas);
	return rc;
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
	rc = uk_paging_pte_read(pt_vaddr, level,
				UK_PAGING_PT_Lx_IDX(vaddr, level), &opte);
	if (unlikely(rc))
		return rc;

	if (UK_PAGING_PT_Lx_PTE_PRESENT(opte, level))
		return UK_PAGING_PAGE_MAPX_ESKIP;

	return vmem_mapx_populate(pt, vaddr, pt_vaddr, level, pte, user);
}

int uk_vma_op_advise(struct uk_vma *vma, __vaddr_t vaddr, __sz len,
		     unsigned long advice)
{
	unsigned int lvl;
	unsigned long flgs;
	int rc;

	/* WILLNEED takes precedence over DONTNEED */
	if (advice & UK_VMA_ADV_WILLNEED) {
		if (vma->page_lvl >= 0) {
			flgs = UK_PAGING_PAGE_FLAG_SIZE(vma->page_lvl) |
				UK_PAGING_PAGE_FLAG_FORCE_SIZE;
			lvl  = vma->page_lvl;
		} else {
			flgs = 0;
			lvl  = UK_PAGING_PAGE_LEVEL;
		}

		rc = vmem_page_mapx(vma->vas->pt, vaddr, 0,
				    len >> UK_PAGING_PAGE_Lx_SHIFT(lvl),
				    vma->attr, flgs,
				    &(struct uk_paging_page_mapx){
					   .map = vmem_mapx_advise,
					   .ctx = &(struct vmem_mapx_ctx){
						   .vma = vma
					   },
				    });
		if (unlikely(rc))
			return rc;
	} else if (advice & UK_VMA_ADV_DONTNEED) {
		/* Note: We are using the same semantic as Linux here, where
		 * DONTNEED means we will actually free the physical memory and
		 * not only swap it out.
		 */
		rc = uk_vma_op_unmap(vma, vaddr, len);
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
	UK_ASSERT(UK_PAGING_PAGE_Lx_ALIGNED(vaddr, MAX(vma->page_lvl,
						       UK_PAGING_PAGE_LEVEL)));
	UK_ASSERT(UK_PAGING_PAGE_Lx_ALIGNED(len, MAX(vma->page_lvl,
						     UK_PAGING_PAGE_LEVEL)));

	return VMA_ADVISE(vma, vaddr, len, advice);
}

int uk_vma_advise(struct uk_vas *vas, __vaddr_t vaddr, __sz len,
		  unsigned long advice, unsigned long flags)
{
	struct uk_paging_free_batch batch;
	struct uk_vma *vma_start = __NULL, *vma_end, *vma;
	int strict = (flags & UK_VMA_FLAG_STRICT_VMA_CHECK);
	unsigned long irqf;
	__vaddr_t vend;
	int rc;

	if (unlikely(len == 0))
		return 0;

	ukplat_spin_lock_irqsave(&vas->lock, irqf);
	uk_paging_batch_begin(&batch, vas->pt);

	rc = vmem_vma_find_range(vas, &vaddr, &len,
				 &vma_start, &vma_end, strict);
	if (unlikely(rc)) {
		if (rc == -ENOENT && !strict)
			rc = 0;
		goto out;
	}

	vend = vaddr + len;
	vma  = vma_start;
	while (vma != vma_end) {
		rc = vmem_vma_advise(vma, vaddr, vma->end - vaddr, advice);
		if (unlikely(rc))
			goto out;

		vma   = uk_list_next_entry(vma, vma_list);
		vaddr = vma->start;
	}

	rc = vmem_vma_advise(vma, vaddr, vend - vaddr, advice);

out:
	uk_paging_batch_end(&batch);
	ukplat_spin_unlock_irqrestore(&vas->lock, irqf);

	uk_paging_free_batch_finish(&batch);
	return rc;
}

#define VMEM_SYNC_INLINE_SEGS 16

struct vmem_sync_seg {
	struct uk_vma *vma;
	__vaddr_t vaddr;
	__sz len;
};

int uk_vma_sync(struct uk_vas *vas, __vaddr_t vaddr, __sz len)
{
	struct vmem_sync_seg inline_segs[VMEM_SYNC_INLINE_SEGS];
	struct vmem_sync_seg *segs = inline_segs;
	struct uk_vma *first = __NULL, *last, *vma;
	__sz count = 0, idx = 0, i;
	unsigned long irqf;
	__vaddr_t cur, end;
	int rc = 0;

	if (!UK_PAGING_PAGE_ALIGNED(vaddr) || !UK_PAGING_PAGE_ALIGNED(len))
		return -EINVAL;
	if (vaddr > __VADDR_MAX - len)
		return -ENOMEM;
	if (!len)
		return 0;

	ukplat_spin_lock_irqsave(&vas->lock, irqf);
	rc = vmem_vma_find_range(vas, &vaddr, &len, &first, &last, 1);
	if (unlikely(rc)) {
		ukplat_spin_unlock_irqrestore(&vas->lock, irqf);
		return rc;
	}

	end = vaddr + len;
	vma = first;
	for (;;) {
		if (vma->ops->sync)
			count++;
		if (vma == last)
			break;
		vma = uk_list_next_entry(vma, vma_list);
	}

	if (!count) {
		ukplat_spin_unlock_irqrestore(&vas->lock, irqf);
		return 0;
	}

	if (unlikely(count > VMEM_SYNC_INLINE_SEGS)) {
		if (!vas->a) {
			ukplat_spin_unlock_irqrestore(&vas->lock, irqf);
			return -ENOMEM;
		}
		segs = uk_malloc(vas->a, count * sizeof(*segs));
		if (!segs) {
			ukplat_spin_unlock_irqrestore(&vas->lock, irqf);
			return -ENOMEM;
		}
	}

	cur = vaddr;
	vma = first;
	for (;;) {
		__sz part = MIN(vma->end, end) - cur;

		if (vma->ops->sync) {
			segs[idx].vma = vma;
			segs[idx].vaddr = cur;
			segs[idx].len = part;
			__atomic_add_fetch(&vma->sync_inflight, 1,
					   __ATOMIC_ACQ_REL);
			idx++;
		}
		if (vma == last)
			break;
		vma = uk_list_next_entry(vma, vma_list);
		cur = vma->start;
	}
	ukplat_spin_unlock_irqrestore(&vas->lock, irqf);

	for (i = 0; i < idx; i++) {
		if (!rc)
			rc = segs[i].vma->ops->sync(segs[i].vma, segs[i].vaddr,
						    segs[i].len);
		__atomic_sub_fetch(&segs[i].vma->sync_inflight, 1,
				   __ATOMIC_RELEASE);
	}

	if (segs != inline_segs)
		uk_free(vas->a, segs);

	return rc;
}

#ifdef CONFIG_LIBUKPAGING
static inline int vmem_largest_level(__vaddr_t vaddr, __sz len,
				   unsigned int max_lvl)
{
	unsigned int lvl = max_lvl;

	while (lvl > UK_PAGING_PAGE_LEVEL) {
		if (UK_PAGING_PAGE_Lx_HAS(lvl) &&
		    UK_PAGING_PAGE_Lx_ALIGNED(vaddr, lvl) &&
		    UK_PAGING_PAGE_Lx_SIZE(lvl) <= len)
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
		return (attr & UK_PAGING_PAGE_ATTR_PROT_READ);
	case UK_VMA_FAULT_WRITE:
		return (attr & UK_PAGING_PAGE_ATTR_PROT_WRITE);
	case UK_VMA_FAULT_EXEC:
		return (attr & UK_PAGING_PAGE_ATTR_PROT_EXEC);
	}

	return 0;
}


static int vmem_mapx_pagefault(struct uk_pagetable *pt __unused,
			       __vaddr_t vaddr, __vaddr_t pt_vaddr,
			       unsigned int level, __pte_t *pte, void *user)
{
	struct vmem_mapx_ctx *ctx = (struct vmem_mapx_ctx *)user;
	__pte_t opte;
	int rc;

	UK_ASSERT(ctx->vma->ops);
	UK_ASSERT(ctx->vma->ops->fault);

	/* Deduplicate concurrent page faults on SMP: if another CPU already
	 * installed a valid PTE with sufficient permissions, refresh our local
	 * TLB entry and skip invoking vma->ops->fault a second time.
	 */
	rc = uk_paging_pte_read(pt_vaddr, level,
				UK_PAGING_PT_Lx_IDX(vaddr, level), &opte);
	if (unlikely(rc))
		return rc;

	if (UK_PAGING_PT_Lx_PTE_PRESENT(opte, level) &&
	    vmem_access_allowed(uk_paging_attr_from_pte(opte, level),
				ctx->type)) {
		uk_paging_tlb_flush_entry(vaddr);
		return UK_PAGING_PAGE_MAPX_ESKIP;
	}

	/* mapx supplies a proposed PTE; fault presence uses the current PTE. */
	{
		struct uk_vm_fault fault = {
			.vaddr = ctx->vaddr,
			.vbase = vaddr,
			.len   = UK_PAGING_PAGE_Lx_SIZE(level),
			.paddr = UK_PAGING_PT_Lx_PTE_PADDR(*pte, level),
			.type  = (ctx->type & ~UK_VMA_FAULT_NONPRESENT) |
				 (UK_PAGING_PT_Lx_PTE_PRESENT(opte, level) ? 0 :
				  UK_VMA_FAULT_NONPRESENT),
			.pte   = *pte,
			.level = level,
			.regs  = ctx->regs,
#if CONFIG_LIBUKVMEM_COW
			.operation = &ctx->operation,
#endif
		};

		rc = ctx->vma->ops->fault(ctx->vma, &fault);
		if (unlikely(rc)) {
			if (rc == -ENOMEM)
				return UK_PAGING_PAGE_MAPX_ETOOBIG;

			return rc;
		}

		UK_ASSERT(UK_PAGING_PAGE_Lx_ALIGNED(fault.paddr, fault.level));

		*pte = UK_PAGING_PT_Lx_PTE_SET_PADDR(fault.pte, fault.level,
						     fault.paddr);
	}

	return 0;
}

static int vmem_fault(struct uk_vas *vas, __vaddr_t vaddr, unsigned int type,
		      struct uk_lcpu_regs *regs)
{
	const unsigned int demand_lvl =
		UK_PAGING_PAGE_SHIFT_Lx(CONFIG_LIBUKVMEM_DEMAND_PAGE_IN_SIZE);
	struct uk_paging_free_batch batch;
	struct uk_pagetable *pt;
	struct vmem_mapx_ctx ctx = {
		.vaddr = vaddr,
		.type  = type,
		.regs  = regs,
	};
	struct uk_paging_page_mapx mapx = {
		.map = vmem_mapx_pagefault,
		.ctx = &ctx,
	};
	__vaddr_t vbase;
	unsigned int lvl = UK_PAGING_PAGE_LEVEL;
	unsigned long irqf;
	unsigned long flags;
	int rc;

	/* Check if a virtual address space is set */
	if (unlikely(!uk_vas_paging_enabled(vas)))
		return -EFAULT;

#if CONFIG_LIBUKVMEM_THREAD_VAS
	/* Shared kernel branches retain their VMA and frame ownership in the
	 * kernel VAS, including pages first touched after a process clone.
	 */
	if (uk_vas_get_kernel() &&
	    uk_paging_pt_get_owner(vas->pt, vaddr) == uk_vas_get_kernel()->pt)
		vas = uk_vas_get_kernel();
#endif

	ukplat_spin_lock_irqsave(&vas->lock, irqf);
	if (unlikely(!uk_vas_paging_enabled(vas))) {
		ukplat_spin_unlock_irqrestore(&vas->lock, irqf);
		return -EFAULT;
	}

	/* If the page fault was caused by an access to a region not covered by
	 * a virtual memory area, fail early.
	 */
	ctx.vma = vmem_vma_find(vas, vaddr, 0);
	if (unlikely(!ctx.vma)) {
		ukplat_spin_unlock_irqrestore(&vas->lock, irqf);
		return -EFAULT;
	}

	UK_ASSERT(vaddr >= ctx.vma->start && vaddr < ctx.vma->end);

	/* Fail if the access is not compatible with the VMA's settings */
	if (unlikely(!vmem_access_allowed(ctx.vma->attr, type))) {
		ukplat_spin_unlock_irqrestore(&vas->lock, irqf);
		return -EFAULT;
	}

	/* Fail early if the VMA does not have a fault handler */
	if (unlikely(!ctx.vma->ops->fault)) {
		ukplat_spin_unlock_irqrestore(&vas->lock, irqf);
		return -EFAULT;
	}

	UK_ASSERT(ctx.vma->vas);
	UK_ASSERT(ctx.vma->vas->pt);
	pt = ctx.vma->vas->pt;

	/* Find the page level at which we want to page-in. If the VMA does not
	 * enforce a specific page size and the configuration allows to page-in
	 * large pages, we first check up to which level we find page tables.
	 * We cannot create pages larger than that. Afterwards, we adjust
	 * according to alignment and VMA boundaries.
	 */
	if (ctx.vma->page_lvl < 0 && demand_lvl > UK_PAGING_PAGE_LEVEL) {
		rc = uk_paging_pt_walk(pt, vaddr, &lvl, __NULL, __NULL);
		if (unlikely(rc)) {
			ukplat_spin_unlock_irqrestore(&vas->lock, irqf);
			return rc;
		}

		vbase = MAX(UK_PAGING_PAGE_Lx_ALIGN_DOWN(vaddr, lvl),
			    ctx.vma->start);

		lvl = vmem_largest_level(vbase, ctx.vma->end - vbase,
					 MIN(lvl, demand_lvl));

		flags = UK_PAGING_PAGE_FLAG_FORCE_SIZE;
	} else {
		lvl   = MAX(ctx.vma->page_lvl, UK_PAGING_PAGE_LEVEL);
		flags = 0;
	}

	vbase = UK_PAGING_PAGE_Lx_ALIGN_DOWN(vaddr, lvl);

	UK_ASSERT(vbase >= ctx.vma->start &&
		  vbase < ctx.vma->end);
	UK_ASSERT(vbase <= __VADDR_MAX - UK_PAGING_PAGE_Lx_SIZE(lvl));
	UK_ASSERT(vbase + UK_PAGING_PAGE_Lx_SIZE(lvl) >= ctx.vma->start &&
		  vbase + UK_PAGING_PAGE_Lx_SIZE(lvl) <= ctx.vma->end);

	uk_paging_batch_begin(&batch, pt);
	rc = vmem_page_mapx(pt, vbase, 0, 1, ctx.vma->attr,
			    UK_PAGING_PAGE_FLAG_SIZE(lvl) | flags, &mapx);
	uk_paging_batch_end(&batch);
	ukplat_spin_unlock_irqrestore(&vas->lock, irqf);

	if (type & UK_VMA_FAULT_SOFT)
		uk_paging_free_batch_finish(&batch);
	else
		vmem_arch_pagefault_finish(&batch, regs);
	return rc;
}

int vmem_pagefault(__vaddr_t vaddr, unsigned int type,
		   struct uk_lcpu_regs *regs)
{
	return vmem_fault(uk_vas_get_active(), vaddr, type, regs);
}

#if CONFIG_LIBUKVMEM_VAS_ACCESS
int uk_vas_fault(struct uk_vas *vas, __vaddr_t vaddr, unsigned int access)
{
	if (!vas || access > UK_VMA_FAULT_EXEC)
		return -EINVAL;
	if (uk_lcpu_irqs_disabled())
		return -EWOULDBLOCK;
	access |= UK_VMA_FAULT_SOFT | UK_VMA_FAULT_IRQS_ENABLED;
	return vmem_fault(vas, vaddr, access, NULL);
}

int uk_vas_store_u32(struct uk_vas *vas, __vaddr_t vaddr, __u32 value)
{
	struct uk_vas *owner;
	struct uk_vma *vma;
	__vaddr_t mapped;
	__paddr_t paddr, page;
	__pte_t pte;
	unsigned long irqf;
	unsigned int level;
	int rc;

	if (!vas || !IS_ALIGNED(vaddr, sizeof(value)))
		return -EINVAL;
	for (;;) {
		rc = uk_vas_fault(vas, vaddr, UK_VMA_FAULT_WRITE);
		if (rc)
			return rc;
		owner = vas;
		if (uk_vas_get_kernel() &&
		    uk_paging_pt_get_owner(vas->pt, vaddr) ==
		    uk_vas_get_kernel()->pt)
			owner = uk_vas_get_kernel();
		ukplat_spin_lock_irqsave(&owner->lock, irqf);
		vma = vmem_vma_find(owner, vaddr, sizeof(value));
		if (!vma || vaddr < vma->start ||
		    vaddr > vma->end - sizeof(value) ||
		    !vmem_access_allowed(vma->attr, UK_VMA_FAULT_WRITE)) {
			rc = -EFAULT;
			goto unlock;
		}
		level = UK_PAGING_PAGE_LEVEL;
		rc = uk_paging_pt_walk(owner->pt, vaddr, &level, NULL, &pte);
		if (rc)
			goto unlock;
		if (!UK_PAGING_PT_Lx_PTE_PRESENT(pte, level) ||
		    !vmem_access_allowed(uk_paging_attr_from_pte(pte, level),
					 UK_VMA_FAULT_WRITE)) {
			ukplat_spin_unlock_irqrestore(&owner->lock, irqf);
			continue;
		}
		paddr = UK_PAGING_PT_Lx_PTE_PADDR(pte, level) +
			(vaddr & (UK_PAGING_PAGE_Lx_SIZE(level) - 1));
		page = UK_PAGING_PAGE_ALIGN_DOWN(paddr);
		mapped = uk_paging_page_kmap(owner->pt, page, 1, 0);
		if (mapped == UK_PAGING_VADDR_INV) {
			rc = -ENOMEM;
			goto unlock;
		}
		uk_store_n((__u32 *)(mapped + paddr - page), value);
		uk_paging_page_kunmap(owner->pt, mapped, 1, 0);
		rc = 0;
unlock:
		ukplat_spin_unlock_irqrestore(&owner->lock, irqf);
		return rc;
	}
}
#endif
#endif /* CONFIG_LIBUKPAGING */
