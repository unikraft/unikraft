/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/arch/ctx.h>
#include <uk/arch/types.h>
#include <uk/event.h>
#include <uk/lcpu.h>
#include <uk/plat/syscall.h>

static int arm64_syscall_adapter(void *data)
{
	struct uk_lcpu_except_err_ctx *ctx;
	struct ukarch_execenv *execenv;

	ctx = (struct uk_lcpu_except_err_ctx *)data;
	UK_ASSERT(ctx);

	execenv = (struct ukarch_execenv *)uk_lcpu_except_err_ctx_get_regs(ctx);

	/* Save extended register state */
	uk_lcpu_ectx_sanitize((struct uk_lcpu_ectx *)&execenv->ectx);
	uk_lcpu_ectx_store((struct uk_lcpu_ectx *)&execenv->ectx);

	/* Save system context state */
	uk_lcpu_sysctx_store((struct uk_lcpu_sysctx *)&execenv->sysctx);

	/* The exception masked IRQs, but a system call may block and
	 * reschedule, which needs them: run it with IRQs on, as x86 does.
	 * The vectors moved us to the thread's auxiliary stack, so another
	 * thread's system call cannot land on this frame.
	 */
	uk_lcpu_enable_irq();
	ukplat_syscall_handler((struct uk_syscall_ctx *)execenv);
	uk_lcpu_disable_irq();

	/* Restore system context state */
	uk_lcpu_sysctx_load((struct uk_lcpu_sysctx *)&execenv->sysctx);

	/* Restore extended register state */
	uk_lcpu_ectx_load((struct uk_lcpu_ectx *)&execenv->ectx);

	return UK_EVENT_HANDLED;
}

UK_EVENT_HANDLER(UK_LCPU_EXCEPT_EVENT_SYSCALL, arm64_syscall_adapter);
