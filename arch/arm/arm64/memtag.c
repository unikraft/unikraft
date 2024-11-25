/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2022, Michalis Pappas <mpappas@fastmail.fm>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <uk/arch/memtag.h>
#include <uk/arch/lcpu.h>
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
	__u64 reg;
	__u64 seed;
	unsigned int mte_version;

	mte_version = (SYSREG_READ(ID_AA64PFR1_EL1) >>
			ID_AA64PFR1_EL1_MTE_SHIFT) &
			ID_AA64PFR1_EL1_MTE_MASK;
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
	SYSREG_WRITE(SCTLR_EL1, (SYSREG_READ(SCTLR_EL1) |
		     (MTE_TCF_ASYNC << SCTLR_EL1_TCF_SHIFT)));
#elif CONFIG_ARM64_FEAT_MTE_TCF_ASYMMETRIC
	SYSREG_WRITE(SCTLR_EL1, (SYSREG_READ(SCTLR_EL1) |
		     (MTE_TCF_ASYMMETRIC << SCTLR_EL1_TCF_SHIFT)));
#else
	SYSREG_WRITE(SCTLR_EL1, (SYSREG_READ(SCTLR_EL1) |
		     (MTE_TCF_SYNC << SCTLR_EL1_TCF_SHIFT)));
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
