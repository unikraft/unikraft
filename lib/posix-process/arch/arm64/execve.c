/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/arch/ctx.h>
#include <uk/essentials.h>
#include <uk/lcpu.h>

void execve_arch_execenv_init(struct ukarch_execenv *execenv_new,
			      struct ukarch_execenv *execenv,
			      __uptr ip, __uptr sp)
{
	UK_ASSERT(execenv_new);
	UK_ASSERT(execenv);
	UK_ASSERT(ip);
	UK_ASSERT(sp);
	UK_ASSERT(IS_ALIGNED(sp, UKARCH_SP_ALIGN));

	uk_lcpu_regs_set(execenv_new->regs, LR, ip);
	uk_lcpu_regs_set(execenv_new->regs, SP, sp);

	/* Copy SPSR to preserve the application's state at
	 * syscall time.
	 */
	uk_lcpu_regs_set(execenv_new->regs, SPSR_EL1,
			 uk_lcpu_regs_get(execenv->regs, SPSR_EL1));

	/* Copy ESR to make sure we restore a sane value */
	uk_lcpu_regs_set(execenv_new->regs, ESR_EL1,
			 uk_lcpu_regs_get(execenv->regs, ESR_EL1));

	/* Leave gpregs and ectx uninitialized for the new
	 * execution context.
	 */

	/* Also copy the current sysctx to avoid ending up with undefined
	 * values that trigger alignment errors.
	 */
	uk_lcpu_sysctx_store((struct uk_lcpu_sysctx *)execenv_new->sysctx);
}
