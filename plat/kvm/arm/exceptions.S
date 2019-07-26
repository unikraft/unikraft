/* SPDX-License-Identifier: ISC */
/*-
 *
 * Copyright (c) 2014 Andrew Turner, All rights reserved.
 * Copyright (c) 2018 Arm Ltd., All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#include <uk/arch/lcpu.h>
#include <uk/asm.h>

.macro EXCHANGE_SP_WITH_X0
	add sp, sp, x0	// new_sp = sp + x0
	sub x0, sp, x0	// new_x0 = new_sp - x0 = sp + x0 - x0 = sp
	sub sp, sp, x0	// new_sp = new_sp - new_x0 = sp + x0 - sp = x0
.endm

.macro ALIGN_STACK
	// First, exchange the SP with x0
	EXCHANGE_SP_WITH_X0

	// Check whether the stack is alignment
	tst x0, #0xf
	// If yes, save and go out. If not, align the stack
	b.eq 0f

	// Start to align the stack.

	// We will use the x1 as temporary, save x1 to stack temporary
	str x1, [x0]

	// Align down sp to 16-byte, save old sp to aligned_sp[__SP_OFFSET]
	bic x1, x0, #0xf
	str x0, [x1, #__SP_OFFSET]

	// Restore x1 before x0 is overridden
	ldr x1, [x0]

	// Save aligned_sp to x0
	bic x0, x0, #0xf
	b 1f
0:
	str x0, [x0, #__SP_OFFSET]
1:
	// Change back the SP from x0
	EXCHANGE_SP_WITH_X0
.endm

.macro ENTER_TRAP, el
	sub sp, sp, #__TRAP_STACK_SIZE

.if	\el != 0
	/* Force align the stack, and save SP to __SP_OFFSET */
	ALIGN_STACK
