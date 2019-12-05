/* SPDX-License-Identifier: ISC */
/*
 * Authors: Dan Williams
 *          Martin Lucina
 *          Costin Lupu <costin.lupu@cs.pub.ro>
 *
 * Copyright (c) 2015-2017 IBM
 * Copyright (c) 2016-2017 Docker, Inc.
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose with or without fee is hereby granted, provided
 * that the above copyright notice and this permission notice appear
 * in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/* Taken from solo5 */

#include <x86/traps.h>

#define ENTRY(X)     .global X ; .type X, @function ; X:

.macro PUSH_CALLER_SAVE
	pushq %rdi
	pushq %rsi
	pushq %rdx
	pushq %rcx
	pushq %rax
	pushq %r8
	pushq %r9
	pushq %r10
	pushq %r11
	pushq %rbx
	pushq %rbp
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15
.endm

.macro POP_CALLER_SAVE
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %rbp
	popq %rbx
	popq %r11
	popq %r10
	popq %r9
	popq %r8
	popq %rax
	popq %rcx
	popq %rdx
	popq %rsi
	popq %rdi
.endm

.macro TRAP_ENTRY trapname, has_ec
ENTRY(ASM_TRAP_SYM(\trapname))
	cld

.if !\has_ec
	pushq $0                            /* no error code, pass 0 */
.endif
	PUSH_CALLER_SAVE
	subq $__REGS_PAD_SIZE, %rsp         /* we have some padding */

	movq %rsp, %rdi
	movq __REGS_OFFSETOF_ORIG_RAX(%rsp), %rsi
	call do_\trapname

	addq $__REGS_PAD_SIZE, %rsp         /* we have some padding */
	POP_CALLER_SAVE
	addq $8, %rsp                       /* discard error code */

	iretq
.endm

.macro IRQ_ENTRY irqno
ENTRY(cpu_irq_\irqno)
	cld

	pushq $0                            /* no error code */
	PUSH_CALLER_SAVE
	subq $__REGS_PAD_SIZE, %rsp         /* we have some padding */

	movq $\irqno, %rdi
	call _ukplat_irq_handle

	addq $__REGS_PAD_SIZE, %rsp         /* we have some padding */
	POP_CALLER_SAVE
	addq $8, %rsp

	iretq
.endm

TRAP_ENTRY divide_error,     0
TRAP_ENTRY debug,            0
TRAP_ENTRY nmi,              0
TRAP_ENTRY int3,             0
TRAP_ENTRY overflow,         0
TRAP_ENTRY bounds,           0
TRAP_ENTRY invalid_op,       0
TRAP_ENTRY no_device,        0
TRAP_ENTRY double_fault,     1
TRAP_ENTRY invalid_tss,      1
TRAP_ENTRY no_segment,       1
TRAP_ENTRY stack_error,      1
TRAP_ENTRY gp_fault,         1
TRAP_ENTRY page_fault,       1
TRAP_ENTRY coproc_error,     0
TRAP_ENTRY alignment_check,  1
TRAP_ENTRY machine_check,    0
TRAP_ENTRY simd_error,       0
TRAP_ENTRY virt_error,       0

IRQ_ENTRY 0
IRQ_ENTRY 1
IRQ_ENTRY 2
IRQ_ENTRY 3
IRQ_ENTRY 4
IRQ_ENTRY 5
IRQ_ENTRY 6
IRQ_ENTRY 7
IRQ_ENTRY 8
IRQ_ENTRY 9
IRQ_ENTRY 10
IRQ_ENTRY 11
IRQ_ENTRY 12
IRQ_ENTRY 13
IRQ_ENTRY 14
IRQ_ENTRY 15
