/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2022, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <uk/arch/lcpu.h>
#include <uk/asm/memtag.h>
#include <uk/random.h>

#define ARM64_FEAT_MTE		1
#define ARM64_FEAT_MTE2		2
#define ARM64_FEAT_MTE3		3

#define PSTATE_TCO_SET() ({					\
	__asm__ __volatile__ ("msr	tco, %0\n"		\
			      : : "r"(PSTATE_TCO_BIT));		\
})

#define PSTATE_TCO_CLEAR() ({					\
	__asm__ __volatile__ ("msr	tco, %0\n"		\
			      : : "r"(0));			\
})

/**
 * Tags address with a random tag.
 *
 * If the address is tagged, this function makes sure that the
 * generated tag does not match the current one.
 *
 * @param  addr input address
 * @return tagged address
 */
static inline __u64 mte_insert_random_tag(__u64 addr)
{
	__u64 reg;

	__asm__ __volatile__("gmi	%0, %1, xzr\n"
			     "irg	%1, %1, %0\n"
			     : "=&r"(reg), "+r" (addr));
	return addr;
}

/**
 * Reads tag from allocation memory
 *
 * @param  addr allocation address
 * @return allocation tag
 */
static inline __u64 mte_load_alloc(__u64 addr)
{
	__u64 tag;

	__asm__ __volatile__ ("ldg	%0, [%1]\n"
			      : "=&r" (tag) : "r"(addr));
	return tag;
}

/**
 * Writes a tag into tag allocation memory.
 * The tag is derived from the input address.
 *
 * @param addr tagged address
 */
static inline void mte_store_alloc(__u64 addr)
{
	__asm__ __volatile__ ("stg	%0, [%0]\n"
			      : : "r"(addr) : "memory");
}

void *uk_arch_memtag_tag_region(void *ptr, __sz size)
{
	__u64 addr = (__u64)ptr;

	UK_ASSERT(!(size % MTE_TAG_GRANULE));

	addr = mte_insert_random_tag(addr);

	for (__sz i = 0; i < size; i += MTE_TAG_GRANULE)
		mte_store_alloc(addr + i);

	return (void *)addr;
}

int uk_arch_memtag_init(void)
{
	unsigned int mte_version;
	__u64 seed;
	__u64 reg;
	int rc;

	mte_version = (SYSREG_READ(ID_AA64PFR1_EL1) >>
			ID_AA64PFR1_EL1_MTE_SHIFT) &
			ID_AA64PFR1_EL1_MTE_MASK;
	if (mte_version < ARM64_FEAT_MTE2) {
		uk_pr_err("FEAT_MTE2 is not implemented\n");
		return -ENOTSUP;
	}

#if CONFIG_LIBUKMEMTAG_TCF_ASYMMETRIC
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

#if CONFIG_LIBUKMEMTAG_TCF_ASYNC
	SYSREG_WRITE(SCTLR_EL1, (SYSREG_READ(SCTLR_EL1) |
		     (SCTLR_EL1_TCF_ASYNC << SCTLR_EL1_TCF_SHIFT)));
#elif CONFIG_LIBUKMEMTAG_TCF_ASYMMETRIC
	SYSREG_WRITE(SCTLR_EL1, (SYSREG_READ(SCTLR_EL1) |
		     (SCTLR_EL1_TCF_ASYMMETRIC << SCTLR_EL1_TCF_SHIFT)));
#else
	SYSREG_WRITE(SCTLR_EL1, (SYSREG_READ(SCTLR_EL1) |
		     (SCTLR_EL1_TCF_SYNC << SCTLR_EL1_TCF_SHIFT)));
#endif
	/* Enable TBI */
	SYSREG_WRITE(TCR_EL1, (SYSREG_READ(TCR_EL1) | TCR_EL1_TBI0_BIT));

	/* TCR_EL1.TCMA0 must be zero */
	SYSREG_WRITE(TCR_EL1, (SYSREG_READ(TCR_EL1) & ~TCR_EL1_TCMA0_BIT));

	/* Use default random tag generation */
	SYSREG_WRITE(GCR_EL1, (SYSREG_READ(GCR_EL1) & GCR_EL1_RRND_BIT));

	SYSREG_WRITE(RGSR_EL1, SYSREG_READ(RGSR_EL1) |
		     ((seed  & RGSR_EL1_SEED_MASK) << RGSR_EL1_SEED_SHIFT));

	/* Exclude tag 0b1111 (ARM DDI 0487H.a Sect. D6.2) */
	reg = SYSREG_READ(GCR_EL1);

	SYSREG_WRITE(GCR_EL1, (reg & ~GCR_EL1_EXCLUDE_MASK));
	SYSREG_WRITE(GCR_EL1, (1UL << 15) | 1);

	/* Enable MTE */
	SYSREG_WRITE(SCTLR_EL1, (SYSREG_READ(SCTLR_EL1) | SCTLR_EL1_ATA_BIT));

	/* Clear Tag Check Ommit */
	PSTATE_TCO_CLEAR();

	return 0;
}
