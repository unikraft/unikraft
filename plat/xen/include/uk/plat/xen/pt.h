/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_XEN_PT_H__
#define __UK_PLAT_XEN_PT_H__

/* Xen shares the fundamental pagetable format with native platform */
#include <uk/plat/native/pt.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UK_PLAT_XEN_PT_LEVELS		UK_PLAT_NATIVE_PT_LEVELS
#define UK_PLAT_XEN_PTES_PER_LEVEL	UK_PLAT_NATIVE_PTES_PER_LEVEL
#define UK_PLAT_XEN_PT_LEVEL_SHIFT	UK_PLAT_NATIVE_PT_LEVEL_SHIFT

#if !__ASSEMBLY__

#define UK_PLAT_XEN_PT_Lx_IDX			\
	UK_PLAT_NATIVE_PT_Lx_IDX

#define UK_PLAT_XEN_PT_Lx_PTES			\
	UK_PLAT_NATIVE_PT_Lx_PTES

#define UK_PLAT_XEN_PT_Lx_PTE_PRESENT		\
	UK_PLAT_NATIVE_PT_Lx_PTE_PRESENT

#define UK_PLAT_XEN_PT_Lx_PTE_CLEAR_PRESENT	\
	UK_PLAT_NATIVE_PT_Lx_PTE_CLEAR_PRESENT

#define UK_PLAT_XEN_PT_Lx_PTE_INVALID		\
	UK_PLAT_NATIVE_PT_Lx_PTE_INVALID

#define UK_PLAT_XEN_PT_Lx_PTE_PADDR		\
	UK_PLAT_NATIVE_PT_Lx_PTE_PADDR

#define UK_PLAT_XEN_PT_Lx_PTE_SET_PADDR		\
	UK_PLAT_NATIVE_PT_Lx_PTE_SET_PADDR


#endif /* !__ASSEMBLY__ */

#ifdef __cplusplus
}
#endif

#endif /* __UK_PLAT_XEN_PT_H__ */
