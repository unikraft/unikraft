/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/arch/lcpu.h>
#include <uk/asm.h>
#include <uk/syscall.h>

/* Used to pop the contents of a `struct uk_syscall_ctx` off the stack */
ENTRY(uk_syscall_ctx_popall)
	/* Do all this with IRQ's disabled, as the final iretq should pop off
	 * a proper rflags anyway.
	 */
	cli

	movq	%rsp, %rdi
	addq	$(__REGS_SIZEOF + UKARCH_SYSREGS_SIZE), %rdi
	call	ukarch_ectx_load

	movq	%rsp, %rdi
	addq	$(__REGS_SIZEOF), %rdi
	call	ukarch_sysregs_switch_ul

	addq	$(__REGS_PAD_SIZE), %rsp

	popq	%r15
	popq	%r14
	popq	%r13
	popq	%r12
	popq	%rbp
	popq	%rbx
	popq	%r11
	popq	%r10
	popq	%r9
	popq	%r8
	popq	%rax
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi

	addq	$(__REGS_PAD_SIZE), %rsp

	swapgs

	iretq
