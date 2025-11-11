/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, Karlsruhe Institute of Technology (KIT).
 *                     All rights reserved.
 * Copyright (c) 2022, OpenSynergy GmbH. All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_NATIVE_ARCH_TLB_H__
#define __UK_PLAT_NATIVE_ARCH_TLB_H__

#include <uk/arch/types.h>
#include <uk/plat/native/page.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !__ASSEMBLY__

#if CONFIG_LIBUKPLAT_NATIVE_TLB

/**
 * Flushes a single entry from the TLB
 *
 * @param vaddr the virtual address of the entry to flush
 */
static inline
void uk_plat_native_tlb_flush_entry(__vaddr_t vaddr)
{
	__u64 page_number = vaddr >> UK_PLAT_NATIVE_PAGE_SHIFT;

	__asm__ __volatile__("	dsb	ishst\n"        /* wait for write complete */
			     "	tlbi	vaae1is, %x0\n" /* invalidate by vaddr */
			     "	dsb	ish\n"          /* wait for invalidate compl */
			     "	isb\n"                  /* sync context */
			     :: "r" (page_number) : "memory");
}

/**
 * Flushes the entire TLB
 */
static inline
void uk_plat_native_tlb_flush(void)
{
	__asm__ __volatile__("	dsb	ishst\n"     /* wait for write complete */
			     "	tlbi	vmalle1is\n" /* invalidate all */
			     "	dsb	ish\n"       /* wait for invalidate complete */
			     "	isb\n"               /* sync context */
			     ::: "memory");
}

#endif /* CONFIG_LIBUKPLAT_NATIVE_TLB */

#endif /* !__ASSEMBLY__ */

#ifdef __cplusplus
}
#endif
#endif /* __UK_PLAT_NATIVE_ARCH_TLB_H__ */
