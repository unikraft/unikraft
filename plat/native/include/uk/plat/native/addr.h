/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_NATIVE_ADDR_H__
#define __UK_PLAT_NATIVE_ADDR_H__

#include <uk/arch/types.h>
#include <uk/plat/native/arch/addr.h>

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_LIBUKPLAT_NATIVE_ADDR

#if !__ASSEMBLY__

#if CONFIG_LIBUKPLAT_NATIVE_PAGING

/**
 * Tests if a certain range of virtual addresses is valid on the current
 * architecture. For example, most 64-bit architectures do not fully implement
 * 64 bits for the virtual address.
 *
 * @param start the start of the virtual address range
 * @param  len the length of the virtual address range
 * @return a non-zero value if the addresses in the range are supported
 */
int uk_plat_native_vaddr_range_isvalid(__vaddr_t start, __sz len);

/**
 * Tests if a virtual address is valid on the current architecture.
 *
 * @param addr the virtual address to test
 * @return a non-zero value if the address is supported
 */
int uk_plat_native_vaddr_isvalid(__vaddr_t addr);

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
int uk_plat_native_paddr_range_isvalid(__paddr_t start, __sz len);

/**
 * Tests if a physical address is valid on the current architecture.
 *
 * @param addr the physical address to test
 * @return a non-zero value if the address is supported
 */
int uk_plat_native_paddr_isvalid(__paddr_t addr);

#endif /* CONFIG_LIBUKPLAT_NATIVE_PAGING */

#endif /* !__ASSEMBLY__ */

#endif /* CONFIG_LIBUKPLAT_NATIVE_ADDR */

#ifdef __cplusplus
}
#endif
#endif /* __UK_PLAT_NATIVE_ADDR_H__ */
