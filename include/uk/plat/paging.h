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

/* Page operation flags */
#define PAGE_FLAG_KEEP_PTES	0x01 /* Preserve PTEs on map/unmap */
#define PAGE_FLAG_KEEP_FRAMES	0x02 /* Preserve frames on unmap */
#define PAGE_FLAG_FORCE_SIZE	0x04 /* Force the page size specified with \
				      * PAGE_FLAG_SIZE() \
				      */

#define PAGE_FLAG_SIZE_SHIFT	4
#define PAGE_FLAG_SIZE_BITS	4
#define PAGE_FLAG_SIZE_MASK	((1UL << PAGE_FLAG_SIZE_BITS) - 1)

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
 * @param pt
 *   The page table instance to switch to. The code of the function
 *   must be mapped into the new address space at the same virtual address.
 *
 * @return
 *   0 on success, a non-zero value otherwise
 */
int ukplat_pt_set_active(struct uk_pagetable *pt);

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
 *   for allocations of physical memory (e.g., mapping with __PADDR_ANY).
 *   The function may reserve some memory in this area for own purposes. The
 *   range must not be assigned to other page tables.
 * @param len
 *   The length (in bytes) of the physical address range
 *
 * @return
 *   0 on success, a non-zero value otherwise
 */
int ukplat_pt_init(struct uk_pagetable *pt, __paddr_t start, __sz len);

/**
 * Adds a physical memory range to the frame allocator of the given page table.
 * The physical memory is available to all page tables sharing the same frame
 * allocator.
 *
 * @param start
 *   Start of the physical address range that will be available for allocations
 *   of physical memory (e.g., mapping with __PADDR_ANY). The function may
 *   reserve some memory in this area for own purposes. The range must not be
 *   assigned to other page tables.
 * @param len
 *   The length (in bytes) of the physical address range
 *
 * @return
 *   0 on success, a non-zero value otherwise
 */
int ukplat_pt_add_mem(struct uk_pagetable *pt, __paddr_t start, __sz len);

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
 *   Clone flags (PAGE_FLAG_CLONE_*)
 *
 *   If PAGE_FLAG_CLONE_NEW is specified, a new (empty) top-level page table is
 *   created. Note that this page table will be completely empty and thus do
 *   not map any code or data segments of the kernel.
 *
 * @return
 *   0 on success, a non-zero value otherwise
 */
int ukplat_pt_clone(struct uk_pagetable *pt, struct uk_pagetable *pt_src,
		    unsigned long flags);

/**
 * Frees the given page table by recursively releasing the page table hierarchy
 * and all mapped physical memory. Mapped physical memory not belonging to the
 * page table is not freed.
 *
 * @param pt
 *   The page table hierarchy to release
 * @param flags
 *   Page flags (PAGE_FLAG_* flags)
 *
 *   If PAGE_FLAG_KEEP_FRAMES is specified, the physical memory is not freed
 *   and may be mapped again. The caller is responsible for freeing the
 *   physical memory. Note that the physical memory might not be contiguous.
 *
 * @return
 *   0 on success, a non-zero value otherwise
 */
int ukplat_pt_free(struct uk_pagetable *pt, unsigned long flags);

/**
 * Performs a page table walk
 *
 * @param pt
 *   The page table instance on which to operate
 * @param vaddr
 *   The virtual address to translate
 * @param[in,out] level
 *   The level in which the walk should stop. Use PAGE_LEVEL to perform a
 *   complete walk. The parameter returns the level in the page table where the
 *   translation ended. This value might be higher than the specified level,
 *   depending on the page table. Can be NULL in which case PAGE_LEVEL is
 *   assumed.
 * @param[out] pt_vaddr
 *   The virtual address of the page table where the translation ended. Can be
 *   NULL. Can be used with ukarch_pte_write() and PT_Lx_IDX() to update the
 *   PTE.
 * @param[out] pte
 *   The PTE where the translation ended. Can be NULL.
 *
 * @return
 *   0 on success, a non-zero value otherwise.
 */
