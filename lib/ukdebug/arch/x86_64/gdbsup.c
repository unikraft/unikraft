/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, Karlsruhe Institute of Technology. All rights reserved.
 * Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include "../../gdbstub.h"

#include <uk/arch/traps.h>
#include <uk/bitops.h>

#ifndef X86_EFLAGS_TF
#define X86_EFLAGS_TF UK_BIT(8)
#endif

/* We get here via traps raised by the platform
 * TODO: Once the crash screen PR is merged, crashes can land us here
 * too, if the "enter debugger on crash" feature is enabled.
 */
static int gdb_arch_dbg_trap(int errnr, struct __regs *regs)
{
	int r;

	/* Unset trap flag, i.e., continue */
	regs->eflags &= ~X86_EFLAGS_TF;

	r = gdb_dbg_trap(errnr, regs);
	if (r < 0) {
		return r;
	} else if (r == GDB_DBG_STEP) { /* Single step */
		regs->eflags |= X86_EFLAGS_TF;
	}

	return 0;
}

static int gdb_arch_debug_handler(void *data)
{
	int r;
	struct ukarch_trap_ctx *ctx = (struct ukarch_trap_ctx *)data;

	if ((r = gdb_arch_dbg_trap(5 /* SIGTRAP */, ctx->regs)) < 0)
		return r;
	else
		return UK_EVENT_HANDLED;
}

UK_EVENT_HANDLER(UKARCH_TRAP_DEBUG, gdb_arch_debug_handler);
