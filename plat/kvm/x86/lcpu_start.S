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

#include <uk/config.h>
#include <uk/asm.h>
#include <uk/plat/common/lcpu.h>
#include <x86/cpu_defs.h>
#include <kvm-x86/traps.h>

#define SEC_BEGIN(x)		.globl x86_##x##_begin; x86_##x##_begin = .;
#define SEC_END(x)		.globl x86_##x##_end; x86_##x##_end = .;
#define SEC_RELOC(x, s, addr)	(x - x86_##s##_begin + addr)

#define LOAD_ADDR16		0x8000
#define RELOC16(x)		SEC_RELOC(x, start16, LOAD_ADDR16)

.section .data.boot.16
.globl x86_start16_addr
x86_start16_addr:
	.quad	LOAD_ADDR16

/* The following section has to be copied to LOAD_ADDR16 (4KiB max!) at runtime
 * as 16-bit real mode entry point
 */
.section .text.boot.16
SEC_BEGIN(start16)

.code16
ENTRY(lcpu_start16_ap)
	/* Clear pointers to startup and platform boot parameters */
	xorl	%edi, %edi
	xorl	%esi, %esi

	jmp	lcpu_start16
END(lcpu_start16_ap)

/*
 * 16-bit boot entry function
 * TODO: At the moment, this does not allow booting on a bare metal system.
 * It is only used as entry point for APs in SMP configuration.
 */
.align 16
gdt32:
/* We can repurpose the null segment to encode the GDT pointer because
 * the null segment is not accessed by the processor in 32-bit mode. We also
 * want the GDT in front of the code so that we can more easily reference the
 * symbols. So we use the two spare bytes to just jump over the GDT.
 */
gdt32_null:
	.word	0x0000
gdt32_ptr:
	.word	(gdt32_end - gdt32 - 1)	/* size - 1	*/
	.long	RELOC16(gdt32)		/* GDT address	*/
gdt32_cs:
	.quad	GDT_DESC_CODE32_VAL	/* 32-bit CS	*/
gdt32_ds:
	.quad	GDT_DESC_DATA32_VAL	/* DS		*/
gdt32_end:

#define CR0_BOOT16_SETTINGS						\
	  X86_CR0_PE	/* Protected mode */

.code16
ENTRY(lcpu_start16)
	/* Disable interrupts */
	cli

	/* Setup protected mode */
	movl	$CR0_BOOT16_SETTINGS, %eax
	movl	%eax, %cr0

	/* Load 32-bit GDT and jump into 32-bit code segment */
	lgdt	RELOC16(gdt32_ptr)
	ljmp	$(gdt32_cs - gdt32), $RELOC16(jump_to32)
.code32
jump_to32:
	/* Set up remaining segment registers */
	movl	$(gdt32_ds - gdt32), %eax
	movl	%eax, %es
	movl	%eax, %ss
	movl	%eax, %ds

	xorl	%eax, %eax
	movl	%eax, %fs
	movl	%eax, %gs

	leal	lcpu_start32, %eax
	jmp	*%eax
END(lcpu_start16)

SEC_END(start16)

/*
 * 32-bit boot entry function
 * System must be in 32-bit protected mode with paging disabled. A GDT must be
 * loaded with descriptors for 4GiB flat CS and DS segments. CS must have
 * execute/read permission and DS must have read/write permission. Interrupts
 * must be disabled. A20 gate must be enabled.
 *
 * NOTE: We assume a pointer to a struct lcpu_sargs in EDI and a pointer to
 * potential platform boot arguments in ESI (e.g., multiboot) if this is the
 * first boot.
 */
.section .data.boot.32
.align 16
gdt64:
gdt64_null:
	.quad	0x0000000000000000	/* null segment */
gdt64_cs:
	.quad	GDT_DESC_CODE64_VAL	/* 64-bit CS	*/
gdt64_ds:
	.quad	GDT_DESC_DATA64_VAL	/* DS		*/
gdt64_end:
gdt64_ptr:
	.word	gdt64_end - gdt64 - 1
	.quad	gdt64

#define CR4_BOOT32_SETTINGS						\
	  X86_CR4_PAE	/* Physical Address Extension */

#define EFER_BOOT32_SETTINGS						\
	  X86_EFER_LME	/* IA-32e Mode */

#define CR0_BOOT32_SETTINGS						\
	  X86_CR0_PE	/* Protected mode */				\
	| X86_CR0_WP	/* Write Protect */				\
	| X86_CR0_PG	/* Paging */

