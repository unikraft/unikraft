/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/asm.h>

/* rdi: signum
 * rsi: siginfo or NULL
 * rdx: ucontext or NULL
 * rcx: handler
 * r8:  app sp
 */
ENTRY(pprocess_signal_arch_jmp_handler)
	/* save uk regs, the application's signal handler
	 * may corrupt anything. TODO Check calling convention.
	 */
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rax
	pushq	%r8
	pushq	%r9
	pushq	%r10
	pushq	%r11
	pushq	%rbx
	pushq	%rbp
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15
	movq	%rsp, %rax	/* temporarily save rsp into rax */

	/* Restore the user stack, user GS, and call the signal handler */
	swapgs
	movq	%r8, %rsp
	pushq	%rax
	call	*%rcx

	swapgs			/* resture uk GS */
	popq	%rsp		/* restore saved rsp from user stack */
	popq	%r15		/* restore uk regs */
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
	ret			/* return back to the caller */
END(pprocess_signal_arch_jmp_handler)
