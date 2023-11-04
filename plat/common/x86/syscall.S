/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2019, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <uk/arch/lcpu.h>
#include <uk/asm/cfi.h>

ENTRY(_ukplat_syscall)
	.cfi_startproc simple
	.cfi_def_cfa rsp, 0
	.cfi_register rip, rcx
	cli
	/*
	 * Push arguments in the order of 'struct __regs' to the stack.
	 * We are going to handover a refernce to this stack area as
	 * `struct __regs *` argument to the system call handler.
	 */
	pushq_cfi $0		/* exception frame filled with zeros */
	pushq_cfi $0		/* (rip, cs, eflags, rsp, ss)        */
	pushq_cfi $0		/*                                   */
	pushq_cfi $0		/*                                   */
	pushq_cfi $0		/*                                   */
	pushq_reg_cfi rax	/* orig_rax */
	pushq_reg_cfi rdi
	pushq_reg_cfi rsi
	pushq_reg_cfi rdx
	pushq_reg_cfi rcx
	.cfi_rel_offset rip, 0
	pushq_reg_cfi rax
	pushq_reg_cfi r8
	pushq_reg_cfi r9
	pushq_reg_cfi r10
	pushq_reg_cfi r11
	pushq_reg_cfi rbx
	pushq_reg_cfi rbp
	pushq_reg_cfi r12
	pushq_reg_cfi r13
	pushq_reg_cfi r14
	pushq_reg_cfi r15

	/* padding */
	subq  $(__REGS_PAD_SIZE), %rsp
	.cfi_adjust_cfa_offset __REGS_PAD_SIZE
	sti

	/*
	 * Handle call
	 * NOTE: Handler function is going to modify saved registers state
	 * NOTE: Stack pointer as "struct __regs *" argument
	 *       (calling convention: 1st arg on %rdi)
	 */
	movq %rsp, %rdi

	/*
	 * Make sure the stack is aligned to 16-bytes. We store the original
	 * stack pointer in the frame pointer (callee saved)
	 */
	movq %rsp, %rbp
	and $~15, %rsp
	.cfi_def_cfa_register rbp

	call ukplat_syscall_handler

	/* Restore original stack pointer */
	movq %rbp, %rsp
	.cfi_def_cfa_register rsp

	cli
	/* Load the updated state back to registers */
	addq $(__REGS_PAD_SIZE), %rsp
	.cfi_adjust_cfa_offset -__REGS_PAD_SIZE
	popq_reg_cfi r15
	popq_reg_cfi r14
	popq_reg_cfi r13
	popq_reg_cfi r12
	popq_reg_cfi rbp
	popq_reg_cfi rbx
	popq_reg_cfi r11
	popq_reg_cfi r10
	popq_reg_cfi r9
	popq_reg_cfi r8
	popq_reg_cfi rax
	popq_reg_cfi rcx
	.cfi_register rip, rcx
	popq_reg_cfi rdx
	popq_reg_cfi rsi
	popq_reg_cfi rdi
	/* orig_rax and exception frame */
	addq $(6 * 8), %rsp
	.cfi_adjust_cfa_offset -(6 * 8)
	sti

	/*
	 * Return from system call, inspired by HermiTux [1]
	 * NOTE: We can't use sysret because it changes protection mode [1]
	 *
	 * [1] Pierre et al., 2019, A binary-compatible Unikernel,
	 *     Proceedings of the 15th ACM SIGPLAN/SIGOPS International
	 *     Conference on Virtual Execution Environments (VEE 2019))
	 */
	jmp *%rcx
	.cfi_endproc
