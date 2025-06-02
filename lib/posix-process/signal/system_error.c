/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/arch/ctx.h>
#include <uk/arch/traps.h>
#include <uk/essentials.h>
#include <uk/event.h>
#include <uk/plat/config.h>
#include <uk/plat/lcpu.h>

#include "signal.h"

void sys_error_handler(struct ukarch_execenv *ee __unused, long arg);

/* Jumps away from exeption context to the actual handler.
 *
 * Notice: isr-safe calls here only
 */
static
int sys_error_handler_except(int signum, struct ukarch_trap_ctx *trap_ctx)
{
	__uptr auxsp, handler_sp, curr_sp;
	struct sys_error_desc *handler_desc;
	struct ukarch_auxspcb *auxspcb;
	struct ukarch_ctx ctx;

	UK_ASSERT(trap_ctx);

	auxsp = ukplat_lcpu_get_auxsp_in_except();
	curr_sp = ukarch_regs_get_sp(trap_ctx->regs);

	/* If there is no auxsp, the fault happened during boot before
	 * an aux stack is set up. If, however, we are executing in auxsp
	 * then we know fore sure we are in uk context (not application).
	 */
	if (!auxsp || SP_IN_AUXSP(curr_sp, auxsp))
		return UK_EVENT_NOT_HANDLED;

	/* Prepare execution stack. Use the aux stack, as it's
	 * the stack handle_self() expects to be opreating on.
	 */
	auxspcb = ukarch_auxsp_get_cb(auxsp);
	handler_sp = ukarch_auxspcb_get_curr_fp(auxspcb);
	handler_sp = ALIGN_DOWN(handler_sp - sizeof(*handler_desc),
				UKARCH_EXECENV_END_ALIGN);

	handler_desc = (struct sys_error_desc *)handler_sp;
	handler_desc->signum = signum;
	handler_desc->auxsp = auxsp;
	handler_desc->vaddr = trap_ctx->fault_address;

	/* Jump away from the exception context */
	ukarch_ctx_init_ehtrampo(&ctx,
				 trap_ctx->regs,
				 handler_sp,
				 sys_error_handler, (long)handler_desc);
	ukarch_ctx_jump(&ctx);
	UK_BUG(); /* noreturn */

	return 0;
}

static int pprocess_signal_pf_handler(void *arg)
{
	return sys_error_handler_except(SIGSEGV, (struct ukarch_trap_ctx *)arg);
}

static int pprocess_signal_invop_handler(void *arg)
{
	return sys_error_handler_except(SIGILL, (struct ukarch_trap_ctx *)arg);
}

static int pprocess_signal_dbg_handler(void *arg)
{
	return sys_error_handler_except(SIGTRAP, (struct ukarch_trap_ctx *)arg);
}

static int pprocess_signal_bus_handler(void *arg)
{
	return sys_error_handler_except(SIGBUS, (struct ukarch_trap_ctx *)arg);
}

static int pprocess_signal_math_handler(void *arg)
{
	return sys_error_handler_except(SIGFPE, (struct ukarch_trap_ctx *)arg);
}

/* Execute system fault handlers last, as:
 * 1. We want to give other kernel components the chance to handle the error
 *    first (page faults, fpu emulation, gdb stub).
 * 2. We cannot return from the isr-safe context if we can't handle the signal.
 */
UK_EVENT_HANDLER_PRIO(UKARCH_TRAP_PAGE_FAULT, pprocess_signal_pf_handler,
		      UK_PRIO_LATEST);
UK_EVENT_HANDLER_PRIO(UKARCH_TRAP_INVALID_OP, pprocess_signal_invop_handler,
		      UK_PRIO_LATEST);
UK_EVENT_HANDLER_PRIO(UKARCH_TRAP_DEBUG, pprocess_signal_dbg_handler,
		      UK_PRIO_LATEST);
UK_EVENT_HANDLER_PRIO(UKARCH_TRAP_BUS_ERROR, pprocess_signal_bus_handler,
		      UK_PRIO_LATEST);
UK_EVENT_HANDLER_PRIO(UKARCH_TRAP_MATH, pprocess_signal_math_handler,
		      UK_PRIO_LATEST);
