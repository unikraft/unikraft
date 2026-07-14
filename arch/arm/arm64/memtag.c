/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2022, Michalis Pappas <mpappas@fastmail.fm>.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>

#include <uk/arch/memtag.h>
#include <uk/arch/lcpu.h>
#include <uk/arch/util.h>
#include <uk/assert.h>
#include <uk/random.h>

void *ukarch_memtag_region(void *ptr, __sz size)
{
	__u64 addr = (__u64)ptr;

	UK_ASSERT(!(size % MTE_TAG_GRANULE));

	addr = mte_insert_random_tag(addr);

	for (__sz i = 0; i < size; i += MTE_TAG_GRANULE)
		mte_store_alloc(addr + i);

	return (void *)addr;
}

int ukarch_memtag_init(void)
{
	unsigned int mte_version;
	__u64 reg;
	__u64 seed;
	int rc;

	mte_version = (UK_ARCH_ARM64_SYSREG_READ(ID_AA64PFR1_EL1) >>
			UK_ARCH_ARM64_ID_AA64PFR1_EL1_MTE_SHIFT) &
			UK_ARCH_ARM64_ID_AA64PFR1_EL1_MTE_MASK;
	if (mte_version < ARM64_FEAT_MTE2) {
		uk_pr_err("FEAT_MTE2 is not implemented\n");
		return -ENOTSUP;
	}

#if CONFIG_ARM64_FEAT_MTE_TCF_ASYMMETRIC
	if (mte_version < ARM64_FEAT_MTE3) {
		uk_pr_err("FEAT_MTE3 is not implemented\n");
		return -ENOTSUP;
	}
#endif

	/* Set seed for RGSR_EL1 */
	rc = uk_random_fill_buffer(&seed, sizeof(seed));
	if (unlikely(rc)) {
		uk_pr_err("Could not generate MTE key (%d)\n", rc);
		return rc;
	}

#if CONFIG_ARM64_FEAT_MTE_TCF_ASYNC
	UK_ARCH_ARM64_SYSREG_WRITE(SCTLR_EL1,
				   (UK_ARCH_ARM64_SYSREG_READ(SCTLR_EL1) |
				   (MTE_TCF_ASYNC <<
				    UK_ARCH_ARM64_SCTLR_EL1_TCF_SHIFT)));
#elif CONFIG_ARM64_FEAT_MTE_TCF_ASYMMETRIC
	UK_ARCH_ARM64_SYSREG_WRITE(SCTLR_EL1,
				   (UK_ARCH_ARM64_SYSREG_READ(SCTLR_EL1) |
				    (MTE_TCF_ASYMMETRIC <<
				     UK_ARCH_ARM64_SCTLR_EL1_TCF_SHIFT)));
#else
	UK_ARCH_ARM64_SYSREG_WRITE(SCTLR_EL1,
				   (UK_ARCH_ARM64_SYSREG_READ(SCTLR_EL1) |
				    (MTE_TCF_SYNC <<
				     UK_ARCH_ARM64_SCTLR_EL1_TCF_SHIFT)));
#endif
	/* Enable TBI */
	UK_ARCH_ARM64_SYSREG_WRITE(TCR_EL1,
				   (UK_ARCH_ARM64_SYSREG_READ(TCR_EL1) |
				    UK_ARCH_ARM64_TCR_EL1_TBI0_BIT));

	/* TCR_EL1.TCMA0 must be zero */
	UK_ARCH_ARM64_SYSREG_WRITE(TCR_EL1,
				   (UK_ARCH_ARM64_SYSREG_READ(TCR_EL1) &
				    ~UK_ARCH_ARM64_TCR_EL1_TCMA0_BIT));

	/* Use default random tag generation */
	UK_ARCH_ARM64_SYSREG_WRITE(GCR_EL1,
				   (UK_ARCH_ARM64_SYSREG_READ(GCR_EL1) &
				    UK_ARCH_ARM64_GCR_EL1_RRND_BIT));

	UK_ARCH_ARM64_SYSREG_WRITE(RGSR_EL1,
				   UK_ARCH_ARM64_SYSREG_READ(RGSR_EL1) |
				   ((seed  & UK_ARCH_ARM64_RGSR_EL1_SEED_MASK) <<
				    UK_ARCH_ARM64_RGSR_EL1_SEED_SHIFT));

	/* Exclude tag 0b1111 (ARM DDI 0487H.a Sect. D6.2) */
	reg = UK_ARCH_ARM64_SYSREG_READ(GCR_EL1);

	UK_ARCH_ARM64_SYSREG_WRITE(GCR_EL1,
				   (reg & ~UK_ARCH_ARM64_GCR_EL1_EXCLUDE_MASK));
	UK_ARCH_ARM64_SYSREG_WRITE(GCR_EL1, (1UL << 15) | 1);

	/* Enable MTE */
	UK_ARCH_ARM64_SYSREG_WRITE(SCTLR_EL1,
				   (UK_ARCH_ARM64_SYSREG_READ(SCTLR_EL1) |
				    UK_ARCH_ARM64_SCTLR_EL1_ATA_BIT));

	/* Clear Tag Check Ommit */
	PSTATE_TCO_CLEAR();

	return 0;
}
