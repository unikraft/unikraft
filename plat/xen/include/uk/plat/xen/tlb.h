/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_XEN_TLB_H__
#define __UK_PLAT_XEN_TLB_H__

#include <uk/config.h>
#include <uk/arch/types.h>
#include <uk/essentials.h>

#if CONFIG_LIBUKPLAT_NATIVE_TLB
#include <uk/plat/native/tlb.h>
#endif /* CONFIG_LIBUKPLAT_NATIVE_TLB */

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
void uk_plat_xen_tlb_flush_entry(__vaddr_t vaddr)
{
	uk_plat_native_tlb_flush_entry(vaddr);
}

/**
 * Flushes the entire TLB
 */
static inline
void uk_plat_xen_tlb_flush(void)
{
	uk_plat_native_tlb_flush();
}

#else /* !CONFIG_LIBUKPLAT_NATIVE_TLB */

/**
 * Flushes a single entry from the TLB
 *
 * @param vaddr the virtual address of the entry to flush
 */
static inline
void uk_plat_xen_tlb_flush_entry(__vaddr_t vaddr __unused)
{
	/* No-op */
}

/**
 * Flushes the entire TLB
 */
static inline
void uk_plat_xen_tlb_flush(void)
{
	/* No-op */
}

#endif /* !CONFIG_LIBUKPLAT_NATIVE_TLB */

#endif /* !__ASSEMBLY__ */

#ifdef __cplusplus
}
#endif

#endif /* __UK_PLAT_XEN_TLB_H__ */
