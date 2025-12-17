/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, Karlsruhe Institute of Technology (KIT).
 *                     All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PAL_PAGE_H__
#define __UK_PAL_PAGE_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Page size definitions
 */

#ifndef UK_PAL_PAGE_LARGE_LEVEL
#error "UK_PAL_PAGE_LARGE_LEVEL not defined"
#endif

#ifndef UK_PAL_PAGE_LARGE_SHIFT
#error "UK_PAL_PAGE_LARGE_SHIFT not defined"
#endif

#ifndef UK_PAL_PAGE_LARGE_SIZE
#error "UK_PAL_PAGE_LARGE_SIZE not defined"
#endif

#ifndef UK_PAL_PAGE_LARGE_MASK
#error "UK_PAL_PAGE_LARGE_MASK not defined"
#endif

#ifndef UK_PAL_PAGE_HUGE_LEVEL
#error "UK_PAL_PAGE_HUGE_LEVEL not defined"
#endif

#ifndef UK_PAL_PAGE_HUGE_SHIFT
#error "UK_PAL_PAGE_HUGE_SHIFT not defined"
#endif

#ifndef UK_PAL_PAGE_HUGE_SIZE
#error "UK_PAL_PAGE_HUGE_SIZE not defined"
#endif

#ifndef UK_PAL_PAGE_HUGE_MASK
#error "UK_PAL_PAGE_HUGE_MASK not defined"
#endif

/**
 * UK_PAL_PAGE_Lx_SHIFT(lvl)
 *
 * NOTE: Must be compile-time resolvable
 *
 * @param lvl a page table level [0..UK_PAL_PT_LEVELS - 1]
 *
 * @return the order of the page size at the given level. Must return the
 *    appropriate order even if the architecture does not support configuring
 *    a page at this level
 */
#ifndef UK_PAL_PAGE_Lx_SHIFT
#error "UK_PAL_PAGE_Lx_SHIFT not defined"
#endif

/**
 * UK_PAL_PAGE_SHIFT_Lx(shift)
 *
 * NOTE: Must be compile-time resolvable
 *
 * @param shift the order of the page size
 *
 * @return the page table level [0..UK_PAL_PT_LEVELS - 1] corresponding to the
 *    given page size order
 */
#ifndef UK_PAL_PAGE_SHIFT_Lx
#error "UK_PAL_PAGE_SHIFT_Lx not defined"
#endif

/**
 * UK_PAL_PAGE_Lx_SIZE(lvl)
 *
 * @param lvl a page table level [0..UK_PAL_PT_LEVELS - 1]
 *
 * @return the size of a page at the given level. Must return the
 *    appropriate size even if the architecture does not support configuring
 *    a page at this level
 */
#ifndef UK_PAL_PAGE_Lx_SIZE
#error "UK_PAL_PAGE_Lx_SIZE not defined"
#endif

/**
 * UK_PAL_PAGE_Lx_MASK(lvl)
 *
 * @param lvl a page table level [0..UK_PAL_PT_LEVELS - 1]
 *
 * @return a mask that selects the page number at the given level and discards
 *    any bits within the page size. Must return the appropriate mask even if
 *    the architecture does not support configuring a page at this level
 */
#ifndef UK_PAL_PAGE_Lx_MASK
#error "UK_PAL_PAGE_Lx_MASK not defined"
#endif

/**
 * UK_PAL_PAGE_Lx_ALIGN_UP(addr, lvl)
 *
 * @param addr a virtual or physical address
 * @param lvl a page table level [0..UK_PAL_PT_LEVELS - 1]
 *
 * @return the address aligned up to the next page at the given level
 */
#ifndef UK_PAL_PAGE_Lx_ALIGN_UP
#error "UK_PAL_PAGE_Lx_ALIGN_UP not defined"
#endif

/**
 * UK_PAL_PAGE_Lx_ALIGN_DOWN(addr, lvl)
 *
 * @param addr a virtual or physical address
 * @param lvl a page table level [0..UK_PAL_PT_LEVELS - 1]
 *
 * @return the address aligned down to the current page at the given level
 */
#ifndef UK_PAL_PAGE_Lx_ALIGN_DOWN
#error "UK_PAL_PAGE_Lx_ALIGN_DOWN not defined"
#endif

