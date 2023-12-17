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
#include <uk/reloc.h>
#include <kvm-x86/traps.h>
#include "lcpu_helpers.S"

#define SEC_BEGIN(x)		.globl x86_##x##_begin; x86_##x##_begin = .;
#define SEC_END(x)		.globl x86_##x##_end; x86_##x##_end = .;

#define START16_PLACEHOLDER	0x1516	/* IS16 */
#define START32_PLACEHOLDER	0x1532	/* IS32 */

.section .data.boot.16
.globl x86_start16_addr
x86_start16_addr:
	.quad	START16_PLACEHOLDER

/* Implement dedicated start16 macro's whose only use-case is this file to cope
 * with the existence of 16-bit code. This is so it does not interfere with the
 * other uses of the ur_* macro's.
 * For example, we do not want symbols for these to occupy unnecessary space in
 * .uk_reloc.
 *
 * NOTE:IF ADDING/REMOVING RELOCATIONS FROM HERE, THEN ADD/REMOVE
 *	CORRESPONDENT SYMBOL TO `lcpu.c` (see IMPORT_START16_SYM
 *	start16_helpers.h macros being used on start16 symbols)
 */
.macro mov_start16	sym:req, reg:req, bytes:req
	mov	$START16_PLACEHOLDER, \reg
.globl	\sym\()_imm\bytes\()_start16
.set	\sym\()_imm\bytes\()_start16, (. - \bytes)
	nop
.endm

.macro data_start16	type:req, sym:req, bytes:req
.globl	\sym\()_data\bytes\()_start16
.set	\sym\()_data\bytes\()_start16, .
	.\type	START16_PLACEHOLDER
.endm

/* The following section has to be copied to LOAD_ADDR16 (4KiB max!) at runtime
 * as 16-bit real mode entry point.
 * `uk_reloc_{mov|data}` may be used in start16 to be able to cope with the
 * fact that R_X86_64_16, R_X86_64_32 and R_X86_64_32S relocations are
 * incompatible with an x86-64 PIE but they will not appear in the final binary,
 * as mkukreloc.py will know they are redundant for the early relocator.
 * Instead, these uk_reloc's will be hardcoded in a corresponding locally
 * declared `struct uk_reloc` residing in the frame of the method that relocates
 * this very code, during Application Processor environment setup.
 */
.section .text.boot.16
SEC_BEGIN(start16)

.code16
ENTRY(lcpu_start16_ap)
	/* Clear pointers to startup and platform boot parameters */
	xorl	%edi, %edi
	xorl	%esi, %esi

	mov_start16	lcpu_start16, %ax, 2
	/* On start-up a core's %cs is set depending on the value of the vector
	 * inside the SIPI message, so make sure we are jumping to the
	 * proper address w.r.t. segmentation.
	 */
	movl	%cs, %ebx
	shll	$4, %ebx
	subl	%ebx, %eax
	jmp	*%eax
END(lcpu_start16_ap)

/*
 * 16-bit boot entry function
 * TODO: At the moment, this does not allow booting on a bare metal system.
 * It is only used as entry point for APs in SMP configuration.
 */
.align 16
.globl gdt32
gdt32:
/* We can repurpose the null segment to encode the GDT pointer because
 * the null segment is not accessed by the processor in 32-bit mode. We also
 * want the GDT in front of the code so that we can more easily reference the
 * symbols. So we use the two spare bytes to just jump over the GDT.
 */
gdt32_null:
	.word	0x0000
.globl gdt32_ptr
gdt32_ptr:
	.word	(gdt32_end - gdt32 - 1)	/* size - 1	*/
	data_start16	long, gdt32, 4	/* GDT address	*/
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
	mov_start16	gdt32_ptr, %ax, 2
	lgdt	(%eax)

	/* ljmp encoding has 5 opcodes, thus 40 bits, which in our case
	 * represent:
	 * [39:31] = 0xea - the identifier opcode of ljmp itself
	 * [31:15] = intersegment 2 byte immediate address to place into %eip
	 * [15: 0] = value to be placed into code segment register
	 * Thus, since we do not know the address of jump_to32 at runtime, we
	 * will generate a reloc symbol to be resolved.
	 * When we get to this point we would place into %eax the address
	 * of jump_to32. For the ljmp to jump to this address we replace the
	 * initial RELOC16(jump_to32) value with jump_to32 by patching at
	 * runtime the [31:15] fields of ljmp - known to be at an offset
	 * of -4 bytes ([15: 0] + [31:15]) from jump_to32, since this
	 * address corresponds to the very next instruction in memory
	 * after our ljmp.
	 */
	mov_start16	jump_to32, %ax, 2
	movw	%ax, -4(%eax)
	ljmp	$(gdt32_cs - gdt32), $START16_PLACEHOLDER

.code32
.globl jump_to32
jump_to32:
	/* Set up remaining segment registers */
	movl	$(gdt32_ds - gdt32), %eax
	movl	%eax, %es
	movl	%eax, %ss
	movl	%eax, %ds

	xorl	%eax, %eax
	movl	%eax, %fs
	movl	%eax, %gs

	mov_start16	lcpu_start32, %eax, 4

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
	ur_data	quad, gdt64, 8, _phys

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
	ur_mov	x86_bpt_pml4, %eax, 4, _phys
	movl	%eax, %cr3

	movl	$CR0_BOOT32_SETTINGS, %eax
	movl	%eax, %cr0

	/* Load 64-bit GDT and jump to 64-bit code segment */
	ur_mov	gdt64_ptr, %eax, 4
	lgdt	(%eax)

	/* Again, we use the same strategy, only this time we generate an actual
	 * uk_reloc entry to be automatically resolved by the early relocator,
	 * instead of relying on the code that relocates the start16 section
	 * before starting the Application Processors, since execution of
	 * lcpu_start32 comes before that.
	 */
	ur_mov	jump_to64, %eax, 4
	movl	%eax, -6(%eax)
	ljmp	$(gdt64_cs - gdt64), $START32_PLACEHOLDER

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

	leaq	lcpu_start64(%rip), %rcx
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
	 * TODO: We are leaking implementation details. `lcpus` is defined
	 * with `UKPLAT_PER_LCPU_DEFINE`. Find a way to properly access
	 * this `lcpus` here.
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
