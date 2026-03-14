/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2022, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <arm/arm64/cpu.h>
#include <arm/arm64/pauth.h>

#include <errno.h>

#include <uk/arch/arm64.h>
#include <uk/arch/types.h>
#include <uk/arch/util.h>
#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/random.h>

/* Check if pointer authentication is available.
 *
 * This checks whether either QARMA or an IMPLEMENTATION DEFINED
 * algorithm is used.
 */
static inline __bool pauth_supported(void)
{
	__u64 apa3, apa, api;
	__u64 reg;

	reg = UK_ARCH_ARM64_SYSREG_READ(ID_AA64ISAR1_EL1);
	apa = (reg >> UK_ARCH_ARM64_ID_AA64ISAR1_EL1_APA_SHIFT) &
	      UK_ARCH_ARM64_ID_AA64ISAR1_EL1_APA_MASK;
	api = (reg >> UK_ARCH_ARM64_ID_AA64ISAR1_EL1_API_SHIFT) &
	      UK_ARCH_ARM64_ID_AA64ISAR1_EL1_API_MASK;

	if (apa || api)
		return __true;

	reg = UK_ARCH_ARM64_SYSREG_READ(ID_AA64ISAR2_EL1);
	apa3 = (reg >> UK_ARCH_ARM64_ID_AA64ISAR2_EL1_APA3_SHIFT) &
	       UK_ARCH_ARM64_ID_AA64ISAR2_EL1_APA3_MASK;

	if (apa3)
		return __true;

	return __false;
}

int __no_pauth ukplat_pauth_init(void)
{
	__u64 key_hi, key_lo;
	__u64 reg;
	int rc;

	if (unlikely(!pauth_supported())) {
		uk_pr_err("The CPU does not implement FEAT_PAUTH\n");
		return -ENOTSUP;
	}

	/* Program instruction Key A */
	rc = uk_random_fill_buffer(&key_hi, sizeof(key_hi));
	if (unlikely(rc)) {
		uk_pr_err("Could not generate APIAKey (%d)\n", rc);
		return rc;
	}
	UK_ARCH_ARM64_SYSREG_WRITE64(APIAKeyHi_EL1, key_hi);

	rc = uk_random_fill_buffer(&key_lo, sizeof(key_lo));
	if (unlikely(rc)) {
		uk_pr_err("Could not generate APIAKey (%d)\n", rc);
		return rc;
	}
	UK_ARCH_ARM64_SYSREG_WRITE64(APIAKeyLo_EL1, key_lo);

	/* Enable pointer authentication */
	reg = UK_ARCH_ARM64_SYSREG_READ64(sctlr_el1);
	reg |= UK_ARCH_ARM64_SCTLR_EL1_EnIA_BIT;
	UK_ARCH_ARM64_SYSREG_WRITE64(sctlr_el1, reg);

	uk_arch_arm64_isb();

	return 0;
}
