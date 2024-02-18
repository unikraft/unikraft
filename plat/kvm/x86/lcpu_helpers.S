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

#include <kvm-x86/serial_console.h>
#include <uk/arch/lcpu.h>

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
 * Super basic enable macro for the widely available COM1 UART port. Can be run
 * in any mode.
 *
 * @clobbers: RAX, RDX
 */
.macro LCPU_EARLY_COM1_INIT
	/* Disable all IRQs */
	xorl	%eax, %eax
	movw	$COM1_INTR, %dx
	outb	%al, (%dx)

	/* Enable DLAB: Begin setting the baudrate divisor */
	movb	$DLAB, %al
	movw	$COM1_CTRL, %dx
	outb	%al, (%dx)

	/* Set low byte of the two byte divisor */
	movb	$COM1_BAUDDIV_LO, %al
	movw	$COM1_DIV_LO, %dx
	outb	%al, (%dx)

	/* Set high byte of the two byte divisor */
	movb	$COM1_BAUDDIV_HI, %al
	movw	$COM1_DIV_HI, %dx
	outb	%al, (%dx)

	/* 8 bits, no parity, one stop bit */
	movb	$PROT, %al
	movw	$COM1_CTRL, %dx
	outb	%al, (%dx)
.endm

/**
 * Super basic early printing to widely available COM1 UART port. Must be run
 * in 64-bit mode of long mode at least so that we can use RIP-relative
 * addressing.
 *
 * @param str: Address of C-string
 * @clobbers: RAX, RDX, RSI
 */
.macro LCPU_EARLY_COM1_PRINT str:req
	/* RSI = str */
	leaq	\str(%rip), %rsi
0:
	/* TX queue must be empty */
	movw	$COM1_STATUS, %dx
	inb	(%dx), %al
	andb	$0x20, %al
	jz	0b

	/* Fetch current character */
	movb	0(%rsi), %al

	/* Is it the NULL terminator? Then we are done printing the C-string. */
	test	%al, %al
	jz	1f

	/* Send the current character out the COM1 data port */
	movw	$COM1_DATA, %dx
	outb	%al, (%dx)

	/* Go to next character */
	incq	%rsi
	jmp	0b
1:
.endm
#endif /* __PLAT_KVM_X86_LCPU_HELPERS_S__ */
