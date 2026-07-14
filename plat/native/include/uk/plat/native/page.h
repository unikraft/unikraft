/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_NATIVE_PAGE_H__
#define __UK_PLAT_NATIVE_PAGE_H__

#include <uk/plat/native/arch/page.h>

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_LIBUKPLAT_NATIVE_PAGE

/**
 * UK_PLAT_NATIVE_PAGE_Lx_SIZE(lvl)
 *
 * @param lvl a page table level [0..UK_PLAT_NATIVE_PT_LEVELS - 1]
 *
 * @return the size of a page at the given level. Must return the
 *    appropriate size even if the architecture does not support configuring
 *    a page at this level
 */
#ifndef UK_PLAT_NATIVE_PAGE_Lx_SIZE
#define UK_PLAT_NATIVE_PAGE_Lx_SIZE(lvl)				\
	(1UL << UK_PLAT_NATIVE_PAGE_Lx_SHIFT(lvl))
#endif

/**
 * UK_PLAT_NATIVE_PAGE_Lx_MASK(lvl)
 *
 * @param lvl a page table level [0..UK_PLAT_NATIVE_PT_LEVELS - 1]
 *
 * @return a mask that selects the page number at the given level and discards
 *    any bits within the page size. Must return the appropriate mask even if
 *    the architecture does not support configuring a page at this level
 */
#ifndef UK_PLAT_NATIVE_PAGE_Lx_MASK
#define UK_PLAT_NATIVE_PAGE_Lx_MASK(lvl)				\
	(~(UK_PLAT_NATIVE_PAGE_Lx_SIZE(lvl) - 1))
#endif

/**
 * UK_PLAT_NATIVE_PAGE_Lx_ALIGN_UP(val, lvl)
 *
 * @param val a value to align up, usually address or size
 * @param lvl a page table level [0..UK_PLAT_NATIVE_PT_LEVELS - 1]
 *
 * @return the quantity aligned up to the next page at the given level
 */
#ifndef UK_PLAT_NATIVE_PAGE_Lx_ALIGN_UP
#define UK_PLAT_NATIVE_PAGE_Lx_ALIGN_UP(val, lvl)			\
	ALIGN_UP(val, UK_PLAT_NATIVE_PAGE_Lx_SIZE(lvl))
#endif

/**
 * UK_PLAT_NATIVE_PAGE_Lx_ALIGN_DOWN(val, lvl)
 *
 * @param val a value to align down, usually address or size
 * @param lvl a page table level [0..UK_PLAT_NATIVE_PT_LEVELS - 1]
 *
 * @return the value aligned down to the current page at the given level
 */
#ifndef UK_PLAT_NATIVE_PAGE_Lx_ALIGN_DOWN
#define UK_PLAT_NATIVE_PAGE_Lx_ALIGN_DOWN(val, lvl)			\
	ALIGN_DOWN(val, UK_PLAT_NATIVE_PAGE_Lx_SIZE(lvl))
#endif

/**
 * UK_PLAT_NATIVE_PAGE_Lx_ALIGNED(val, lvl)
 *
 * @param val a value to check for alignment, usually address or size
 * @param lvl a page table level [0..UK_PLAT_NATIVE_PT_LEVELS - 1]
 *
 * @return a non-zero value if the passed value is aligned to the
 *         page size at the given level, 0 otherwise
 */
#ifndef UK_PLAT_NATIVE_PAGE_Lx_ALIGNED
#define UK_PLAT_NATIVE_PAGE_Lx_ALIGNED(val, lvl)			\
	(!((val) & (UK_PLAT_NATIVE_PAGE_Lx_SIZE(lvl) - 1)))
#endif

/* Helper macros for the smallest page size */
#ifndef UK_PLAT_NATIVE_PAGE_LEVEL
#define UK_PLAT_NATIVE_PAGE_LEVEL		0
#endif

#ifndef UK_PLAT_NATIVE_PAGE_SHIFT
#define UK_PLAT_NATIVE_PAGE_SHIFT				\
	UK_PLAT_NATIVE_PAGE_Lx_SHIFT(UK_PLAT_NATIVE_PAGE_LEVEL)
#endif

#ifndef UK_PLAT_NATIVE_PAGE_SIZE
#define UK_PLAT_NATIVE_PAGE_SIZE				\
	UK_PLAT_NATIVE_PAGE_Lx_SIZE(UK_PLAT_NATIVE_PAGE_LEVEL)
#endif

#ifndef UK_PLAT_NATIVE_PAGE_MASK
#define UK_PLAT_NATIVE_PAGE_MASK				\
	UK_PLAT_NATIVE_PAGE_Lx_MASK(UK_PLAT_NATIVE_PAGE_LEVEL)
#endif

