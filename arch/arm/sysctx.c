/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#include <uk/arch/ctx.h>
#include <uk/assert.h>

__uptr ukarch_sysctx_get_tlsp(struct ukarch_sysctx *sysctx)
{
	UK_ASSERT(sysctx);

	return sysctx->tpidr_el0;
}

void ukarch_sysctx_set_tlsp(struct ukarch_sysctx *sysctx, __uptr tlsp)
{
	UK_ASSERT(sysctx);

	uk_pr_debug("System call updated userland TLS pointer register to %p "
		    "(before: %p)\n",
		    (void *)sysctx->tpidr_el0, (void *)tlsp);

	sysctx->tpidr_el0 = tlsp;
}

void ukarch_sysctx_store(struct ukarch_sysctx *sysctx)
{
	UK_ASSERT(sysctx);

	sysctx->tpidr_el0 = SYSREG_READ(TPIDR_EL0);
}

void ukarch_sysctx_load(struct ukarch_sysctx *sysctx)
{
	UK_ASSERT(sysctx);

	SYSREG_WRITE(TPIDR_EL0, sysctx->tpidr_el0);
}
