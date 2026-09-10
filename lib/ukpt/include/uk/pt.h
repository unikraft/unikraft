/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PT_H__
#define __UK_PT_H__

#include <uk/arch/types.h>
#include <uk/config.h>

#include <uk/plat/pal/addr.h>
#include <uk/plat/pal/page.h>
#include <uk/plat/pal/paging.h>
#include <uk/plat/pal/pt.h>
#include <uk/plat/pal/tlb.h>

#include <uk/pal/addr.h>
#include <uk/pal/page.h>
#include <uk/pal/paging.h>
#include <uk/pal/pt.h>
#include <uk/pal/tlb.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !__ASSEMBLY__

#include <uk/assert.h>

struct uk_pt {
	/** Vaddr of the top level page table */
	__vaddr_t pt_vbase;
	/** Paddr of the top level page table */
	__paddr_t pt_pbase;
	/** Offset at which the page table frames of this hierarchy can be
	 * accessed (vaddr = paddr + pt_frame_off). The offset must be valid
	 * on the *active* hierarchy, even if it's not the hierarchy being
	 * manipulated.
	 */
	__vaddr_t pt_frame_off;
};

/**
 * Empty page table entry template. See uk_pt_map() and uk_pt_table_link()
 */
#define UK_PT_TMPL_NONE			UK_PAL_PT_Lx_PTE_INVALID(0)
#define UK_PT_TMPL_LEVEL_NONE		UK_PAL_PAGE_LEVEL

/**
 * Returns the virtual address at which a page table frame of the given
 * hierarchy can be accessed. See uk_pt::pt_frame_off
 *
 * @param pt the page table hierarchy the frame belongs to
 * @param paddr the physical address of the page table frame
 *
 * @return the virtual address of the page table frame
 */
static inline __vaddr_t uk_pt_frame_vaddr(struct uk_pt *pt, __paddr_t paddr)
{
	UK_ASSERT(pt);

	return (__vaddr_t)paddr + pt->pt_frame_off;
}

/**
 * Initializes a page table object from an existing page table hierarchy. The
 * hierarchy is not modified.
 *
 * @param pt the page table object to initialize
 * @param pt_pbase the physical address of the top-level page table
 * @param frame_off the offset at which the page table frames of the hierarchy
 *    can be accessed. See uk_pt::pt_frame_off
 *
 * @return 0 on success, a non-zero error value otherwise
 */
int uk_pt_init(struct uk_pt *pt, __paddr_t pt_pbase, __vaddr_t frame_off);

/**
 * Initializes a page table object from the page table hierarchy that is
 * currently configured in hardware, and marks it as the active hierarchy.
 *
 * @param pt the page table object to initialize
 * @param frame_off the offset at which the page table frames of the hierarchy
 *    can be accessed. See uk_pt::pt_frame_off
 *
 * @return 0 on success, a non-zero error value otherwise
 */
int uk_pt_init_active(struct uk_pt *pt, __vaddr_t frame_off);

/**
 * Switches the MMU to the provided page table hierarchy. The code
 * executing this function must be mapped in the new hierarchy at
 * the same virtual address.
 *
 * @param pt the page table hierarchy to activate
 *
 * @return 0 on success, a non-zero error value otherwise
 */
int uk_pt_activate(struct uk_pt *pt);

/**
 * Checks if the given page table hierarchy is the currently active
 *
 * @param pt the page table hierarchy to test
 *
 * @return a non-zero value if the hierarchy is active, 0 otherwise
 */
int uk_pt_isactive(struct uk_pt *pt);

/**
 * Walks the page table hierarchy for the given virtual address
 *
 * The walk descends until the requested level is reached, or until a page
 * table entry is encountered that is not present or describes a page. Note
 * that stopping early is not an error: the caller is expected to inspect the
 * level that has been reached.
 *
 * @param pt the page table hierarchy to walk
 * @param vaddr the virtual address to walk to
 * @param[in,out] level the level to descend to on input, the level that has
 *    been reached on output
 * @param[out] tbl_vaddr if not __NULL, receives the virtual address of the
 *    page table at the level that has been reached
 * @param[out] pte if not __NULL, receives the page table entry for vaddr at
 *    the level that has been reached
 *
 * @return 0 on success, a non-zero error value otherwise
 */
