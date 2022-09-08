// This file is provided under a dual BSD/GPLv2 license.  When using or
// redistributing this file, you may do so under either license.
//
// GPL LICENSE SUMMARY
//
// Copyright(c) 2016-2018 Intel Corporation.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of version 2 of the GNU General Public License as
// published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact Information:
// Jarkko Sakkinen <jarkko.sakkinen@linux.intel.com>
// Intel Finland Oy - BIC 0357606-4 - Westendinkatu 7, 02160 Espoo
//
// BSD LICENSE
//
// Copyright(c) 2016-2018 Intel Corporation.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//   * Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in
//     the documentation and/or other materials provided with the
//     distribution.
//   * Neither the name of Intel Corporation nor the names of its
//     contributors may be used to endorse or promote products derived
//     from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Authors:
//
// Jarkko Sakkinen <jarkko.sakkinen@linux.intel.com>
// Haim Cohen <haim.cohen@intel.com>

#include <sgx_asm.h>

	.section ".tcs", "a"
	.balign	4096

	.fill	1, 8, 0			# STATE (set by CPU)
	.fill	1, 8, 0			# FLAGS
	.long	encl_ssa		# OSSA
	.fill	1, 4, 0
	.fill	1, 4, 0			# CSSA (set by CPU)
	.fill	1, 4, 1			# NSSA
	.long	encl_entry		# OENTRY
	.fill	1, 4, 0
	.fill	1, 8, 0			# AEP (set by EENTER and ERESUME)
	.fill	1, 8, 0			# OFSBASE
	.fill	1, 8, 0			# OGSBASE
	.fill	1, 4, 0xFFFFFFFF 	# FSLIMIT
	.fill	1, 4, 0xFFFFFFFF	# GSLIMIT

	.text

encl_entry:
	# %rbx contains the base address for TCS, which is also the first
	# address inside the enclave. By adding $le_stack_end to it, we get the
	# absolute address for the stack.
	lea	(encl_stack)(%rbx), %rax
	xchg	%rsp, %rax
	push	%rax

	push	%rcx # push the address after EENTER
	push	%rbx # push the enclave base address

	call	encl_body

	pop	%rbx # pop the enclave base address

	# Restore XSAVE registers to a synthetic state.
	mov     $0xFFFFFFFF, %rax
	mov     $0xFFFFFFFF, %rdx
	lea	(xsave_area)(%rbx), %rdi
	fxrstor	(%rdi)

	# Clear GPRs
	xor     %rcx, %rcx
	xor     %rdx, %rdx
	xor     %rdi, %rdi
	xor     %rsi, %rsi
	xor     %r8, %r8
	xor     %r9, %r9
	xor     %r10, %r10
	xor     %r11, %r11
	xor     %r12, %r12
	xor     %r13, %r13
	xor     %r14, %r14
	xor     %r15, %r15

	# Reset status flags
	add     %rdx, %rdx # OF = SF = AF = CF = 0; ZF = PF = 1

	pop	%rbx # pop the address after EENTER

	# Restore the caller stack.
	pop	%rax
	mov	%rax, %rsp

	# EEXIT
	mov	$4, %rax
	enclu

	.global sgx_ereport
sgx_ereport:
	push	%rbx
	xor	%rax, %rax /* EREPORT */
	mov	%rdi, %rbx  /* TARGETINFO */
	mov	%rsi, %rcx /* REPORTDATA */
	ENCLU
	pop	%rbx
	ret

	.global sgx_egetkey
sgx_egetkey:
	push	%rbx
	mov	$0x01, %rax /* EGETKEY */
	mov	%rdi, %rbx /* KEYREQUEST */
	mov	%rsi, %rcx  /* KEY */
	ENCLU
	pop	%rbx
	ret

	.section ".data", "aw"

encl_ssa:
	.space 4096

xsave_area:
	.fill	1, 4, 0x037F		# FCW
	.fill	5, 4, 0
	.fill	1, 4, 0x1F80		# MXCSR
	.fill	1, 4, 0xFFFF		# MXCSR_MASK
	.fill	123, 4, 0
	.fill	1, 4, 0x80000000	# XCOMP_BV[63] = 1, compaction mode
	.fill	12, 4, 0

	.balign 4096
	.space 4096 * 3
encl_stack:
