/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/plat/xen/sysctx.h>
#include <uk/plat/native/sysctx.h>

#include <xen-x86/hypercall.h>

__isr void uk_plat_xen_sysctx_store(struct uk_plat_native_sysctx *sysctx)
{
	uk_plat_native_sysctx_store(sysctx);
}

__isr void uk_plat_xen_sysctx_load(struct uk_plat_native_sysctx *sysctx)
{
	HYPERVISOR_set_segment_base(SEGBASE_GS_USER, sysctx->gsbase);
	HYPERVISOR_set_segment_base(SEGBASE_FS, sysctx->fsbase);
}

__isr void uk_plat_xen_tlsp_set(__uptr tlsp)
{
	HYPERVISOR_set_segment_base(SEGBASE_FS, tlsp);
}
