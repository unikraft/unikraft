/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __VMEM_H__
#define __VMEM_H__

#include <uk/vmem.h>
#include <uk/assert.h>
#include <uk/arch/paging.h>
#ifdef CONFIG_HAVE_PAGING
#include <uk/plat/paging.h>
#endif /* CONFIG_HAVE_PAGING */

#ifdef CONFIG_HAVE_PAGING
/**
 * Architecture-independent page fault handler
 *
 * @param vaddr
 *   The faulting virtual address
 * @param type
 *   The type of the page fault (see UK_VMA_FAULT_*)
 * @param regs
 *   Trap frame from the page fault
 *
 * @return
 *   0 on success, a negative error code otherwise
 */
int vmem_pagefault(__vaddr_t vaddr, unsigned int type, struct __regs *regs);

/**
 * Computes the number of pages depending on the VMA's configured page size.
 *
 * @param vma
 *   The VMA to operate on
 * @param len
 *   The length of the area in bytes which to convert to the number of pages
 *   in the given VMA
 * @param[out] flags
 *   Receives the flags parameter initialized with the page size to be used in
 *   one of the functions of the paging API
 *
 * @return
 *   The number of pages in the VMA's page size
 */
static inline unsigned long
vmem_len_to_pages(struct uk_vma *vma, __sz len, unsigned long *flags)
{
	int to_lvl;

	UK_ASSERT(vma);
	UK_ASSERT(flags);

	to_lvl = MAX(vma->page_lvl, PAGE_LEVEL);
	*flags = PAGE_FLAG_SIZE(to_lvl);

	UK_ASSERT(PAGE_Lx_ALIGNED(len, to_lvl));
	return len / PAGE_Lx_SIZE(to_lvl);
}
#endif /* CONFIG_HAVE_PAGING */

/* Macros for safe VMA op invocation */
#define _VMA_OP(vma, op, def, ...)					\
	(((vma)->ops->op) ? (vma)->ops->op(vma, __VA_ARGS__) : (def))

#define VMA_GETBASE(vma, ...)	_VMA_OP(vma, get_base, __VADDR_INV, __VA_ARGS__)
#define VMA_FAULT(vma, ...)	_VMA_OP(vma, fault, -EFAULT, __VA_ARGS__)
#define VMA_UNMAP(vma, ...)	_VMA_OP(vma, unmap, 0, __VA_ARGS__)
#define VMA_SPLIT(vma, ...)	_VMA_OP(vma, split, 0, __VA_ARGS__)
#define VMA_MERGE(vma, ...)	_VMA_OP(vma, merge, 0, __VA_ARGS__)
#define VMA_SETATTR(vma, ...)	_VMA_OP(vma, set_attr, 0, __VA_ARGS__)
#define VMA_ADVISE(vma, ...)	_VMA_OP(vma, advise, 0, __VA_ARGS__)

/**
 * Returns the length of a VMA in bytes.
 */
static inline __sz vmem_vma_len(struct uk_vma *vma)
{
	UK_ASSERT(vma->end > vma->start);

	return vma->end - vma->start;
}

/* Default VMA op handlers */
int vma_op_deny();

int vma_op_unmap(struct uk_vma *vma, __vaddr_t vaddr, __sz len);
int vma_op_set_attr(struct uk_vma *vma, unsigned long attr);
int vma_op_advise(struct uk_vma *vma, __vaddr_t vaddr, __sz len,
		  unsigned long advice);

#endif /* __VMEM_H__ */