.code32
.section .text.boot.32
ENTRY(lcpu_start32)
	/* Enable physical address extension (PAE) */
	movl	$CR4_BOOT32_SETTINGS, %eax
	movl	%eax, %cr4

	/* Switch to IA-32e mode (long mode) */
	xorl	%edx, %edx
	movl	$EFER_BOOT32_SETTINGS, %eax
	movl	$X86_MSR_EFER, %ecx
	wrmsr

	/* Set boot page table and enable paging */
	movl	$x86_bpt_pml4, %eax
	movl	%eax, %cr3

	movl	$CR0_BOOT32_SETTINGS, %eax
	movl	%eax, %cr0

	/* Load 64-bit GDT and jump to 64-bit code segment */
	lgdt	gdt64_ptr
	ljmp	$(gdt64_cs - gdt64), $jump_to64
.code64
jump_to64:
	/* Set up remaining segment registers */
	movl	$(gdt64_ds - gdt64), %eax
	movl	%eax, %es
	movl	%eax, %ss
	movl	%eax, %ds

	xorl	%eax, %eax
	movl	%eax, %fs
	movl	%eax, %gs

	leaq	lcpu_start64, %rcx
	jmp	*%rcx
END(lcpu_start32)

/*
 * 64-bit boot entry function
 * System must be in 64-bit mode with paging enabled. The first 4GiB have to be
 * identity mapped. A GDT must be loaded with descriptors for 4GiB flat CS and
 * DS segments. CS must have execute/read permission and DS must have
 * read/write permission. ES and ES must be the same as DS. Interrupts must be
 * disabled.
 *
 * NOTE: We assume a pointer to a struct lcpu_sargs in RDI and a pointer to
 * potential platform boot arguments in RSI (e.g., multiboot) if this is the
 * first boot. In that case, we setup the CPU with the provided startup
 * arguments.
 *
 * NOTE: Code should be position-independent
 */
#include "lcpu_helpers.S"

.code64
.section .text.boot.64
ENTRY(lcpu_start64)
	/* Save the startup args pointer */
	movq	%rdi, %r8

	/* Request basic CPU features and APIC ID
	 * TODO: This APIC ID is limited to 256. Better get from leaf 0x1f
	 */
	movl	$1, %eax
	cpuid
	shrl	$24, %ebx

	/* Use APIC_ID * LCPU_SIZE for indexing the cpu structure */
	movl	$LCPU_SIZE, %eax
	imul	%ebx, %eax

	/* Compute pointer into CPU struct array and store it in RBP
	 * We do not use the frame pointer, yet
	 */
	leaq	lcpus(%rip), %rbp
	addq	%rax, %rbp

	/* Put CPU into init state */
	movl	$LCPU_STATE_INIT, LCPU_STATE_OFFSET(%rbp)

	/* Enable FPU and SSE */
	LCPU_ENABLE_FPU_SSE

#if (__AVX__ || CONFIG_HAVE_X86PKU)
	/* Enable XSAVE feature */
	LCPU_ENABLE_XSAVE fail
#if __AVX__
	/* Enable AVX */
	LCPU_ENABLE_AVX fail
#endif /* __AVX__ */
#endif /* __AVX__ || CONFIG_HAVE_X86PKU */

	/* Request extended CPU features */
	movl	$7, %eax
	xorl	%ecx, %ecx
	cpuid

	/* Enable FS and GS base */
	LCPU_ENABLE_FSGSBASE no_fsgsbase
no_fsgsbase:
#if CONFIG_HAVE_X86PKU
	/* Enable memory protection keys (PKU) */
	LCPU_ENABLE_PKU no_pku
no_pku:
#endif /* CONFIG_HAVE_X86PKU */

	/* Check if we have startup arguments supplied */
	test	%r8, %r8
	jz	no_args

	/* Initialize the CPU configuration with the supplied startup args */
	movq	LCPU_SARGS_ENTRY_OFFSET(%r8), %rax
	movq	LCPU_SARGS_STACKP_OFFSET(%r8), %rsp

	jmp	jump_to_entry

no_args:
	/* Load the stack pointer and the entry address from the CPU struct */
	movq	LCPU_ENTRY_OFFSET(%rbp), %rax
	movq	LCPU_STACKP_OFFSET(%rbp), %rsp

jump_to_entry:
	/* According to System V AMD64 the stack pointer must be aligned to
	 * 16-bytes. In other words, the value (RSP+8) must be a multiple of
	 * 16 when control is transferred to the function entry point (i.e.,
	 * the compiler expects a misalignment due to the return address having
	 * been pushed onto the stack).
	 */
	andq	$~0xf, %rsp
	subq	$0x8, %rsp

	movq	%rbp, %rdi
#if !__OMIT_FRAMEPOINTER__
	/* Reset frame pointer */
	xorq	%rbp, %rbp
#endif /* !__OMIT_FRAMEPOINTER__ */

	/* Arguments for entry function
	 * arg0 @ RDI = this CPU, arg1 @ RSI = boot parameters (if available)
	 */
	jmp	*%rax

fail:
	movl	$LCPU_STATE_HALTED, LCPU_STATE_OFFSET(%rbp)

fail_loop:
	cli
1:
	hlt
	jmp	1b
END(lcpu_start64)
