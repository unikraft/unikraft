/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, Karlsruhe Institute of Technology (KIT).
 *                     All rights reserved.
 * Copyright (c) 2021, University Politehnica of Bucharest.
 *                     All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_NATIVE_ARCH_PT_H__
#define __UK_PLAT_NATIVE_ARCH_PT_H__

#include <uk/arch/types.h>
#include <uk/arch/x86_64.h>

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_LIBUKPLAT_NATIVE_PT

#if CONFIG_LIBUKPLAT_NATIVE_PT_5LEVEL
#define UK_PLAT_NATIVE_PT_LEVELS	5
#else /* !CONFIG_LIBUKPLAT_NATIVE_PT_5LEVEL */
#define UK_PLAT_NATIVE_PT_LEVELS	4
#endif /* !CONFIG_LIBUKPLAT_NATIVE_PT_5LEVEL */

#define UK_PLAT_NATIVE_PT_LEVEL_SHIFT	9
#define UK_PLAT_NATIVE_PTES_PER_LEVEL	(1UL << UK_PLAT_NATIVE_PT_LEVEL_SHIFT)

#define __X86_64_PT_L0_SHIFT		12
#define __X86_64_PT_Lx_SHIFT(lvl)					\
	(__X86_64_PT_L0_SHIFT + (UK_PLAT_NATIVE_PT_LEVEL_SHIFT * (lvl)))
#define __X86_64_PT_SHIFT_Lx(shift)				\
	(((shift) - __X86_64_PT_L0_SHIFT) / UK_PLAT_NATIVE_PT_LEVEL_SHIFT)

#define UK_PLAT_NATIVE_PT_Lx_IDX(vaddr, lvl)			\
	(((vaddr) >> __X86_64_PT_Lx_SHIFT(lvl)) &			\
	 (UK_PLAT_NATIVE_PTES_PER_LEVEL - 1))

#define UK_PLAT_NATIVE_PT_Lx_PTES(lvl)	UK_PLAT_NATIVE_PTES_PER_LEVEL

#define UK_PLAT_NATIVE_PT_Lx_PTE_PADDR(pte, lvl)		\
	(((__paddr_t)(pte) & __X86_64_PTE_PADDR_MASK) & UK_PLAT_NATIVE_PAGE_MASK)

#define UK_PLAT_NATIVE_PT_Lx_PTE_SET_PADDR(pte, lvl, paddr)		    \
	(((pte) & ~(__X86_64_PTE_PADDR_MASK & UK_PLAT_NATIVE_PAGE_MASK)) |   \
	 (__pte_t)((paddr) & __X86_64_PTE_PADDR_MASK))

#define UK_PLAT_NATIVE_PT_Lx_PTE_INVALID(lvl)		0x0UL

#define UK_PLAT_NATIVE_PT_Lx_PTE_PRESENT(pte, lvl)		\
	((pte) & UK_ARCH_X86_64_PTE_PRESENT)
#define UK_PLAT_NATIVE_PT_Lx_PTE_CLEAR_PRESENT(pte, lvl)	\
	((pte) & ~UK_ARCH_X86_64_PTE_PRESENT)

#if !__ASSEMBLY__

#if CONFIG_LIBUKPLAT_NATIVE_PAGING

#include <uk/assert.h>

typedef __u64 __pte_t;		/* page table entry */

#define UK_PLAT_NATIVE_DIRECTMAP_AREA_START 0xffffff8000000000 /* -512 GiB */
#define UK_PLAT_NATIVE_DIRECTMAP_AREA_END   0xffffffffffffffff

/**
 * Reads a page table entry from the given page table.
 *
 * @param pt_vaddr virtual address of the mapped page table. The address can be
 *    used to access the page table
 * @param lvl the level of the page table [0..UK_PLAT_NATIVE_PT_LEVELS - 1]
 * @param idx the index of the PTE to read [0..UK_PLAT_NATIVE_PT_Lx_PTES(lvl) - 1]
 * @param [out] pte pointer to a variable that will receive the page table entry
 *
 * @return 0 if the page table entry could be read, an non-zero error value
 *    otherwise
 */
static inline int
uk_plat_native_pte_read(__vaddr_t pt_vaddr, unsigned int lvl,
			unsigned int idx, __pte_t *pte)
{
	(void)lvl;

	UK_ASSERT(idx < UK_PLAT_NATIVE_PT_Lx_PTES(lvl));

	*pte = *((__pte_t *)pt_vaddr + idx);

	return 0;
}

/**
 * Writes a page table entry to the given page table. Note: TLB entries have to
 * be flushed manually, if needed.
 *
 * @param pt_vaddr virtual address of the mapped page table. The address can be
 *    used to access the page table
 * @param lvl the level of the page table [0..UK_PLAT_NATIVE_PT_LEVELS - 1]
 * @param idx the index of the PTE to write [0..UK_PLAT_NATIVE_PT_Lx_PTES(lvl) - 1]
 * @param pte the value of the page table entry to write
 *
 * @return 0 if the page table entry could be written, an non-zero error value
 *    otherwise
 */
static inline
int uk_plat_native_pte_write(__vaddr_t pt_vaddr, unsigned int lvl,
			     unsigned int idx, __pte_t pte)
{
	(void)lvl;

	UK_ASSERT(idx < UK_PLAT_NATIVE_PT_Lx_PTES(lvl));

	*((__pte_t *)pt_vaddr + idx) = pte;

	return 0;
}

/**
 * Retrieves the currently active page table
 *
 * @return the physical address of the active top-level page table (i.e.,
 *    UK_PLAT_NATIVE_PT_LEVELS - 1) that is configured in hardware for use
 *    in address translation. May return UK_PLAT_NATIVE_PADDR_INV on error
 */
static inline
__paddr_t uk_plat_native_pt_read_base(void)
{
	__pte_t cr3;

	__asm__ __volatile__("movq %%cr3, %0" : "=r"(cr3));

	return UK_PLAT_NATIVE_PT_Lx_PTE_PADDR(cr3, UK_PLAT_NATIVE_PT_LEVELS);
}

/**
 * Sets the currently active page table. This will also flush the TLB.
 *
 * @param pt_paddr the physical address of the top-level page table
 *                 (i.e., UK_PLAT_NATIVE_PT_LEVELS - 1) that should
 *                 be configured in hardware for use in address translation
 *
 * @return 0 if the page table could be activated, a non-zero error value
 *    otherwise
 */
static inline
int uk_plat_native_pt_write_base(__paddr_t pt_paddr)
{
	__asm__ __volatile__("movq %0, %%cr3" :: "r"(pt_paddr));

	return 0;
}

/**
 * Create a table descriptor
 *
 * @param paddr the physical address the page table descriptor translates to
 * @param attr protection and memory type attributes to populate the new
 *             descriptor with
 * @param level the level of the page table [0..UK_PLAT_NATIVE_PT_LEVELS - 1]
 *              to create this descriptor for
 * @param tmpl a template page table descriptor to base the new descriptor
 *                 on
 * @param tmpl_level the level [0..UK_PLAT_NATIVE_PT_LEVELS - 1] of
 *                       the template
 *
 * @return the new page table descriptor
 */
static inline __pte_t
uk_plat_native_pt_pte_create(__paddr_t pt_paddr, unsigned int level __unused,
			     __pte_t tmpl, unsigned int tmpl_level __unused)
{
	__pte_t pt_pte;

	pt_pte = pt_paddr & __X86_64_PTE_PADDR_MASK;

	/* We do not apply any restrictive protections in PT PTEs but control
	 * protections in the PTEs mapping pages only
	 */
	pt_pte |= (UK_ARCH_X86_64_PTE_PRESENT | UK_ARCH_X86_64_PTE_RW);

	/* Do not use the PWT/PCD bits for the PT PTEs. We only use them for
	 * page PTEs
	 */
	pt_pte &= ~(UK_ARCH_X86_64_PTE_PWT | UK_ARCH_X86_64_PTE_PCD);

	/* Take all other bits from template. We also keep the flags that are
	 * ignored by the architecture. The caller might have stored custom
	 * data in these fields
	 */
	pt_pte |= tmpl & (UK_ARCH_X86_64_PTE_US |
			  UK_ARCH_X86_64_PTE_ACCESSED |
			  UK_ARCH_X86_64_PTE_DIRTY | /* ignored */
			  UK_ARCH_X86_64_PTE_GLOBAL | /* ignored */
			  UK_ARCH_X86_64_PTE_USER1_MASK |
			  UK_ARCH_X86_64_PTE_USER2_MASK |
			  UK_ARCH_X86_64_PTE_MPK_MASK /* ignored */
			      );

	return pt_pte;
}

#endif /* CONFIG_LIBUKPLAT_NATIVE_PAGING */

#endif /* !__ASSEMBLY__ */

#endif /* CONFIG_LIBUKPLAT_NATIVE_PT */

#ifdef __cplusplus
}
#endif
#endif /* __UK_PLAT_NATIVE_ARCH_PT_H__ */
