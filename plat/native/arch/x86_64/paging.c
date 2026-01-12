/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, Karlsruhe Institute of Technology (KIT).
 *                     All rights reserved.
 * Copyright (c) 2021, University Politehnica of Bucharest.
 *                     All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>

#include <uk/arch/types.h>
#include <uk/arch.h>
#include <uk/assert.h>
#include <uk/plat/native/addr.h>
#include <uk/plat/native/paging.h>
#include <uk/plat/native/page.h>

#include <x86/cpu.h>

int uk_plat_native_paging_init(void)
{
	__u32 eax, ebx, ecx, edx;
	__u32 max_addr_bit;
	__u64 efer;

	/* Check for availability of extended features */
	uk_arch_cpuid(0x80000000, 0, &eax, &ebx, &ecx, &edx);
	if (eax < 0x80000008)
		return -ENOTSUP;

	/* Check for 1GiB page support. We assume 64-bit and NX support */
	uk_arch_cpuid(0x80000001, 0, &eax, &ebx, &ecx, &edx);

	UK_ASSERT(edx & UK_ARCH_CPUID81_LM);
	UK_ASSERT(edx & UK_ARCH_CPUID81_NX);

	if (unlikely(!(edx & UK_ARCH_CPUID81_PAGE1GB))) {
		uk_pr_crit("%s not supported.\n", "1GiB pages");
		return -ENOTSUP;
	}

	/* Enable the NX bit */
	efer = uk_arch_rdmsrl(UK_ARCH_MSR_EFER);
	efer |= UK_ARCH_EFER_NXE;
	uk_arch_wrmsrl(UK_ARCH_MSR_EFER, efer);

#if UK_PLAT_NATIVE_PT_LEVELS == 5
	uk_arch_cpuid(0x7, 0, &eax, &ebx, &ecx, &edx);

	if (unlikely(!(ecx & UK_ARCH_CPUID7_ECX_LA57))) {
		uk_pr_crit("%s not supported.\n", "5-level paging");
		return -ENOTSUP;
	}
#endif /* UK_PLAT_NATIVE_PT_LEVELS == 5 */

	/* Check for PAT support */
	uk_arch_cpuid(0x1, 0, &eax, &ebx, &ecx, &edx);
	if (unlikely(!(edx & UK_ARCH_CPUID1_EDX_PAT))) {
		uk_pr_crit("Page table attributes are not supported.\n");
		return -ENOTSUP;
	}
	/* Reset PAT to default value */
	uk_arch_wrmsrl(UK_ARCH_MSR_PAT, __X86_64_PAT_DEFAULT);

	uk_arch_cpuid(0x80000008, 0, &eax, &ebx, &ecx, &edx);

	max_addr_bit = (eax & __X86_64_PG_VADDR_MASK) >> __X86_64_PG_VADDR_SHIFT;
	if (unlikely(max_addr_bit < __X86_64_VADDR_BITS)) {
		uk_pr_crit("%d-bit %s addresses not supported.\n",
			   __X86_64_VADDR_BITS, "virtual");
		return -ENOTSUP;
	}

	max_addr_bit = (eax & __X86_64_PG_PADDR_MASK) >> __X86_64_PG_PADDR_SHIFT;
	uk_plat_native_x86_64_pg_maxphysaddr = (1UL << max_addr_bit) - 1;

	return 0;
}
