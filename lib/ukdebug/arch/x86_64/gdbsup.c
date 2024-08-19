/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, Karlsruhe Institute of Technology. All rights reserved.
 * Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include "gdbsup.h"
#include "../../gdbstub.h"

#include <uk/arch/traps.h>
#include <uk/bitops.h>
#include <uk/isr/string.h>

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

/* This table maps struct __regs to the gdb register file */
static struct {
	unsigned int offset;
	unsigned int length;
} gdb_register_map[] = {
	{__REGS_OFFSETOF_RAX, 8},
	{__REGS_OFFSETOF_RBX, 8},
	{__REGS_OFFSETOF_RCX, 8},
	{__REGS_OFFSETOF_RDX, 8},
	{__REGS_OFFSETOF_RSI, 8},
	{__REGS_OFFSETOF_RDI, 8},
	{__REGS_OFFSETOF_RBP, 8},
	{__REGS_OFFSETOF_RSP, 8},
	{__REGS_OFFSETOF_R8, 8},
	{__REGS_OFFSETOF_R9, 8},
	{__REGS_OFFSETOF_R10, 8},
	{__REGS_OFFSETOF_R11, 8},
	{__REGS_OFFSETOF_R12, 8},
	{__REGS_OFFSETOF_R13, 8},
	{__REGS_OFFSETOF_R14, 8},
	{__REGS_OFFSETOF_R15, 8},

	{__REGS_OFFSETOF_RIP, 8},
	{__REGS_OFFSETOF_EFLAGS, 4},

	{__REGS_OFFSETOF_CS, 4},
	{__REGS_OFFSETOF_SS, 4}
};

#define GDB_REGISTER_MAP_COUNT ARRAY_SIZE(gdb_register_map)

__ssz gdb_arch_read_register(int regnr, struct __regs *regs,
			     void *buf, __sz buf_len __maybe_unused)
{
	if (unlikely(regnr < 0))
		return -EINVAL;

	if ((__sz)regnr < GDB_REGISTER_MAP_COUNT) {
		UK_ASSERT(buf_len >= gdb_register_map[regnr].length);

		memcpy_isr(buf,
			   (char *)regs + gdb_register_map[regnr].offset,
			   gdb_register_map[regnr].length);

		return gdb_register_map[regnr].length;
	}

	/* TODO: Implement getting the following registers */

	switch (regnr) {
	case GDB_REGS_DS:
	case GDB_REGS_ES:
	case GDB_REGS_FS:
	case GDB_REGS_GS:
		UK_ASSERT(buf_len >= 4);
		memset_isr(buf, 0, 4);
		return 4;

	case GDB_REGS_ST0:
	case GDB_REGS_ST1:
	case GDB_REGS_ST2:
	case GDB_REGS_ST3:
	case GDB_REGS_ST4:
	case GDB_REGS_ST5:
	case GDB_REGS_ST6:
	case GDB_REGS_ST7:
		UK_ASSERT(buf_len >= 10);
		memset_isr(buf, 0, 10);
		return 10;

	case GDB_REGS_FCTRL:
	case GDB_REGS_FSTAT:
	case GDB_REGS_FTAG:
	case GDB_REGS_FI_SEG:
	case GDB_REGS_FI_OFF:
	case GDB_REGS_FO_SEG:
	case GDB_REGS_FO_OFF:
	case GDB_REGS_FOP:
		UK_ASSERT(buf_len >= 4);
		memset_isr(buf, 0, 4);
		return 4;
	}

	return -EINVAL;
}

__ssz gdb_arch_write_register(int regnr, struct __regs *regs,
			      void *buf, __sz buf_len)
{
	if (unlikely(regnr < 0))
		return -EINVAL;

	if ((__sz)regnr < GDB_REGISTER_MAP_COUNT) {
		if (buf_len < gdb_register_map[regnr].length)
			return -EINVAL;

		memcpy_isr((char *)regs + gdb_register_map[regnr].offset,
			   buf, gdb_register_map[regnr].length);

		return gdb_register_map[regnr].length;
	}

	/* TODO: Implement setting the following registers */

	switch (regnr) {
	case GDB_REGS_DS:
	case GDB_REGS_ES:
	case GDB_REGS_FS:
	case GDB_REGS_GS:
		return 0;

	case GDB_REGS_ST0:
	case GDB_REGS_ST1:
	case GDB_REGS_ST2:
	case GDB_REGS_ST3:
	case GDB_REGS_ST4:
	case GDB_REGS_ST5:
	case GDB_REGS_ST6:
	case GDB_REGS_ST7:
		return 0;

	case GDB_REGS_FCTRL:
	case GDB_REGS_FSTAT:
	case GDB_REGS_FTAG:
	case GDB_REGS_FI_SEG:
	case GDB_REGS_FI_OFF:
	case GDB_REGS_FO_SEG:
	case GDB_REGS_FO_OFF:
	case GDB_REGS_FOP:
		return 0;
	}

	return -EINVAL;
}
