/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */


#include <errno.h>
#include <uk/sev.h>
#include <uk/essentials.h>
#include <uk/assert.h>
#include <uk/print.h>
#include <uk/arch/lcpu.h>

int uk_sev_mem_encrypt_init(void)
{
	__u32 eax, ebx, ecx, edx;
	__u32 encryption_bit;

	ukarch_x86_cpuid(0x8000001f, 0, &eax, &ebx, &ecx, &edx);
	if (unlikely(!(eax & X86_AMD64_CPUID_EAX_SEV_ENABLED))) {
		uk_pr_crit("%s not supported.\n", "AMD SEV");
		return -ENOTSUP;
	}

	encryption_bit = ebx & X86_AMD64_CPUID_EBX_MEM_ENCRYPTION_MASK;

	if (unlikely(encryption_bit != CONFIG_LIBUKSEV_PTE_MEM_ENCRYPT_BIT)) {
		UK_CRASH("Invalid encryption bit configuration, please set "
			   "it to %d in the config.\n",
			   encryption_bit);
	}

	return 0;
};
