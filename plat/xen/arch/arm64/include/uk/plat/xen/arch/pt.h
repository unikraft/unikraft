/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_XEN_ARCH_PT_H__
#define __UK_PLAT_XEN_ARCH_PT_H__

#include <uk/config.h>
#include <uk/arch/types.h>
#include <uk/plat/native/pt.h>
#include <uk/plat/native/tlb.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UK_PLAT_XEN_PT_LEVELS	3

#if !__ASSEMBLY__

#if CONFIG_LIBXENPLAT_PAGING

#include <uk/arch/arm64.h>

static inline
__paddr_t uk_plat_xen_pt_read_base(void)
{
	__paddr_t reg;

	__asm__ __volatile__("mrs %x0, ttbr1_el1\n" : "=r" (reg));

	return (reg & UK_ARCH_ARM64_TTBR1_EL1_BADDR_MASK);
}

static inline
int uk_plat_xen_pt_write_base(__paddr_t pt_paddr)
{
	__paddr_t reg = (pt_paddr & UK_ARCH_ARM64_TTBR1_EL1_BADDR_MASK);

	__asm__ __volatile__("msr ttbr1_el1, %x0\n"
			     "isb\n"
			     :: "r" (reg) : "memory");

	uk_plat_native_tlb_flush_local();

	return 0;
}

#endif /* CONFIG_LIBXENPLAT_PAGING */

#endif /* !__ASSEMBLY__ */

#ifdef __cplusplus
}
#endif
#endif /* __UK_PLAT_XEN_ARCH_PT_H__ */
