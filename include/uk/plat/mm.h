/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Stefan Teodorescu <stefanl.teodorescu@gmail.com>
 *
 * Copyright (c) 2021, University Politehnica of Bucharest. All rights reserved.
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

#ifndef __UKPLAT_MM__
#define __UKPLAT_MM__

#include <sys/types.h>
#include <uk/config.h>
#include <uk/framealloc.h>

#ifndef CONFIG_PT_API
#error Using this header requires enabling the virtual memory management API
#endif /* CONFIG_PT_API */

#ifdef CONFIG_PARAVIRT
#include <uk/asm/mm_pv.h>
#else
#include <uk/asm/mm_native.h>
#endif	/* CONFIG_PARAVIRT */

#define UKPLAT_SUPPORT_LARGE_PAGES	0x1
#define UKPLAT_READONLY_PAGETABLE	0x2

#define PT_PAGES_CACHE_SIZE 		PAGE_SIZE
#define PT_PAGES_CACHE_MAX_PAGES	64
#define PT_PAGES_CACHE_MIN_PAGES	16

struct uk_pagetable {
	__vaddr_t pt_base;
	__paddr_t pt_base_paddr;
	struct uk_framealloc *fa;

	/* This variable represents the offset between the virtual address of the page
	 * table memory area and the physical address of it. This offset changes at
	 * runtime between the booting phase and the running phase after that.
	 *
	 * While booting, the physical addresses and the virtual addresses are equal
	 * (either running with paging disabled or with a linear mapping), which means
	 * this variable has the value 0.
	 *
	 * After initializing the new set of page tables, these can be placed at any
	 * virtual address. The offset in this case is DIRECTMAP_AREA_START, defined
	 * in include/uk/mem_layout.h
	 *
	 * TODO: find if there is a better way to achieve this behavior
	 */
	unsigned long virt_offset;

	/* Cache to keep some already mapped page tables. This is necessary to
	 * not end up in a situation where you cannot map new page tables,
	 * because you need to map other new page tables and so on.
	 */
	__vaddr_t *pages_cache;
	size_t pages_cache_length;
};

/**
 * Return the active page table (the one that defines the virtual address space
 * at the moment of the execution of this function).
 */
struct uk_pagetable *ukplat_get_active_pt(void);

/**
 * Create a mapping from a virtual address to a physical address, with given
 * protections and flags.
 *
 * @param pt: the page table instance on which to operate.
 * @param vaddr: the virtual address of the page that is to be mapped.
 * @param paddr: the physical address of the frame to which the virtual page
 * is mapped to. This parameter can be equal to PAGE_PADDR_ANY when the caller
 * is not interested in the physical address where the mapping is created.
 * @param prot: protection permissions of the page (obtained by or'ing
 * PAGE_PROT_* flags).
 * @param flags: flags of the page (obtained by or'ing PAGE_FLAG_* flags).
 *
 * @return: 0 on success and -1 on failure. ukplat_page_map can fail if:
 * - the given physical or virtual addresses are not aligned to page size;
 * - any page in the region is already mapped to another frame;
 * - if PAGE_PADDR_ANY flag is selected and there are no more available
 *   free frames in the physical memory;
 * - the platform rejects the mapping (for example, because large pages are
 *   not supported).
 *
 * In case of failure, the mapping is not created.
 */
int ukplat_page_map(struct uk_pagetable *pt,__vaddr_t vaddr, __paddr_t paddr,
		unsigned long prot, unsigned long flags);

/**
 * Create a mapping from a region starting at a virtual address to a physical
 * address, with given protections and flags.
 *
 * @param pt: the page table instance on which to operate.
 * @param vaddr: the virtual address of the page where the region that is to be
 * mapped starts.
 * @param paddr: the physical address of the starting frame of the region to
 * which the virtual region is mapped to. This parameter can be equal to
 * PAGE_PADDR_ANY when the caller is not interested in the physical address
 * where the mappings are created.
 * @param prot: protection permissions of the pages (obtained by or'ing
 * PAGE_PROT_* flags).
 * @param flags: flags of the page (obtained by or'ing PAGE_FLAG_* flags).
 *
 * @return: 0 on success and -1 on failure. ukplat_page_map_many can fail if:
 * - the given physical or virtual addresses are not aligned to page size;
 * - any page in the region is already mapped to another frame;
 * - if PAGE_PADDR_ANY flag is selected and there are no more available
 *   free frames in the physical memory;
 * - the platform rejects the mapping (for example, because large pages are
 *   not supported).
 *
 * In case of failure, no new mapping is created.
 */
int ukplat_page_map_many(struct uk_pagetable *pt, __vaddr_t vaddr,
		__paddr_t paddr, size_t pages, unsigned long prot,
		unsigned long flags);

/**
 * Frees a mapping for a page and marks the underlying frame as unused.
 *
 * @param pt: the page table instance on which to operate.
 * @param vaddr: the virtual address of the page that is to be unmapped.
 *
 * @return: 0 in case of success and -1 on failure. The call fails if:
 * - the given page is not mapped to any frame;
 * - the virtual address given is not aligned to page (simple/large/huge) size.
 * - the platform rejected the unmapping.
 */
