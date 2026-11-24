/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __VMEM_H__
#define __VMEM_H__

#include <stdbool.h>
#include <uk/vmem.h>
#include <uk/assert.h>
#ifdef CONFIG_LIBUKPAGING
#include <uk/paging.h>
#endif /* CONFIG_LIBUKPAGING */

#ifdef CONFIG_LIBUKPAGING
static inline bool vmem_vas_range_allowed(struct uk_vas *vas,
					  __vaddr_t start, __sz len)
{
#if CONFIG_LIBUKVMEM_VAS_BOUNDS
	if (vas->range_end)
		return start >= vas->range_start && start < vas->range_end &&
			len <= vas->range_end - start;
#else
	(void)vas;
	(void)start;
	(void)len;
#endif
	return true;
}

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
int vmem_pagefault(__vaddr_t vaddr, unsigned int type,
		   struct uk_lcpu_regs *regs);

/** Complete deferred work after dropping the VAS and paging locks. */
void vmem_arch_pagefault_finish(struct uk_paging_free_batch *batch,
				struct uk_lcpu_regs *regs);

#if CONFIG_LIBUKVMEM_COW
/**
 * Serialize a mapping transition across its unlocked TLB completion.
 * The caller keeps the VAS and this initially zeroed object alive, and must
 * neither block nor yield, nor access private VAS mappings, until end().
 * Only resident allocations and shared kernel mappings may be used there.
 */
struct vmem_vas_transition {
	struct uk_vas *vas;
};

/** Begin in IRQ-enabled thread context, outside VAS and paging locks. */
int vmem_vas_transition_begin(struct uk_vas *vas,
			      struct vmem_vas_transition *transition);

/** Begin from a VMA fault callback holding the VAS lock, with IRQs off. */
int vmem_vas_transition_mark_locked(struct uk_vas *vas,
				    struct vmem_vas_transition *transition);

/**
 * End with IRQs enabled and all VAS/paging locks released, after every TLB
 * batch belonging to the transition has completed. A copied frame must stay
 * read-only until old translations have been acknowledged; promoting it to
 * writable and completing that batch precedes end().
 */
void vmem_vas_transition_end(struct vmem_vas_transition *transition);

/** Wait outside all VAS/paging locks, with IRQs enabled. Never schedules. */
void vmem_vas_transition_wait(struct uk_vas *vas);

/** Wait from a hardware fault without relaxing the interrupted IRQ mask. */
int vmem_arch_pagefault_wait(struct uk_vas *vas, struct uk_lcpu_regs *regs);
#endif

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

	to_lvl = MAX(vma->page_lvl, UK_PAGING_PAGE_LEVEL);
	*flags = UK_PAGING_PAGE_FLAG_SIZE(to_lvl);

	UK_ASSERT(UK_PAGING_PAGE_Lx_ALIGNED(len, to_lvl));
	return len / UK_PAGING_PAGE_Lx_SIZE(to_lvl);
}
#endif /* CONFIG_LIBUKPAGING */

/* Macros for safe VMA op invocation */
#define _VMA_OP(vma, op, def, ...)					\
	(((vma)->ops->op) ? (vma)->ops->op(vma, __VA_ARGS__) : (def))

#define VMA_GETBASE(vma, ...)	_VMA_OP(vma, get_base, UK_PAGING_VADDR_INV, \
					__VA_ARGS__)
#define VMA_FAULT(vma, ...)	_VMA_OP(vma, fault, -EFAULT, __VA_ARGS__)
#define VMA_UNMAP(vma, ...)	_VMA_OP(vma, unmap, 0, __VA_ARGS__)
#define VMA_SPLIT(vma, ...)	_VMA_OP(vma, split, 0, __VA_ARGS__)
#define VMA_MERGE(vma, ...)	_VMA_OP(vma, merge, 0, __VA_ARGS__)
#define VMA_SETATTR(vma, ...)	_VMA_OP(vma, set_attr, 0, __VA_ARGS__)
#define VMA_ADVISE(vma, ...)	_VMA_OP(vma, advise, 0, __VA_ARGS__)

#endif /* __VMEM_H__ */
