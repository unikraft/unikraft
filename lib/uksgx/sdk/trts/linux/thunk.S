/*
 * Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

.macro DEFINE_THUNK indirect_register, thunk_name
	.globl	\thunk_name
	.hidden	\thunk_name
	.type	\thunk_name, @function
\thunk_name:
	.cfi_startproc
	jmp	*%\indirect_register
	.cfi_endproc
.endm

.text
.globl	__x86_return_thunk
.hidden	__x86_return_thunk
.type	__x86_return_thunk, @function

.section .nipx,"ax",@progbits
__x86_return_thunk:
	.cfi_startproc
	ret
	.cfi_endproc


DEFINE_THUNK rax, __x86_indirect_thunk_rax
DEFINE_THUNK rbx, __x86_indirect_thunk_rbx
DEFINE_THUNK rcx, __x86_indirect_thunk_rcx
DEFINE_THUNK rdx, __x86_indirect_thunk_rdx
DEFINE_THUNK rbp, __x86_indirect_thunk_rbp
DEFINE_THUNK rsi, __x86_indirect_thunk_rsi
DEFINE_THUNK rdi, __x86_indirect_thunk_rdi
DEFINE_THUNK r8, __x86_indirect_thunk_r8
DEFINE_THUNK r9, __x86_indirect_thunk_r9
DEFINE_THUNK r10, __x86_indirect_thunk_r10
DEFINE_THUNK r11, __x86_indirect_thunk_r11
DEFINE_THUNK r12, __x86_indirect_thunk_r12
DEFINE_THUNK r13, __x86_indirect_thunk_r13
DEFINE_THUNK r14, __x86_indirect_thunk_r14
DEFINE_THUNK r15, __x86_indirect_thunk_r15
