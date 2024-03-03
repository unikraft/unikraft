/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/arch/ctx.h>
#include <uk/arch/lcpu.h>
#include <uk/asm.h>

/**
 * Loads a given execution environment on the currently executing CPU.
 *
 * NOTE: This function cannot be returned from, it overwrites the entire current
 *       context.
 *
 * @X0 execenv
 *   Reference to execution environment to load
 */
ENTRY(ukarch_execenv_load)
	/* Mask IRQ to make sure restore would not be interrupted by IRQ */
	msr	daifset, #2

	/**
	 * Assign pointer to execution environment to load (first argument).
	 * We do this because it will be easy to keep track of it as, unlike
	 * x0, we do not have to store/restore x19 across function calls.
	 * As per AAPCS64, x19-x28 are callee-saved.
	 */
	mov	x19, x0

	/**
	 * Load execenv's stored ECTX which resides at offset:
	 * sizeof(struct __regs) + sizeof(struct ukarch_sysctx) from beginning
	 * of execenv.
	 */
	add	x0, x0, #(__REGS_SIZEOF + UKARCH_SYSCTX_SIZE)
	bl	ukarch_ectx_load
	/**
	 * As stated previously, after function calls, x19 preserved value of
	 * execenv pointer so restore that into %rdi.
	 */
	mov	x0, x19

	/**
	 * Load execenv's stored system context which resides at offset:
	 * sizeof(struct __regs) from beginning of execenv.
	 */
	add	x0, x0, #(__REGS_SIZEOF)
	bl	ukarch_sysctx_load

	/**
	 * Load execenv's stored general purpose registers which resides at
	 * the beginning.
	 */
	mov	sp, x19

	/* Restore pstate and exception status register */
	ldp	x22, x23, [sp, #16 * 16]
	msr	spsr_el1, x22
	msr	esr_el1, x23

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