int ukplat_pt_walk(struct uk_pagetable *pt, __vaddr_t vaddr,
		   unsigned int *level, __vaddr_t *pt_vaddr, __pte_t *pte);

/* Forward declaration */
struct ukplat_page_mapx;

/**
 * Page mapper function that allows controlling the mappings that are created
 * in a call to ukplat_page_mapx().
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
 *   - UKPLAT_PAGE_MAPX_ESKIP to skip the current PTE (do not apply the changes)
 *   - UKPLAT_PAGE_MAPX_ETOOBIG to indicate that the mapping should be retried
 *     using a smaller page size
 */
typedef int (*ukplat_page_mapx_func_t)(struct uk_pagetable *pt,
				       __vaddr_t vaddr, __vaddr_t pt_vaddr,
				       unsigned int level, __pte_t *pte,
				       void *ctx);

/* Page mapper function return codes */
#define UKPLAT_PAGE_MAPX_ESKIP		1 /* Skip the current PTE */
#define UKPLAT_PAGE_MAPX_ETOOBIG	2 /* Retry with smaller page */

struct ukplat_page_mapx {
	/** Handler called before updating the PTE in the page table */
	ukplat_page_mapx_func_t map;
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
 *   This parameter can be __PADDR_ANY to dynamically allocate physical memory
 *   as needed. Note that, the physical memory might not be contiguous.
 *
 *   paddr should be 0 if physical addresses should be handled by the mapx page
 *   mapper.
 * @param pages
 *   The number of pages in requested page size to map
 * @param attr
 *   Page attributes to set for the new mapping (see PAGE_ATTR_* flags)
 * @param flags
 *   Page flags (see PAGE_FLAG_* flags)
 *
 *   The page size can be specified with PAGE_FLAG_SIZE(). If
 *   PAGE_FLAG_FORCE_SIZE is not specified, the function tries to map the given
 *   range (i.e., pages * requested page size) using the largest possible pages.
 *   The actual mapping thus may use larger or smaller pages than requested
 *   depending on address alignment, supported page sizes, and, if paddr is
 *   __PADDR_ANY, the available contiguous physical memory. If
 *   PAGE_FLAG_FORCE_SIZE is specified, only mappings of the given page size
 *   are created.
 *
 *   If PAGE_FLAG_KEEP_PTES is specified, the new mapping will incorporate the
 *   PTEs currently present in the page table. The physical address and
 *   permission flags will be updated according to the new mapping.
 * @param mapx
 *   Optional page mapper object. If the page mapper is supplied, it is called
 *   right before applying a new PTE, giving the mapper a chance to affect the
 *   mapping. Depending on the return code of the mapper it is also possible to
 *   skip the current PTE or force a smaller page size (if PAGE_FLAG_FORCE_SIZE
 *   is not set). Note that the page mapper is not called for PTEs referencing
 *   other page tables.
 *
 *   If paddr is __PADDR_ANY, the PTE supplied to the mapper will point to newly
 *   allocated physical memory that can be initialized before becoming visible.
 *   Use PT_Lx_PTE_PADDR() to retrieve the physical address from the PTE and
 *   ukplat_page_kmap() to temporarily map the physical memory. Note that, if
 *   the PTE currently present in the page table already points to a valid
 *   mapping (i.e., PT_Lx_PTE_PRESENT() returns a non-zero value), no new
 *   physical memory will be allocated. Instead, the physical address will
 *   remain unchanged. It is the mapper's responsibilty to properly free the
 *   referenced physical memory, if the physical address is changed.
 *
 *   Before calling the page mapper, existing large pages may be split up if a
 *   smaller page size is enforced.
 *
 * @return
 *   0 on success, a non-zero value otherwise. May fail if:
 *   - the physical or virtual address is not aligned to the page size
 *   - a page in the region is already mapped and no mapx is supplied
 *   - a page table could not be set up
 *   - if __PADDR_ANY flag is set and there are no more free frames
 *   - the platform rejected the operation
 *   - the mapx page mapper returned a fatal error
 */
int ukplat_page_mapx(struct uk_pagetable *pt, __vaddr_t vaddr,
		     __paddr_t paddr, unsigned long pages,
		     unsigned long attr, unsigned long flags,
		     struct ukplat_page_mapx *mapx);

#define ukplat_page_map(pt, va, pa, pages, attr, flags)			\
	ukplat_page_mapx(pt, va, pa, pages, attr, flags, __NULL)

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
 *   Page flags (PAGE_FLAG_* flags)
 *
 *   The page size can be specified with PAGE_FLAG_SIZE(). If
 *   PAGE_FLAG_FORCE_SIZE is not specified, the range is split into pages of
 *   the largest sizes that satisfy the requested operation.
 *
 *   If PAGE_FLAG_KEEP_FRAMES is specified, the physical memory is not freed
 *   and may be mapped again. The caller is responsible for freeing the
 *   physical memory. Note that the physical memory might not be contiguous.
 *
 *   If PAGE_FLAG_KEEP_PTES is specified, the page table hierarchy will stay
 *   intact. PTEs will only be invalidated (e.g., unsetting the present bit).
 * @return
 *   0 on success, a non-zero value otherwise. May fail if:
 *   - the virtual address is not aligned to the page size
 *   - the platform rejected the operation
 *
 *   Note that it is not an error to unmap physical memory not previously being
 *   allocated with the associated frame allocator. However, unmapping one of
 *   multiple mappings of the same physical frame will release the frame if it
 *   belongs to this page table's frame allocator (i.e., no reference counting).
 *   In this case, use PAGE_FLAG_KEEP_FRAMES for all mappings but the last one.
 */
int ukplat_page_unmap(struct uk_pagetable *pt, __vaddr_t vaddr,
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
 *   Page flags (PAGE_FLAG_* flags)
 *
 *   The page size can be specified with PAGE_FLAG_SIZE(). If
 *   PAGE_FLAG_FORCE_SIZE is not specified, the range is split into pages of
 *   the largest sizes that satisfy the requested operation.
 *
 * @return
 *   0 on success, a non-zero value otherwise. May fail if:
 *   - the virtual address is not aligned to the page size
 *   - the platform rejected the operation
 */
int ukplat_page_set_attr(struct uk_pagetable *pt, __vaddr_t vaddr,
			 unsigned long pages, unsigned long new_attr,
			 unsigned long flags);

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
 *   Page flags (PAGE_FLAG_* flags)
 *
 *   Currently, the only valid flag is PAGE_FLAG_SIZE() to specify the page
 *   size. Note that the actual mapping in the page table may use an arbitrary
 *   page size.
 *
 * @return
 *   The virtual address of the temporary mapping on success, __VADDR_INV
 *   otherwise
 */
__vaddr_t ukplat_page_kmap(struct uk_pagetable *pt, __paddr_t paddr,
			   unsigned long pages, unsigned long flags);

/**
 * Removes a temporary mapping previously established using ukplat_page_kmap().
 * The function must not be used to unmap arbitrary address ranges.
 *
 * @param pt
 *   The page table instance on which to operate
 * @param vaddr
 *   The virtual address of the temporary mapping which should be unmapped
 * @param pages
 *   The number of pages in requested page size to unmap
 * @param flags
 *   Page flags (PAGE_FLAG_* flags)
 *
 *   Currently, the only valid flag is PAGE_FLAG_SIZE() to specify the page
 *   size for the purpose of describing the mapping length.
 */
void ukplat_page_kunmap(struct uk_pagetable *pt, __vaddr_t vaddr,
			unsigned long pages, unsigned long flags);

/**
 * Initialize paging subsystem. This function is architecurally generic. It
 * begins by assigning the free memory regions to the page frame allocator,
 * unmapping the static boot page tables and finishes by mapping all the
 * memory regions flagged as UKPLAT_MEMRF_MAP.
 */
int ukplat_paging_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __UKPLAT_PAGING_H__ */