int uk_pt_walk(struct uk_pt *pt, __vaddr_t vaddr, unsigned int *level,
	       __vaddr_t *tbl_vaddr, __pte_t *pte);

/**
 * Initializes a page table frame so that it can be linked into a hierarchy.
 * All page table entries are invalidated.
 *
 * @param tbl_vaddr the virtual address of the page table frame
 * @param level the level of the page table [0..UK_PAL_PT_LEVELS - 2]
 *
 * @return 0 on success, a non-zero error value otherwise
 */
int uk_pt_table_create(__vaddr_t tbl_vaddr, unsigned int level);

/**
 * Tests if a page table does not contain any present page table entry. A
 * caller may use this to decide if a page table can be unlinked and its
 * frame reused.
 *
 * @param tbl_vaddr the virtual address of the page table
 * @param level the level of the page table [0..UK_PAL_PT_LEVELS - 2]
 *
 * @return a non-zero value if the page table is empty, 0 otherwise
 */
int uk_pt_table_isempty(__vaddr_t tbl_vaddr, unsigned int level);

/**
 * Links a page table into the hierarchy
 *
 * @param pt the page table hierarchy
 * @param vaddr the virtual address for which the page table is linked
 * @param level the level of the page table that receives the link. The linked
 *    page table is of level - 1
 * @param tbl_paddr the physical address of the page table to link
 * @param tmpl a page table entry to derive attributes from, or
 *    UK_PT_TMPL_NONE
 * @param tmpl_level the level of the template page table entry
 *
 * @return 0 on success, a non-zero error value otherwise. Fails with -EEXIST
 *    if the page table entry is already present, -ENOENT if a page table on
 *    the way to the given level is missing
 */
int uk_pt_table_link(struct uk_pt *pt, __vaddr_t vaddr, unsigned int level,
		     __paddr_t tbl_paddr, __pte_t tmpl,
		     unsigned int tmpl_level);

int uk_pt_table_link_at(struct uk_pt *pt, __vaddr_t tbl_vaddr,
			unsigned int level, __vaddr_t vaddr,
			__paddr_t link_paddr, __pte_t tmpl,
			unsigned int tmpl_level);

/**
 * Removes a page table from the hierarchy. The page table itself is not
 * modified and its frame remains owned by the caller.
 *
 * @param pt the page table hierarchy
 * @param vaddr the virtual address for which the page table is unlinked
 * @param level the level of the page table that holds the link
 * @param[out] tbl_paddr if not __NULL, receives the physical address of the
 *    page table that has been unlinked
 *
 * @return 0 on success, a non-zero error value otherwise. Fails with -ENOENT
 *    if there is no page table linked at the given level
 */
int uk_pt_table_unlink(struct uk_pt *pt, __vaddr_t vaddr, unsigned int level,
		       __paddr_t *tbl_paddr);

int uk_pt_table_unlink_at(struct uk_pt *pt, __vaddr_t tbl_vaddr,
			  unsigned int level, __vaddr_t vaddr,
			  __paddr_t *unlinked_paddr);

/**
 * Replaces a mapping with a page table that establishes the same mapping
 * using the next smaller page size. The supplied page table frame is
 * initialized and linked in place of the mapping.
 *
 * @param pt the page table hierarchy
 * @param vaddr a virtual address within the mapping to split
 * @param level the level of the mapping to split
 * @param tbl_vaddr the virtual address of the page table frame to use
 * @param tbl_paddr the physical address of the page table frame to use
 *
 * @return 0 on success, a non-zero error value otherwise. Fails with -ENOENT
 *    if there is no mapping at the given level, -ENOTSUP if the next smaller
 *    page size cannot be used to map pages
 */
int uk_pt_table_split(struct uk_pt *pt, __vaddr_t vaddr, unsigned int level,
		      __vaddr_t tbl_vaddr, __paddr_t tbl_paddr);

int uk_pt_table_split_at(struct uk_pt *pt, __vaddr_t parent_vaddr,
			 unsigned int level, __vaddr_t vaddr,
			 __vaddr_t tbl_vaddr, __paddr_t tbl_paddr);

