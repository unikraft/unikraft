/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <string.h>
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

	/* The new program starts with its registers zeroed but for those set
	 * below, as on Linux: the execenv sits on a stack fresh from the
	 * allocator, whose contents are stale.  x0 must be 0 (no exit handler
	 * for libc to register), and a stale FPCR would carry trap enables
	 * and rounding modes into the program.
	 */
	memset(execenv_new->regs, 0, sizeof(execenv_new->regs));
	memset(execenv_new->ectx, 0, sizeof(execenv_new->ectx));

	/* The new program starts where the eret returns to: ELR_EL1 */
	uk_lcpu_regs_set(execenv_new->regs, PC, ip);
	uk_lcpu_regs_set(execenv_new->regs, SP, sp);

	/* Copy SPSR to preserve the application's state at
	 * syscall time.
	 */
	uk_lcpu_regs_set(execenv_new->regs, SPSR_EL1,
			 uk_lcpu_regs_get(execenv->regs, SPSR_EL1));

	/* Copy ESR to make sure we restore a sane value */
	uk_lcpu_regs_set(execenv_new->regs, ESR_EL1,
			 uk_lcpu_regs_get(execenv->regs, ESR_EL1));

	/* Also copy the current sysctx to avoid ending up with undefined
	 * values that trigger alignment errors.
	 */
	uk_lcpu_sysctx_store((struct uk_lcpu_sysctx *)execenv_new->sysctx);
}
