/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *
 * Copyright (c) 2021, Karlsruhe Institute of Technology (KIT).
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

#ifndef __UKARCH_PAGING_H__
#define __UKARCH_PAGING_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <uk/arch/types.h>
#include <uk/asm/paging.h>

/* Page attributes. See <uk/asm/paging.h> for more */
#define PAGE_ATTR_PROT_RW	(PAGE_ATTR_PROT_READ | PAGE_ATTR_PROT_WRITE)
#define PAGE_ATTR_PROT_RWX	(PAGE_ATTR_PROT_RW | PAGE_ATTR_PROT_EXEC)

/**
 * PT_LEVELS definition
 *
 * Number of page table levels (e.g., 4 for 4-level page tables). Must be > 0.
 */
#if !defined(PT_LEVELS) || (PT_LEVELS <= 0)
#error Missing or invalid PT_LEVELS definition in architectural paging header
#endif

#ifndef __ASSEMBLY__
#include <uk/essentials.h>

/* Cause a compiler error if not declared */
typedef __pte_t __pte_t;	/* page table entry */

/**
 * __VADDR_INV / __PADDR_INV definitions
 *
 * A value that can be used to express an invalid virtual / physical address.
 *
 * Note: The value must be aligned to the largest supported page size!
 */

#ifndef __VADDR_INV
#error Incomplete definition of virtual address type
#endif

#ifndef __PADDR_INV
#error Incomplete definition of physical address type
#endif

/* Just an alias to express an arbitrary virtual / physical address */
#define __VADDR_ANY			__VADDR_INV
#define __PADDR_ANY			__PADDR_INV

/**
 * PT_Lx_IDX(vaddr, lvl)
 *
 * @param vaddr a virtual address
 * @param lvl a page table level [0..PT_LEVELS - 1]
 *
 * @return the index of the page table entry (starting from 0) in the page
 *    table at the given level that is used to translate the virtual address
 */
#ifndef PT_Lx_IDX
unsigned int PT_Lx_IDX(__vaddr_t vaddr, unsigned int lvl);
#endif

/**
 * PT_Lx_PTES(lvl) macro
 *
 * NOTE: Must be compile-time resolvable
 *
 * @param lvl a page table level [0..PT_LEVELS - 1]
 *
 * @return the number of page table entries per page table at the given level
 */
#ifndef PT_Lx_PTES
#error PT_Lx_PTES not defined
#endif

/**
 * PT_Lx_PTE_PRESENT(pte, lvl)
 *
 * @param pte a page table entry from a page table at the given level
 * @param lvl a page table level [0..PT_LEVELS - 1]
 *
 * @return a non-zero value if the page table entry is present, that is the HW
 *    would use it for address translation. The function must return 0 for
 *    PT_Lx_PTE_INVALID(lvl).
 */
#ifndef PT_Lx_PTE_PRESENT
int PT_Lx_PTE_PRESENT(__pte_t pte, unsigned int lvl);
#endif

/**
 * PT_Lx_PTE_CLEAR_PRESENT(pte, lvl)
 *
 * @param pte a page table entry from a page table at the given level
 * @param lvl a page table level [0..PT_LEVELS - 1]
 *
 * @return a modified version of the input PTE so that the HW will not use the
 *    PTE for address translation. Any bits ignored by the HW in this invalid
 *    state should remain untouched.
 */
#ifndef PT_Lx_PTE_CLEAR_PRESENT
__pte_t PT_Lx_PTE_CLEAR_PRESENT(__pte_t pte, unsigned int lvl);
#endif

/**
 * PT_Lx_PTE_INVALID(lvl)
 *
 * @param lvl a page table level [0..PT_LEVELS - 1]
 *
 * @return a PTE value that expresses an invalid state in a page table at the
 *    given level where the HW will not use the page table entry to translate
 *    a virtual address.
 */
#ifndef PT_Lx_PTE_INVALID
__pte_t PT_Lx_PTE_INVALID(unsigned int level);
#endif

