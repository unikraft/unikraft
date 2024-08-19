/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, Karlsruhe Institute of Technology. All rights reserved.
 * Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UKDEBUG_GDBSTUB_H__
#define __UKDEBUG_GDBSTUB_H__

#include <uk/config.h>
#include <uk/arch/lcpu.h>

#define GDB_DBG_CONT    0x0001	/* Continue execution */
#define GDB_DBG_STEP    0x0002	/* Single-step one instruction */

int gdb_dbg_trap(int errnr, struct __regs *regs);

#endif /* __UKDEBUG_GDBSTUB_H__ */