/**
 * UK_PAL_PAGE_Lx_ALIGNED(addr, lvl)
 *
 * @param addr a virtual or physical address
 * @param lvl a page table level [0..UK_PAL_PT_LEVELS - 1]
 *
 * @return a non-zero value if the given address is aligned to the page size at
 *    the given level, 0 otherwise
 */
#ifndef UK_PAL_PAGE_Lx_ALIGNED
#error "UK_PAL_PAGE_Lx_ALIGNED not defined"
#endif

/* Helper macros for the smallest page size */
#ifndef UK_PAL_PAGE_LEVEL
#error "UK_PAL_PAGE_LEVEL not defined"
#endif

#ifndef UK_PAL_PAGE_SHIFT
#error "UK_PAL_PAGE_SHIFT not defined"
#endif

#ifndef UK_PAL_PAGE_SIZE
#error "UK_PAL_PAGE_SIZE not defined"
#endif

#ifndef UK_PAL_PAGE_MASK
#error "UK_PAL_PAGE_MASK not defined"
#endif

#ifndef UK_PAL_PAGE_ALIGN_UP
#error "UK_PAL_PAGE_ALIGN_UP not defined"
#endif

#ifndef UK_PAL_PAGE_ALIGN_DOWN
#error "UK_PAL_PAGE_ALIGN_DOWN not defined"
#endif

#ifndef UK_PAL_PAGE_ALIGNED
#error "UK_PAL_PAGE_ALIGNED not defined"
#endif

#ifndef UK_PAL_PAGE_LARGE_ALIGN_UP
#error "UK_PAL_PAGE_LARGE_ALIGN_UP not defined"
#endif

#ifndef UK_PAL_PAGE_LARGE_ALIGN_DOWN
#error "UK_PAL_PAGE_LARGE_ALIGN_DOWN not defined"
#endif

#ifndef UK_PAL_PAGE_LARGE_ALIGNED
#error "UK_PAL_PAGE_LARGE_ALIGNED not defined"
#endif

#ifndef UK_PAL_PAGE_HUGE_ALIGN_UP
#error "UK_PAL_PAGE_HUGE_ALIGN_UP not defined"
#endif

#ifndef UK_PAL_PAGE_HUGE_ALIGN_DOWN
#error "UK_PAL_PAGE_HUGE_ALIGN_DOWN not defined"
#endif

#ifndef UK_PAL_PAGE_HUGE_ALIGNED
#error "UK_PAL_PAGE_HUGE_ALIGNED not defined"
#endif

/* Page Protections */
#ifndef UK_PAL_PAGE_ATTR_PROT_NONE
#error "UK_PAL_PAGE_ATTR_PROT_NONE not defined"
#endif

#ifndef UK_PAL_PAGE_ATTR_PROT_READ
#error "UK_PAL_PAGE_ATTR_PROT_READ not defined"
#endif

#ifndef UK_PAL_PAGE_ATTR_PROT_WRITE
#error "UK_PAL_PAGE_ATTR_PROT_WRITE not defined"
#endif

#ifndef UK_PAL_PAGE_ATTR_PROT_EXEC
#error "UK_PAL_PAGE_ATTR_PROT_EXEC not defined"
#endif

#if !__ASSEMBLY__
/**
 * UK_PAL_PAGE_Lx_HAS(lvl)
 *
 * @param lvl a page table level [0..UK_PAL_PT_LEVELS - 1]
 *
 * @return a non-zero value if the architecture supports mapping a page at
 *    the given level, 0 otherwise. For example, x86 can map pages at levels
 *    0 (small = 4 KiB), 1 (large = 2 MiB), and 2 (huge = 1 GiB)
 */
#ifndef UK_PAL_PAGE_Lx_HAS
#error "UK_PAL_PAGE_Lx_HAS not defined"
#endif

/**
 * UK_PAL_PAGE_Lx_IS(pte, lvl)
 *
 * @param pte a page table entry from a page table at the given level
 * @param lvl a page table level [0..UK_PAL_PT_LEVELS - 1]
 *
 * @return a non-zero value if the page table entry describes
 *    a page, 0 otherwise. The return value is undefined if
 *    !UK_PAL_PT_Lx_PTE_PRESENT(pte, lvl)
 */
#ifndef UK_PAL_PAGE_Lx_IS
#error "UK_PAL_PAGE_Lx_IS not defined"
#endif

#endif /* !__ASSEMBLY__ */
#ifdef __cplusplus
}
#endif

#endif /* __UK_PAL_PAGE_H__ */
