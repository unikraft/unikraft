/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2009, Citrix Systems, Inc.
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation.
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
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/* Taken from Mini-OS arch/x86/x86_64.S */

#include <uk/plat/common/sw_ctx.h>

#define ENTRY(X) .globl X ; X :

ENTRY(asm_thread_starter)
	popq %rdi
	popq %rbx
	pushq $0
	xorq %rbp,%rbp
	call *%rbx
	call uk_sched_thread_exit

ENTRY(asm_ctx_start)
	mov %rdi, %rsp      /* set SP */
	push %rsi           /* push IP and return */
	ret

ENTRY(asm_sw_ctx_switch)
	pushq %rbp
	pushq %rbx
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15
	movq %rsp, OFFSETOF_SW_CTX_SP(%rdi)       /* save ESP */
	movq OFFSETOF_SW_CTX_SP(%rsi), %rsp       /* restore ESP */
	movq $1f, OFFSETOF_SW_CTX_IP(%rdi)        /* save EIP */
	pushq OFFSETOF_SW_CTX_IP(%rsi)            /* restore EIP */
	ret
1:
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %rbx
	popq %rbp
	ret
