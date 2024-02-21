/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *          Cristian Vijelie <cristianvijelie@gmail.com>
 *
 * Copyright (c) 2022, Karlsruhe Institute of Technology (KIT)
 *                     All rights reserved.
 * Copyright (c) 2022, University POLITEHNICA of Bucharest.
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

#ifndef __PLAT_KVM_X86_LCPU_HELPERS_S__
#define __PLAT_KVM_X86_LCPU_HELPERS_S__

#include <uk/arch/lcpu.h>
#include <uk/asm/cet.h>

/**
 * Enable the floating point unit (FPU) and Streaming SIMD Extensions (SSE).
 * They are part of the base x86-64 architecture so there is no need to check
 * for availability.
 *
 * NOTE: Expects that a handler for #XM is set up
 *
 * @clobbers: RDI
 */
.macro LCPU_ENABLE_FPU_SSE
	/* Enable numeric exceptions and coprocessor monitor */
	movq	%cr0, %rdi
	orl	$(X86_CR0_NE | X86_CR0_MP), %edi
	movq	%rdi, %cr0

	fninit
#if __SSE__
	jmp	ldmxcsr_rval_addr + 0x4

ldmxcsr_rval_addr:
	.long	0x1f80		/* Power-on default */

	movq	%cr4, %rdi
	orl	$(X86_CR4_OSFXSR | X86_CR4_OSXMMEXCPT), %edi
	movq	%rdi, %cr4

	ur_mov	ldmxcsr_rval_addr, %ebx, 4

	ldmxcsr	(%ebx)

#endif /* __SSE__ */
.endm

/**
 * Enable the XSAVE feature. Jump to the fail label if not successful.
 *
 * @in: ECX according to cpuid(eax=0x1)
 * @clobbers: RDI
 */
.macro LCPU_ENABLE_XSAVE fail_label
	testl	$(X86_CPUID1_ECX_XSAVE), %ecx
	jz	\fail_label

	movq	%cr4, %rdi
	orl	$(X86_CR4_OSXSAVE), %edi
	movq	%rdi, %cr4
.endm

/**
 * Enable Advanced Vector Extensions (AVX). Jump to the fail label if not
 * successful.
 *
 * @in: ECX according to cpuid(eax=0x1)
 * @clobbers: RAX, RDI
 */
.macro LCPU_ENABLE_AVX fail_label
	testl	$(X86_CPUID1_ECX_AVX), %ecx
	jz	\fail_label

	movq	%rcx, %rdi
	xorl	%ecx, %ecx
	xgetbv
	orl	$(X86_XCR0_SSE | X86_XCR0_AVX), %eax
	xsetbv
	movq	%rdi, %rcx
.endm

/**
 * Enable FS/GSBASE. Jump to the fail label if not successful.
 *
 * @in: EBX according to cpuid(eax=7, ecx=0)
 * @clobbers: RDI
 */
.macro LCPU_ENABLE_FSGSBASE fail_label
	testl	$(X86_CPUID7_EBX_FSGSBASE), %ebx
	jz	\fail_label

	movq	%cr4, %rdi
	orl	$(X86_CR4_FSGSBASE), %edi
	movq	%rdi, %cr4
.endm

/**
 * Enable Memory Protection Keys (PKU). Jump to the fail label if not
 * successful.
 *
 * @in: ECX according to cpuid(eax=7, ecx=0)
 * @clobbers: RAX, RDI
 */
.macro LCPU_ENABLE_PKU fail_label
	testl	$(X86_CPUID7_ECX_PKU), %ecx
	jz	\fail_label

	movq	%cr4, %rdi

	/* Only enable if we have XSAVE */
	testl	$(X86_CR4_OSXSAVE), %edi
	jz	\fail_label

	orl	$(X86_CR4_PKE), %edi
	movq	%rdi, %cr4

	/* Also enable XSAVE for the PKRU */
	movq	%rcx, %rdi
	xorl	%ecx, %ecx
	xgetbv
	orl	$(X86_XCR0_PKRU), %eax
	xsetbv
	movq	%rdi, %rcx
.endm

/**
 * Enable Control-flow Enforcement Technology. Jump to the fail label
 * if not successful
 * 
 * @in: nothing (we do our own CPUID)
 * @clobbers: RAX, RBX, RCX, RDX
 */
.macro LCPU_ENABLE_CET no_cet_label
	movq	$7, %rax
	xorq	%rcx, %rcx
	cpuid

	testl $(X86_CPUID7_EDX_CET_IBT), %edx
	jz no_ibt_support
	jmp enable_ibt

no_ibt_support:
	testl	$(X86_CPUID7_ECX_CET_SS), %ecx
	jz		\no_cet_label
	jmp enable_cr4_cet

enable_ibt:
	movl $(MSR_IA32_S_CET), %ecx
	movl $(X86_CET_ENDBR_EN | X86_CET_NO_TRACK_EN), %eax
	xorl %edx, %edx
	wrmsr
enable_cr4_cet:
	movq	%cr4, %rax
	orq		$(X86_CR4_CET), %rax
	movq	%rax, %cr4
.endm
#endif /* __PLAT_KVM_X86_LCPU_HELPERS_S__ */
