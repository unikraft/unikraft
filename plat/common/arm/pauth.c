/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2022, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <arm/arm64/cpu.h>
#include <arm/arm64/pauth.h>

#include <errno.h>

#include <uk/arch/types.h>
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

	reg = SYSREG_READ(ID_AA64ISAR1_EL1);
	apa = (reg >> ID_AA64ISAR1_EL1_APA_SHIFT) & ID_AA64ISAR1_EL1_APA_MASK;
	api = (reg >> ID_AA64ISAR1_EL1_API_SHIFT) & ID_AA64ISAR1_EL1_API_MASK;
	if (apa || api)
		return __true;

	reg = SYSREG_READ(ID_AA64ISAR2_EL1);
	apa3 = (reg >> ID_AA64ISAR2_EL1_APA3_SHIFT) & ID_AA64ISAR2_EL1_APA3_MASK;
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
	SYSREG_WRITE64(APIAKeyHi_EL1, key_hi);

	rc = uk_random_fill_buffer(&key_lo, sizeof(key_lo));
	if (unlikely(rc)) {
		uk_pr_err("Could not generate APIAKey (%d)\n", rc);
		return rc;
	}
	SYSREG_WRITE64(APIAKeyLo_EL1, key_lo);

	/* Enable pointer authentication */
	reg = SYSREG_READ64(sctlr_el1);
	reg |= SCTLR_EL1_EnIA_BIT;
	SYSREG_WRITE64(sctlr_el1, reg);

	isb();

	return 0;
}
