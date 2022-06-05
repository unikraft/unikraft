/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *          Stefan Teodorescu <stefanl.teodorescu@gmail.com>
 *
 * Copyright (c) 2021, Karlsruhe Institute of Technology (KIT).
 *                     All rights reserved.
 * Copyright (c) 2021, University Politehnica of Bucharest.
 *                     All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __UKPLAT_PAGING_H__
#define __UKPLAT_PAGING_H__

#include <uk/config.h>
#include <uk/arch/types.h>
#include <uk/arch/paging.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONFIG_PAGING
#error Using this header requires enabling the paging API
#endif /* CONFIG_PAGING */

struct uk_falloc;

struct uk_pagetable {
	__vaddr_t pt_vbase;
	__paddr_t pt_pbase;

	struct uk_falloc *fa;

	/* Architecture-dependent part */
	struct ukarch_pagetable arch;

#ifdef CONFIG_PAGING_STATS
	unsigned long nr_lx_pages[PT_LEVELS];
	unsigned long nr_lx_splits[PT_LEVELS];
	unsigned long nr_pt_pages[PT_LEVELS];
#endif /* CONFIG_PAGING_STATS */
};

/* Page attributes. See <uk/asm/paging.h> for more */
#define PAGE_ATTR_PROT_RW	(PAGE_ATTR_PROT_READ | PAGE_ATTR_PROT_WRITE)
#define PAGE_ATTR_PROT_RWX	(PAGE_ATTR_PROT_RW | PAGE_ATTR_PROT_EXEC)

/* Page operation flags */
#define PAGE_FLAG_KEEP_PTES	0x01 /* Preserve PTEs on map/unmap */
#define PAGE_FLAG_KEEP_FRAMES	0x02 /* Preserve frames on unmap */
#define PAGE_FLAG_FORCE_SIZE	0x04 /* Force the page size specified with \
				      * PAGE_FLAG_SIZE() \
				      */

#define PAGE_FLAG_SIZE_SHIFT	4
#define PAGE_FLAG_SIZE_MASK	((1UL << PAGE_FLAG_SIZE_SHIFT) - 1)

#define PAGE_FLAG_SIZE(lvl)					\
	(((lvl) & PAGE_FLAG_SIZE_MASK) << PAGE_FLAG_SIZE_SHIFT)

#define PAGE_FLAG_SIZE_TO_LEVEL(flag)				\
	(((flag) >> PAGE_FLAG_SIZE_SHIFT) & PAGE_FLAG_SIZE_MASK)

/* Page table clone flags */
#define PAGE_FLAG_CLONE_NEW	0x01 /* Create an empty page table */

/**
 * Returns the active page table (the one that defines the virtual address
 * space at the moment of the execution of this function).
 */
struct uk_pagetable *ukplat_pt_get_active(void);

/**
 * Switches the active page table to the specified one.
 *
 * @param pt the page table instance to switch to. The code of the function
 *   must be mapped into the new address space at the same virtual address.
 *
 * @return 0 on success, a non-zero value otherwise
 */
int ukplat_pt_set_active(struct uk_pagetable *pt);

/**
 * Initializes a new page table from the currently configured page table
 * in hardware and assigns the given physical address range to be available
 * for allocations and mappings.
 *
 * @param pt pointer to an uninitialized page table that will be set to the
 *   page table hierarchy currently configured in hardware.
 * @param start start of the physical address range that will be available
 *   for allocations of physical memory (e.g., mapping with __PADDR_ANY).
 *   The function may reserve some memory in this area for own purposes. The
 *   range must not be assigned to other page tables.
 * @param len the length (in bytes) of the physical address range.
 *
 * @return 0 on success, a non-zero value otherwise.
 */
int ukplat_pt_init(struct uk_pagetable *pt, __paddr_t start, __sz len);

/**
 * Adds a physical memory range to the frame allocator of the given page table.
 * The physical memory is available to all page tables sharing the same frame
 * allocator.
 *
 * @param start start of the physical address range that will be available
 *   for allocations of physical memory (e.g., mapping with __PADDR_ANY).
 *   The function may reserve some memory in this area for own purposes. The
 *   range must not be assigned to other page tables.
 * @param len the length (in bytes) of the physical address range.
 *
 * @return 0 on success, a non-zero value otherwise
 */
int ukplat_pt_add_mem(struct uk_pagetable *pt, __paddr_t start, __sz len);

/**
 * Initializes a new page table as clone of another page table. The new page
 * table shares the physical address range available for new allocations and
 * mappings with the source page table.
 *
 * @param pt pointer to an uninitialized page table that will receive a
 *   clone of the source page table.
 * @param pt_src pointer to the source page table that will be cloned.
 * @param flags clone flags (PAGE_FLAG_CLONE_*). If PAGE_FLAG_CLONE_NEW is
 *   specified, a new (empty) top-level page table is created. Note that
 *   this page table will be completely empty and thus do not map any code or
 *   data segments of the kernel.
 *
 * @return 0 on success, a non-zero value otherwise.
 */
int ukplat_pt_clone(struct uk_pagetable *pt, struct uk_pagetable *pt_src,
		    unsigned long flags);

