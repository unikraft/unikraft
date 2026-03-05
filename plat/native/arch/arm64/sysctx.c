/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/arch/util.h>
#include <uk/arch/ctx.h>
#include <uk/arch/types.h>
#include <uk/assert.h>

void uk_plat_native_sysctx_store(struct uk_plat_native_sysctx *sysctx)
{
	UK_ASSERT(sysctx);

	sysctx->tpidr_el0 = UK_ARCH_ARM64_SYSREG_READ(TPIDR_EL0);
}

void uk_plat_native_sysctx_load(struct uk_plat_native_sysctx *sysctx)
{
	UK_ASSERT(sysctx);

	UK_ARCH_ARM64_SYSREG_WRITE(TPIDR_EL0, sysctx->tpidr_el0);
}
