/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_NATIVE_PT_H__
#define __UK_PLAT_NATIVE_PT_H__

#include <uk/arch/types.h>
#include <uk/plat/native/arch/pt.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !__ASSEMBLY__

#if CONFIG_LIBUKPLAT_NATIVE_PT

#if CONFIG_LIBUKPLAT_NATIVE_PAGING

/**
 * UK_PLAT_NATIVE_PT_Lx_IDX(vaddr, lvl)
 *
 * @param vaddr a virtual address
 * @param lvl a page table level [0..UK_PLAT_NATIVE_PT_LEVELS - 1]
 *
 * @return the index of the page table entry (starting from 0) in the page
 *    table at the given level that is used to translate the virtual address
 */
#ifndef UK_PLAT_NATIVE_PT_Lx_IDX
unsigned int UK_PLAT_NATIVE_PT_Lx_IDX(__vaddr_t vaddr, unsigned int lvl);
#endif

/**
 * UK_PLAT_NATIVE_PT_Lx_PTE_PRESENT(pte, lvl)
 *
 * @param pte a page table entry from a page table at the given level
 * @param lvl a page table level [0..UK_PLAT_NATIVE_PT_LEVELS - 1]
 *
 * @return a non-zero value if the page table entry is present, that is the HW
 *    would use it for address translation. The function must return 0 for
 *    UK_PLAT_NATIVE_PT_Lx_PTE_INVALID(lvl).
 */
#ifndef UK_PLAT_NATIVE_PT_Lx_PTE_PRESENT
int UK_PLAT_NATIVE_PT_Lx_PTE_PRESENT(__pte_t pte, unsigned int lvl);
#endif

/**
 * UK_PLAT_NATIVE_PT_Lx_PTE_CLEAR_PRESENT(pte, lvl)
 *
 * @param pte a page table entry from a page table at the given level
 * @param lvl a page table level [0..UK_PLAT_NATIVE_PT_LEVELS - 1]
 *
 * @return a modified version of the input PTE so that the HW will not use the
 *    PTE for address translation. Any bits ignored by the HW in this invalid
 *    state should remain untouched.
 */
#ifndef UK_PLAT_NATIVE_PT_Lx_PTE_CLEAR_PRESENT
__pte_t UK_PLAT_NATIVE_PT_Lx_PTE_CLEAR_PRESENT(__pte_t pte, unsigned int lvl);
#endif

/**
 * UK_PLAT_NATIVE_PT_Lx_PTE_INVALID(lvl)
 *
 * @param lvl a page table level [0..UK_PLAT_NATIVE_PT_LEVELS - 1]
 *
 * @return a PTE value that expresses an invalid state in a page table at the
 *    given level where the HW will not use the page table entry to translate
 *    a virtual address.
 */
#ifndef UK_PLAT_NATIVE_PT_Lx_PTE_INVALID
__pte_t UK_PLAT_NATIVE_PT_Lx_PTE_INVALID(unsigned int level);
#endif

/**
 * UK_PLAT_NATIVE_PT_Lx_PTE_PADDR(pte, lvl)
 *
 * @param pte a page table entry from a page table at the given level
 * @param lvl a page table level [0..UK_PLAT_NATIVE_PT_LEVELS - 1]
 *
 * @return the physical address to which the given page table entry points
 */
#ifndef UK_PLAT_NATIVE_PT_Lx_PTE_PADDR
__paddr_t UK_PLAT_NATIVE_PT_Lx_PTE_PADDR(__pte_t pte, unsigned int lvl);
#endif

/**
 * UK_PLAT_NATIVE_PT_Lx_PTE_SET_PADDR(pte, lvl, paddr)
 *
 * @param pte a page table entry from a page table at the given level
 * @param lvl a page table level [0..UK_PLAT_NATIVE_PT_LEVELS - 1]
 * @param paddr the physical address which to set in the PTE
 *
 * @return the PTE with the updated physical address
 */
#ifndef UK_PLAT_NATIVE_PT_Lx_PTE_SET_PADDR
__pte_t UK_PLAT_NATIVE_PT_Lx_PTE_SET_PADDR(__pte_t pte, unsigned int lvl,
					   __paddr_t paddr);
#endif

/**
 * Creates a page table entry.
 *
 * @param paddr the physical address the page table entry translates to
 * @param attr protection and memory type attributes to populate the new
 *             entry with
 * @param level the level of the page table [0..UK_PLAT_NATIVE_PT_LEVELS - 1]
 *              to create this entry for
 * @param tmpl a template page table entry to base the new entry on
 * @param tmpl_level the level [0..UK_PLAT_NATIVE_PT_LEVELS - 1] of
 *                       the template
 *
 * @return the new page table entry
 */
__pte_t uk_plat_native_pte_create(__paddr_t paddr, unsigned long attr,
				  unsigned int level, __pte_t tmpl,
				  unsigned int tmpl_level);

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
int uk_plat_native_pte_read(__vaddr_t pt_vaddr, unsigned int lvl,
			    unsigned int idx, __pte_t *pte);

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
int uk_plat_native_pte_write(__vaddr_t pt_vaddr, unsigned int lvl,
			     unsigned int idx, __pte_t pte);

/**
 * Changes the attributes of a page table entry
 *
 * @param pte original page table entry
 * @param new_attr protection and memory type attributes to populate the new
 *                 entry with
 * @param level the level at the page table the entry resides in
 *              [0..UK_PLAT_NATIVE_PT_LEVELS - 1]
 *
 * @return page table entry with updated attributes
 */
__pte_t uk_plat_native_pte_change_attr(__pte_t pte, unsigned long new_attr,
				       unsigned int level);

/**
 * Retrieves the attributes of a page table entry
 *
 * @param pte the page table entry to retrieve the attributes of
 * @param level the level at the page table the entry resides in
 *              [0..UK_PLAT_NATIVE_PT_LEVELS - 1]
 *
 * @return page table entry attributes
 */
unsigned long uk_plat_native_attr_from_pte(__pte_t pte, unsigned int level);

/**
 * Retrieves the currently active page table
 *
 * @return the physical address of the active top-level page table (i.e.,
 *    UK_PLAT_NATIVE_PT_LEVELS - 1) that is configured in hardware for use
 *    in address translation. May return UK_PLAT_NATIVE_PADDR_INV on error
 */
__paddr_t uk_plat_native_pt_read_base(void);

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
int uk_plat_native_pt_write_base(__paddr_t pt_paddr);

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
__pte_t uk_plat_native_pt_pte_create(__paddr_t pt_paddr, unsigned int level,
				     __pte_t tmpl, unsigned int tmpl_level);

#endif /* CONFIG_LIBUKPLAT_NATIVE_PAGING */

#endif /* !__ASSEMBLY__ */

#endif /* CONFIG_LIBUKPLAT_NATIVE_PT */

#ifdef __cplusplus
}
#endif
#endif /* __UK_PLAT_NATIVE_PT_H__ */
