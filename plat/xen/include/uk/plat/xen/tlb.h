/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_XEN_TLB_H__
#define __UK_PLAT_XEN_TLB_H__

/* Xen platform does not support paging; declare no-ops */
#include <uk/arch/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !__ASSEMBLY__

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

#endif /* !__ASSEMBLY__ */

#ifdef __cplusplus
}
#endif

#endif /* __UK_PLAT_XEN_TLB_H__ */