/**
 * Creates a single mapping. All page tables on the way to the given level
 * must exist (see uk_pt_table_link()).
 *
 * @param pt the page table hierarchy
 * @param vaddr the virtual address to map at. Must be aligned to the page
 *    size at the given level
 * @param paddr the physical address to map. Must be aligned to the page size
 *    at the given level
 * @param level the level to create the mapping at
 * @param attr the page attributes (UK_PAL_PAGE_ATTR_* flags)
 * @param tmpl a page table entry to derive attributes from, or
 *    UK_PT_TMPL_NONE
 * @param tmpl_level the level of the template page table entry
 *
 * @return 0 on success, a non-zero error value otherwise. Fails with -EEXIST
 *    if there is already a mapping, -ENOENT if a page table on the way to the
 *    given level is missing
 */
int uk_pt_map(struct uk_pt *pt, __vaddr_t vaddr, __paddr_t paddr,
	      unsigned int level, unsigned long attr, __pte_t tmpl,
	      unsigned int tmpl_level);

int uk_pt_map_at(struct uk_pt *pt, __vaddr_t tbl_vaddr, unsigned int level,
		 __vaddr_t vaddr, __paddr_t paddr, unsigned long attr,
		 __pte_t tmpl, unsigned int tmpl_level);

/**
 * Removes a single mapping. The physical memory that the mapping refers to is
 * not affected.
 *
 * @param pt the page table hierarchy
 * @param vaddr the virtual address of the mapping
 * @param level the level of the mapping
 * @param[out] paddr if not __NULL, receives the physical address that the
 *    mapping referred to
 *
 * @return 0 on success, a non-zero error value otherwise. Fails with -ENOENT
 *    if there is no mapping at the given level
 */
int uk_pt_unmap(struct uk_pt *pt, __vaddr_t vaddr, unsigned int level,
		__paddr_t *paddr);

int uk_pt_unmap_at(struct uk_pt *pt, __vaddr_t tbl_vaddr, unsigned int level,
		   __vaddr_t vaddr, __paddr_t *paddr);

/**
 * Changes the attributes of a single mapping
 *
 * @param pt the page table hierarchy
 * @param vaddr the virtual address of the mapping
 * @param level the level of the mapping
 * @param attr the new page attributes (UK_PAL_PAGE_ATTR_* flags)
 *
 * @return 0 on success, a non-zero error value otherwise. Fails with -ENOENT
 *    if there is no mapping at the given level
 */
int uk_pt_set_attr(struct uk_pt *pt, __vaddr_t vaddr, unsigned int level,
		   unsigned long attr);

int uk_pt_set_attr_at(struct uk_pt *pt, __vaddr_t tbl_vaddr,
		      unsigned int level, __vaddr_t vaddr, unsigned long attr);

/**
 * Writes a prepared page table entry, performing the necessary TLB
 * maintenance. This is the lowest-level operation of this library and does
 * not validate the entry.
 *
 * @param pt the page table hierarchy
 * @param tbl_vaddr the virtual address of the page table to write to
 * @param level the level of the page table
 * @param idx the index of the page table entry
 * @param vaddr the virtual address the page table entry describes, or
 *    UK_PAL_VADDR_INV if it is not known. In the latter case no TLB
 *    maintenance is performed and the caller is responsible for flushing
 * @param pte the page table entry to write
 *
 * @return 0 on success, a non-zero error value otherwise
 */
int uk_pt_pte_write_at(struct uk_pt *pt, __vaddr_t tbl_vaddr,
		       unsigned int level, unsigned int idx, __vaddr_t vaddr,
		       __pte_t pte);

/**
 * Invalidates the TLB entry for the given virtual address, if the given page
 * table hierarchy is the active one.
 *
 * @param pt the page table hierarchy
 * @param vaddr the virtual address to invalidate, or UK_PAL_VADDR_INV to do
 *    nothing
 */
void uk_pt_flush_entry(struct uk_pt *pt, __vaddr_t vaddr);

#endif /* !__ASSEMBLY__ */

#ifdef __cplusplus
}
#endif
#endif /* __UK_PT_H__ */
