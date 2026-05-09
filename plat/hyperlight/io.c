/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/plat/io.h>
#include <uk/config.h>

#ifdef CONFIG_PAGING
#include <uk/plat/paging.h>
#include <uk/assert.h>
#endif

__paddr_t ukplat_virt_to_phys(const volatile void *address)
{
#ifdef CONFIG_PAGING
	struct uk_pagetable *pt = ukplat_pt_get_active();
	__vaddr_t vaddr = (__vaddr_t) address;
	__pte_t pte;
	unsigned int level = PAGE_LEVEL;
	unsigned long offset;
	int rc __maybe_unused;

	rc = ukplat_pt_walk(pt, PAGE_ALIGN_DOWN(vaddr), &level, __NULL, &pte);
	UK_ASSERT(rc == 0);

	UK_ASSERT(PT_Lx_PTE_PRESENT(pte, level));
	UK_ASSERT(PAGE_Lx_IS(pte, level));

	offset = vaddr - PAGE_Lx_ALIGN_DOWN(vaddr, level);

	return PT_Lx_PTE_PADDR(pte, level) + offset;
#else /* CONFIG_PAGING */
	/* Hyperlight uses identity mapping */
	return (__paddr_t)address;
#endif /* CONFIG_PAGING */
}
