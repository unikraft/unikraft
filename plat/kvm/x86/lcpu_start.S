/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Cristian Vijelie <cristianvijelie@gmail.com>
 *
 * Copyright (c) 2021, University Politehnica of Bucharest. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *	contributors may be used to endorse or promote products derived from
 *	this software without specific prior written permission.
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
 */

#include <x86/cpu_defs.h>
#include <kvm-x86/traps.h>

#define ENTRY(x) 	.globl x; .type x,%function; x:
#define END(x)  	.size x, . - x

#define LOAD_ADDR	0x8000
#define RELOC(x)	(x - r_base + LOAD_ADDR)

/* Written with help from Daniel Brodsky */
.section .text
ENTRY(_lcpu_start16)
r_base = .
.code16
	cli
	jmp 	load_gdt32

gdt32:
	.quad 	0x0000000000000000	/* null segment		*/
gdt32_cs:
	.quad 	0x00DF9A000000FFFF	/* 32-bit CS		*/
gdt32_ds:
	.quad 	0x00DF92000000FFFF	/* data segment		*/
gdt32_end:
gdt32_ptr:
	.word 	(gdt32_end-gdt32-1)	/* size - 1		*/
	.long 	gdt32			/* gdt address		*/

gdt64:
	.quad 	0x0000000000000000
gdt64_cs:
	.quad 	GDT_DESC_CODE_VAL	/* 64bit CS		*/
gdt64_ds:
	.quad 	GDT_DESC_DATA_VAL	/* DS			*/
	.quad 	0x0000000000000000	/* TSS part 1 (via C)	*/
	.quad 	0x0000000000000000	/* TSS part 2 (via C)	*/
gdt64_end:
gdt64_ptr:
	.word 	(gdt64_end-gdt64-1)
	.quad 	gdt64

.type mxcsr_ptr, @object
mxcsr_ptr:
	.long 0x1f80			/* Intel SDM power-on default */

load_gdt32:
	lgdt 	RELOC(gdt32_ptr)
	movl 	%cr0, %eax
	orl 	X86_CR0_PE, %eax
	movl 	%eax, %cr0
	ljmp 	$8, $RELOC(_lcpu_start32)
.align 32
END(_lcpu_start16)

.code32
.align 32
ENTRY(_lcpu_start32)
	/* Set up the remaining segment registers */
	mov 	$(gdt64_ds-gdt64), %eax
	mov 	%eax, %es
	mov 	%eax, %ss
	mov 	%eax, %ds
	mov 	%eax, %fs
	mov 	%eax, %gs

	/* 1: enable pae */
	movl 	%cr4, %eax
	orl 	$X86_CR4_PAE, %eax
	movl 	%eax, %cr4

	/* 2: enable long mode */
	movl 	$0xc0000080, %ecx
	rdmsr
	orl 	$X86_EFER_LME, %eax
	orl 	$X86_EFER_NXE, %eax
	wrmsr

	/* 3: Load PML pointer */
	movl 	cr3_val, %eax
	movl 	%eax, %cr3

	/* 4: Enable paging */
	movl	%cr0, %eax
	orl	$X86_CR0_PG, %eax
	movl	%eax, %cr0

	lgdt 	RELOC(gdt64_ptr)
	ljmp 	$(gdt64_cs-gdt64), $RELOC(_lcpu_start64)
END(_lcpu_start32)
.align 64

.code64
.align 32
ENTRY(_lcpu_start64)
	/* Wait for the BSP to finish */
spin:
	pause
	cmpb 	$0, bspdone
	jz 	spin

	/* get the AP id */
	mov	$1, %eax
	cpuid
	shrl	$24, %ebx

	xor	%rcx, %rcx
	mov	%bl, %cl

	mov	$1, %rsi
	shlq	%cl, %rsi

	/* Notify the BSP that the core has started successfully */
	lock	addq %rsi, smp_aprunning

	/* Taken from entry64.S */
	movq 	%rcx, %r9
	movq 	%cr0, %rsi
	movq 	%cr4, %rdi

	/* FPU and SSE are part of base x86-64, so no need to check for their
	 * availability before enabling and initializing. */
	andl 	$(~(X86_CR0_EM | X86_CR0_TS)), %esi
	orl 	$(X86_CR0_MP | X86_CR0_NE | X86_CR0_WP), %esi
	movq 	%rsi, %cr0
	fninit
#if __SSE__
	orl 	$(X86_CR4_OSFXSR | X86_CR4_OSXMMEXCPT), %edi
	movq 	%rdi, %cr4
	ldmxcsr (mxcsr_ptr)
#endif /* __SSE__ */
#if (__AVX__ || CONFIG_HAVE_X86PKU)
	/* Check capabilities subject to availability as indicated by cpuid.
	 * First, start off with "standard features" */
	movl 	$0x1, %eax
	cpuid
	/* ecx and edx now contain capability information, so we can now
	 * enable capabilities based on the indicated features */
	/* note: OSXSAVE needs to be enabled before AVX and PKU */
	testl 	$(X86_CPUID1_ECX_XSAVE), %ecx
	jz 	noxsave
	orl 	$(X86_CR4_OSXSAVE), %edi
	movq 	%rdi, %cr4
#if __AVX__
	/* now enable AVX. This needs to be last checking cpuid features from
	 * the eax=1 cpuid call, because it clobbers ecx */
	testl 	$(X86_CPUID1_ECX_AVX), %ecx
	jz 	noavx
	xorl 	%ecx, %ecx
	xgetbv
	orl 	$(X86_XCR0_SSE | X86_XCR0_AVX), %eax
	xsetbv
noavx:
#endif /* __AVX__ */
/* Do not enable AVX without XSAVE, otherwise we'll get #UD */
noxsave:
#endif /* __AVX__ || CONFIG_HAVE_X86PKU */
	/* Now, check for extended features. */
	movl 	$0x7, %eax
	movl 	$0x0, %ecx
	cpuid
	/* ebx, ecx, edx now contain extended capabilities information. */
	/* check for and enable FS/GSBASE */
	testl 	$(X86_CPUID7_EBX_FSGSBASE), %ebx
	jz 	nofsgsbase
	orl $	(X86_CR4_FSGSBASE), %edi
	movq 	%rdi, %cr4
nofsgsbase:
#if CONFIG_HAVE_X86PKU
	/* check for Memory Protection Keys (PKU) */
	testl 	$(X86_CPUID7_ECX_PKU), %ecx
	jz 	nopku
	/* only enable PKU if we support XSAVE */
	testl 	$(X86_CR4_OSXSAVE), %edi
	jz 	nopku
	/* PKU is supported, enable it via CR4 */
	orl 	$(X86_CR4_PKE), %edi
	movq 	%rdi, %cr4
	/* also enable XSAVE for the PKRU */
	xorl 	%ecx, %ecx
	xgetbv
	orl 	$(X86_XCR0_PKRU), %eax
	xsetbv
nopku:
#endif /* CONFIG_HAVE_X86PKU */

	/* Load the stack pointer and the entry address */
	movq	stackptr, %rsi
	movq	(%rsi, %r9, 8), %rsp

	movq	entryptr, %rdi
	movq	(%rdi, %r9, 8), %rax

	jmp 	*%rax
END(_lcpu_start64)
