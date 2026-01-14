/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/arch.h>
#include <uk/arch/ctx.h>
#include <uk/essentials.h>

void execve_arch_execenv_init(struct ukarch_execenv *execenv_new,
			      struct ukarch_execenv *execenv,
			      __uptr ip, __uptr sp)
{
	UK_ASSERT(execenv_new);
	UK_ASSERT(execenv);
	UK_ASSERT(ip);
	UK_ASSERT(sp);
	UK_ASSERT(IS_ALIGNED(sp, UKARCH_SP_ALIGN));

	uk_lcpu_regs_set(execenv_new->regs, RIP, ip);
	uk_lcpu_regs_set(execenv_new->regs, RSP, sp);

	/* Prepare for iretq
	 * FIXME re-arch: use GDT macros once moved out of plat/common
	 */
	uk_lcpu_regs_set(execenv_new->regs, RFLAGS,
			 uk_lcpu_regs_get(execenv->regs, RFLAGS));

	uk_lcpu_regs_set(execenv_new->regs, CS,
			 UK_ARCH_GDT_DESC_OFFSET(UK_ARCH_GDT_DESC_CODE));
	uk_lcpu_regs_set(execenv_new->regs, SS,
			 UK_ARCH_GDT_DESC_OFFSET(UK_ARCH_GDT_DESC_DATA));

	/* Copy current ectx to inerhit platform-initialized regs like mxcsr */
	uk_lcpu_ectx_sanitize((struct uk_lcpu_ectx *)execenv_new->ectx);
	uk_lcpu_ectx_store((struct uk_lcpu_ectx *)execenv_new->ectx);

	/* Also copy the current sysregs to avoid ending up with undefined
	 * values that trigger alignment errors.
	 */
	uk_lcpu_sysctx_store((struct uk_lcpu_sysctx *)execenv_new->sysctx);
}
