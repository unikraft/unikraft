/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_PAL_PT_H__
#define __UK_PLAT_PAL_PT_H__

#include <uk/arch/types.h>
#include <uk/plat/native/pt.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UK_PAL_PT_LEVELS	    UK_PLAT_NATIVE_PT_LEVELS
#define UK_PAL_PTES_PER_LEVEL	    UK_PLAT_NATIVE_PTES_PER_LEVEL
#define UK_PAL_PT_LEVEL_SHIFT	    UK_PLAT_NATIVE_PT_LEVEL_SHIFT

#if !__ASSEMBLY__

#define UK_PAL_PT_Lx_IDX(vaddr, lvl)			    \
	UK_PLAT_NATIVE_PT_Lx_IDX(vaddr, lvl)

#define UK_PAL_PT_Lx_PTES(lvl)				    \
	UK_PLAT_NATIVE_PT_Lx_PTES(lvl)

#define UK_PAL_PT_Lx_PTE_PRESENT(pte, lvl)		    \
	UK_PLAT_NATIVE_PT_Lx_PTE_PRESENT(pte, lvl)

#define UK_PAL_PT_Lx_PTE_CLEAR_PRESENT(pte, lvl)	    \
	UK_PLAT_NATIVE_PT_Lx_PTE_CLEAR_PRESENT(pte, lvl)

#define UK_PAL_PT_Lx_PTE_INVALID(lvl)			    \
	UK_PLAT_NATIVE_PT_Lx_PTE_INVALID(lvl)

#define UK_PAL_PT_Lx_PTE_PADDR(pte, lvl)		    \
	UK_PLAT_NATIVE_PT_Lx_PTE_PADDR(pte, lvl)

#define UK_PAL_PT_Lx_PTE_SET_PADDR(pte, lvl, paddr)	    \
	UK_PLAT_NATIVE_PT_Lx_PTE_SET_PADDR(pte, lvl, paddr)

#if CONFIG_LIBUKPLAT_NATIVE_PAGING

static inline
__pte_t uk_pal_pte_create(__paddr_t paddr, unsigned long attr,
			  unsigned int level, __pte_t tmpl,
			  unsigned int tmpl_level)
{
	return uk_plat_native_pte_create(paddr, attr, level, tmpl, tmpl_level);
}

static inline
int uk_pal_pte_read(__vaddr_t pt_vaddr, unsigned int lvl,
		    unsigned int idx, __pte_t *pte)
{
	return uk_plat_native_pte_read(pt_vaddr, lvl, idx, pte);
}

static inline
int uk_pal_pte_write(__vaddr_t pt_vaddr, unsigned int lvl,
		     unsigned int idx, __pte_t pte)
{
	return uk_plat_native_pte_write(pt_vaddr, lvl, idx, pte);
}

static inline
__pte_t uk_pal_pte_change_attr(__pte_t pte, unsigned long new_attr,
			       unsigned int level)
{
	return uk_plat_native_pte_change_attr(pte, new_attr, level);
}

static inline
unsigned long uk_pal_attr_from_pte(__pte_t pte, unsigned int level)
{
	return uk_plat_native_attr_from_pte(pte, level);
}

static inline
__paddr_t uk_pal_pt_read_base(void)
{
	return uk_plat_native_pt_read_base();
}

static inline
int uk_pal_pt_write_base(__paddr_t pt_paddr)
{
	return uk_plat_native_pt_write_base(pt_paddr);
}

static inline
__pte_t uk_pal_pt_pte_create(__paddr_t pt_paddr, unsigned int level,
			     __pte_t tmpl, unsigned int tmpl_level)
{
	return uk_plat_native_pt_pte_create(pt_paddr, level, tmpl, tmpl_level);
}

#endif /* CONFIG_LIBUKPLAT_NATIVE_PAGING */

#endif /* !__ASSEMBLY__ */

#ifdef __cplusplus
}
#endif
#endif /* __UK_PLAT_PAL_PT_H__ */
