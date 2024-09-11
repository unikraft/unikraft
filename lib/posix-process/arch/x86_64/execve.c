/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

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

	execenv_new->regs.rip = ip;
	execenv_new->regs.rsp = sp;

	/* Prepare for iretq
	 * FIXME re-arch: use GDT macros once moved out of plat/common
	 */
	execenv_new->regs.eflags = execenv->regs.eflags;
	execenv_new->regs.cs = 8; /* GDT_DESC_OFFSET(GDT_DESC_CODE) */
	execenv_new->regs.ss = 16; /* GDT_DESC_OFFSET(GDT_DESC_DATA) */

	/* Copy current ectx to inerhit platform-initialized regs like mxcsr */
	ukarch_ectx_sanitize((struct ukarch_ectx *)&execenv_new->ectx);
	ukarch_ectx_store((struct ukarch_ectx *)&execenv_new->ectx);

	/* Also copy the current sysregs to avoid ending up with undefined
	 * values that trigger alignment errors.
	 */
	ukarch_sysctx_store((struct ukarch_sysctx *)&execenv_new->sysctx);
}