/**
 * Frees the given page table by recursively releasing the page table hierarchy
 * and all mapped physical memory. Mapped physical memory not belonging to the
 * page table is not freed.
 *
 * @param pt the page table hierarchy to release.
 * @param flags page flags (PAGE_FLAG_* flags). If PAGE_FLAG_KEEP_FRAMES is
 *   specified, mapped physical memory is not freed.
 */
int ukplat_pt_free(struct uk_pagetable *pt, unsigned long flags);

/**
 * Performs a page table walk
 *
 * @param pt the page table instance on which to operate.
 * @param vaddr the virtual address to translate.
 * @param [in,out] level the level in which the walk should stop. Use PAGE_LEVEL
 *   to perform a complete walk. The parameter returns the level in the page
 *   table where the translation ended. This value might be higher than the
 *   specified level, depending on the page table. Can be NULL in which case
 *   PAGE_LEVEL is assumed.
 * @param [out] pt_vaddr the virtual address of the page table where the
 *   translation ended. Can be NULL. Can be used with ukarch_pte_write() and
 *   PT_Lx_IDX() to update the PTE.
 * @param [out] pte the PTE where the translation ended. Can be NULL.
 *
 * @return 0 on success, a non-zero value otherwise.
 */
int ukplat_pt_walk(struct uk_pagetable *pt, __vaddr_t vaddr,
		   unsigned int *level, __vaddr_t *pt_vaddr, __pte_t *pte);

/**
 * Creates a mapping from a range of continuous virtual addresses to a range of
 * physical addresses using the specified attributes.
 *
 * @param pt the page table instance on which to operate.
 * @param vaddr the virtual address of the first page in the new mapping.
 * @param paddr the physical address of the memory which the virtual region is
 *   mapped to. This parameter can be __PADDR_ANY to dynamically allocate
 *   physical memory as needed.
 * @param pages the number of pages in requested page size to map.
 * @param attr page attributes to set for the new mapping (PAGE_ATTR_* flags).
 * @param flags page flags (PAGE_FLAG_* flags). The page size can be specified
 *   with PAGE_FLAG_SIZE(). If PAGE_FLAG_FORCE_SIZE is not specified, the
 *   function tries to map the given range (i.e., pages * requested page size)
 *   using the largest possible pages. The actual mapping thus may use larger
 *   or smaller pages than requested depending on address alignment, supported
 *   page sizes, and available continuous physical memory (if paddr is
 *   __PADDR_ANY).
 *
 * @return 0 on success, a non-zero value otherwise. May fail if:
 *   - the physical or virtual address is not aligned to the page size;
 *   - a page in the region is already mapped;
 *   - a page table could not be set up;
 *   - if __PADDR_ANY flag is set and there are no more free frames;
 *   - the platform rejected the operation
 */
int ukplat_page_map(struct uk_pagetable *pt, __vaddr_t vaddr,
		    __paddr_t paddr, unsigned long pages,
		    unsigned long attr, unsigned long flags);

/**
 * Removes the mappings from a range of continuous virtual addresses and frees
 * the underlying frames.
 *
 * @param pt the page table instance on which to operate.
 * @param vaddr the virtual address of the first page that is to be unmapped.
 * @param pages the number of pages in requested page size to unmap.
 * @param flags page flags (PAGE_FLAG_* flags). The page size can be specified
 *   with PAGE_FLAG_SIZE(). If PAGE_FLAG_FORCE_SIZE is not specified,
 *   the range is split into pages of the largest sizes that satisfy the
 *   requested operation. If PAGE_FLAG_KEEP_FRAMES is specified, the physical
 *   memory is not freed and may be mapped again. Note that if the mapping has
 *   been created with __PADDR_ANY the physical memory might not be
 *   contiguous.
 *
 * @return 0 on success, a non-zero value otherwise. May fail if:
 *   - the virtual address is not aligned to the page size;
 *   - the platform rejected the operation
 *
 *   Note that it is not an error to unmap physical memory not previously being
 *   allocated with the associated frame allocator. However, unmapping one of
 *   multiple mappings of the same physical frame will release the frame if it
 *   belongs to this page table's frame allocator. In this case, use
 *   PAGE_FLAG_KEEP_FRAMES for all mapping but the last one.
 */
int ukplat_page_unmap(struct uk_pagetable *pt, __vaddr_t vaddr,
		      unsigned long pages, unsigned long flags);

/**
 * Sets new attributes for a range of continuous virtual addresses.
 *
 * @param pt the page table instance on which to operate.
 * @param vaddr the virtual address of the first page whose attributes should
 *   be changed.
 * @param pages the number of pages in requested page size to change.
 * @param new_attr the new page attributes (PAGE_ATTR_* flags).
 * @param flags page flags (PAGE_FLAG_* flags). The page size can be specified
 *   with PAGE_FLAG_SIZE(). If PAGE_FLAG_FORCE_SIZE is not specified,
 *   the range is split into pages of the largest sizes that satisfy the
 *   requested operation.
 *
 * @return 0 on success, a non-zero value otherwise. May fail if:
 *   - the virtual address is not aligned to the page size;
 *   - there is an invalid mapping in the range;
 *   - the platform rejected the operation
 */
int ukplat_page_set_attr(struct uk_pagetable *pt, __vaddr_t vaddr,
			 unsigned long pages, unsigned long new_attr,
			 unsigned long flags);

#ifdef __cplusplus
}
#endif

#endif /* __UKPLAT_PAGING_H__ */