#ifndef UK_PLAT_NATIVE_PAGE_ALIGN_UP
#define UK_PLAT_NATIVE_PAGE_ALIGN_UP(val)			\
	UK_PLAT_NATIVE_PAGE_Lx_ALIGN_UP(val, UK_PLAT_NATIVE_PAGE_LEVEL)
#endif

#ifndef UK_PLAT_NATIVE_PAGE_ALIGN_DOWN
#define UK_PLAT_NATIVE_PAGE_ALIGN_DOWN(val)			\
	UK_PLAT_NATIVE_PAGE_Lx_ALIGN_DOWN(val, UK_PLAT_NATIVE_PAGE_LEVEL)
#endif

#ifndef UK_PLAT_NATIVE_PAGE_ALIGNED
#define UK_PLAT_NATIVE_PAGE_ALIGNED(val)			\
	UK_PLAT_NATIVE_PAGE_Lx_ALIGNED(val, UK_PLAT_NATIVE_PAGE_LEVEL)
#endif

#ifndef UK_PLAT_NATIVE_PAGE_LARGE_ALIGN_UP
#define UK_PLAT_NATIVE_PAGE_LARGE_ALIGN_UP(val)			\
	UK_PLAT_NATIVE_PAGE_Lx_ALIGN_UP(val, UK_PLAT_NATIVE_PAGE_LARGE_LEVEL)
#endif

#ifndef UK_PLAT_NATIVE_PAGE_LARGE_ALIGN_DOWN
#define UK_PLAT_NATIVE_PAGE_LARGE_ALIGN_DOWN(val)		\
	UK_PLAT_NATIVE_PAGE_Lx_ALIGN_DOWN(val, UK_PLAT_NATIVE_PAGE_LARGE_LEVEL)
#endif

#ifndef UK_PLAT_NATIVE_PAGE_LARGE_ALIGNED
#define UK_PLAT_NATIVE_PAGE_LARGE_ALIGNED(val)			\
	UK_PLAT_NATIVE_PAGE_Lx_ALIGNED(val, UK_PLAT_NATIVE_PAGE_LARGE_LEVEL)
#endif

#ifndef UK_PLAT_NATIVE_PAGE_HUGE_ALIGN_UP
#define UK_PLAT_NATIVE_PAGE_HUGE_ALIGN_UP(val)			\
	UK_PLAT_NATIVE_PAGE_Lx_ALIGN_UP(val, UK_PLAT_NATIVE_PAGE_HUGE_LEVEL)
#endif

#ifndef UK_PLAT_NATIVE_PAGE_HUGE_ALIGN_DOWN
#define UK_PLAT_NATIVE_PAGE_HUGE_ALIGN_DOWN(val)		\
	UK_PLAT_NATIVE_PAGE_Lx_ALIGN_DOWN(val, UK_PLAT_NATIVE_PAGE_HUGE_LEVEL)
#endif

#ifndef UK_PLAT_NATIVE_PAGE_HUGE_ALIGNED
#define UK_PLAT_NATIVE_PAGE_HUGE_ALIGNED(val)			\
	UK_PLAT_NATIVE_PAGE_Lx_ALIGNED(val, UK_PLAT_NATIVE_PAGE_HUGE_LEVEL)
#endif

#if !__ASSEMBLY__
/**
 * UK_PLAT_NATIVE_PAGE_Lx_HAS(lvl)
 *
 * @param lvl a page table level [0..UK_PLAT_NATIVE_PT_LEVELS - 1]
 *
 * @return a non-zero value if the architecture supports mapping a page at
 *    the given level, 0 otherwise. For example, x86 can map pages at levels
 *    0 (small = 4 KiB), 1 (large = 2 MiB), and 2 (huge = 1 GiB)
 */
#ifndef UK_PLAT_NATIVE_PAGE_Lx_HAS
int UK_PLAT_NATIVE_PAGE_Lx_HAS(unsigned int lvl);
#endif

/**
 * UK_PLAT_NATIVE_PAGE_Lx_IS(pte, lvl)
 *
 * @param pte a page table entry from a page table at the given level
 * @param lvl a page table level [0..UK_PLAT_NATIVE_PT_LEVELS - 1]
 *
 * @return a non-zero value if the page table entry describes a page,
 *    0 otherwise. The return value is undefined if
 *    !UK_PLAT_NATIVE_PT_Lx_PTE_PRESENT(pte, lvl)
 */
#ifndef UK_PLAT_NATIVE_PAGE_Lx_IS
int UK_PLAT_NATIVE_PAGE_Lx_IS(__pte_t pte, unsigned int lvl);
#endif

#endif /* !__ASSEMBLY__ */

#endif /* CONFIG_LIBUKPLAT_NATIVE_PAGE */

#ifdef __cplusplus
}
#endif
#endif /* __UK_PLAT_NATIVE_PAGE_H__ */
