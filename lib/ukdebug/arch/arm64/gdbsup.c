/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, Karlsruhe Institute of Technology. All rights reserved.
 * Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include "../../gdbstub.h"

#include <uk/arch/traps.h>
#include <uk/arch/lcpu.h>
#include <uk/essentials.h>
#include <uk/nofault.h>
#include <uk/bitops.h>
#include <uk/compiler.h>

#define BRK_OPCODE_MASK		0xffe0001fUL
#define BRK_OPCODE		0xd4200000UL

static void gdb_arch_enable_single_step(struct __regs *regs)
{
	__sz mdscr = SYSREG_READ(mdscr_el1);

	mdscr |= MDSCR_EL1_SS;
	mdscr |= MDSCR_EL1_KDE;

	SYSREG_WRITE(mdscr_el1, mdscr);

	regs->spsr_el1 |= SPSR_EL1_SS;
}

static void gdb_arch_disable_single_step(struct __regs *regs)
{
	__sz mdscr = SYSREG_READ(mdscr_el1);

	mdscr &= ~MDSCR_EL1_SS;
	mdscr &= ~MDSCR_EL1_KDE;

	SYSREG_WRITE(mdscr_el1, mdscr);

	regs->spsr_el1 &= ~SPSR_EL1_SS;
}

/* We get here via traps raised by the platform
 * TODO: Once the crash screen PR is merged, crashes can land us here
 * too, if the "enter debugger on crash" feature is enabled.
 *
 * TODO: Arm needs extra configuration for hardware breakpoints. At present,
 * we only support software breakpoints so there is no need to configure
 * hardware breakpoints yet. See D2.7 and D2.8.4 of DDI0487K.
 */
static int gdb_arch_dbg_trap(int errnr, struct __regs *regs)
{
	int r;

	gdb_arch_disable_single_step(regs);

	r = gdb_dbg_trap(errnr, regs);
	if (r < 0) {
		return r;
	} else if (r == GDB_DBG_STEP) { /* Single step */
		gdb_arch_enable_single_step(regs);

		regs->spsr_el1 &= ~SPSR_EL1_D;
	}

	return 0;
}

static int gdb_arch_check_brk(unsigned long pc)
{
	__u32 opcode;

	if (uk_nofault_memcpy((void *)&opcode, (void *)pc, sizeof(opcode),
			      UK_NOFAULTF_NOPAGING) != sizeof(opcode))
		return -EFAULT;

	return ((opcode & BRK_OPCODE_MASK) != BRK_OPCODE);
}

static int gdb_arch_debug_handler(void *data)
{
	int have_brk, r;
	struct ukarch_trap_ctx *ctx = (struct ukarch_trap_ctx *)data;

	r = gdb_arch_dbg_trap(5 /* SIGTRAP */, ctx->regs);
	if (unlikely(r < 0))
		return r;

	/* If we return from an brk trap, we have to explicitly skip the
	 * corresponding brk instruction. Otherwise, we will not make
	 * any progress and break again. However, software breakpoints
	 * temporarily overwrite instructions with brk so that we are also
	 * returning from an brk trap in this case but must not skip any
	 * instructions as GDB will have restored the original instruction by
	 * now. If the current PC is still brk, then it is compiled in and
	 * must be skipped. Otherwise, just continue at the current PC.
	 */
	have_brk = gdb_arch_check_brk(ctx->regs->elr_el1);
	if (unlikely(have_brk < 0))
		return have_brk;

	if ((ESR_EC_FROM(ctx->esr) == ESR_EL1_EC_BRK64) && (have_brk == 0)) {
		ctx->regs->elr_el1 += 4; /* instructions are all 4 bytes wide */
		ctx->regs->spsr_el1 &= ~SPSR_EL1_SS;
	}

	return UK_EVENT_HANDLED;
}

UK_EVENT_HANDLER(UKARCH_TRAP_DEBUG, gdb_arch_debug_handler);