/**
 * PT_Lx_PTE_PADDR(pte, lvl)
 *
 * @param pte a page table entry from a page table at the given level
 * @param lvl a page table level [0..PT_LEVELS - 1]
 *
 * @return the physical address to which the given page table entry points
 */
#ifndef PT_Lx_PTE_PADDR
__paddr_t PT_Lx_PTE_PADDR(__pte_t pte, unsigned int lvl);
#endif

/**
 * PT_Lx_PTE_SET_PADDR(pte, lvl, paddr)
 *
 * @param pte a page table entry from a page table at the given level
 * @param lvl a page table level [0..PT_LEVELS - 1]
 * @param paddr the physical address which to set in the PTE
 *
 * @return the PTE with the updated physical address
 */
#ifndef PT_Lx_PTE_SET_PADDR
__pte_t PT_Lx_PTE_SET_PADDR(__pte_t pte, unsigned int lvl, __paddr_t paddr);
#endif
#endif /* !__ASSEMBLY__ */

/**
 * PAGE_Lx_SHIFT(lvl)
 *
 * NOTE: Must be compile-time resolvable
 *
 * @param lvl a page table level [0..PT_LEVELS - 1]
 *
 * @return the order of the page size at the given level. Must return the
 *    appropriate order even if the architecture does not support configuring
 *    a page at this level
 */
#ifndef PAGE_Lx_SHIFT
#error PAGE_Lx_SHIFT not defined
#endif

/**
 * PAGE_SHIFT_Lx(shift)
 *
 * NOTE: Must be compile-time resolvable
 *
 * @param shift the order of the page size
 *
 * @return the page table level [0..PT_LEVELS - 1] corresponding to the
 *    given page size order
 */
#ifndef PAGE_SHIFT_Lx
#error PAGE_SHIFT_Lx not defined
#endif

/**
 * PAGE_Lx_SIZE(lvl)
 *
 * @param lvl a page table level [0..PT_LEVELS - 1]
 *
 * @return the size of a page at the given level. Must return the
 *    appropriate size even if the architecture does not support configuring
 *    a page at this level
 */
#ifndef PAGE_Lx_SIZE
#define PAGE_Lx_SIZE(lvl)		(1UL << PAGE_Lx_SHIFT(lvl))
#endif

/**
 * PAGE_Lx_MASK(lvl)
 *
 * @param lvl a page table level [0..PT_LEVELS - 1]
 *
 * @return a mask that selects the page number at the given level and discards
 *    any bits within the page size. Must return the appropriate mask even if
 *    the architecture does not support configuring a page at this level
 */
#ifndef PAGE_Lx_MASK
#define PAGE_Lx_MASK(lvl)		(~(PAGE_Lx_SIZE(lvl) - 1))
#endif

/**
 * PAGE_Lx_ALIGN_UP(addr, lvl)
 *
 * @param addr a virtual or physical address
 * @param lvl a page table level [0..PT_LEVELS - 1]
 *
 * @return the address aligned up to the next page at the given level
 */
#ifndef PAGE_Lx_ALIGN_UP
#define PAGE_Lx_ALIGN_UP(addr, lvl)	ALIGN_UP(addr, PAGE_Lx_SIZE(lvl))
#endif

/**
 * PAGE_Lx_ALIGN_DOWN(addr, lvl)
 *
 * @param addr a virtual or physical address
 * @param lvl a page table level [0..PT_LEVELS - 1]
 *
 * @return the address aligned down to the current page at the given level
 */
#ifndef PAGE_Lx_ALIGN_DOWN
#define PAGE_Lx_ALIGN_DOWN(addr, lvl)	ALIGN_DOWN(addr, PAGE_Lx_SIZE(lvl))
#endif

/**
 * PAGE_Lx_ALIGNED(addr, lvl)
 *
 * @param addr a virtual or physical address
 * @param lvl a page table level [0..PT_LEVELS - 1]
 *
 * @return a non-zero value if the given address is aligned to the page size at
 *    the given level, 0 otherwise
 */
