/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_NATIVE_TLB_H__
#define __UK_PLAT_NATIVE_TLB_H__

#include <uk/arch/types.h>
#include <uk/plat/native/arch/tlb.h>

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
void uk_plat_native_tlb_flush_entry(__vaddr_t vaddr);

/**
 * Flushes the entire TLB
 */
void uk_plat_native_tlb_flush(void);

#endif /* CONFIG_LIBUKPLAT_NATIVE_TLB */

#endif /* !__ASSEMBLY__ */

#ifdef __cplusplus
}
#endif
#endif /* __UK_PLAT_NATIVE_TLB_H__ */
