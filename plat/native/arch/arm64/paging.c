/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2022, OpenSynergy GmbH. All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/arch/arm64.h>
#include <uk/arch/types.h>
#include <uk/arch/util.h>
#include <uk/assert.h>
#include <uk/plat/native/addr.h>

int uk_plat_native_paging_init(void)
{
	/* Sanity checks to make sure that the PE supports the minimum
	 * requirements and the platform has set up a sane environment.
	 * This is expected to be useful during bring-up, so keep it
	 * conditional to UKDEBUG, to improve boot performance.
	 */
	unsigned int ia_size;
	unsigned int oa_size;
	unsigned int tgran4;

	__u64 reg = UK_ARCH_ARM64_SYSREG_READ64(TCR_EL1);

	/* Check 48-bit addressing mode configured */
	if (unlikely(reg & UK_ARCH_ARM64_TCR_EL1_DS_BIT))
		UK_CRASH("48-bit addressing in not enabled\n");

	/* Check 48-bit IA configured. */
	ia_size = 64 - ((reg & UK_ARCH_ARM64_TCR_EL1_T0SZ_MASK) >>
			UK_ARCH_ARM64_TCR_EL1_T0SZ_SHIFT);

	if (unlikely(ia_size != __ARM64_PADDR_BITS))
		UK_CRASH("Invalid paddr width: %d bits\n", ia_size);

	/* Check 48-bit OA configured */
	oa_size = tcr_ips_bits[(reg >> UK_ARCH_ARM64_TCR_EL1_IPS_SHIFT) &
			       UK_ARCH_ARM64_TCR_EL1_IPS_MASK];

	if (unlikely(oa_size != __ARM64_VADDR_BITS))
		UK_CRASH("Invalid vaddr width: %d bits", oa_size);

	/* Check 4K granule size configured */
	if (unlikely((reg & (UK_ARCH_ARM64_TCR_EL1_TG0_MASK <<
			     UK_ARCH_ARM64_TCR_EL1_TG0_SHIFT)) !=
	    UK_ARCH_ARM64_TCR_EL1_TG0_4K))
		UK_CRASH("4KiB granule size is not enabled\n");

	/* Check TTBR0 walks are enabled */
	if (unlikely(reg & UK_ARCH_ARM64_TCR_EL1_EPD0_BIT))
		UK_CRASH("TTBR0_EL1 table walks are not enabled\n");

	/* Check TTBR1 walks are disabled */
	if (unlikely(!(reg & UK_ARCH_ARM64_TCR_EL1_EPD1_BIT)))
		UK_CRASH("TTBR1_EL1 table walks are not disabled\n");

	/* Check 4K granule size supported */
	reg = UK_ARCH_ARM64_SYSREG_READ64(ID_AA64MMFR0_EL1);

	tgran4 = (reg >> UK_ARCH_ARM64_ID_AA64MMFR0_EL1_TGRAN4_SHIFT) &
		  UK_ARCH_ARM64_ID_AA64MMFR0_EL1_TGRAN4_MASK;

	if (unlikely(tgran4 != UK_ARCH_ARM64_ID_AA64MMFR0_EL1_TGRAN4_SUPPORTED))
		UK_CRASH("4KiB granule not supported\n");

	return 0;
}