#ifndef PAGE_Lx_ALIGNED
#define PAGE_Lx_ALIGNED(addr, lvl)	(!((addr) & (PAGE_Lx_SIZE(lvl) - 1)))
#endif

#ifndef __ASSEMBLY__
/**
 * PAGE_Lx_HAS(lvl)
 *
 * @param lvl a page table level [0..PT_LEVELS - 1]
 *
 * @return a non-zero value if the architecture supports mapping a page at the
 *    the given level, 0 otherwise. For example, x86 can map pages at levels
 *    0 (small = 4 KiB), 1 (large = 2 MiB), and 2 (huge = 1 GiB)
 */
#ifndef PAGE_Lx_HAS
int PAGE_Lx_HAS(unsigned int lvl);
#endif

/**
 * PAGE_Lx_IS(pte, lvl)
 *
 * @param pte a page table entry from a page table at the given level
 * @param lvl a page table level [0..PT_LEVELS - 1]
 *
 * @return a non-zero value if the page table entry describes a page,
 *    0 otherwise. The return value is undefined if !PT_Lx_PTE_PRESENT(pte, lvl)
 */
#ifndef PAGE_Lx_IS
int PAGE_Lx_IS(__pte_t pte, unsigned int lvl);
#endif
#endif /* !__ASSEMBLY__ */

/* Some helper macros for the smallest page size */
#ifndef PAGE_LEVEL
#define PAGE_LEVEL			0
#endif

#ifndef PAGE_SHIFT
#define PAGE_SHIFT			PAGE_Lx_SHIFT(PAGE_LEVEL)
#endif

#ifndef PAGE_SIZE
#define PAGE_SIZE			PAGE_Lx_SIZE(PAGE_LEVEL)
#endif

#ifndef PAGE_MASK
#define PAGE_MASK			PAGE_Lx_MASK(PAGE_LEVEL)
#endif

#ifndef PAGE_ALIGN_UP
#define PAGE_ALIGN_UP(addr)		PAGE_Lx_ALIGN_UP(addr, PAGE_LEVEL)
#endif

#ifndef PAGE_ALIGN_DOWN
#define PAGE_ALIGN_DOWN(addr)		PAGE_Lx_ALIGN_DOWN(addr, PAGE_LEVEL)
#endif

#ifndef PAGE_ALIGNED
#define PAGE_ALIGNED(addr)		PAGE_Lx_ALIGNED(addr, PAGE_LEVEL)
#endif

#ifndef __ASSEMBLY__
/**
 * PT_Lx_PT_PAGES(lvl, pages) macro
 *
 * Computes the number of page table pages in the given level to map the
 * specified number of pages
 *
 * @param lvl page table level in the range [0..PT_LEVELS - 1]
 * @param pages number of pages to map
 *
 * @return the number of page table pages required
 */
#define PT_Lx_PT_PAGES(lvl, pages)					\
	(DIV_ROUND_UP((pages), PT_Lx_PTES(level)))

/**
 * PT_PAGES(pages) macro
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
	(PT_Lx_PT_PAGES(0, pages))
#define _PT_L2_PT_PAGES(pages)						\
	(PT_Lx_PT_PAGES(1, pages) + _PT_L1_PT_PAGES(PT_Lx_PT_PAGES(1, pages)))
#define _PT_L3_PT_PAGES(pages)						\
	(PT_Lx_PT_PAGES(2, pages) + _PT_L2_PT_PAGES(PT_Lx_PT_PAGES(2, pages)))
#define _PT_L4_PT_PAGES(pages)						\
	(PT_Lx_PT_PAGES(3, pages) + _PT_L3_PT_PAGES(PT_Lx_PT_PAGES(3, pages)))
#define _PT_L5_PT_PAGES(pages)						\
	(PT_Lx_PT_PAGES(4, pages) + _PT_L4_PT_PAGES(PT_Lx_PT_PAGES(4, pages)))

#define __PT_PAGES(lvls, pages)		_PT_L##lvls##_PT_PAGES(pages)
#define _PT_PAGES(lvls, pages)		__PT_PAGES(lvls, pages)
#define PT_PAGES(pages)			_PT_PAGES(PT_LEVELS, pages)

/** PAGE_COUNT(len) macro
 *
 * Computes the total number of pages required to map an area of a given
 * length.
 *
 * @param len length of the area to map
 *
 * @return number of pages required to map an area of a given length
 */
