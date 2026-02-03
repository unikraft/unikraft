/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_XEN_ARCH_SYSCTX_H__
#define __UK_PLAT_XEN_ARCH_SYSCTX_H__

#include <uk/plat/native/sysctx.h>

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

/* On x86 reading the segment registers is done as on native */

__isr static inline
void uk_plat_xen_sysctx_store(struct uk_plat_native_sysctx *sysctx)
{
	uk_plat_native_sysctx_store(sysctx);
}

__isr static inline
__uptr uk_plat_xen_tlsp_get(void)
{
	return uk_plat_native_tlsp_get();
}

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */

#endif /* __UK_PLAT_XEN_ARCH_SYSCTX_H__ */
