/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_PAL_PT_H__
#define __UK_PLAT_PAL_PT_H__

#include <uk/arch/types.h>
#include <uk/plat/xen/pt.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UK_PAL_PT_LEVELS	UK_PLAT_XEN_PT_LEVELS
#define UK_PAL_PTES_PER_LEVEL	UK_PLAT_XEN_PTES_PER_LEVEL
#define UK_PAL_PT_LEVEL_SHIFT	UK_PLAT_XEN_PT_LEVEL_SHIFT

#if !__ASSEMBLY__

#define UK_PAL_PT_Lx_IDX(vaddr, lvl)			    \
	UK_PLAT_XEN_PT_Lx_IDX(vaddr, lvl)

#define UK_PAL_PT_Lx_PTES(lvl)				    \
	UK_PLAT_XEN_PT_Lx_PTES(lvl)

#define UK_PAL_PT_Lx_PTE_PRESENT(pte, lvl)		    \
	UK_PLAT_XEN_PT_Lx_PTE_PRESENT(pte, lvl)

#define UK_PAL_PT_Lx_PTE_CLEAR_PRESENT(pte, lvl)	    \
	UK_PLAT_XEN_PT_Lx_PTE_CLEAR_PRESENT(pte, lvl)

#define UK_PAL_PT_Lx_PTE_INVALID(lvl)			    \
	UK_PLAT_XEN_PT_Lx_PTE_INVALID(lvl)

#define UK_PAL_PT_Lx_PTE_PADDR(pte, lvl)		    \
	UK_PLAT_XEN_PT_Lx_PTE_PADDR(pte, lvl)

#define UK_PAL_PT_Lx_PTE_SET_PADDR(pte, lvl, paddr)	    \
	UK_PLAT_XEN_PT_Lx_PTE_SET_PADDR(pte, lvl, paddr)

/* Xen platform does not currently support paging, do not define pt(e) ops */

#endif /* !__ASSEMBLY__ */

#ifdef __cplusplus
}
#endif

#endif /* __UK_PLAT_PAL_PT_H__ */
