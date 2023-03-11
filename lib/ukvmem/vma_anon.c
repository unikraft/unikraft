/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <stddef.h>
#include <errno.h>

#include "vmem.h"

#include <uk/config.h>
#include <uk/assert.h>
#include <uk/arch/limits.h>
#include <uk/arch/paging.h>
#ifdef CONFIG_HAVE_PAGING
#include <uk/plat/paging.h>
#include <uk/falloc.h>
#endif /* CONFIG_HAVE_PAGING */
#include <uk/isr/string.h>

#ifdef CONFIG_LIBUKVMEM_ANON_BASE
static __vaddr_t vma_op_anon_get_base(struct uk_vas *vas __unused,
				      void *data __unused,
				      unsigned long flags __unused)
{
	return CONFIG_LIBUKVMEM_ANON_BASE;
}
#endif /* CONFIG_LIBUKVMEM_ANON_BASE */

static int vma_op_anon_fault(struct uk_vma *vma, struct uk_vm_fault *fault)
{
	struct uk_pagetable * const pt = vma->vas->pt;
	unsigned long pages = fault->len / PAGE_SIZE;
	__paddr_t paddr = __PADDR_ANY;
	__vaddr_t vaddr;
	int rc;

	UK_ASSERT(PAGE_ALIGNED(fault->len));
	UK_ASSERT(fault->len == PAGE_Lx_SIZE(fault->level));
	UK_ASSERT(fault->type & UK_VMA_FAULT_NONPRESENT);

	rc = pt->fa->falloc(pt->fa, &paddr, pages, FALLOC_FLAG_ALIGNED);
	if (unlikely(rc))
		return rc;

	if (!(vma->flags & UK_VMA_FLAG_UNINITIALIZED)) {
		vaddr = ukplat_page_kmap(pt, paddr, pages, 0);
		if (unlikely(vaddr == __VADDR_INV)) {
			pt->fa->ffree(pt->fa, paddr, pages);
			return -ENOMEM;
		}

		memset_isr((void *)vaddr, 0, fault->len);
		ukplat_page_kunmap(pt, vaddr, pages, 0);
	}

	fault->paddr = paddr;
	return 0;
}

const struct uk_vma_ops uk_vma_anon_ops = {
#ifdef CONFIG_LIBUKVMEM_ANON_BASE
	.get_base	= vma_op_anon_get_base,
#else /* CONFIG_LIBUKVMEM_ANON_BASE */
	.get_base	= __NULL,
#endif /* !CONFIG_LIBUKVMEM_ANON_BASE */
	.new		= __NULL,
	.destroy	= __NULL,
	.fault		= vma_op_anon_fault,
	.unmap		= vma_op_unmap,		/* default */
	.split		= __NULL,
	.merge		= __NULL,
	.set_attr	= vma_op_set_attr,	/* default */
	.advise		= vma_op_advise,	/* default */
};
