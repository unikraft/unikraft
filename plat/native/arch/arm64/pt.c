/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2022, OpenSynergy GmbH. All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/arch/arm64.h>
#include <uk/arch/types.h>
#include <uk/asm/lcpu.h>
#include <uk/assert.h>
#include <uk/plat/native/addr.h>
#include <uk/plat/native/page.h>
#include <uk/plat/native/pt.h>

__pte_t uk_plat_native_pte_create(__paddr_t paddr, unsigned long attr,
				  unsigned int level, __pte_t tmpl,
				  unsigned int tmpl_level __unused)
{
	__pte_t pte;

	static unsigned long pte_lx_map_paddr_mask[] = {
		UK_ARCH_ARM64_PTE_L0_PAGE_PADDR_MASK,
		UK_ARCH_ARM64_PTE_L1_BLOCK_PADDR_MASK,
		UK_ARCH_ARM64_PTE_L2_BLOCK_PADDR_MASK,
	};

	UK_ASSERT(UK_PLAT_NATIVE_PAGE_ALIGNED(paddr));
	UK_ASSERT(UK_PLAT_NATIVE_PAGE_Lx_HAS(level));

	pte = paddr & pte_lx_map_paddr_mask[level];

	if (!tmpl)
		pte |= UK_ARCH_ARM64_PTE_ATTR_AF;
	else
		pte |= tmpl & (UK_ARCH_ARM64_PTE_ATTR_CONTIGUOUS |
			       UK_ARCH_ARM64_PTE_ATTR_DBM |
			       UK_ARCH_ARM64_PTE_ATTR_nG |
			       UK_ARCH_ARM64_PTE_ATTR_SH_MASK |
			       UK_ARCH_ARM64_PTE_ATTR_AF |
			       UK_ARCH_ARM64_PTE_ATTR_IDX_MASK);

	if (level == UK_PLAT_NATIVE_PAGE_LEVEL)
		pte |= UK_ARCH_ARM64_PTE_TYPE_PAGE;
	else
		pte |= UK_ARCH_ARM64_PTE_TYPE_BLOCK;

	if (!(attr & UK_PLAT_NATIVE_PAGE_ATTR_PROT_WRITE))
		pte |= UK_ARCH_ARM64_PTE_ATTR_AP(UK_ARCH_ARM64_PTE_ATTR_AP_RO);

	if (!(attr & UK_PLAT_NATIVE_PAGE_ATTR_PROT_EXEC))
		pte |= UK_ARCH_ARM64_PTE_ATTR_XN;

	switch (attr & UK_ARCH_ARM64_PTE_ATTR_SH_MASK) {
	case UK_ARCH_ARM64_PTE_ATTR_SH(UK_ARCH_ARM64_PTE_ATTR_SH_IS):
		pte |= UK_ARCH_ARM64_PTE_ATTR_SH(UK_ARCH_ARM64_PTE_ATTR_SH_IS);
		break;
	case UK_ARCH_ARM64_PTE_ATTR_SH(UK_ARCH_ARM64_PTE_ATTR_SH_OS):
		pte |= UK_ARCH_ARM64_PTE_ATTR_SH(UK_ARCH_ARM64_PTE_ATTR_SH_OS);
		break;
	case UK_ARCH_ARM64_PTE_ATTR_SH(UK_ARCH_ARM64_PTE_ATTR_SH_NS):
		pte |= UK_ARCH_ARM64_PTE_ATTR_SH(UK_ARCH_ARM64_PTE_ATTR_SH_NS);
		break;
	default:
		UK_ASSERT(0 && "Invalid shareability type\n");
	}

	switch (attr & (UK_PLAT_NATIVE_PAGE_ATTR_TYPE_MASK <<
			UK_PLAT_NATIVE_PAGE_ATTR_TYPE_SHIFT)) {
	case UK_PLAT_NATIVE_PAGE_ATTR_TYPE_NORMAL_WB:
		pte |= UK_ARCH_ARM64_PTE_ATTR_IDX(NORMAL_WB);
		break;
	case UK_PLAT_NATIVE_PAGE_ATTR_TYPE_NORMAL_WT:
		pte |= UK_ARCH_ARM64_PTE_ATTR_IDX(NORMAL_WT);
		break;
	case UK_PLAT_NATIVE_PAGE_ATTR_TYPE_NORMAL_NC:
		pte |= UK_ARCH_ARM64_PTE_ATTR_IDX(NORMAL_NC);
		break;
	case UK_PLAT_NATIVE_PAGE_ATTR_TYPE_DEVICE_nGnRnE:
		pte |= UK_ARCH_ARM64_PTE_ATTR_IDX(DEVICE_nGnRnE);
		break;
	case UK_PLAT_NATIVE_PAGE_ATTR_TYPE_DEVICE_nGnRE:
		pte |= UK_ARCH_ARM64_PTE_ATTR_IDX(DEVICE_nGnRE);
		break;
	case UK_PLAT_NATIVE_PAGE_ATTR_TYPE_DEVICE_GRE:
		pte |= UK_ARCH_ARM64_PTE_ATTR_IDX(DEVICE_GRE);
		break;
	case UK_PLAT_NATIVE_PAGE_ATTR_TYPE_NORMAL_WB_TAGGED:
		pte |= UK_ARCH_ARM64_PTE_ATTR_IDX(NORMAL_WB_TAGGED);
		break;
	default:
		UK_ASSERT(0 && "Invalid memory type\n");
	}

	return pte;
}

