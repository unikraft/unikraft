/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/arch/lcpu.h>
#include <uk/asm.h>
#include <uk/syscall.h>

ENTRY(uk_syscall_ctx_popall)
	/* Mask IRQ to make sure restore would not be interrupted by IRQ */
	msr	daifset, #2

	mov	x0, sp
	add	x0, x0, #(__REGS_SIZEOF + UKARCH_SYSREGS_SIZE)
	bl	ukarch_ectx_store

	mov	x0, sp
	add	x0, x0, #(__REGS_SIZEOF)
	bl	ukarch_sysregs_switch_ul

	/* Restore pstate and exception status register */
	ldp x22, x23, [sp, #16 * 16]
	msr spsr_el1, x22
	msr esr_el1, x23

	/* Restore LR and exception PC */
	ldp	x30, x21, [sp, #16 * 15]
	msr	elr_el1, x21

	/* Restore general purpose registers */
	ldp	x28, x29, [sp, #16 * 14]
	ldp	x26, x27, [sp, #16 * 13]
	ldp	x24, x25, [sp, #16 * 12]
	ldp	x22, x23, [sp, #16 * 11]
	ldp	x20, x21, [sp, #16 * 10]
	/* Skip x18, x19 */
	ldp	x16, x17, [sp, #16 * 8]
	ldp	x14, x15, [sp, #16 * 7]
	ldp	x12, x13, [sp, #16 * 6]
	ldp	x10, x11, [sp, #16 * 5]
	ldp	x8, x9, [sp, #16 * 4]
	ldp	x6, x7, [sp, #16 * 3]
	ldp	x4, x5, [sp, #16 * 2]
	ldp	x2, x3, [sp, #16 * 1]
	ldp	x0, x1, [sp, #16 * 0]

	/* Restore stack pointer */
	ldr	x18, [sp, #__SP_OFFSET]
	mov	x19, sp
	mov	sp, x18

	/* Restore x18, x19 */
	ldp	x18, x19, [x19, #16 * 9]

	eret
