/* SPDX-License-Identifier: ISC */
/* Copyright (c) 2014 Andrew Turner, All rights reserved.
 * Copyright (c) 2018 Arm Ltd., All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the ISC License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/arch/ctx.h>
#include <uk/asm.h>
#include <uk/asm/lcpu.h>
#include <uk/plat/config.h>
#include <uk/plat/native/except.h>
#include <uk/pcpuvar.h>
#include <uk/reloc.h>

#define TYPE_CRIT				0
#define TYPE_TRAP				1
#define TYPE_IRQ				2

.extern uk_plat_native_except_switch_stack
.extern uk_plat_native_except_stack_base
.extern _uk_pcpuvar_base

.macro EXCHANGE_SP_WITH_X0
	add sp, sp, x0	// new_sp = sp + x0
	sub x0, sp, x0	// new_x0 = new_sp - x0 = sp + x0 - x0 = sp
	sub sp, sp, x0	// new_sp = new_sp - new_x0 = sp + x0 - sp = x0
.endm

.macro ALIGN_STACK
	/* First, exchange the SP with x0 */
	EXCHANGE_SP_WITH_X0

	/* Check whether the stack is aligned */
	tst x0, #0xf
	/* If yes, save and go out. If not, align the stack */
	b.eq 0f

	/* Start to align the stack. */

	/* We will use the x1 as temporary, save x1 to stack temporary */
	str x1, [x0]

	/* Align down sp to 16-byte, save old sp to
	 * aligned sp[UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_SP]
	 */
	bic x1, x0, #0xf
	str x0, [x1, #UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_SP]

	/* Restore x1 before x0 is overridden */
	ldr x1, [x0]

	/* Save aligned_sp to x0 */
	bic x0, x0, #0xf
	b 1f
0:
	str x0, [x0, #UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_SP]
1:
	/* Change back the SP from x0 */
	EXCHANGE_SP_WITH_X0
.endm

.macro SAVE_REGS, except_type
	/* Use TPIDRRO_EL0 and SP_EL0 to save x0 and x1, as we can't
	 * assume that there is space for temporary register storage
	 * in the application's stack.
	 * TPIDRRO_EL0 contains the application's TLS pointer, which is
	 * set by the kernel, and thus we can restore on exception
	 * return.
	 * SP_EL0 is only accessible in EL1, so we are sure that the
	 * application does not rely on it in any way.
	 */
	msr     tpidrro_el0, x0
	msr     sp_el0, x1

	uk_pcpuvar_arm64_ldr	x0, UK_PLAT_NATIVE_EXCEPT_SWITCH_STACK_SYM, x1
	cbz			x0, 0f
	EXCHANGE_SP_WITH_X0
	b	1f
0:
	/* Fetch `lcpu_except_stack` of LCPU:
	 * CPU_EXCEPT_STACK_SIZE  CPU_EXCEPT_STACK_SIZE  CPU_EXCEPT_STACK_SIZE
	 *<---------------------><---------------------><--------------------->
	 *|===================================================================|
	 *|                     |                      |                      |
	 *|	crit stack      |       trap stack     |        IRQ stack     |
	 *|                     |                      |                      |
	 *|===================================================================|
	 * ^
	 * EXCEPT_STACK_BASE_SYM
	 */
	uk_pcpuvar_arm64_ldr	x0, UK_PLAT_NATIVE_EXCEPT_STACK_BASE_SYM, x1

	/* Make it so that SP contains SP_EL0 and x0 contains old SP */
	EXCHANGE_SP_WITH_X0

.if \except_type == TYPE_CRIT
	/* If we are a crit, use the crit stack. */
	add     sp, sp, #CPU_EXCEPT_STACK_SIZE
.endif
.if \except_type == TYPE_TRAP
	/* If we are a trap, use the trap stack. */
	add     sp, sp, #CPU_EXCEPT_STACK_SIZE * 2
.endif
.if \except_type == TYPE_IRQ
	/* If we are an IRQ, use the IRQ stack. */
	add     sp, sp, #CPU_EXCEPT_STACK_SIZE * 3
.endif

1:
	/* Store old SP previously saved into x0 */
	str     x0, [sp, #-16]

.if \except_type == TYPE_TRAP
	/* Is this trap a system call? */
	mrs	x0, esr_el1
	and	x0, x0, #UK_ARCH_ARM64_ESR_EC_MASK
	orr	x0, xzr, x0, lsr #UK_ARCH_ARM64_ESR_EC_SHIFT
	cmp	x0, #UK_ARCH_ARM64_ESR_EL1_EC_SVC64
	bne	2f

	/* If we are in a system call trap then switch to auxiliary stack
	 * from the current lcpu pointer
	 */
	uk_pcpuvar_arm64_ldr	x0, UK_PLAT_NATIVE_AUXSP_SYM, x1
	sub			x0, x0, #UKARCH_AUXSPCB_SIZE
	ldr			x0, [x0, #UKARCH_AUXSPCB_OFFSETOF_CURR_FP]

	/**
	 * Current stack pointer now points to the current frame pointer
	 * within the auxiliary stack. Subtract UKARCH_EXECENV_END_ALIGN
	 * to make room for the 8-byte auxsp pointer, since the layout is
	 * struct uk_syscall_ctx {
	 *	struct ukarch_execenv execenv;
	 *	__uptr auxsp;  <-- make room for these
	 *     ..... space left unused because of alignment subtraction ....
	 * };
	 * We cannot just simply push or subtract 8 bytes because we break
	 * the alignment required by EXECENV, so we must subtract more.
	 * This leads to some wasted bytes but it's fine because they are not
	 * permanent and it is mandatory that we maintain alignment. This
	 * is an optimization so that we do not have to fetch `auxsp` in the
	 * syscall C entry as well (which usually involves reading some
	 * system register). The final stack layout for entering the syscall
	 * C handler should look like the following:
	 *
	 *               lcpu->auxsp (AUXSP aligned)
	 *                  +-------------+ ^
	 *                  |             | | EXECENV_END_ALIGN
	 *                ^ |<----------->| |             ^
	 *         8 bytes| |pushed auxsp | |             |
	 *                v |-------------| v             |
	 *                ^ |  struct     | ^             |
	 *                | |   regs      | | REGS_SIZE   |
	 *                | |-------------| v             |
	 *                | |  struct     | ^             |uk_syscall_ctx
	 *      struct    | |  sysctx     | |SYSCTX_SIZE  |
	 *  ukarch_execenv| |-------------| v             |
	 *                | |             | ^             |
	 *                | |  struct     | |             |
	 *                | | ukarch_ectx | |ECTX_SIZE    |
	 *                | |             | |             |
	 *                | |             | |             |
	 *                | |             | |             |
	 *                v +-------------+ v             v
	 *                       stack
	 *                       pointer
	 *
	 * Where ukarch_sysctx/ukarch_ectx are to be filled in by the next C
	 * caller and __regs we fill in below.
	 */
	sub	x0, x0, #UKARCH_EXECENV_END_ALIGN

	/* NOTE: We should normally align the stack before doing this
	 * subtraction because we must ensure that the `ectx` field
	 * is aligned to the corresponding ECTX alignment.
	 * However, this is guaranteed to already be the case for the
	 * auxiliary stack because it is allocated with this exact alignment
	 * in mind.
	 */
	sub	x0, x0, #UKARCH_EXECENV_SIZE

	/* We now have in SP the trap stack and in x0 the auxiliary stack */
	EXCHANGE_SP_WITH_X0  /* Switch them */

	/* Restore old SP we stored before system call check */
	ldr	x0, [x0, #-16]
	str	x0, [sp, #UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_SP]  /* Store old SP in auxiliary stack */
	uk_pcpuvar_arm64_ldr	x0, UK_PLAT_NATIVE_AUXSP_SYM, x1
	str	x0, [sp, #UKARCH_EXECENV_SIZE]
	b	3f
.endif
2:
	sub	sp, sp, #UK_PLAT_NATIVE_REGS_SIZE
3:
	/* Restore x0, x1 */
	mrs     x0, tpidrro_el0
	mrs     x1, sp_el0

	/* Save general purpose registers */
	stp	x0, x1, [sp, #16 * 0]
	stp	x2, x3, [sp, #16 * 1]
	stp	x4, x5, [sp, #16 * 2]
	stp	x6, x7, [sp, #16 * 3]
	stp	x8, x9, [sp, #16 * 4]
	stp	x10, x11, [sp, #16 * 5]
	stp	x12, x13, [sp, #16 * 6]
	stp	x14, x15, [sp, #16 * 7]
	stp	x16, x17, [sp, #16 * 8]
	stp	x18, x19, [sp, #16 * 9]
	stp	x20, x21, [sp, #16 * 10]
	stp	x22, x23, [sp, #16 * 11]
	stp	x24, x25, [sp, #16 * 12]
	stp	x26, x27, [sp, #16 * 13]
	stp	x28, x29, [sp, #16 * 14]

	/* Save LR and exception PC */
	mrs	x21, elr_el1
	stp	x30, x21, [sp, #16 * 15]

	/* Save pstate and exception status register */
	mrs	x22, spsr_el1
	mrs	x23, esr_el1
	stp	x22, x23, [sp, #16 * 16]
.endm

.macro RESTORE_REGS
	/* Mask IRQ to make sure restore would not be interrupted by IRQ */
	msr	daifset, #2

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

	/* Restore stack pointer for exception from EL1 */
	ldr	x18, [sp, #UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_SP]
	mov	x19, sp
	mov	sp, x18

	/* Restore x18, x19 */
	ldp	x18, x19, [x19, #16 * 9]

	eret
.endm

/*
 * Most aarch64 SoC is using 64-byte cache line. Align the
 * exception handlers to 64-byte will benefit the cache hit
 * rate of handlers.
 */
.align 6
el1_sync:
	SAVE_REGS TYPE_TRAP
	mov x0, sp
	mrs x1, far_el1
	bl uk_plat_native_except_err_handler
	RESTORE_REGS

.align 6
el1_irq:
	SAVE_REGS TYPE_IRQ
	msr daifclr, #4 /* Unmask SError */
	mov x0, sp
	bl uk_plat_native_except_irq_handler
	RESTORE_REGS

/* Bad Abort numbers */
#define BAD_SYNC  0
#define BAD_IRQ   1
#define BAD_FIQ   2
#define BAD_ERROR 3

#define el_invalid(name, reason, el)		\
.align 6;					\
name##_invalid:					\
	SAVE_REGS TYPE_CRIT;			\
	mov x0, sp;				\
	mov x1, el;				\
	mov x2, #(reason);			\
	mrs x3, far_el1;			\
	b   uk_plat_native_except_inv_handler;	\
ENDPROC(name##_invalid);			\

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