int ukplat_page_unmap(struct uk_pagetable *pt, __vaddr_t vaddr);

/**
 * Frees a mapping for a page, but keeps the underlying physical frame reserved
 * (because it's mapped at another virtual address as well).
 *
 * @param pt: the page table instance on which to operate.
 * @param vaddr: the virtual address of the page that is to be unmapped.
 *
 * @return: 0 in case of success and -1 on failure. The call fails if:
 * - the given page is not mapped to any frame;
 * - the virtual address given is not aligned to page (simple/large/huge) size.
 * - the platform rejected the unmapping.
 */
int ukplat_page_unshare(struct uk_pagetable *pt, __vaddr_t vaddr);

/**
 * Return page table entry corresponding to given virtual address.
 *
 * @param pt: the page table instance on which to operate.
 * @param vaddr: the virtual address, aligned to the corresponding page
 * dimesion (simple, large or huge) size.
 *
 * @return: page table entry (PTE) obtained by doing a page table walk.
 */
__pte_t ukplat_virt_to_pte(struct uk_pagetable *pt, __vaddr_t vaddr);

/**
 * Initialize internal page table bookkeeping for using the PT API when
 * attaching to an existing page table.
 *
 * @param paddr_start: the physical address of the beginning of the area that
 * should be managed by the API.
 * @param len: the length of the (physical) memory area that should be managed.
 *
 * @return: page table instance of the modified boot time page table.
 */
struct uk_pagetable *uk_pt_init(__paddr_t paddr_start, size_t len);

/**
 * Add more physical memory to be managed by the API.
 *
 * @param paddr_start: the first address in the usable physical memory
 * @param len: the length (in bytes) of the physical memory that will be
 * managed by the API.
 */
void uk_pt_add_mem(__paddr_t paddr_start, size_t len);

/**
 * Build a new page table structure from scratch.
 *
 * @param fa: frame allocator that the page table will use.
 * @param kernel_paddr_start: physical address of the beginning of the kernel
 * (where it was loaded in memory by the platform); it is usually 1MB.
 * @param kernel_vaddr_start: virtual address of where the kernel should be
 * mapped; if the build is non-PIE, then this is equal to the physical address.
 * @param kernel_area_size: the length of the kernel area, which is to be
 * mapped.
 *
 * This function creates a new page table structure that maps itself, the
 * kernel and some extra bootstrapping memory needed to make it usable.
 */
struct uk_pagetable *uk_pt_build(struct uk_framealloc *fa,
		__paddr_t kernel_paddr_start,
		__vaddr_t kernel_vaddr_start,
		size_t kernel_area_size);

/**
 * Retrieves platform specific supported virtual memory features.
 *
 * For example, some platforms require the page tables to be mapped as
 * read-only, or don't support large pages.
 *
 * @return: a value obtained by or'ing flags of mm related platform features.
 */
unsigned long ukplat_mm_supported_features(void);

/* TODO: rename and change variable names to make it more generic, as it can be
 * used not just for the heap.
 */
/**
 * Create virtual mappings for a new heap of a given length at a given virtual
 * address.
 *
 * @param vaddr: the virtual address of the beginning of the area where the
 * heap will be mapped.
 * @param len: the length (in bytes) of the heap.
 *
 * @return: 0 in case of success and -1 on failure. The call can fail if:
 * - the given interval [vaddr, vaddr + len] is not contained in the interval
 *   [HEAP_AREA_BEGIN, HEAP_AREA_END];
 * - ukplat_page_map_many fails.
 */
static inline int uk_heap_map(__vaddr_t vaddr, size_t len)
{
	size_t heap_pages, heap_large_pages;

	/*
	if (vaddr < HEAP_AREA_START || vaddr + len > HEAP_AREA_END)
		return -1;
	*/

	if (ukplat_mm_supported_features() & UKPLAT_SUPPORT_LARGE_PAGES) {
		/* Map heap in large and regular pages */
		heap_large_pages = len >> PAGE_LARGE_SHIFT;

		if (ukplat_page_map_many(ukplat_get_active_pt(), vaddr,
				PAGE_PADDR_ANY,
				heap_large_pages,
				PAGE_PROT_READ | PAGE_PROT_WRITE,
				PAGE_FLAG_LARGE))
			return -1;

		/*
		 * If the heap is not properly aligned to PAGE_LARGE_SIZE,
		 * map the rest in regular pages
		 */
		if ((heap_large_pages << PAGE_LARGE_SHIFT) < len) {
			heap_pages = (len - (heap_large_pages << PAGE_LARGE_SHIFT))
				>> PAGE_SHIFT;
		} else {
			heap_pages = 0;
		}

		return ukplat_page_map_many(ukplat_get_active_pt(),
				vaddr + (heap_large_pages << PAGE_LARGE_SHIFT),
				PAGE_PADDR_ANY, heap_pages,
				PAGE_PROT_READ | PAGE_PROT_WRITE, 0);
	} else {
		return ukplat_page_map_many(ukplat_get_active_pt(), vaddr,
				PAGE_PADDR_ANY,
				len >> PAGE_SHIFT,
				PAGE_PROT_READ | PAGE_PROT_WRITE, 0);
	}

	return 0;
}

#endif /* __UKPLAT_MM__ */
