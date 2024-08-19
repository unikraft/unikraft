/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, Karlsruhe Institute of Technology. All rights reserved.
 * Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UKDEBUG_ARCH_GDBSUP_H__
#define __UKDEBUG_ARCH_GDBSUP_H__

#include <uk/arch/lcpu.h>

/* The following list must match the description in target.xml */
enum gdb_arch_register_index {
	/* General registers */
	GDB_REGS_RAX,
	GDB_REGS_RBX,
	GDB_REGS_RCX,
	GDB_REGS_RDX,
	GDB_REGS_RSI,
	GDB_REGS_RDI,
	GDB_REGS_RBP,
	GDB_REGS_RSP,
	GDB_REGS_R8,
	GDB_REGS_R9,
	GDB_REGS_R10,
	GDB_REGS_R11,
	GDB_REGS_R12,
	GDB_REGS_R13,
	GDB_REGS_R14,
	GDB_REGS_R15,

	GDB_REGS_RIP,
	GDB_REGS_EFLAGS,

	/* Segment registers */
	GDB_REGS_CS,
	GDB_REGS_SS,
	GDB_REGS_DS,
	GDB_REGS_ES,
	GDB_REGS_FS,
	GDB_REGS_GS,

	/* x87 FPU */
	GDB_REGS_ST0,
	GDB_REGS_ST1,
	GDB_REGS_ST2,
	GDB_REGS_ST3,
	GDB_REGS_ST4,
	GDB_REGS_ST5,
	GDB_REGS_ST6,
	GDB_REGS_ST7,

	GDB_REGS_FCTRL,
	GDB_REGS_FSTAT,
	GDB_REGS_FTAG,
	GDB_REGS_FI_SEG,
	GDB_REGS_FI_OFF,
	GDB_REGS_FO_SEG,
	GDB_REGS_FO_OFF,
	GDB_REGS_FOP,

	GDB_REGS_NUM
};

__ssz gdb_arch_read_register(int regnr, struct __regs *regs,
			     void *buf, __sz buf_len);
__ssz gdb_arch_write_register(int regnr, struct __regs *regs,
			      void *buf, __sz buf_len);

#endif /* __UKDEBUG_ARCH_GDBSUP_H__ */