.endif

	/* Save general purpose registers */
	stp x0, x1, [sp, #16 * 0]
	stp x2, x3, [sp, #16 * 1]
	stp x4, x5, [sp, #16 * 2]
	stp x6, x7, [sp, #16 * 3]
	stp x8, x9, [sp, #16 * 4]
	stp x10, x11, [sp, #16 * 5]
	stp x12, x13, [sp, #16 * 6]
	stp x14, x15, [sp, #16 * 7]
	stp x16, x17, [sp, #16 * 8]
	stp x18, x19, [sp, #16 * 9]
	stp x20, x21, [sp, #16 * 10]
	stp x22, x23, [sp, #16 * 11]
	stp x24, x25, [sp, #16 * 12]
	stp x26, x27, [sp, #16 * 13]
	stp x28, x29, [sp, #16 * 14]

	/* Save LR and exception PC */
	mrs x21, elr_el1
	stp x30, x21, [sp, #16 * 15]

	/* Save pstate and exception status register */
	mrs x22, spsr_el1
	mrs x23, esr_el1
	stp x22, x23, [sp, #16 * 16]

.if \el == 0
	/* Save stack pointer for lower level exception */
	mrs x18, sp_el0
	str x18, [sp, #__SP_EL0_OFFSET]
.endif

.endm

.macro LEAVE_TRAP, el
	/* Mask IRQ to make sure restore would not be interrupted by IRQ */
	msr daifset, #2
	/* Restore pstate and exception status register */
	ldp x22, x23, [sp, #16 * 16]
	msr spsr_el1, x22
	msr esr_el1, x23

	/* Restore LR and exception PC */
	ldp x30, x21, [sp, #16 * 15]
	msr elr_el1, x21

	/* Restore general purpose registers */
	ldp x28, x29, [sp, #16 * 14]
	ldp x26, x27, [sp, #16 * 13]
	ldp x24, x25, [sp, #16 * 12]
	ldp x22, x23, [sp, #16 * 11]
	ldp x20, x21, [sp, #16 * 10]
	ldp x16, x17, [sp, #16 * 8]
	ldp x14, x15, [sp, #16 * 7]
	ldp x12, x13, [sp, #16 * 6]
	ldp x10, x11, [sp, #16 * 5]
	ldp x8, x9, [sp, #16 * 4]
	ldp x6, x7, [sp, #16 * 3]
	ldp x4, x5, [sp, #16 * 2]
	ldp x2, x3, [sp, #16 * 1]
	ldp x0, x1, [sp, #16 * 0]

.if \el == 0
	/* Restore stack pointer for exception from EL0 */
	ldr x18, [sp, #__SP_EL0_OFFSET]
	msr sp_el0, x18
.else
	/* Restore stack pointer for exception from EL1 */
	ldr x18, [sp, #__SP_OFFSET]
	mov sp, x18
.endif
	add sp, sp, #__TRAP_STACK_SIZE

	/* Restore x18,x19 */
	ldp x18, x19, [sp, #16 * 9]

	eret
.endm

/*
 * Most aarch64 SoC is using 64-byte cache line. Align the
 * exception handlers to 64-byte will benefit the cache hit
 * rate of handlers.
 */
.align 6
el1_sync:
	ENTER_TRAP 1
	mov x0, sp
	mrs x1, far_el1
	bl trap_el1_sync
	LEAVE_TRAP 1

.align 6
el1_irq:
	ENTER_TRAP 1
	msr daifclr, #(8 | 4 | 1)
	mov x0, sp
	bl trap_el1_irq
	LEAVE_TRAP 1

/* Bad Abort numbers */
#define BAD_SYNC  0
#define BAD_IRQ   1
#define BAD_FIQ   2
#define BAD_ERROR 3

#define el_invalid(name, reason, el)	\
.align 6;				\
name##_invalid:				\
	ENTER_TRAP  el;			\
	mov x0, sp;			\
	mov x1, el;			\
	mov x2, #(reason);		\
	mrs x3, far_el1;		\
	b   invalid_trap_handler;	\
ENDPROC(name##_invalid);		\

el_invalid(el1_sync, BAD_SYNC, 1);
el_invalid(el0_sync, BAD_SYNC, 0);
el_invalid(el1_irq, BAD_IRQ, 1);
el_invalid(el0_irq, BAD_IRQ, 0);
el_invalid(el1_fiq, BAD_FIQ, 1);
el_invalid(el0_fiq, BAD_FIQ, 0);
el_invalid(el1_error, BAD_ERROR, 1);
el_invalid(el0_error, BAD_ERROR, 0);

/*
 * Macro for Exception vectors.
 */
.macro vector_entry label
.align  7
	b \label
.endm

/*
 * Exception vectors.
 *
 * AArch64 unikernel runs in EL1 mode using the SP_EL1 stack. The vectors
 * don't have a fixed address, only alignment (2^11) requirements.
 */
.align  11
ENTRY(vector_table)
	/* Current Exception level with SP_EL0 */
	vector_entry el1_sync_invalid	/* Synchronous EL1t       */
	vector_entry el1_irq_invalid	/* IRQ EL1t               */
	vector_entry el1_fiq_invalid	/* FIQ EL1t               */
	vector_entry el1_error_invalid	/* Error EL1t             */

	/* Current Exception level with SP_EL1 */
	vector_entry el1_sync		/* Synchronous EL1h       */
	vector_entry el1_irq		/* IRQ EL1h               */
	vector_entry el1_fiq_invalid	/* FIQ EL1h               */
	vector_entry el1_error_invalid	/* Error EL1h             */

	/* Lower Exception level using AArch64 */
	vector_entry el0_sync_invalid	/* Synchronous 64-bit EL0 */
	vector_entry el0_irq_invalid	/* IRQ 64-bit EL0         */
	vector_entry el0_fiq_invalid	/* FIQ 64-bit EL0         */
	vector_entry el0_error_invalid	/* Error 64-bit EL0       */

	/* Lower Exception level using AArch32 */
	vector_entry el0_sync_invalid	/* Synchronous 32-bit EL0 */
	vector_entry el0_irq_invalid	/* IRQ 32-bit EL0         */
	vector_entry el0_fiq_invalid	/* FIQ 32-bit EL0         */
	vector_entry el0_error_invalid	/* Error 32-bit EL0       */
END(vector_table)
