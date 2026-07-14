/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/plat/xen/sysctx.h>

/* On ARM64 all sysctx ops are delegated to native */

__isr void uk_plat_xen_sysctx_load(struct uk_plat_native_sysctx *sysctx)
{
	uk_plat_native_sysctx_load(sysctx);
}

__isr void uk_plat_xen_sysctx_store(struct uk_plat_native_sysctx *sysctx)
{
	uk_plat_native_sysctx_store(sysctx);
}
