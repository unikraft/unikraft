/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Based on KVM platform code
 *
 * Copyright (c) 2024 Microsoft Corporation
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

#include <uk/config.h>
#include <uk/asm.h>
#include <uk/lcpu.h>
#include <uk/arch/x86_64.h>
#include <uk/reloc.h>
#include <hyperlight-x86/traps.h>

/* Boot startup args offsets */
#define LCPU_SARGS_STACKP_OFFSET	0x08

/*
 * Hyperlight 64-bit boot entry function
 *
 * We are called from entry64.S with:
 * - RDI = pointer to lcpu_sargs (with entry function and stack pointer)
 * - RSI = pointer to hyperlight_entry_args
 */

.code64
.section .text.boot.64
ENTRY(lcpu_start64)
	/* Save startup args and entry_args pointers */
	movq	%rdi, %r8	/* lcpu_sargs */
	movq	%rsi, %r9	/* hyperlight_entry_args */

	/* Enable FPU */
	movq	%cr0, %rax
	andq	$~(UK_ARCH_X86_64_CR0_EM | UK_ARCH_X86_64_CR0_TS), %rax  /* Clear EM and TS */
	orq	$UK_ARCH_X86_64_CR0_MP, %rax                   /* Set MP */
	movq	%rax, %cr0

	/* Enable SSE and FSGSBASE */
	movq	%cr4, %rax
	orq	$(UK_ARCH_X86_64_CR4_OSFXSR | UK_ARCH_X86_64_CR4_OSXMMEXCPT | UK_ARCH_X86_64_CR4_FSGSBASE), %rax
	movq	%rax, %cr4

	/* Initialize the MXCSR register */
	pushq	$0x1f80           /* Default MXCSR value */
	ldmxcsr	(%rsp)
	addq	$8, %rsp

	/* Hyperlight is single-CPU, use LCPU index 0 */
	leaq	uk_lcpus(%rip), %rbp
	movl	$UK_LCPU_STATE_INIT, UK_LCPU_STATE_OFFSET(%rbp)

	/* Load the stack pointer from startup args */
	movq	LCPU_SARGS_STACKP_OFFSET(%r8), %rsp

	/* According to System V AMD64 ABI, stack must be 16-byte aligned
	 * with (RSP+8) being a multiple of 16 at function entry.
	 */
	andq	$~0xf, %rsp
	subq	$0x8, %rsp

	/* Prepare arguments for hyperlight_entry(lcpu, entry_args) */
	movq	%rbp, %rdi           /* arg0 = this CPU struct */
	movq	%r9, %rsi            /* arg1 = hyperlight_entry_args */

#if !__OMIT_FRAMEPOINTER__
	xorq	%rbp, %rbp           /* Reset frame pointer */
#endif

	/* Jump to hyperlight_entry which will initialize the platform */
	leaq	hyperlight_entry(%rip), %rax
	jmp	*%rax

END(lcpu_start64)
