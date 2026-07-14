/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, Karlsruhe Institute of Technology (KIT).
 *                     All rights reserved.
 * Copyright (c) 2022, OpenSynergy GmbH. All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_NATIVE_ARCH_PT_H__
#define __UK_PLAT_NATIVE_ARCH_PT_H__

#include <uk/arch/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_LIBUKPLAT_NATIVE_PT

#define UK_PLAT_NATIVE_PT_LEVELS		4
#define UK_PLAT_NATIVE_PTES_PER_LEVEL		512
#define UK_PLAT_NATIVE_PT_LEVEL_SHIFT		9

#define UK_PLAT_NATIVE_PT_Lx_PTES(lvl)	UK_PLAT_NATIVE_PTES_PER_LEVEL

#define UK_PLAT_NATIVE_PT_Lx_PTE_PRESENT(pte, lvl)	    \
	(((pte) & UK_ARCH_ARM64_PTE_VALID_BIT))

#define UK_PLAT_NATIVE_PT_Lx_PTE_CLEAR_PRESENT(pte, lvl)    \
	((pte) & ~UK_ARCH_ARM64_PTE_VALID_BIT)

#define __ARM64_PT_MAP_LEVEL_MAX			    \
	(UK_PLAT_NATIVE_PT_LEVELS - 2)

#define UK_PLAT_NATIVE_PT_Lx_IDX(vaddr, lvl)		    \
	(((vaddr) >> UK_PLAT_NATIVE_PAGE_Lx_SHIFT(lvl)) &   \
	 (UK_PLAT_NATIVE_PTES_PER_LEVEL - 1))

/* Any PTE with bit[0] == 0 is invalid */
#define UK_PLAT_NATIVE_PT_Lx_PTE_INVALID(lvl)	0x0UL

#if !__ASSEMBLY__

#if CONFIG_LIBUKPLAT_NATIVE_PAGING

#include <uk/assert.h>

/* The implementation for arm64 supports a 48-bit virtual address space
 * with 4KiB translation granule, as defined by VMSAv8.
 *
 * Notice:
 * The Unikraft paging API uses the x86_64 convention, where pages are
 * defined at L0, and the top level table is at L3. This is the opposite
 * of the convention use by VMSAv8-A, where pages are defined at L3 and
 * the top level table is defined at L0 (48-bit) / L-1 (52-bit). This
 * implementation uses the Unikraft convention.
 */

typedef __u64 __pte_t;		/* page table entry */

#define UK_PLAT_NATIVE_DIRECTMAP_AREA_START 0x0000ff8000000000
#define UK_PLAT_NATIVE_DIRECTMAP_AREA_END   0x0000ffffffffffff

static inline
__paddr_t UK_PLAT_NATIVE_PT_Lx_PTE_PADDR(__pte_t pte, unsigned int lvl)
{
	__paddr_t paddr;
	static __u64 pte_lx_map_paddr_mask[] = {
		UK_ARCH_ARM64_PTE_L0_PAGE_PADDR_MASK,
		UK_ARCH_ARM64_PTE_L1_BLOCK_PADDR_MASK,
		UK_ARCH_ARM64_PTE_L2_BLOCK_PADDR_MASK
	};

	if (UK_PLAT_NATIVE_PAGE_Lx_IS(pte, lvl)) {
		UK_ASSERT(lvl <= __ARM64_PT_MAP_LEVEL_MAX);
		paddr = pte & pte_lx_map_paddr_mask[lvl];
	} else {
		UK_ASSERT(lvl > UK_PLAT_NATIVE_PAGE_LEVEL &&
			  lvl < UK_PLAT_NATIVE_PT_LEVELS);
		paddr = pte & UK_ARCH_ARM64_PTE_Lx_TABLE_PADDR_MASK;
	}
	return paddr;
}

static inline
__paddr_t UK_PLAT_NATIVE_PT_Lx_PTE_SET_PADDR(__pte_t pte, unsigned int lvl,
					     __paddr_t paddr)
{
	static __u64 pte_lx_map_paddr_mask[] = {
		UK_ARCH_ARM64_PTE_L0_PAGE_PADDR_MASK,
		UK_ARCH_ARM64_PTE_L1_BLOCK_PADDR_MASK,
		UK_ARCH_ARM64_PTE_L2_BLOCK_PADDR_MASK
	};

	if (UK_PLAT_NATIVE_PAGE_Lx_IS(pte, lvl)) {
		UK_ASSERT(lvl <= __ARM64_PT_MAP_LEVEL_MAX);
		paddr &= pte_lx_map_paddr_mask[lvl];
		pte &= ~pte_lx_map_paddr_mask[lvl];
	} else {
		UK_ASSERT(lvl > UK_PLAT_NATIVE_PAGE_LEVEL &&
			  lvl < UK_PLAT_NATIVE_PT_LEVELS);
		paddr &= UK_ARCH_ARM64_PTE_Lx_TABLE_PADDR_MASK;
		pte &= ~UK_ARCH_ARM64_PTE_Lx_TABLE_PADDR_MASK;
	}
	return pte | paddr;
}

static inline
int uk_plat_native_pte_read(__vaddr_t pt_vaddr, unsigned int lvl,
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
	uk_arch_arm64_dsb(ishst);
	uk_arch_arm64_isb();

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
	__paddr_t reg;

	__asm__ __volatile__("mrs %x0, ttbr0_el1\n" : "=r" (reg));

	return (reg & UK_ARCH_ARM64_TTBR0_EL1_BADDR_MASK);
}

/**
 * Sets the currently active page table. This will also flush the TLB.
 *
 * @param pt_paddr the physical address of the top-level page table
 *    (i.e., UK_PLAT_NATIVE_PT_LEVELS - 1) that should be configured in hardware
 *    for use in address translation
 *
 * @return 0 if the page table could be activated, a non-zero error value
 *    otherwise
 */
static inline
int uk_plat_native_pt_write_base(__paddr_t pt_paddr)
{
	__paddr_t reg = (pt_paddr & UK_ARCH_ARM64_TTBR0_EL1_BADDR_MASK);

	__asm__ __volatile__("msr ttbr0_el1, %x0\n"
			     "isb\n"
			     :: "r" (reg));
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
 * @param tmpl_level the level [0..UK_PLAT_NATIVE_PT_LEVELS - 1] of the
 *                       template
 *
 * @return the new page table descriptor
 */
static inline __pte_t
uk_plat_native_pt_pte_create(__paddr_t pt_paddr, unsigned int level __unused,
			     __pte_t tmpl __unused, unsigned int tmpl_level __unused)
{
	__pte_t pt_pte;

	pt_pte = pt_paddr & UK_ARCH_ARM64_PTE_Lx_TABLE_PADDR_MASK;

	pt_pte |= UK_ARCH_ARM64_PTE_TYPE_TABLE;

	return pt_pte;
}

#endif /* CONFIG_LIBUKPLAT_NATIVE_PAGING */

#endif /* !__ASSEMBLY__ */

#endif /* CONFIG_LIBUKPLAT_NATIVE_PT */

#ifdef __cplusplus
}
#endif
#endif /* __UK_PLAT_NATIVE_ARCH_PT_H__ */
