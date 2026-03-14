/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_XEN_AUXSP_H__
#define __UK_PLAT_XEN_AUXSP_H__

#include <uk/arch/types.h>
#include <uk/pcpuvar.h>

#define UK_PLAT_XEN_AUXSP_SYM	uk_plat_xen_auxsp

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

extern __uk_pcpuvar __uptr uk_plat_xen_auxsp;

static inline void uk_plat_xen_set_auxsp(__uptr auxsp)
{
	uk_pcpuvar_current_set(uk_plat_xen_auxsp, auxsp);
}

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */

#endif /* __UK_PLAT_XEN_AUXSP_H__ */
