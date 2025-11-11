/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, Karlsruhe Institute of Technology (KIT).
 *                     All rights reserved.
 * Copyright (c) 2021, University Politehnica of Bucharest.
 *                     All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>

#include <uk/arch/types.h>
#include <uk/asm/arch.h>
#include <uk/assert.h>
#include <uk/plat/native/arch/paging.h>
#include <uk/plat/native/addr.h>
#include <uk/plat/native/page.h>
#include <uk/plat/native/pt.h>

#include <x86/cpu.h>

__pte_t uk_plat_native_pte_create(__paddr_t paddr, unsigned long attr,
				  unsigned int level, __pte_t tmpl,
				  unsigned int tmpl_level __unused)
{
	__pte_t pte;

	UK_ASSERT(UK_PLAT_NATIVE_PAGE_ALIGNED(paddr));

	pte = paddr & __X86_64_PTE_PADDR_MASK;
	pte |= X86_PTE_PRESENT;

	if (level > UK_PLAT_NATIVE_PAGE_LEVEL) {
		UK_ASSERT(level <= UK_PLAT_NATIVE_PAGE_HUGE_LEVEL);
		pte |= X86_PTE_PSE;
	}

	if (attr & UK_PLAT_NATIVE_PAGE_ATTR_PROT_WRITE)
		pte |= X86_PTE_RW;

	if (!(attr & UK_PLAT_NATIVE_PAGE_ATTR_PROT_EXEC))
		pte |= X86_PTE_NX;

	if (attr & X86_PAGE_ATTR_WRITECOMBINE) {
		pte |= X86_PTE_PCD;
		pte |= X86_PTE_PWT;
	}

	/* Take all other bits from template */
	pte |= tmpl & (X86_PTE_US |
			   X86_PTE_ACCESSED |
			   X86_PTE_DIRTY |
			   X86_PTE_GLOBAL |
			   X86_PTE_USER1_MASK |
			   X86_PTE_USER2_MASK |
			   X86_PTE_MPK_MASK);

	return pte;
}

__pte_t uk_plat_native_pte_change_attr(__pte_t pte,
				       unsigned long new_attr,
				       unsigned int level __unused)
{
	pte &= ~(X86_PTE_RW | X86_PTE_NX | X86_PTE_PCD | X86_PTE_PWT);

	if (new_attr & UK_PLAT_NATIVE_PAGE_ATTR_PROT_WRITE)
		pte |= X86_PTE_RW;

	if (!(new_attr & UK_PLAT_NATIVE_PAGE_ATTR_PROT_EXEC))
		pte |= X86_PTE_NX;

	if (new_attr & X86_PAGE_ATTR_WRITECOMBINE) {
		pte |= X86_PTE_PCD;
		pte |= X86_PTE_PWT;
	}

	return pte;
}

unsigned long uk_plat_native_attr_from_pte(__pte_t pte,
					   unsigned int level __unused)
{
	unsigned long attr = UK_PLAT_NATIVE_PAGE_ATTR_PROT_READ;

	if (pte & X86_PTE_RW)
		attr |= UK_PLAT_NATIVE_PAGE_ATTR_PROT_WRITE;

	if (!(pte & X86_PTE_NX))
		attr |= UK_PLAT_NATIVE_PAGE_ATTR_PROT_EXEC;

	if ((pte & X86_PTE_PWT) && (pte & X86_PTE_PCD))
		attr |= X86_PAGE_ATTR_WRITECOMBINE;

	return attr;
}
