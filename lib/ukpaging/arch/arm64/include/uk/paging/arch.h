/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2022, OpenSynergy GmbH. All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/asm/arch.h>
#include <uk/arch/types.h>
#include <uk/asm/lcpu.h>
#include <uk/essentials.h>
#include <uk/plat/pal/paging.h>

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_LIBUKPAGING
#if !__ASSEMBLY__

#include <uk/assert.h>

/* We expect the physical memory to be mapped 1:1 into the following area in
 * every virtual address space. This allows us to easily translate from virtual
 * to physical page table addresses and vice versa. We also access the metadata
 * for the frame allocators through this area.
 */
#define PGARCH_DIRECTMAP_AREA_START					\
	UK_PAL_DIRECTMAP_AREA_START

#define PGARCH_DIRECTMAP_AREA_END					\
	UK_PAL_DIRECTMAP_AREA_END

#define PGARCH_DIRECTMAP_AREA_SIZE					\
	(PGARCH_DIRECTMAP_AREA_END - PGARCH_DIRECTMAP_AREA_START + 1)

struct uk_pagetable;

static inline
__vaddr_t pgarch_directmap_paddr_to_vaddr(__paddr_t paddr)
{
	UK_ASSERT(paddr < PGARCH_DIRECTMAP_AREA_SIZE);
	return (__vaddr_t)paddr + PGARCH_DIRECTMAP_AREA_START;
}

static inline
__paddr_t pgarch_directmap_vaddr_to_paddr(__vaddr_t vaddr)
{
	UK_ASSERT(vaddr >= PGARCH_DIRECTMAP_AREA_START);
	UK_ASSERT(vaddr < PGARCH_DIRECTMAP_AREA_END);
	return (__paddr_t)(vaddr - PGARCH_DIRECTMAP_AREA_START);
}

/*
 * Page tables
 */
static inline
__vaddr_t pgarch_pt_pte_to_vaddr(struct uk_pagetable *pt __unused, __pte_t pte,
				 unsigned int level __maybe_unused)
{
	return pgarch_directmap_paddr_to_vaddr(UK_PAL_PT_Lx_PTE_PADDR(pte, level));
}

static inline
__vaddr_t pgarch_pt_map(struct uk_pagetable *pt __unused,
			__paddr_t pt_paddr, unsigned int level __unused)
{
	return pgarch_directmap_paddr_to_vaddr(pt_paddr);
}

static inline
__paddr_t pgarch_pt_unmap(struct uk_pagetable *pt __unused,
			  __vaddr_t pt_vaddr, unsigned int level __unused)
{
	return pgarch_directmap_vaddr_to_paddr(pt_vaddr);
}

/* Temporary kernel mapping */
static inline
__vaddr_t pgarch_kmap(struct uk_pagetable *pt __unused,
		      __paddr_t paddr, __sz len __unused)
{
	return pgarch_directmap_paddr_to_vaddr(paddr);
}

static inline
void pgarch_kunmap(struct uk_pagetable *pt __unused,
		   __vaddr_t vaddr __unused, __sz len __unused)
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
