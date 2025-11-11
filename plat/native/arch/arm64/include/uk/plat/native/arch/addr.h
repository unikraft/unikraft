/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, Karlsruhe Institute of Technology (KIT).
 *                     All rights reserved.
 * Copyright (c) 2022, OpenSynergy GmbH. All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_NATIVE_ARCH_ADDR_H__
#define __UK_PLAT_NATIVE_ARCH_ADDR_H__

#include <uk/arch/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_LIBUKPLAT_NATIVE_ADDR

#if !__ASSEMBLY__

#define __ARM64_PADDR_BITS	48
#define __ARM64_VADDR_BITS	48

#define __ARM64_PADDR_MAX			    \
	((1ULL << __ARM64_PADDR_BITS) - 1)
#define __ARM64_VADDR_MAX			    \
	((1ULL << __ARM64_VADDR_BITS) - 1)

#define __ARM64_PADDR_VALID(paddr)		    \
	((paddr) <= (__paddr_t)__ARM64_PADDR_MAX)
#define __ARM64_VADDR_VALID(vaddr)		    \
	(((vaddr) & __ARM64_VADDR_MAX) <= (__vaddr_t)__ARM64_VADDR_MAX)

/* Any address controlled by TTBR1, ie bit[55] == 1 */
#define __ARM64_INVALID_ADDR		0xBAADBAADBAADBAADUL

#define UK_PLAT_NATIVE_VADDR_INV					\
	UK_PLAT_NATIVE_PAGE_Lx_ALIGN_DOWN(__ARM64_INVALID_ADDR,		\
					  __ARM64_PT_MAP_LEVEL_MAX)

#define UK_PLAT_NATIVE_PADDR_INV					\
	UK_PLAT_NATIVE_PAGE_Lx_ALIGN_DOWN(__ARM64_INVALID_ADDR,		\
					  __ARM64_PT_MAP_LEVEL_MAX)

#if CONFIG_LIBUKPLAT_NATIVE_PAGING

#include <uk/assert.h>

#define UK_PLAT_NATIVE_DIRECTMAP_AREA_START 0x0000ff8000000000
#define UK_PLAT_NATIVE_DIRECTMAP_AREA_END   0x0000ffffffffffff

/**
 * Tests if a certain range of virtual addresses is valid on the current
 * architecture. For example, most 64-bit architectures do not fully implement
 * 64 bits for the virtual address.
 *
 * @param start the start of the virtual address range
 * @param  len the length of the virtual address range
 * @return a non-zero value if the addresses in the range are supported
 */
static inline
int uk_plat_native_vaddr_range_isvalid(__vaddr_t start, __sz len)
{
	__vaddr_t end = start + len - 1;

	UK_ASSERT(start <= end);

	return (__ARM64_VADDR_VALID(end)) && (__ARM64_VADDR_VALID(start));
}

/**
 * Tests if a virtual address is valid on the current architecture.
 *
 * @param addr the virtual address to test
 * @return a non-zero value if the address is supported
 */
static inline
int uk_plat_native_vaddr_isvalid(__vaddr_t addr)
{
	return __ARM64_VADDR_VALID(addr);
}

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
static inline
int uk_plat_native_paddr_range_isvalid(__paddr_t start, __sz len)
{
	__paddr_t end = start + len - 1;

	UK_ASSERT(start <= end);

	return (__ARM64_PADDR_VALID(end)) && (__ARM64_PADDR_VALID(start));
}

/**
 * Tests if a physical address is valid on the current architecture.
 *
 * @param addr the physical address to test
 * @return a non-zero value if the address is supported
 */
static inline
int uk_plat_native_paddr_isvalid(__paddr_t addr)
{
	return __ARM64_PADDR_VALID(addr);
}
#endif /* CONFIG_LIBUKPLAT_NATIVE_PAGING */

#endif /* !__ASSEMBLY__ */

#endif /* CONFIG_LIBUKPLAT_NATIVE_ADDR */

#ifdef __cplusplus
}
#endif
#endif /* __UK_PLAT_NATIVE_ARCH_H__ */
