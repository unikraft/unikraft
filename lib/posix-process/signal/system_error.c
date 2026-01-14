/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/arch/ctx.h>
#include <uk/essentials.h>
#include <uk/event.h>
#include <uk/plat/config.h>
#include <uk/lcpu.h>

#include "signal.h"

void sys_error_handler(struct ukarch_execenv *ee __unused, long arg);

/* Jumps away from exeption context to the actual handler.
 *
 * Notice: isr-safe calls here only
 */
static
int sys_error_handler_except(int signum,
			     struct uk_lcpu_except_err_ctx *trap_ctx)
{
	__uptr auxsp, handler_sp, curr_sp;
	struct sys_error_desc *handler_desc;
	struct ukarch_auxspcb *auxspcb;
	struct ukarch_ctx ctx;

	UK_ASSERT(trap_ctx);

	auxsp = uk_lcpu_get_auxsp_in_except();
	curr_sp = uk_lcpu_regs_get(uk_lcpu_except_err_ctx_get_regs(trap_ctx),
				   SP);

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
	handler_desc->vaddr = uk_lcpu_except_err_ctx_get_fault_addr(trap_ctx);

	/* Jump away from the exception context */
	ukarch_ctx_init_ehtrampo(&ctx,
				 (struct uk_lcpu_regs *)
				 uk_lcpu_except_err_ctx_get_regs(trap_ctx),
				 handler_sp,
				 sys_error_handler, (long)handler_desc);
	ukarch_ctx_jump(&ctx);
	UK_BUG(); /* noreturn */

	return 0;
}

static int pprocess_signal_pf_handler(void *arg)
{
	return sys_error_handler_except(SIGSEGV, arg);
}

static int pprocess_signal_invop_handler(void *arg)
{
	return sys_error_handler_except(SIGILL, arg);
}

static int pprocess_signal_dbg_handler(void *arg)
{
	return sys_error_handler_except(SIGTRAP, arg);
}

static int pprocess_signal_bus_handler(void *arg)
{
	return sys_error_handler_except(SIGBUS, arg);
}

static int pprocess_signal_math_handler(void *arg)
{
	return sys_error_handler_except(SIGFPE, arg);
}

/* Execute system fault handlers last, as:
 * 1. We want to give other kernel components the chance to handle the error
 *    first (page faults, fpu emulation, gdb stub).
 * 2. We cannot return from the isr-safe context if we can't handle the signal.
 */
UK_EVENT_HANDLER_PRIO(UK_LCPU_EXCEPT_EVENT_ERR_PAGE_FAULT,
		      pprocess_signal_pf_handler, UK_PRIO_LATEST);
UK_EVENT_HANDLER_PRIO(UK_LCPU_EXCEPT_EVENT_ERR_INVALID_OP,
		      pprocess_signal_invop_handler, UK_PRIO_LATEST);
UK_EVENT_HANDLER_PRIO(UK_LCPU_EXCEPT_EVENT_DEBUG,
		      pprocess_signal_dbg_handler, UK_PRIO_LATEST);
UK_EVENT_HANDLER_PRIO(UK_LCPU_EXCEPT_EVENT_ERR_BUS_ERROR,
		      pprocess_signal_bus_handler, UK_PRIO_LATEST);
UK_EVENT_HANDLER_PRIO(UK_LCPU_EXCEPT_EVENT_ERR_MATH,
		      pprocess_signal_math_handler, UK_PRIO_LATEST);
