/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_PAL_ADDR_H__
#define __UK_PLAT_PAL_ADDR_H__

#include <uk/arch/types.h>
#include <uk/plat/native/addr.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !__ASSEMBLY__

#define UK_PAL_VADDR_INV	    UK_PLAT_NATIVE_VADDR_INV
#define UK_PAL_PADDR_INV	    UK_PLAT_NATIVE_PADDR_INV

#if CONFIG_LIBUKPLAT_NATIVE_PAGING

#define UK_PAL_DIRECTMAP_AREA_START  UK_PLAT_NATIVE_DIRECTMAP_AREA_START
#define UK_PAL_DIRECTMAP_AREA_END    UK_PLAT_NATIVE_DIRECTMAP_AREA_END

static inline
int uk_pal_vaddr_range_isvalid(__vaddr_t start, __sz len)
{
	return uk_plat_native_vaddr_range_isvalid(start, len);
}

static inline
int uk_pal_vaddr_isvalid(__vaddr_t addr)
{
	return uk_plat_native_vaddr_isvalid(addr);
}

static inline
int uk_pal_paddr_range_isvalid(__paddr_t start, __sz len)
{
	return uk_plat_native_paddr_range_isvalid(start, len);
}

static inline
int uk_pal_paddr_isvalid(__paddr_t addr)
{
	return uk_plat_native_paddr_isvalid(addr);
}

#endif /* CONFIG_LIBUKPLAT_NATIVE_PAGING */

#endif /* !__ASSEMBLY__ */

#ifdef __cplusplus
}
#endif
#endif /* __UK_PLAT_PAL_ADDR_H__ */
