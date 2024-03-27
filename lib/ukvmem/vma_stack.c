/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <stddef.h>
#include <string.h>
#include <errno.h>

#include "vmem.h"

#include <uk/config.h>
#include <uk/essentials.h>
#include <uk/assert.h>
#include <uk/arch/limits.h>
#include <uk/arch/paging.h>
#ifdef CONFIG_HAVE_PAGING
#include <uk/plat/paging.h>
#include <uk/falloc.h>
#endif /* CONFIG_HAVE_PAGING */
#include <uk/isr/string.h>

static inline bool in_top_guard_pages_range(struct uk_vma *vma, __vaddr_t vaddr)
{
	UK_ASSERT(vma);
	return IN_RANGE(vaddr,
			vma->end - STACK_TOP_GUARD_SIZE, STACK_TOP_GUARD_SIZE);
}

static inline bool in_bottom_guard_pages_range(struct uk_vma *vma,
					       __vaddr_t vaddr)
{
	UK_ASSERT(vma);
	return IN_RANGE(vaddr,
			vma->start, STACK_BOTTOM_GUARD_SIZE);
}

static inline bool in_guard_pages_range(struct uk_vma *vma, __vaddr_t vaddr)
{
	UK_ASSERT(vma);
	return in_top_guard_pages_range(vma, vaddr) ||
	       in_bottom_guard_pages_range(vma, vaddr);
}

#ifdef CONFIG_LIBUKVMEM_STACK_BASE
static __vaddr_t vma_op_stack_get_base(struct uk_vas *vas __unused,
				       void *data __unused,
				       unsigned long flags __unused)
{
	return CONFIG_LIBUKVMEM_STACK_BASE;
}
#endif /* CONFIG_LIBUKVMEM_STACK_BASE */

static int vma_op_stack_fault(struct uk_vma *vma, struct uk_vm_fault *fault)
{
	struct uk_pagetable * const pt = vma->vas->pt;
	__paddr_t paddr = __PADDR_ANY;
	__vaddr_t vaddr;
	int rc;

	UK_ASSERT(PAGE_ALIGNED(fault->len));
	UK_ASSERT(fault->len == PAGE_SIZE);
	UK_ASSERT(fault->type & UK_VMA_FAULT_NONPRESENT);

	/* Check if the access is in the guard page. For software-generated
	 * faults, we ignore the request to fault in the page. For actual
	 * accesses we fail.
	 */
	if (in_guard_pages_range(vma, fault->vbase)) {
		if (likely(fault->type & UK_VMA_FAULT_SOFT))
			   return UKPLAT_PAGE_MAPX_ESKIP;

		uk_pr_crit("Guard page 0x%lx of stack VMA 0x%lx - 0x%lx hit!\n",
			   fault->vbase, vma->start, vma->end);

		return -EFAULT;
	}

	rc = pt->fa->falloc(pt->fa, &paddr, 1, 0);
	if (unlikely(rc))
		return rc;

	if (!(vma->flags & UK_VMA_FLAG_UNINITIALIZED)) {
		vaddr = ukplat_page_kmap(pt, paddr, 1, 0);
		if (unlikely(vaddr == __VADDR_INV)) {
			pt->fa->ffree(pt->fa, paddr, 1);
			return -ENOMEM;
		}

		memset_isr((void *)vaddr, 0, PAGE_SIZE);
		ukplat_page_kunmap(pt, vaddr, 1, 0);
	}

	fault->paddr = paddr;
	return 0;
}

/* We deny splits and merges so that the guard page is preserved */
const struct uk_vma_ops uk_vma_stack_ops = {
#ifdef CONFIG_LIBUKVMEM_STACK_BASE
	.get_base	= vma_op_stack_get_base,
#else /* CONFIG_LIBUKVMEM_STACK_BASE */
	.get_base	= __NULL,
#endif /* !CONFIG_LIBUKVMEM_STACK_BASE */
	.new		= __NULL,
	.destroy	= __NULL,
	.fault		= vma_op_stack_fault,
	.unmap		= vma_op_unmap,		/* default */
	.split		= vma_op_deny,		/* deny */
	.merge		= vma_op_deny,		/* deny */
	.set_attr	= vma_op_set_attr,	/* default */
	.advise		= vma_op_advise,	/* default */
};
