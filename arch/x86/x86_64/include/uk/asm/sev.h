/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UKARCH_SEV_H__
#define __UKARCH_SEV_H__

/* CPUID Fn800_001F[EBX] bits 5:0 indicate the location of the c-bit in the PTE */
#define X86_AMD64_CPUID_EBX_MEM_ENCRYPTION_MASK		((1UL << 6) - 1)

/* CPUID Fn800_001F[EAX] memory encryption-related bits */
#define X86_AMD64_CPUID_EAX_SEV_ENABLED			(1UL << 1)
#define X86_AMD64_CPUID_EAX_SEV_ES_ENABLED		(1UL << 3)
#define X86_AMD64_CPUID_EAX_SEV_SNP_ENABLED		(1UL << 4)

#endif /* __UKARCH_SEV_H__ */
