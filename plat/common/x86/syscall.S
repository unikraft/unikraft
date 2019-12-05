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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#include <uk/arch/lcpu.h>

#define ENTRY(X) .globl X ; X :

ENTRY(_ukplat_syscall)
	cli
	/*
	 * Push arguments in the order of 'struct __regs' to the stack.
	 * We are going to handover a refernce to this stack area as
	 * `struct __regs *` argument to the system call handler.
	 */
	pushq $0	/* exception frame filled with zeros */
	pushq $0	/* (rip, cs, eflags, rsp, ss)        */
	pushq $0	/*                                   */
	pushq $0	/*                                   */
	pushq $0	/*                                   */
	pushq %rax 	/* orig_rax */
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
	/* padding */
	subq  $(__REGS_PAD_SIZE), %rsp
	sti

	/*
	 * Handle call
	 * NOTE: Handler function is going to modify saved registers state
	 * NOTE: Stack pointer as "struct __regs *" argument
	 *       (calling convention: 1st arg on %rdi)
	 */
	movq %rsp, %rdi
	call ukplat_syscall_handler

	cli
	/* Load the updated state back to registers */
	addq $(__REGS_PAD_SIZE), %rsp
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
	/* orig_rax and exception frame */
	addq $(6 * 8), %rsp
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
