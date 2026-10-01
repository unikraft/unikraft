/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_XEN_PT_H__
#define __UK_PLAT_XEN_PT_H__

#include <uk/config.h>
#include <uk/plat/native/pt.h>
#include <uk/plat/xen/arch/pt.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UK_PLAT_XEN_PT_LEVELS		UK_PLAT_XEN_ARCH_PT_LEVELS

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

#if CONFIG_HAVE_PAGING

static inline
__pte_t uk_plat_xen_pte_create(__paddr_t paddr, unsigned long attr,
			       unsigned int level, __pte_t tmpl,
			       unsigned int tmpl_level)
{
	return uk_plat_native_pte_create(paddr, attr, level, tmpl, tmpl_level);
}

static inline
int uk_plat_xen_pte_read(__vaddr_t pt_vaddr, unsigned int lvl,
			 unsigned int idx, __pte_t *pte)
{
	return uk_plat_native_pte_read(pt_vaddr, lvl, idx, pte);
}

static inline
int uk_plat_xen_pte_write(__vaddr_t pt_vaddr, unsigned int lvl,
			  unsigned int idx, __pte_t pte)
{
	return uk_plat_native_pte_write(pt_vaddr, lvl, idx, pte);
}

static inline
unsigned long uk_plat_xen_attr_from_pte(__pte_t pte, unsigned int level)
{
	return uk_plat_native_attr_from_pte(pte, level);
}

static inline
__pte_t uk_plat_xen_pt_pte_create(__paddr_t pt_paddr, unsigned int level,
				  __pte_t tmpl, unsigned int tmpl_level)
{
	return uk_plat_native_pt_pte_create(pt_paddr, level, tmpl, tmpl_level);
}

static inline
__paddr_t uk_plat_xen_pt_read_base(void)
{
	return uk_plat_xen_arch_pt_read_base();
}

static inline
int uk_plat_xen_pt_write_base(__paddr_t pt_paddr)
{
	return uk_plat_xen_arch_pt_write_base(pt_paddr);
}

#endif /* CONFIG_HAVE_PAGING */

#endif /* !__ASSEMBLY__ */

#ifdef __cplusplus
}
#endif

#endif /* __UK_PLAT_XEN_PT_H__ */
