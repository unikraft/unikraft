/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#include <uk/assert.h>
#include <uk/arch/ctx.h>

__uptr ukarch_sysctx_get_tlsp(struct ukarch_sysctx *sysctx)
{
	UK_ASSERT(sysctx);

	return sysctx->fs_base;
}

void ukarch_sysctx_set_tlsp(struct ukarch_sysctx *sysctx, __uptr tlsp)
{
	UK_ASSERT(sysctx);

	uk_pr_debug("System call updated userland TLS pointer register to %p (before: %p)\n",
		    (void *)sysctx->fs_base, (void *)tlsp);

	sysctx->fs_base = tlsp;
}

__uptr ukarch_sysctx_get_gs_base(struct ukarch_sysctx *sysctx)
{
	UK_ASSERT(sysctx);

	return sysctx->gs_base;
}

void ukarch_sysctx_set_gs_base(struct ukarch_sysctx *sysctx, __uptr gs_base)
{
	UK_ASSERT(sysctx);

	sysctx->gs_base = gs_base;
}

void ukarch_sysctx_store(struct ukarch_sysctx *sysctx)
{
	UK_ASSERT(sysctx);

	sysctx->gs_base = rdgsbase();
	sysctx->fs_base = rdfsbase();
}

void ukarch_sysctx_load(struct ukarch_sysctx *sysctx)
{
	UK_ASSERT(sysctx);

	wrgsbase(sysctx->gs_base);
	wrfsbase(sysctx->fs_base);
}
