/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, Karlsruhe Institute of Technology (KIT).
 *                     All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PAL_ADDR_H__
#define __UK_PAL_ADDR_H__

#include <uk/arch/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !__ASSEMBLY__

/**
 * UK_PAL_VADDR_INV / UK_PAL_PADDR_INV definitions
 *
 * A value that can be used to express an invalid virtual / physical address.
 *
 * Note: The value must be aligned to the largest supported page size!
 */
#ifndef UK_PAL_VADDR_INV
#error "UK_PAL_VADDR_INV not defined"
#endif

#ifndef UK_PAL_PADDR_INV
#error "UK_PAL_PADDR_INV not defined"
#endif

#if CONFIG_HAVE_PAGING

#ifndef UK_PAL_DIRECTMAP_AREA_START
#error "UK_PAL_DIRECTMAP_AREA_START not defined"
#endif

#ifndef UK_PAL_DIRECTMAP_AREA_END
#error "UK_PAL_DIRECTMAP_AREA_END not defined"
#endif

/**
 * Tests if a certain range of virtual addresses is valid on the current
 * architecture. For example, most 64-bit architectures do not fully implement
 * 64 bits for the virtual address.
 *
 * @param start the start of the virtual address range
 * @param  len the length of the virtual address range
 * @return a non-zero value if the addresses in the range are supported
 */
int uk_pal_vaddr_range_isvalid(__vaddr_t start, __sz len);

/**
 * Tests if a virtual address is valid on the current architecture.
 *
 * @param addr the virtual address to test
 * @return a non-zero value if the address is supported
 */
int uk_pal_vaddr_isvalid(__vaddr_t addr);

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
int uk_pal_paddr_range_isvalid(__paddr_t start, __sz len);

/**
 * Tests if a physical address is valid on the current architecture.
 *
 * @param addr the physical address to test
 * @return a non-zero value if the address is supported
 */
int uk_pal_paddr_isvalid(__paddr_t addr);

#endif /* CONFIG_HAVE_PAGING */

#endif /* !__ASSEMBLY__ */
#ifdef __cplusplus
}
#endif
#endif /* __UK_PAL_ADDR_H__ */
