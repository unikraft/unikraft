/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_XEN_SYSCTX_H__
#define __UK_PLAT_XEN_SYSCTX_H__

#include <uk/plat/native/sysctx.h>
#include <uk/plat/xen/arch/sysctx.h>

#define UK_PLAT_XEN_SYSCTX_LOAD_FNSYM	\
	uk_plat_xen_sysctx_load
#define UK_PLAT_XEN_SYSCTX_STORE_FNSYM	\
	uk_plat_xen_sysctx_store

#if !__ASSEMBLY__

void uk_plat_xen_sysctx_load(struct uk_plat_native_sysctx *sysctx);
void uk_plat_xen_sysctx_store(struct uk_plat_native_sysctx *sysctx);

__uptr uk_plat_xen_tlsp_get(void);
void uk_plat_xen_tlsp_set(__uptr tlsp);

#endif /* !__ASSEMBLY__ */

#endif /* __UK_PLAT_XEN_SYSCTX_H__ */