__pte_t uk_plat_native_pte_change_attr(__pte_t pte __unused,
				       unsigned long new_attr __unused,
				       unsigned int level __unused)
{
	UK_CRASH("%s: Not implemented", __func__);

	return 0;
}

unsigned long uk_plat_native_attr_from_pte(__pte_t pte,
					   unsigned int level __unused)
{
	unsigned long attr = UK_PLAT_NATIVE_PAGE_ATTR_PROT_READ;

	if (!(pte & UK_ARCH_ARM64_PTE_ATTR_AP(UK_ARCH_ARM64_PTE_ATTR_AP_RO)))
		attr |= UK_PLAT_NATIVE_PAGE_ATTR_PROT_WRITE;

	if (!(pte & UK_ARCH_ARM64_PTE_ATTR_PXN))
		attr |= UK_PLAT_NATIVE_PAGE_ATTR_PROT_EXEC;

	switch (pte & UK_ARCH_ARM64_PTE_ATTR_SH_MASK) {
	case (UK_ARCH_ARM64_PTE_ATTR_SH(UK_ARCH_ARM64_PTE_ATTR_SH_NS)):
		attr |= UK_ARCH_ARM64_PTE_ATTR_SH(UK_ARCH_ARM64_PTE_ATTR_SH_NS);
		break;
	case (UK_ARCH_ARM64_PTE_ATTR_SH(UK_ARCH_ARM64_PTE_ATTR_SH_IS)):
		attr |= UK_ARCH_ARM64_PTE_ATTR_SH(UK_ARCH_ARM64_PTE_ATTR_SH_IS);
		break;
	case (UK_ARCH_ARM64_PTE_ATTR_SH(UK_ARCH_ARM64_PTE_ATTR_SH_OS)):
		attr |= UK_ARCH_ARM64_PTE_ATTR_SH(UK_ARCH_ARM64_PTE_ATTR_SH_OS);
		break;
	default:
		UK_ASSERT(0 && "Invalid shareability type\n");
	};

	switch (pte & UK_ARCH_ARM64_PTE_ATTR_IDX_MASK) {
	case UK_ARCH_ARM64_PTE_ATTR_IDX(NORMAL_WB):
		attr |= UK_PLAT_NATIVE_PAGE_ATTR_TYPE_NORMAL_WB;
		break;
	case UK_ARCH_ARM64_PTE_ATTR_IDX(NORMAL_WT):
		attr |= UK_PLAT_NATIVE_PAGE_ATTR_TYPE_NORMAL_WT;
		break;
	case UK_ARCH_ARM64_PTE_ATTR_IDX(NORMAL_NC):
		attr |= UK_PLAT_NATIVE_PAGE_ATTR_TYPE_NORMAL_NC;
		break;
	case UK_ARCH_ARM64_PTE_ATTR_IDX(DEVICE_nGnRnE):
		attr |= UK_PLAT_NATIVE_PAGE_ATTR_TYPE_DEVICE_nGnRnE;
		break;
	case UK_ARCH_ARM64_PTE_ATTR_IDX(DEVICE_nGnRE):
		attr |= UK_PLAT_NATIVE_PAGE_ATTR_TYPE_DEVICE_nGnRE;
		break;
	case UK_ARCH_ARM64_PTE_ATTR_IDX(DEVICE_GRE):
		attr |= UK_PLAT_NATIVE_PAGE_ATTR_TYPE_DEVICE_GRE;
		break;
	case UK_ARCH_ARM64_PTE_ATTR_IDX(NORMAL_WB_TAGGED):
		attr |= UK_PLAT_NATIVE_PAGE_ATTR_TYPE_NORMAL_WB_TAGGED;
		break;
	default:
		UK_CRASH("Invalid memory type\n");
	}

	return attr;
}
