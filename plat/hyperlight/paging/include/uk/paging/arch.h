/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/*
 * Hyperlight-specific paging arch layer.
 *
 * Overrides lib/ukpaging/arch/<arch>/include/uk/paging/arch.h.
 *
 * Hyperlight's guest memory has two regions with different
 * physical-to-virtual translations:
 *
 *   1. Identity-mapped low region (guest code/data/heap):
 *      GVA == GPA.  These pages are CoW-protected by the host.
 *
 *   2. Scratch region (page tables, I/O buffers, CoW copies):
 *      GPAs at the top of guest RAM, GVAs near the top of the
 *      address space (see hyperlight/mem.h).  Linear offset: GVA = hl_scratch_base_gva +
 *                                   (GPA - hl_scratch_base_gpa).
 *
 * There is no single-offset directmap.  Instead,
 * pgarch_directmap_paddr_to_vaddr() dispatches based on which
 * region the GPA falls in.
 */

#include <uk/arch/types.h>
#include <uk/essentials.h>

#include <uk/plat/pal/addr.h>
#include <uk/plat/pal/page.h>
#include <uk/plat/pal/paging.h>
#include <uk/plat/pal/pt.h>
#include <uk/plat/pal/tlb.h>

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_LIBUKPAGING
#if !__ASSEMBLY__

#include <uk/assert.h>
#include <hyperlight/mem.h>

#if UK_PAL_PT_LEVELS < 4
#error "Unsupported number of page table levels"
#endif

struct uk_pagetable;

/**
 * Translate a guest physical address to a guest virtual address.
 *
 * For scratch-region GPAs (>= hl_scratch_base_gpa), applies the
 * scratch offset.  For identity-mapped low GPAs, returns the GPA
 * directly (since GVA == GPA in the identity-mapped region).
 */
static inline
__vaddr_t pgarch_directmap_paddr_to_vaddr(__paddr_t paddr)
{
	if (paddr >= hl_scratch_base_gpa)
		return (__vaddr_t)(hl_scratch_base_gva +
				   (paddr - hl_scratch_base_gpa));
	return (__vaddr_t)paddr;
}

/**
 * Translate a guest virtual address back to a guest physical address.
 *
 * Reverse of pgarch_directmap_paddr_to_vaddr.
 */
static inline
__paddr_t pgarch_directmap_vaddr_to_paddr(__vaddr_t vaddr)
{
	if (vaddr >= hl_scratch_base_gva)
		return (__paddr_t)(hl_scratch_base_gpa +
				   (vaddr - hl_scratch_base_gva));
	return (__paddr_t)vaddr;
}

/*
 * Page tables
 */
static inline
__vaddr_t pgarch_pt_pte_to_vaddr(struct uk_pagetable *pt __unused, __pte_t pte,
				 unsigned int level __maybe_unused)
{
	return pgarch_directmap_paddr_to_vaddr(
		UK_PAL_PT_Lx_PTE_PADDR(pte, level));
}

static inline
__vaddr_t pgarch_pt_map(struct uk_pagetable *pt __unused, __paddr_t pt_paddr,
			unsigned int level __unused)
{
	return pgarch_directmap_paddr_to_vaddr(pt_paddr);
}

static inline
__paddr_t pgarch_pt_unmap(struct uk_pagetable *pt __unused, __vaddr_t pt_vaddr,
			  unsigned int level __unused)
{
	return pgarch_directmap_vaddr_to_paddr(pt_vaddr);
}

/* Temporary kernel mapping */
static inline
__vaddr_t pgarch_kmap(struct uk_pagetable *pt __unused, __paddr_t paddr,
		      __sz len __unused)
{
	return pgarch_directmap_paddr_to_vaddr(paddr);
}

static inline
void pgarch_kunmap(struct uk_pagetable *pt __unused, __vaddr_t vaddr __unused,
		   __sz len __unused)
{
	/* nothing to do */
}

int pgarch_pt_add_mem(struct uk_pagetable *pt, __paddr_t start, __sz len);

int pgarch_pt_init(struct uk_pagetable *pt, __paddr_t start, __sz len);

#endif /* !__ASSEMBLY__ */
#endif /* CONFIG_LIBUKPAGING */

#ifdef __cplusplus
}
#endif
