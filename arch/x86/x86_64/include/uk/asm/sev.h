/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UKARCH_SEV_H__
#define __UKARCH_SEV_H__
#include "stdbool.h"
#include <uk/essentials.h>

/* CPUID Fn800_001F[EBX] bits 5:0 indicate the location of the c-bit in the PTE */
#define X86_AMD64_CPUID_EBX_MEM_ENCRYPTION_MASK		((1UL << 6) - 1)

/* CPUID Fn800_001F[EAX] memory encryption-related bits */
#define X86_AMD64_CPUID_EAX_SEV_ENABLED			(1UL << 1)
#define X86_AMD64_CPUID_EAX_SEV_ES_ENABLED		(1UL << 3)
#define X86_AMD64_CPUID_EAX_SEV_SNP_ENABLED		(1UL << 4)


static inline void vmgexit(void){
	asm volatile("rep;vmmcall;");
}

#define PVALIDATE_OPCODE				".byte 0xF2, 0x0F, 0x01, 0xFF\n\t"
#define PVALIDATE_SUCCESS				0
#define PVALIDATE_FAIL_INPUT				1
#define PVALIDATE_FAIL_SIZEMISMATCH			6
#define PVALIDATE_PAGE_SIZE_4K				0
#define PVALIDATE_PAGE_SIZE_2M				1

static inline int pvalidate(__u64 vaddr, int page_size, int validated)
{
	int rc;
	int rmp_not_updated;

	asm volatile(PVALIDATE_OPCODE
		     : "=a"(rc), "=@ccc"(rmp_not_updated)
		     : "a"(vaddr), "c"(page_size), "d"(validated)
		     : "memory", "cc");

	if (rmp_not_updated) {
		return -1;
	}
	return rc;
}



#endif /* __UKARCH_SEV_H__ */
