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
	GDB_REGS_X0,
	GDB_REGS_X1,
	GDB_REGS_X2,
	GDB_REGS_X3,
	GDB_REGS_X4,
	GDB_REGS_X5,
	GDB_REGS_X6,
	GDB_REGS_X7,
	GDB_REGS_X8,
	GDB_REGS_X9,
	GDB_REGS_X10,
	GDB_REGS_X11,
	GDB_REGS_X12,
	GDB_REGS_X13,
	GDB_REGS_X14,
	GDB_REGS_X15,
	GDB_REGS_X16,
	GDB_REGS_X17,
	GDB_REGS_X18,
	GDB_REGS_X19,
	GDB_REGS_X20,
	GDB_REGS_X21,
	GDB_REGS_X22,
	GDB_REGS_X23,
	GDB_REGS_X24,
	GDB_REGS_X25,
	GDB_REGS_X26,
	GDB_REGS_X27,
	GDB_REGS_X28,
	GDB_REGS_X29,
	GDB_REGS_FP = GDB_REGS_X29, /* Frame pointer */
	GDB_REGS_X30,
	GDB_REGS_LR = GDB_REGS_X30, /* Link register */

	GDB_REGS_PC,
	GDB_REGS_CPSR,

	GDB_REGS_NUM
};

__ssz gdb_arch_read_register(int regnr, struct __regs *regs,
			     void *buf, __sz buf_len);
__ssz gdb_arch_write_register(int regnr, struct __regs *regs,
			      void *buf, __sz buf_len);

#endif /* __UKDEBUG_ARCH_GDBSUP_H__ */
