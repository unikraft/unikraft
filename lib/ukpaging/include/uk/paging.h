/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, Karlsruhe Institute of Technology (KIT).
 *                     All rights reserved.
 * Copyright (c) 2021, University Politehnica of Bucharest.
 *                     All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PAGING_H__
#define __UK_PAGING_H__

#include <uk/arch/types.h>
#include <uk/config.h>

/* Include platform implementation before the API definition
 * to enforce API header checks.
 */
#include <uk/plat/pal/addr.h>
#include <uk/plat/pal/page.h>
#include <uk/plat/pal/paging.h>
#include <uk/plat/pal/pt.h>
#include <uk/plat/pal/tlb.h>

#if CONFIG_LIBUKPAGING
#include <uk/pal/addr.h>
#include <uk/pal/page.h>
#include <uk/pal/paging.h>
#include <uk/pal/pt.h>
#include <uk/pal/tlb.h>
#endif /* CONFIG_LIBUKPAGING */

#ifdef __cplusplus
extern "C" {
#endif

/* Page attributes */
#define UK_PAGING_PAGE_ATTR_PROT_NONE	UK_PAL_PAGE_ATTR_PROT_NONE
#define UK_PAGING_PAGE_ATTR_PROT_READ	UK_PAL_PAGE_ATTR_PROT_READ
#define UK_PAGING_PAGE_ATTR_PROT_WRITE	UK_PAL_PAGE_ATTR_PROT_WRITE
#define UK_PAGING_PAGE_ATTR_PROT_EXEC	UK_PAL_PAGE_ATTR_PROT_EXEC

#define UK_PAGING_PAGE_ATTR_PROT_RW			    \
	(UK_PAGING_PAGE_ATTR_PROT_READ | UK_PAGING_PAGE_ATTR_PROT_WRITE)
#define UK_PAGING_PAGE_ATTR_PROT_RWX			    \
	(UK_PAGING_PAGE_ATTR_PROT_RW | UK_PAGING_PAGE_ATTR_PROT_EXEC)

/**
 * UK_PAGING_PT_LEVELS definition
 * Number of page table levels (e.g., 4 for 4-level page tables). Must be > 0.
 */
#define UK_PAGING_PT_LEVELS		UK_PAL_PT_LEVELS

/**
 * UK_PAGING_PAGE_Lx_SHIFT(lvl)
 *
 * NOTE: Must be compile-time resolvable
 *
 * @param lvl a page table level [0..UK_PAGING_PT_LEVELS - 1]
 *
 * @return the order of the page size at the given level. Must return the
 *    appropriate order even if the architecture does not support configuring
 *    a page at this level
 */
#define UK_PAGING_PAGE_Lx_SHIFT		UK_PAL_PAGE_Lx_SHIFT

/**
 * UK_PAGING_PAGE_SHIFT_Lx(shift)
 *
 * NOTE: Must be compile-time resolvable
 *
 * @param shift the order of the page size
 *
 * @return the page table level [0..UK_PAGING_PT_LEVELS - 1] corresponding
 *	to the given page size order
 */
#define UK_PAGING_PAGE_SHIFT_Lx		UK_PAL_PAGE_SHIFT_Lx

/**
 * UK_PAGING_PAGE_Lx_SIZE(lvl)
 *
 * @param lvl a page table level [0..UK_PAGING_PT_LEVELS - 1]
 *
 * @return the size of a page at the given level. Must return the
 *    appropriate size even if the architecture does not support configuring
 *    a page at this level
 */
#define UK_PAGING_PAGE_Lx_SIZE		UK_PAL_PAGE_Lx_SIZE

/**
 * UK_PAGING_PAGE_Lx_MASK(lvl)
 *
 * @param lvl a page table level [0..UK_PAGING_PT_LEVELS - 1]
 *
 * @return a mask that selects the page number at the given level and discards
 *    any bits within the page size. Must return the appropriate mask even if
 *    the architecture does not support configuring a page at this level
 */
#define UK_PAGING_PAGE_Lx_MASK(lvl)	UK_PAL_PAGE_Lx_MASK(lvl)

/**
 * UK_PAGING_PAGE_Lx_ALIGN_UP(val, lvl)
 *
 * @param val value to align up, usually address or size
 * @param lvl a page table level [0..UK_PAGING_PT_LEVELS - 1]
 *
 * @return the value aligned up to the next page at the given level
 */
#define UK_PAGING_PAGE_Lx_ALIGN_UP(val, lvl)		    \
	UK_PAL_PAGE_Lx_ALIGN_UP(val, lvl)

/**
 * UK_PAGING_PAGE_Lx_ALIGN_DOWN(val, lvl)
 *
 * @param val a value to align down, usually address or size
 * @param lvl a page table level [0..UK_PAGING_PT_LEVELS - 1]
 *
 * @return the value aligned down to the current page at the given level
 */
#define UK_PAGING_PAGE_Lx_ALIGN_DOWN(val, lvl)		    \
	UK_PAL_PAGE_Lx_ALIGN_DOWN(val, lvl)

/**
 * UK_PAGING_PAGE_Lx_ALIGNED(addr, lvl)
 *
 * @param val a value to check for alignment, usually address or size
 * @param lvl a page table level [0..UK_PAGING_PT_LEVELS - 1]
 *
 * @return a non-zero value if the passed value is aligned to the page size at
 *    the given level, 0 otherwise
 */
#define UK_PAGING_PAGE_Lx_ALIGNED(val, lvl)		    \
	UK_PAL_PAGE_Lx_ALIGNED(val, lvl)

/** UK_PAGING_PAGE_COUNT(len) macro
 *
 * Computes the total number of pages required to map an area of a given
 * length.
 *
 * @param len length of the area to map
 *
 * @return number of pages required to map an area of a given length
 */
#define UK_PAGING_PAGE_COUNT(len)				    \
	DIV_ROUND_UP((len), UK_PAL_PAGE_SIZE)

/* Some helper macros for the smallest page size */
#define UK_PAGING_PAGE_LEVEL			UK_PAL_PAGE_LEVEL
#define UK_PAGING_PAGE_SHIFT			UK_PAL_PAGE_SHIFT
#define UK_PAGING_PAGE_SIZE			UK_PAL_PAGE_SIZE
#define UK_PAGING_PAGE_MASK			UK_PAL_PAGE_MASK

#define UK_PAGING_PAGE_ALIGN_UP(val)		\
	UK_PAL_PAGE_ALIGN_UP(val)
#define UK_PAGING_PAGE_ALIGN_DOWN(val)		\
	UK_PAL_PAGE_ALIGN_DOWN(val)
#define UK_PAGING_PAGE_ALIGNED(val)		\
	UK_PAL_PAGE_ALIGNED(val)

#define UK_PAGING_PAGE_LARGE_LEVEL		UK_PAL_PAGE_LARGE_LEVEL
#define UK_PAGING_PAGE_LARGE_SHIFT		UK_PAL_PAGE_LARGE_SHIFT
#define UK_PAGING_PAGE_LARGE_SIZE		UK_PAL_PAGE_LARGE_SIZE
#define UK_PAGING_PAGE_LARGE_MASK		UK_PAL_PAGE_LARGE_MASK

#define UK_PAGING_PAGE_LARGE_ALIGN_UP(val)	\
	UK_PAL_PAGE_LARGE_ALIGN_UP(val)
#define UK_PAGING_PAGE_LARGE_ALIGN_DOWN(val)	\
	UK_PAL_PAGE_LARGE_ALIGN_DOWN(val)
#define UK_PAGING_PAGE_LARGE_ALIGNED(val)	\
	UK_PAL_PAGE_LARGE_ALIGNED(val)

#define UK_PAGING_PAGE_HUGE_LEVEL		UK_PAL_PAGE_HUGE_LEVEL
#define UK_PAGING_PAGE_HUGE_SHIFT		UK_PAL_PAGE_HUGE_SHIFT
#define UK_PAGING_PAGE_HUGE_SIZE		UK_PAL_PAGE_HUGE_SIZE
#define UK_PAGING_PAGE_HUGE_MASK		UK_PAL_PAGE_HUGE_MASK

#define UK_PAGING_PAGE_HUGE_ALIGN_UP(val)	\
	UK_PAL_PAGE_HUGE_ALIGN_UP(val)
#define UK_PAGING_PAGE_HUGE_ALIGN_DOWN(val)	\
	UK_PAL_PAGE_HUGE_ALIGN_DOWN(val)
#define UK_PAGING_PAGE_HUGE_ALIGNED(val)	\
	UK_PAL_PAGE_HUGE_ALIGNED(val)

#if !__ASSEMBLY__ /* TODO Move these inside CONFIG_LIBUKPAGING? */

/**
 * UK_PAGING_VADDR_INV / UK_PAGING_PADDR_INV definitions
 *
 * A value that can be used to express an invalid virtual / physical address.
 *
 * Note: The value must be aligned to the largest supported page size!
 */

#define UK_PAGING_VADDR_INV			UK_PAL_VADDR_INV
#define UK_PAGING_PADDR_INV			UK_PAL_PADDR_INV

/* Just an alias to express an arbitrary virtual / physical address */
#define UK_PAGING_VADDR_ANY			UK_PAGING_VADDR_INV
#define UK_PAGING_PADDR_ANY			UK_PAGING_PADDR_INV

/**
 * UK_PAGING_PT_Lx_IDX(vaddr, lvl)
 *
 * @param vaddr a virtual address
 * @param lvl a page table level [0..UK_PAGING_PT_LEVELS - 1]
 *
 * @return the index of the page table entry (starting from 0) in the page
 *    table at the given level that is used to translate the virtual address
 */
#define UK_PAGING_PT_Lx_IDX(vaddr, lvl)		    \
	UK_PAL_PT_Lx_IDX(vaddr, lvl)

/**
 * UK_PAGING_PT_Lx_PTES(lvl) macro
 *
 * NOTE: Must be compile-time resolvable
 *
 * @param lvl a page table level [0..UK_PAGING_PT_LEVELS - 1]
 *
 * @return the number of page table entries per page table at the given level
 */
#define UK_PAGING_PT_Lx_PTES(lvl)			    \
	UK_PAL_PT_Lx_PTES(lvl)

/**
 * UK_PAGING_PT_Lx_PTE_PRESENT(pte, lvl)
 *
 * @param pte a page table entry from a page table at the given level
 * @param lvl a page table level [0..UK_PAGING_PT_LEVELS - 1]
 *
 * @return a non-zero value if the page table entry is present, that is the HW
 *    would use it for address translation. The function must return 0 for
 *    UK_PAGING_PT_Lx_PTE_INVALID(lvl).
 */
#define UK_PAGING_PT_Lx_PTE_PRESENT(pte, lvl)		    \
	UK_PAL_PT_Lx_PTE_PRESENT(pte, lvl)

/**
 * UK_PAGING_PT_Lx_PTE_CLEAR_PRESENT(pte, lvl)
 *
 * @param pte a page table entry from a page table at the given level
 * @param lvl a page table level [0..UK_PAGING_PT_LEVELS - 1]
 *
 * @return a modified version of the input PTE so that the HW will not use the
 *    PTE for address translation. Any bits ignored by the HW in this invalid
 *    state should remain untouched.
 */
#define UK_PAGING_PT_Lx_PTE_CLEAR_PRESENT(pte, lvl)		\
	UK_PAL_PT_Lx_PTE_CLEAR_PRESENT(pte, lvl)

/**
 * UK_PAGING_PT_Lx_PTE_INVALID(lvl)
 *
 * @param lvl a page table level [0..UK_PAGING_PT_LEVELS - 1]
 *
 * @return a PTE value that expresses an invalid state in a page table at the
 *    given level where the HW will not use the page table entry to translate
 *    a virtual address.
 */
#define UK_PAGING_PT_Lx_PTE_INVALID(lvl)				\
		UK_PAL_PT_Lx_PTE_INVALID(lvl)

/**
 * UK_PAGING_PT_Lx_PTE_PADDR(pte, lvl)
 *
 * @param pte a page table entry from a page table at the given level
 * @param lvl a page table level [0..UK_PAGING_PT_LEVELS - 1]
 *
 * @return the physical address to which the given page table entry points
 */
#define UK_PAGING_PT_Lx_PTE_PADDR(pte, lvl)		\
	UK_PAL_PT_Lx_PTE_PADDR(pte, lvl)

/**
 * UK_PAGING_PT_Lx_PTE_SET_PADDR(pte, lvl, paddr)
 *
 * @param pte a page table entry from a page table at the given level
 * @param lvl a page table level [0..UK_PAGING_PT_LEVELS - 1]
 * @param paddr the physical address which to set in the PTE
 *
 * @return the PTE with the updated physical address
 */
#define UK_PAGING_PT_Lx_PTE_SET_PADDR(pte, lvl, paddr)		    \
	UK_PAL_PT_Lx_PTE_SET_PADDR(pte, lvl, paddr)

/**
 * UK_PAGING_PAGE_Lx_HAS(lvl)
 *
 * @param lvl a page table level [0..UK_PAGING_PT_LEVELS - 1]
 *
 * @return a non-zero value if the architecture supports mapping a page at
 *    the given level, 0 otherwise. For example, x86 can map pages at levels
 *    0 (small = 4 KiB), 1 (large = 2 MiB), and 2 (huge = 1 GiB)
 */
#define UK_PAGING_PAGE_Lx_HAS(lvl)		\
	UK_PAL_PAGE_Lx_HAS(lvl)

/**
 * UK_PAGING_PAGE_Lx_IS(pte, lvl)
 *
 * @param pte a page table entry from a page table at the given level
 * @param lvl a page table level [0..UK_PAGING_PT_LEVELS - 1]
 *
 * @return a non-zero value if the page table entry describes a page,
 *    0 otherwise. The return value is undefined if
 *    !UK_PAGING_PT_Lx_PTE_PRESENT(pte, lvl)
 */
#define UK_PAGING_PAGE_Lx_IS(pte, lvl)		    \
	UK_PAL_PAGE_Lx_IS(pte, lvl)

#endif /* !__ASSEMBLY__ */

#if CONFIG_LIBUKPAGING
#if !__ASSEMBLY__

struct uk_falloc;

struct uk_pagetable {
	__vaddr_t pt_vbase;
	__paddr_t pt_pbase;

	struct uk_falloc *fa;

#ifdef CONFIG_LIBUKPAGING_STATS
	unsigned long nr_lx_pages[UK_PAL_PT_LEVELS];
	unsigned long nr_lx_splits[UK_PAL_PT_LEVELS];
	unsigned long nr_pt_pages[UK_PAL_PT_LEVELS];
#endif /* CONFIG_LIBUKPAGING_STATS */
};

/* Page operation flags */
#define UK_PAGING_PAGE_FLAG_KEEP_PTES	0x01 /* Preserve PTEs on map/unmap */
#define UK_PAGING_PAGE_FLAG_KEEP_FRAMES	0x02 /* Preserve frames on unmap */
#define UK_PAGING_PAGE_FLAG_FORCE_SIZE	0x04 /* Force the page size specified
					      *	with UK_PAGING_PAGE_FLAG_SIZE()
					      */
#define UK_PAGING_PAGE_FLAG_SIZE_SHIFT	4
#define UK_PAGING_PAGE_FLAG_SIZE_BITS	4
#define UK_PAGING_PAGE_FLAG_SIZE_MASK	\
	((1UL << UK_PAGING_PAGE_FLAG_SIZE_BITS) - 1)

#define UK_PAGING_PAGE_FLAG_SIZE(lvl)				\
	(((lvl) & UK_PAGING_PAGE_FLAG_SIZE_MASK) <<		\
	 UK_PAGING_PAGE_FLAG_SIZE_SHIFT)

#define UK_PAGING_PAGE_FLAG_SIZE_TO_LEVEL(flag)			\
	(((flag) >> UK_PAGING_PAGE_FLAG_SIZE_SHIFT) &		\
	 UK_PAGING_PAGE_FLAG_SIZE_MASK)

/* Page table clone flags */
#define UK_PAGING_PAGE_FLAG_CLONE_NEW	0x01 /* Create an empty page table */

/**
 * UK_PAGING_PT_Lx_PT_PAGES(lvl, pages) macro
 *
 * Computes the number of page table pages in the given level to map the
 * specified number of pages
 *
 * @param lvl page table level in the range [0..UK_PAL_PT_LEVELS - 1]
 * @param pages number of pages to map
 *
 * @return the number of page table pages required
 */
#define UK_PAGING_PT_Lx_PT_PAGES(lvl, pages)			\
	(DIV_ROUND_UP((pages), UK_PAL_PT_Lx_PTES(lvl)))

/**
 * UK_PAGING_PT_PAGES(pages) macro
 *
 * Computes the total number of page table pages over all levels required to
 * map the given number of data pages with the smallest page size. This
 * supports 5 levels maximum at the moment.
 *
 * @param pages number of pages to map
 *
 * @return number of page table pages required to map the pages considering
 *    the whole page table hierarchy
 */
#define _PT_L1_PT_PAGES(pages)						\
	(UK_PAGING_PT_Lx_PT_PAGES(0, pages))
#define _PT_L2_PT_PAGES(pages)						\
	(UK_PAGING_PT_Lx_PT_PAGES(1, pages) +				\
	 _PT_L1_PT_PAGES(UK_PAGING_PT_Lx_PT_PAGES(1, pages)))
#define _PT_L3_PT_PAGES(pages)						\
	(UK_PAGING_PT_Lx_PT_PAGES(2, pages) +				\
	 _PT_L2_PT_PAGES(UK_PAGING_PT_Lx_PT_PAGES(2, pages)))
#define _PT_L4_PT_PAGES(pages)						\
	(UK_PAGING_PT_Lx_PT_PAGES(3, pages) +				\
	 _PT_L3_PT_PAGES(UK_PAGING_PT_Lx_PT_PAGES(3, pages)))
#define _PT_L5_PT_PAGES(pages)						\
	(UK_PAGING_PT_Lx_PT_PAGES(4, pages) +				\
	 _PT_L4_PT_PAGES(UK_PAGING_PT_Lx_PT_PAGES(4, pages)))

#define __PT_PAGES(lvls, pages)		_PT_L##lvls##_PT_PAGES(pages)
#define _PT_PAGES(lvls, pages)		__PT_PAGES(lvls, pages)
#define UK_PAGING_PT_PAGES(pages)	_PT_PAGES(UK_PAL_PT_LEVELS, pages)

static inline
int uk_paging_paddr_isvalid(__paddr_t addr)
{
	return uk_pal_paddr_isvalid(addr);
}

static inline
int uk_paging_paddr_range_isvalid(__paddr_t start, __sz len)
{
	return uk_pal_paddr_range_isvalid(start, len);
}

static inline
int uk_paging_vaddr_isvalid(__vaddr_t addr)
{
	return uk_pal_vaddr_isvalid(addr);
}

static inline
int uk_paging_vaddr_range_isvalid(__vaddr_t start, __sz len)
{
	return uk_pal_vaddr_range_isvalid(start, len);
}

static inline
int uk_paging_pte_read(__vaddr_t pt_vaddr, unsigned int lvl,
		       unsigned int idx, __pte_t *pte)
{
	return uk_pal_pte_read(pt_vaddr, lvl, idx, pte);
}

static inline int
uk_paging_pte_write(__vaddr_t pt_vaddr, unsigned int lvl,
		    unsigned int idx, __pte_t pte)
{
	return uk_pal_pte_write(pt_vaddr, lvl, idx, pte);
}

static inline
__paddr_t uk_paging_pt_read_base(void)
{
	return uk_pal_pt_read_base();
}

static inline
int uk_paging_pt_write_base(__paddr_t pt_paddr)
{
	return uk_pal_pt_write_base(pt_paddr);
}

static inline
void uk_paging_tlb_flush_entry(__vaddr_t vaddr)
{
	uk_pal_tlb_flush_entry(vaddr);
}

static inline
void uk_paging_tlb_flush(void)
{
	uk_pal_tlb_flush();
}

/**
 * Returns the active page table (the one that defines the virtual address
 * space at the moment of the execution of this function).
 */
struct uk_pagetable *uk_paging_pt_get_active(void);

/**
 * Switches the active page table to the specified one.
 *
 * @param pt
 *   The page table instance to switch to. The code of the function
 *   must be mapped into the new address space at the same virtual address.
 *
 * @return
 *   0 on success, a non-zero value otherwise
 */
int uk_paging_pt_set_active(struct uk_pagetable *pt);

/**
 * Initializes a new page table from the currently configured page table
 * in hardware and assigns the given physical address range to be available
 * for allocations and mappings.
 *
 * @param pt
 *   An uninitialized page table that will be set to the page table hierarchy
 *   currently configured in hardware
 * @param start
 *   Start of the physical address range that will be available
 *   for allocations of physical memory (e.g., mapping with UK_PAL_PADDR_INV).
 *   The function may reserve some memory in this area for own purposes. The
 *   range must not be assigned to other page tables.
 * @param len
 *   The length (in bytes) of the physical address range
 *
 * @return
 *   0 on success, a non-zero value otherwise
 */
int uk_paging_pt_init(struct uk_pagetable *pt, __paddr_t start, __sz len);

/**
 * Adds a physical memory range to the frame allocator of the given page table.
 * The physical memory is available to all page tables sharing the same frame
 * allocator.
 *
 * @param start
 *   Start of the physical address range that will be available for allocations
 *   of physical memory (e.g., mapping with UK_PAL_PADDR_INV). The function may
 *   reserve some memory in this area for own purposes. The range must not be
 *   assigned to other page tables.
 * @param len
 *   The length (in bytes) of the physical address range
 *
 * @return
 *   0 on success, a non-zero value otherwise
 */
int uk_paging_pt_add_mem(struct uk_pagetable *pt, __paddr_t start, __sz len);

/**
 * Initializes a new page table as clone of another page table. The new page
 * table shares the physical address range available for new allocations and
 * mappings with the source page table.
 *
 * @param pt
 *   An uninitialized page table that will receive a clone of the source page
 *   table
 * @param pt_src
 *   The source page table that will be cloned
 * @param flags
 *   Clone flags (UK_PAGING_PAGE_FLAG_CLONE_*)
 *
 *   If UK_PAGING_PAGE_FLAG_CLONE_NEW is specified, a new (empty) top-level
 *   page table is created. Note that this page table will be completely empty
 *   and thus do not map any code or data segments of the kernel.
 *
 * @return
 *   0 on success, a non-zero value otherwise
 */
int uk_paging_pt_clone(struct uk_pagetable *pt, struct uk_pagetable *pt_src,
		       unsigned long flags);

/**
 * Frees the given page table by recursively releasing the page table hierarchy
 * and all mapped physical memory. Mapped physical memory not belonging to the
 * page table is not freed.
 *
 * @param pt
 *   The page table hierarchy to release
 * @param flags
 *   Page flags (UK_PAGING_PAGE_FLAG_* flags)
 *
 *   If UK_PAGING_PAGE_FLAG_KEEP_FRAMES is specified, the physical memory is
 *   not freed and may be mapped again. The caller is responsible for freeing
 *   the physical memory. Note that the physical memory might not be contiguous.
 *
 * @return
 *   0 on success, a non-zero value otherwise
 */
int uk_paging_pt_free(struct uk_pagetable *pt, unsigned long flags);

/**
 * Performs a page table walk
 *
 * @param pt
 *   The page table instance on which to operate
 * @param vaddr
 *   The virtual address to translate
 * @param[in,out] level
 *   The level in which the walk should stop. Use UK_PAL_PAGE_LEVEL to perform a
 *   complete walk. The parameter returns the level in the page table where the
 *   translation ended. This value might be higher than the specified level,
 *   depending on the page table. Can be NULL in which case UK_PAL_PAGE_LEVEL is
 *   assumed.
 * @param[out] pt_vaddr
 *   The virtual address of the page table where the translation ended. Can be
 *   NULL. Can be used with uk_paging_pte_write() and UK_PAL_PT_Lx_IDX() to
 *   update the PTE.
 * @param[out] pte
 *   The PTE where the translation ended. Can be NULL.
 *
 * @return
 *   0 on success, a non-zero value otherwise.
 */
int uk_paging_pt_walk(struct uk_pagetable *pt, __vaddr_t vaddr,
		      unsigned int *level, __vaddr_t *pt_vaddr, __pte_t *pte);

/* Forward declaration */
struct uk_paging_page_mapx;

/**
 * Page mapper function that allows controlling the mappings that are created
 * in a call to uk_paging_page_mapx().
 *
 * @param pt
 *   The page table that the mapping is done in
 * @param vaddr
 *   The virtual address for which a mapping is established
 * @param pt_vaddr
 *   The virtual address of the actual hardware page table that is modified.
 *   Use this to retrieve the current PTE, if needed.
 * @param level
 *   The page table level at which the mapping will be created
 * @param[in,out] pte
 *   The new PTE that will be set. The handler may freely modify the PTE to
 *   control the mapping.
 * @param ctx
 *   An optional user-supplied context
 *
 * @return
 *   - 0 on success (i.e., the PTE should be applied)
 *   - a negative error code to indicate a fatal error
 *   - UK_PAGING_PAGE_MAPX_ESKIP to skip the current PTE (do not apply the
 *     changes)
 *   - UK_PAGING__PAGE_MAPX_ETOOBIG to indicate that the mapping should be
 *     retried using a smaller page size
 */
typedef int (*uk_paging_page_mapx_func_t)(struct uk_pagetable *pt,
					  __vaddr_t vaddr, __vaddr_t pt_vaddr,
					  unsigned int level, __pte_t *pte,
					  void *ctx);

/* Page mapper function return codes */
#define UK_PAGING_PAGE_MAPX_ESKIP	1 /* Skip the current PTE */
#define UK_PAGING_PAGE_MAPX_ETOOBIG	2 /* Retry with smaller page */

struct uk_paging_page_mapx {
	/** Handler called before updating the PTE in the page table */
	uk_paging_page_mapx_func_t map;
	/** Optional user context */
	void *ctx;
};

/**
 * Creates a mapping from a range of contiguous virtual addresses to a range of
 * physical addresses using the specified attributes.
 *
 * @param pt
 *   The page table instance on which to operate
 * @param vaddr
 *   The virtual address of the first page in the new mapping
 * @param paddr
 *   The physical address of the memory which the virtual region is mapped to.
 *   This parameter can be UK_PAL_PADDR_INV to dynamically allocate physical
 *   memory as needed. Note that, the physical memory might not be contiguous.
 *
 *   paddr should be 0 if physical addresses should be handled by the mapx page
 *   mapper.
 * @param pages
 *   The number of pages in requested page size to map
 * @param attr
 *   Page attributes to set for the new mapping (see PAGE_ATTR_* flags)
 * @param flags
 *   Page flags (see UK_PAGING_PAGE_FLAG_* flags)
 *
 *   The page size can be specified with UK_PAGING_PAGE_FLAG_SIZE(). If
 *   UK_PAGING_PAGE_FLAG_FORCE_SIZE is not specified, the function tries to
 *   map the given range (i.e., pages * requested page size) using the largest
 *   possible pages. The actual mapping thus may use larger or smaller pages
 *   than requested depending on address alignment, supported page sizes, and,
 *   if paddr is UK_PAL_PADDR_INV, the available contiguous physical memory.
 *   If UK_PAGING_PAGE_FLAG_FORCE_SIZE is specified, only mappings of the given
 *   page size are created.
 *
 *   If UK_PAGING_PAGE_FLAG_KEEP_PTES is specified, the new mapping will
 *   incorporate the PTEs currently present in the page table. The physical
 *   address and permission flags will be updated according to the new mapping.
 * @param mapx
 *   Optional page mapper object. If the page mapper is supplied, it is called
 *   right before applying a new PTE, giving the mapper a chance to affect the
 *   mapping. Depending on the return code of the mapper it is also possible to
 *   skip the current PTE or force a smaller page size (if
 *   UK_PAGING_PAGE_FLAG_FORCE_SIZE is not set). Note that the page mapper is
 *   not called for PTEs referencing other page tables.
 *
 *   If paddr is UK_PAL_PADDR_INV, the PTE supplied to the mapper will point to
 *   newly allocated physical memory that can be initialized before becoming
 *   visible. Use UK_PAL_PT_Lx_PTE_PADDR() to retrieve the physical address from
 *   the PTE and uk_paging_page_kmap() to temporarily map the physical memory.
 *   Note that, if the PTE currently present in the page table already points to
 *   a valid mapping (i.e., UK_PAL_PT_Lx_PTE_PRESENT() returns a non-zero
 *   value), no new physical memory will be allocated. Instead, the physical
 *   address will remain unchanged. It is the mapper's responsibilty to properly
 *   free the referenced physical memory, if the physical address is changed.
 *
 *   Before calling the page mapper, existing large pages may be split up if a
 *   smaller page size is enforced.
 *
 * @return
 *   0 on success, a non-zero value otherwise. May fail if:
 *   - the physical or virtual address is not aligned to the page size
 *   - a page in the region is already mapped and no mapx is supplied
 *   - a page table could not be set up
 *   - if UK_PAL_PADDR_INV flag is set and there are no more free frames
 *   - the platform rejected the operation
 *   - the mapx page mapper returned a fatal error
 */
int uk_paging_page_mapx(struct uk_pagetable *pt, __vaddr_t vaddr,
			__paddr_t paddr, unsigned long pages,
			unsigned long attr, unsigned long flags,
			struct uk_paging_page_mapx *mapx);

#define uk_paging_page_map(pt, va, pa, pages, attr, flags)		\
	uk_paging_page_mapx(pt, va, pa, pages, attr, flags, __NULL)

/**
 * Removes the mappings from a range of contiguous virtual addresses and frees
 * the underlying frames. The operation skips address ranges that do not have a
 * valid mapping.
 *
 * @param pt
 *   The page table instance on which to operate
 * @param vaddr
 *   The virtual address of the first page that is to be unmapped
 * @param pages
 *   The number of pages in requested page size to unmap
 * @param flags
 *   Page flags (UK_PAGING_PAGE_FLAG_* flags)
 *
 *   The page size can be specified with UK_PAGING_PAGE_FLAG_SIZE(). If
 *   UK_PAGING_PAGE_FLAG_FORCE_SIZE is not specified, the range is split into
 *   pages of the largest sizes that satisfy the requested operation.
 *
 *   If UK_PAGING_PAGE_FLAG_KEEP_FRAMES is specified, the physical memory is
 *   not freed and may be mapped again. The caller is responsible for freeing
 *   the physical memory. Note that the physical memory might not be contiguous.
 *
 *   If UK_PAGING_PAGE_FLAG_KEEP_PTES is specified, the page table hierarchy
 *   will stay intact. PTEs will only be invalidated (e.g., unsetting the
 *   present bit).
 *
 * @return
 *   0 on success, a non-zero value otherwise. May fail if:
 *   - the virtual address is not aligned to the page size
 *   - the platform rejected the operation
 *
 *   Note that it is not an error to unmap physical memory not previously being
 *   allocated with the associated frame allocator. However, unmapping one of
 *   multiple mappings of the same physical frame will release the frame if it
 *   belongs to this page table's frame allocator (i.e., no reference counting).
 *   In this case, use UK_PAGING_PAGE_FLAG_KEEP_FRAMES for all mappings but the
 *   last one.
 */
int uk_paging_page_unmap(struct uk_pagetable *pt, __vaddr_t vaddr,
			 unsigned long pages, unsigned long flags);

/**
 * Sets new attributes for a range of continuous virtual addresses. The
 * operation skips address ranges that do not have a valid mapping.
 *
 * @param pt
 *   The page table instance on which to operate
 * @param vaddr
 *   The virtual address of the first page whose attributes should be changed
 * @param pages
 *   The number of pages in requested page size to change
 * @param new_attr
 *   The new page attributes (PAGE_ATTR_* flags)
 * @param flags
 *   Page flags (UK_PAGING_PAGE_FLAG_* flags)
 *
 *   The page size can be specified with UK_PAGING_PAGE_FLAG_SIZE(). If
 *   UK_PAGING_PAGE_FLAG_FORCE_SIZE is not specified, the range is split into
 *   pages of the largest sizes that satisfy the requested operation.
 *
 * @return
 *   0 on success, a non-zero value otherwise. May fail if:
 *   - the virtual address is not aligned to the page size
 *   - the platform rejected the operation
 */
int uk_paging_page_set_attr(struct uk_pagetable *pt, __vaddr_t vaddr,
			    unsigned long pages, unsigned long new_attr,
			    unsigned long flags);

/**
 * Translates a virtual address to the corresponding physical address on
 * the currently active page table.
 *
 * @param address The virtual address to translate
 * @return The corresponding physical address
 */
__paddr_t uk_paging_virt_to_phys(__vaddr_t address);

/**
 * Creates a temporary writable virtual mapping of the given physical address
 * range for kernel use.
 *
 * The function should only be used for short-lived kernel-internal mappings,
 * for instance, to initialize the contents of a frame before mapping it
 * somewhere else. The mapping will be done in an architecture-dependent
 * reserved area in the virtual address space. The mapping is guaranteed to
 * succeed without memory allocations (physical and virtual).
 *
 * However, note that the number of concurrently k'mapped pages may be limited.
 *
 * @param pt
 *   The page table instance on which to operate
 * @param paddr
 *   The base address of the physical address range to map
 * @param pages
 *   The number of pages in requested page size to map
 * @param flags
 *   Page flags (UK_PAGING_PAGE_FLAG_* flags)
 *
 *   Currently, the only valid flag is UK_PAGING_PAGE_FLAG_SIZE() to specify
 *   the page size. Note that the actual mapping in the page table may use an
 *   arbitrary page size.
 *
 * @return
 *   The virtual address of the temporary mapping on success, UK_PAL_VADDR_INV
 *   otherwise
 */
__vaddr_t uk_paging_page_kmap(struct uk_pagetable *pt, __paddr_t paddr,
			      unsigned long pages, unsigned long flags);

/**
 * Removes a temporary mapping previously established using
 * uk_paging_page_kmap(). The function must not be used to
 * unmap arbitrary address ranges.
 *
 * @param pt
 *   The page table instance on which to operate
 * @param vaddr
 *   The virtual address of the temporary mapping which should be unmapped
 * @param pages
 *   The number of pages in requested page size to unmap
 * @param flags
 *   Page flags (UK_PAGING_PAGE_FLAG_* flags)
 *
 *   Currently, the only valid flag is UK_PAGING_PAGE_FLAG_SIZE() to specify
 *   the page size for the purpose of describing the mapping length.
 */
void uk_paging_page_kunmap(struct uk_pagetable *pt, __vaddr_t vaddr,
			   unsigned long pages, unsigned long flags);

/**
 * Initialize paging subsystem. This function is architecurally generic. It
 * begins by assigning the free memory regions to the page frame allocator,
 * unmapping the static boot page tables and finishes by mapping all the
 * memory regions flagged as UKPLAT_MEMRF_MAP.
 */
int uk_paging_init(void);

#else /* !CONFIG_LIBUKPAGING */

static inline
__paddr_t uk_paging_virt_to_phys(__vaddr_t address)
{
	return (__paddr_t)address;
}

#endif /* !__ASSEMBLY__ */
#endif /* !CONFIG_LIBUKPAGING */

#ifdef __cplusplus
}
#endif
#endif /* __UK_PAGING_H__ */
