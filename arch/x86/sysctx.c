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

	return sysctx->fsbase;
}

void ukarch_sysctx_set_tlsp(struct ukarch_sysctx *sysctx, __uptr tlsp)
{
	UK_ASSERT(sysctx);

	uk_pr_debug("Sysctx %p TLS pointer register updated to %p (before: %p)\n",
		    sysctx, (void *)tlsp, (void *)sysctx->fsbase);

	sysctx->fsbase = tlsp;
}

__uptr ukarch_sysctx_get_gsbase(struct ukarch_sysctx *sysctx)
{
	UK_ASSERT(sysctx);

	return sysctx->gsbase;
}

void ukarch_sysctx_set_gsbase(struct ukarch_sysctx *sysctx, __uptr gsbase)
{
	UK_ASSERT(sysctx);

	uk_pr_debug("Sysctx %p GS_BASE register updated to %p (before: %p)\n",
		    sysctx, (void *)gsbase, (void *)sysctx->gsbase);

	sysctx->gsbase = gsbase;
}

void ukarch_sysctx_store(struct ukarch_sysctx *sysctx)
{
	UK_ASSERT(sysctx);

	sysctx->gsbase = rdgsbase();
	sysctx->fsbase = rdfsbase();
}

void ukarch_sysctx_load(struct ukarch_sysctx *sysctx)
{
	UK_ASSERT(sysctx);

	wrgsbase(sysctx->gsbase);
	wrfsbase(sysctx->fsbase);
}
