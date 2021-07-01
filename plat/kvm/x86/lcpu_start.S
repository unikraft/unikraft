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
#include <kvm-x86/smp_defs.h>
#include <kvm-x86/traps.h>
#include <kvm-x86/asm_common.S>

#define SEC_BEGIN(x)		.globl _sec_##x##_begin; _sec_##x##_begin = .;
#define SEC_END(x)		.globl _sec_##x##_end; _sec_##x##_end = .;
#define SEC_RELOC(x, s, addr)	(x - _sec_##s##_begin + addr)

#define LOAD_ADDR16		0x8000
#define RELOC16(x)		SEC_RELOC(x, start16, LOAD_ADDR16)

/* The following section is copied to LOAD_ADDR16 at runtime */
SEC_BEGIN(start16)

.code16
ENTRY(_lcpu_start16)
	cli
	lgdt 	RELOC16(gdt32_ptr)
	movl 	%cr0, %eax
	orl 	X86_CR0_PE, %eax
	movl 	%eax, %cr0
	ljmp 	$8, $RELOC16(jump_to32)
.code32
jump_to32:
	lea 	_lcpu_start32, %eax
	jmp	*%eax
END(_lcpu_start16)

.align 32
gdt32:
	.quad 	0x0000000000000000	/* null segment	*/
gdt32_cs:
	.quad 	0x00DF9A000000FFFF	/* 32-bit CS	*/
gdt32_ds:
	.quad 	0x00DF92000000FFFF	/* data segment	*/
gdt32_end:
gdt32_ptr:
	.word 	(gdt32_end-gdt32-1)	/* size - 1	*/
	.long 	gdt32			/* gdt address	*/

.align 64
SEC_END(start16)

/* defined in plat/kvm/include/kvm-x86/asm_common.h */
GDT_64

.type mxcsr_ptr, @object
mxcsr_ptr:
	.long 0x1f80	/* Intel SDM power-on default */

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

	lgdt 	gdt64_ptr
	ljmp 	$(gdt64_cs-gdt64), $_lcpu_start64
END(_lcpu_start32)

.code64
.align 64
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

	/* Enable other CPU capabilities */
	movq 	%rbx, %r9
	PREPARE_OTHER_INSTR

	/* Use CPU_ID * X86_CPU_SIZE for indexing the cpu structure */
	movq 	%r9, %rax
	movq	$(X86_CPU_SIZE), %r10
	mulq 	%r10
	movq 	%rax, %r10

	/* Load the stack pointer and the entry address */
	leaq	cpus, %rsi
	movq	X86_CPU_ENTRY_OFFSET(%rsi, %r10, 1), %rax
	movq	X86_CPU_STACKP_OFFSET(%rsi, %r10, 1), %rsp

	test	%rax, %rax
	jz 	default

	/* Set the AP as busy */
	movq	$(LCPU_STATE_BUSY), X86_CPU_STATE_OFFSET(%rsi, %r10, 1)
	jmp	*%rax

default:
	jmp _lcpu_entry_default

	/* Not reached */
	hlt
END(_lcpu_start64)
