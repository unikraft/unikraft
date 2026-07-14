/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_PAL_TLB_H__
#define __UK_PLAT_PAL_TLB_H__

#include <uk/arch/types.h>
#include <uk/plat/xen/tlb.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !__ASSEMBLY__

static inline
void uk_pal_tlb_flush_entry(__vaddr_t vaddr)
{
	uk_plat_xen_tlb_flush_entry(vaddr);
}

static inline
void uk_pal_tlb_flush(void)
{
	uk_plat_xen_tlb_flush();
}

#endif /* !__ASSEMBLY__ */

#ifdef __cplusplus
}
#endif

#endif /* __UK_PLAT_PAL_TLB_H__ */
