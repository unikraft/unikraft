/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <stddef.h>
#include <errno.h>

#include <uk/config.h>
#include <uk/essentials.h>
#include <uk/assert.h>
#include <uk/alloc.h>
#include <uk/vma_ops.h>
#include <uk/arch/limits.h>
#include <uk/arch/paging.h>
#ifdef CONFIG_HAVE_PAGING
#include <uk/plat/paging.h>
#endif /* CONFIG_HAVE_PAGING */

struct uk_vma_dma {
	struct uk_vma base;

	/** Physical base address of the mapped physical region */
	__paddr_t paddr;
};

#ifdef CONFIG_LIBUKVMEM_DMA_BASE
static __vaddr_t vma_op_dma_get_base(struct uk_vas *vas __unused,
				     void *data __unused,
				     unsigned long flags __unused)
{
	return CONFIG_LIBUKVMEM_DMA_BASE;
}
#endif /* CONFIG_LIBUKVMEM_DMA_BASE */

int vma_op_dma_new(struct uk_vas *vas, __vaddr_t vaddr __unused,
		   __sz len __unused, void *data, unsigned long attr __unused,
		   unsigned long *flags __unused, struct uk_vma **vma)
{
	struct uk_vma_dma_args *args = (struct uk_vma_dma_args *)data;
	struct uk_vma_dma *vma_dma;

	UK_ASSERT(data);
	UK_ASSERT(PAGE_ALIGNED(args->paddr));
	UK_ASSERT(args->paddr <= __PADDR_MAX - len);
	UK_ASSERT(ukarch_paddr_range_isvalid(args->paddr, len));

	vma_dma = uk_malloc(vas->a, sizeof(struct uk_vma_dma));
	if (unlikely(!vma_dma))
		return -ENOMEM;

	UK_ASSERT(args);
	vma_dma->paddr = args->paddr;
	vma_dma->base.name = __NULL;

	UK_ASSERT(vma);
	*vma = (struct uk_vma *)vma_dma;

	return 0;
}
static int vma_op_dma_fault(struct uk_vma *vma, struct uk_vm_fault *fault)
{
	struct uk_vma_dma *vma_dma = (struct uk_vma_dma *)vma;

	UK_ASSERT(PAGE_ALIGNED(fault->len));
	UK_ASSERT(fault->type & UK_VMA_FAULT_NONPRESENT);
	UK_ASSERT(fault->vbase >= vma->start && fault->vbase < vma->end);

	fault->paddr = vma_dma->paddr + (fault->vbase - vma->start);

	return 0;
}

static int vma_op_dma_unmap(struct uk_vma *vma, __vaddr_t vaddr, __sz len)
{
	UK_ASSERT(vaddr >= vma->start);
	UK_ASSERT(vaddr + len <= vma->end);

	return ukplat_page_unmap(vma->vas->pt, vaddr, len >> PAGE_SHIFT,
				 PAGE_FLAG_KEEP_FRAMES);
}

static int vma_op_dma_split(struct uk_vma *vma, __vaddr_t vaddr,
			    struct uk_vma **new_vma)
{
	struct uk_vma_dma *vma_dma = (struct uk_vma_dma *)vma;
	struct uk_vma_dma *v;
	__sz off;

	v = uk_malloc(vma->vas->a, sizeof(struct uk_vma_dma));
	if (unlikely(!v))
		return -ENOMEM;

	UK_ASSERT(vaddr >= vma->start);
	off = vaddr - vma->start;

	UK_ASSERT(vma_dma->paddr <= __PADDR_MAX - off);
	v->paddr = vma_dma->paddr + off;

	UK_ASSERT(new_vma);
	*new_vma = &v->base;

	return 0;
}

static int vma_op_dma_merge(struct uk_vma *vma, struct uk_vma *next)
{
	struct uk_vma_dma *vma_dma = (struct uk_vma_dma *)vma;
	struct uk_vma_dma *next_dma = (struct uk_vma_dma *)next;
	__sz off;

	UK_ASSERT(next->start == vma->end);
	UK_ASSERT(next->start > vma->start);

	off = next->start - vma->start;
	if (next_dma->paddr != vma_dma->paddr + off)
		return -EPERM;

	return 0;
}

const struct uk_vma_ops uk_vma_dma_ops = {
#ifdef CONFIG_LIBUKVMEM_DMA_BASE
	.get_base	= vma_op_dma_get_base,
#else /* CONFIG_LIBUKVMEM_DMA_BASE */
	.get_base	= __NULL,
#endif /* !CONFIG_LIBUKVMEM_DMA_BASE */
	.new		= vma_op_dma_new,
	.destroy	= __NULL,
	.fault		= vma_op_dma_fault,
	.unmap		= vma_op_dma_unmap,
	.split		= vma_op_dma_split,
	.merge		= vma_op_dma_merge,
	.set_attr	= uk_vma_op_set_attr,	/* default */
	.advise		= __NULL,
};
