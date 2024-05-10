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
 * @RDI execenv
 *   Reference to execution environment to load
 */
ENTRY(ukarch_execenv_load)
	.cfi_startproc simple
	.cfi_def_cfa rsp, 0

	/**
	 * Do all this with IRQ's disabled, as the final iretq should pop off
	 * a proper rflags anyway.
	 */
	cli

	/**
	 * Assign pointer to execution environment to load (first argument).
	 * We do this because it will be easy to keep track of it as, unlike
	 * %rdi, we do not have to store/restore %r12 across function calls.
	 * As per AMD64 ABI, %r12-r15 are callee-saved.
	 */
	movq	%rdi, %r12

	/**
	 * Load execenv's stored ECTX which resides at offset:
	 * sizeof(struct __regs) + sizeof(struct ukarch_sysctx) from beginning
	 * of execenv.
	 */
	addq	$(__REGS_SIZEOF + UKARCH_SYSCTX_SIZE), %rdi
	call	ukarch_ectx_load
	/**
	 * As stated previously, after function calls, %r12 preserved value of
	 * execenv pointer so restore that into %rdi.
	 */
	movq	%r12, %rdi

	/**
	 * Load execenv's stored system context which resides at offset:
	 * sizeof(struct __regs) from beginning of execenv.
	 */
	addq	$(__REGS_SIZEOF), %rdi
	call	ukarch_sysctx_load

	/**
	 * Load execenv's stored general purpose registers which resides at
	 * the beginning.
	 */
	movq	%r12, %rsp
	.cfi_undefined rsp
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

	iretq
	.cfi_endproc
