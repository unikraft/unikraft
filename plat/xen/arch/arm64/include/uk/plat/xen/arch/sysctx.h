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

/* On ARM64 all sysctx ops are delegated to native */

__isr static inline
__uptr uk_plat_xen_tlsp_get(void)
{
	return uk_plat_native_tlsp_get();
}

__isr static inline
void uk_plat_xen_tlsp_set(__uptr tlsp)
{
	uk_plat_native_tlsp_set(tlsp);
}

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */

#endif /* __UK_PLAT_XEN_ARCH_SYSCTX_H__ */
