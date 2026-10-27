/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/config.h>
#include <uk/asm.h>
#include <uk/lcpu.h>
#include <uk/arch/x86_64.h>
#include <uk/reloc.h>

#define LCPU_SARGS_STACKP_OFFSET	0x08

/*
 * lcpu_start64 — CPU bootstrap for the Hyperlight platform.
 *
 * Called from _hyperlight_entry with:
 *   RDI = pointer to lcpu_sargs (entry function + stack pointer)
 *   RSI = pointer to hyperlight_entry_args
 *
 * Initializes FPU/SSE (and AVX where available), switches to the boot
 * stack, and calls hyperlight_entry() in C.
 */
.code64
.section .text.boot.64
ENTRY(lcpu_start64)
	movq	%rdi, %r8	/* lcpu_sargs */
	movq	%rsi, %r9	/* hyperlight_entry_args */

	/*
	 * Enable FPU: clear EM (emulation) and TS (task switched),
	 * set MP (monitor coprocessor).
	 */
	movq	%cr0, %rax
	andq	$~(UK_ARCH_X86_64_CR0_EM | UK_ARCH_X86_64_CR0_TS), %rax
	orq	$UK_ARCH_X86_64_CR0_MP, %rax
	movq	%rax, %cr0

	/*
	 * Enable SSE (OSFXSR + OSXMMEXCPT) and FSGSBASE instructions.
	 * FSGSBASE lets us read/write FS/GS base without MSR access.
	 */
	movq	%cr4, %rax
	orq	$(UK_ARCH_X86_64_CR4_OSFXSR | UK_ARCH_X86_64_CR4_OSXMMEXCPT | UK_ARCH_X86_64_CR4_FSGSBASE), %rax
	movq	%rax, %cr4

	/*
	 * Enable XSAVE, then AVX and AVX-512 where the CPU has them.
	 * Programs built for x86-64-v3 (e.g. glibc's libc.a on Ubuntu 26.04)
	 * use AVX2 without checking for it, and fault with #UD otherwise.
	 * The extended context store picks XSAVE up from CPUID at boot.
	 */
	movl	$1, %eax
	xorl	%ecx, %ecx
	cpuid
	testl	$(UK_ARCH_X86_64_CPUID1_ECX_XSAVE), %ecx
	jz	.Lno_xsave
	movl	%ecx, %r10d

	movq	%cr4, %rax
	orq	$(UK_ARCH_X86_64_CR4_OSXSAVE), %rax
	movq	%rax, %cr4

	/* Only ask XSETBV for components CPUID 0xD says it supports */
	movl	$0xd, %eax
	xorl	%ecx, %ecx
	cpuid
	movl	%eax, %r11d

	movl	$(UK_ARCH_X86_64_XCR0_X87 | UK_ARCH_X86_64_XCR0_SSE), %esi
	testl	$(UK_ARCH_X86_64_CPUID1_ECX_AVX), %r10d
	jz	.Lset_xcr0
	testl	$(UK_ARCH_X86_64_XCR0_AVX), %r11d
	jz	.Lset_xcr0
	orl	$(UK_ARCH_X86_64_XCR0_AVX), %esi

	movl	$7, %eax
	xorl	%ecx, %ecx
	cpuid
	testl	$(UK_ARCH_X86_64_CPUID7_EBX_AVX512F), %ebx
	jz	.Lset_xcr0
	movl	%r11d, %eax
	andl	$(UK_ARCH_X86_64_XCR0_AVX512_OPMASK |			\
		  UK_ARCH_X86_64_XCR0_AVX512_ZMM_HI256 |		\
		  UK_ARCH_X86_64_XCR0_AVX512_HI16_ZMM), %eax
	cmpl	$(UK_ARCH_X86_64_XCR0_AVX512_OPMASK |			\
		  UK_ARCH_X86_64_XCR0_AVX512_ZMM_HI256 |		\
		  UK_ARCH_X86_64_XCR0_AVX512_HI16_ZMM), %eax
	jne	.Lset_xcr0
	orl	%eax, %esi

.Lset_xcr0:
	xorl	%ecx, %ecx
	xgetbv
	orl	%esi, %eax
	xsetbv
.Lno_xsave:

	/* Initialize MXCSR to default (mask all SIMD exceptions) */
	pushq	$0x1f80
	ldmxcsr	(%rsp)
	addq	$8, %rsp

	/* Hyperlight is single-CPU — use LCPU index 0 */
	leaq	uk_lcpus(%rip), %rbp
	movl	$UK_LCPU_STATE_INIT, UK_LCPU_STATE_OFFSET(%rbp)

	/* Switch to the boot stack (pre-faulted in entry64.S) */
	movq	LCPU_SARGS_STACKP_OFFSET(%r8), %rsp

	/* System V AMD64 ABI: (RSP+8) must be 16-byte aligned at call */
	andq	$~0xf, %rsp
	subq	$0x8, %rsp

	/* Call hyperlight_entry(lcpu, entry_args) */
	movq	%rbp, %rdi
	movq	%r9, %rsi

#if !__OMIT_FRAMEPOINTER__
	xorq	%rbp, %rbp
#endif

	leaq	hyperlight_entry(%rip), %rax
	jmp	*%rax

END(lcpu_start64)
