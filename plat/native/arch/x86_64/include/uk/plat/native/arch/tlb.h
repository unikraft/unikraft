/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, Karlsruhe Institute of Technology (KIT).
 *                     All rights reserved.
 * Copyright (c) 2021, University Politehnica of Bucharest.
 *                     All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_NATIVE_ARCH_TLB_H__
#define __UK_PLAT_NATIVE_ARCH_TLB_H__

#include <uk/arch/types.h>
#include <uk/plat/native/pt.h>

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_LIBUKPLAT_NATIVE_TLB

#if !__ASSEMBLY__

#if CONFIG_LIBUKPLAT_NATIVE_PAGING

/**
 * Flushes a single entry from the TLB
 *
 * @param vaddr the virtual address of the entry to flush
 */
static inline
void uk_plat_native_tlb_flush_entry(__vaddr_t vaddr)
{
	__asm__ __volatile__("invlpg (%0)" :: "r"(vaddr) : "memory");
}

/**
 * Flushes the entire TLB
 */
static inline
void uk_plat_native_tlb_flush(void)
{
	/* Overwriting CR3 will flush the TLB for all non-global PTEs */
	uk_plat_native_pt_write_base(uk_plat_native_pt_read_base());
}

#endif /* CONFIG_LIBUKPLAT_NATIVE_PAGING */

#endif /* !__ASSEMBLY__ */

#endif /* CONFIG_LIBUKPLAT_NATIVE_TLB*/

#ifdef __cplusplus
}
#endif
#endif /* __UK_PLAT_NATIVE_ARCH_TLB_H__ */
