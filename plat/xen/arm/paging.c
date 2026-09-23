/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>

#include <uk/arch/arm64.h>
#include <uk/arch/types.h>
#include <uk/arch/util.h>
#include <uk/config.h>
#include <uk/essentials.h>
#include <uk/plat/pal/page.h>
#include <uk/plat/pal/pt.h>
#include <uk/plat/xen/paging.h>
#include <uk/print.h>

#define XEN_VA_BITS						\
	(UK_PAL_PAGE_SHIFT + UK_PAL_PT_LEVEL_SHIFT * UK_PAL_PT_LEVELS)

/* TCR_EL1.TG1 encoding of a 4KiB granule (Arm ARM D19.2.139) */
#define XEN_TCR_EL1_TG1_4K	2

/* Make sure that the environment entry64.S has set up is the one the
 * page table primitives expect.
 */
int uk_plat_xen_paging_init(void)
{
	__u64 reg = UK_ARCH_ARM64_SYSREG_READ64(TCR_EL1);
	unsigned int ia_size;

	/* Check PT in TTBR1_EL1 */
	if (unlikely(reg & UK_ARCH_ARM64_TCR_EL1_EPD1_BIT)) {
		uk_pr_err("TTBR1_EL1 table walks are not enabled\n");
		return -ENOTSUP;
	}

	/* Check 39-bit address space */
	ia_size = 64 - ((reg >> UK_ARCH_ARM64_TCR_EL1_T1SZ_SHIFT) &
			UK_ARCH_ARM64_TCR_EL1_T0SZ_MASK);

	if (unlikely(ia_size != XEN_VA_BITS)) {
		uk_pr_err("Invalid vaddr width: %u bits\n", ia_size);
		return -ENOTSUP;
	}

	/* Check 4KiB granule */
	if (unlikely(((reg >> UK_ARCH_ARM64_TCR_EL1_TG1_SHIFT) &
		      UK_ARCH_ARM64_TCR_EL1_TG1_MASK) !=
		     XEN_TCR_EL1_TG1_4K)) {
		uk_pr_err("4KiB granule size is not enabled\n");
		return -ENOTSUP;
	}

	return 0;
}
