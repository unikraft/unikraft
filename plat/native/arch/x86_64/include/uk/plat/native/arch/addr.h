/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, Karlsruhe Institute of Technology (KIT).
 *                     All rights reserved.
 * Copyright (c) 2021, University Politehnica of Bucharest.
 *                     All rights reserved.
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

/* We use a non-canonical address larger than 52 bits (max phys) */
#define __X86_64_INVALID_ADDR		0xBAADBAADBAADBAADUL

#define UK_PLAT_NATIVE_VADDR_INV	\
	UK_PLAT_NATIVE_PAGE_HUGE_ALIGN_DOWN(__X86_64_INVALID_ADDR)
#define UK_PLAT_NATIVE_PADDR_INV	\
	UK_PLAT_NATIVE_PAGE_HUGE_ALIGN_DOWN(__X86_64_INVALID_ADDR)

#if CONFIG_LIBUKPLAT_NATIVE_PAGING

#include <uk/assert.h>

extern __paddr_t uk_plat_native_x86_64_pg_maxphysaddr;

#if UK_PLAT_NATIVE_PT_LEVELS == 5
#define __X86_64_VADDR_BITS		57
#else
#define __X86_64_VADDR_BITS		48
#endif

#define __X86_64_PTE_PADDR_BITS		52
#define __X86_64_PTE_PADDR_MASK		((1UL << __X86_64_PTE_PADDR_BITS) - 1)

#define __X86_64_PG_VADDR_SHIFT		8
#define __X86_64_PG_VADDR_BITS		16
#define __X86_64_PG_VADDR_MASK					\
	(((1UL << __X86_64_PG_VADDR_BITS) - 1) << __X86_64_PG_VADDR_SHIFT)

#define __X86_64_PG_PADDR_SHIFT		0
#define __X86_64_PG_PADDR_BITS		8
#define __X86_64_PG_PADDR_MASK					\
	(((1UL << __X86_64_PG_PADDR_BITS) - 1) << __X86_64_PG_PADDR_SHIFT)

/* In canonical form bit 47 is replicated into the bits [48..63]. We compute
 * this via sign extension
 */
#define __X86_64_VADDR_CANONICALIZE(vaddr)					\
	((__vaddr_t)(((__ssz)(vaddr) << (64 - __X86_64_VADDR_BITS)) >>	\
		(64 - __X86_64_VADDR_BITS)))

#define __X86_64_PG_VALID_PADDR(paddr)	\
	((paddr) <= uk_plat_native_x86_64_pg_maxphysaddr)

#define UK_PLAT_NATIVE_DIRECTMAP_AREA_START 0xffffff8000000000 /* -512 GiB */
#define UK_PLAT_NATIVE_DIRECTMAP_AREA_END   0xffffffffffffffff

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

	if (__X86_64_VADDR_CANONICALIZE(start) != start)
		return 0;

	if (__X86_64_VADDR_CANONICALIZE(end) != end)
		return 0;

	/* Check if start and end have both the last valid bit set or unset. As
	 * both addresses are in canonical form this ensures that the range
	 * does not cross the non-canonical range
	 */
	return (((start ^ end) & (1UL << (__X86_64_VADDR_BITS - 1))) == 0);
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
	return __X86_64_VADDR_CANONICALIZE(addr) == addr;
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
	return (__X86_64_PG_VALID_PADDR(start) && __X86_64_PG_VALID_PADDR(end));
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
	return __X86_64_PG_VALID_PADDR(addr);
}

#endif /* CONFIG_LIBUKPLAT_NATIVE_PAGING */

#endif /* !__ASSEMBLY__ */

#endif /* CONFIG_LIBUKPLAT_NATIVE_ADDR */

#ifdef __cplusplus
}
#endif
#endif /* __UK_PLAT_NATIVE_ARCH_ADDR_H__ */
