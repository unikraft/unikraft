/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2021, Michalis Pappas <mpappas@fastmail.fm>.
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
#include <arm/arm64/cpu.h>
#include <arm/arm64/pauth.h>
#include <errno.h>
#include <uk/arch/types.h>
#include <uk/assert.h>
#include <uk/essentials.h>

#ifdef CONFIG_HAVE_RANDOM
#include <uk/arch/random.h>
#endif /* CONFIG_HAVE_RANDOM */

static void ukplat_pauth_gen_key(__u64 *key_hi, __u64 *key_lo)
{
	int ret;

	ret = ukarch_random_init();
	if (unlikely(ret))
		UK_CRASH("Arch random not available (%d)\n", ret);

	ret = ukarch_random_u64(key_lo);
	if (unlikely(ret))
		UK_CRASH("Could not generate PAuth key\n");

	ret = ukarch_random_u64(key_hi);
	if (unlikely(ret))
		UK_CRASH("Could not generate PAuth key\n");
}

int __no_pauth ukplat_pauth_init(void)
{
	__u64 reg;
	__u64 apa, api;
	__u64 key_hi, key_lo;

	/* Check if pointer authentication is available.
	 *
	 * This checks whether either QARMA or an IMPLEMENTATION DEFINED
	 * algorithm is used. If the platform supports PAuth, one of
	 * the two must be present.
	 */
	reg = SYSREG_READ(ID_AA64ISAR1_EL1);
	apa = (reg >> ID_AA64ISAR1_EL1_APA_SHIFT) & ID_AA64ISAR1_EL1_APA_MASK;
	api = (reg >> ID_AA64ISAR1_EL1_API_SHIFT) & ID_AA64ISAR1_EL1_API_MASK;
	if (unlikely(!apa && !api))
		return -ENOTSUP;

	/* Program instruction Key A */
	ukplat_pauth_gen_key(&key_hi, &key_lo);
	SYSREG_WRITE64(APIAKeyHi_EL1, key_hi);
	SYSREG_WRITE64(APIAKeyLo_EL1, key_lo);

	/* Enable pointer authentication */
	reg = SYSREG_READ64(sctlr_el1);
	reg |= SCTLR_EL1_EnIA_BIT;
	SYSREG_WRITE64(sctlr_el1, reg);

	isb();

	return 0;
}