#define PAGE_COUNT(len)			DIV_ROUND_UP((len), PAGE_SIZE)

/**
 * Tests if a certain range of virtual addresses is valid on the current
 * architecture. For example, most 64-bit architectures do not fully implement
 * 64 bits for the virtual address.
 *
 * @param start the start of the virtual address range
 * @param  len the length of the virtual address range
 * @return a non-zero value if the addresses in the range are supported
 */
int ukarch_vaddr_range_isvalid(__vaddr_t start, __sz len);

/**
 * Tests if a virtual address is valid on the current architecture.
 *
 * @param addr the virtual address to test
 * @return a non-zero value if the address is supported
 */
int ukarch_vaddr_isvalid(__vaddr_t addr);

/**
 * Tests if a certain range of physical addresses is valid on the current
 * architecture. This only ensures that the physical addresses are supported
 * by the hardware but does not correspond to the actual physical memory
 * installed in the system.
 *
 * @param start the start of the physical address range
 * @param len the length of the physical address range
 * @return a non-zero value if the addresses in the range are supported
 */
int ukarch_paddr_range_isvalid(__paddr_t start, __sz len);

/**
 * Tests if a physical address is valid on the current architecture.
 *
 * @param addr the physical address to test
 * @return a non-zero value if the address is supported
 */
int ukarch_paddr_isvalid(__paddr_t addr);

/**
 * Reads a page table entry from the given page table.
 *
 * @param pt_vaddr virtual address of the mapped page table. The address can be
 *    used to access the page table
 * @param lvl the level of the page table [0..PT_LEVELS - 1]
 * @param idx the index of the PTE to read [0..PT_Lx_PTES(lvl) - 1]
 * @param [out] pte pointer to a variable that will receive the page table entry
 *
 * @return 0 if the page table entry could be read, an non-zero error value
 *    otherwise
 */
int ukarch_pte_read(__vaddr_t pt_vaddr, unsigned int lvl, unsigned int idx,
		    __pte_t *pte);

/**
 * Writes a page table entry to the given page table. Note: TLB entries have to
 * be flushed manually, if needed.
 *
 * @param pt_vaddr virtual address of the mapped page table. The address can be
 *    used to access the page table
 * @param lvl the level of the page table [0..PT_LEVELS - 1]
 * @param idx the index of the PTE to write [0..PT_Lx_PTES(lvl) - 1]
 * @param pte the value of the page table entry to write
 *
 * @return 0 if the page table entry could be written, an non-zero error value
 *    otherwise
 */
int ukarch_pte_write(__vaddr_t pt_vaddr, unsigned int lvl, unsigned int idx,
		     __pte_t pte);

/**
 * Retrieves the currently active page table
 *
 * @return the physical address of the active top-level page table (i.e.,
 *    PT_LEVELS - 1) that is configured in hardware for use in address
 *    translation. May return __PADDR_INV on error
 */
__paddr_t ukarch_pt_read_base(void);

/**
 * Sets the currently active page table. This will also flush the TLB.
 *
 * @param pt_paddr the physical address of the top-level page table
 *    (i.e., PT_LEVELS - 1) that should be configured in hardware for use in
 *    address translation
 *
 * @return 0 if the page table could be activated, a non-zero error value
 *    otherwise
 */
int ukarch_pt_write_base(__paddr_t pt_paddr);

/**
 * Flushes a single entry from the TLB
 *
 * @param vaddr the virtual address of the entry to flush
 */
void ukarch_tlb_flush_entry(__vaddr_t vaddr);

/**
 * Flushes the entire TLB
 */
void ukarch_tlb_flush(void);
#endif /* !__ASSEMBLY__ */

#ifdef __cplusplus
}
#endif

#endif /* __UKARCH_PAGING_H__ */
