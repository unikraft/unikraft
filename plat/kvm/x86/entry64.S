/* SPDX-License-Identifier: BSD-2-Clause */
/*-
 * Copyright (c) 2016 Martin Lucina.  All Rights Reserved.
 *
 * Based on rumprun/hw arch/amd64/locore.S, which is:
 * Copyright (c) 2014, 2015 Antti Kantee.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <x86/cpu_defs.h>
#include <kvm-x86/traps.h>
#include <kvm-x86/multiboot_defs.h>

#define ENTRY(x) .globl x; .type x,%function; x:
#define END(x)   .size x, . - x

#define MYMULTIBOOT_FLAGS \
    (MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_AOUT_KLUDGE)

.section .data.boot

.align 4
_multiboot_header:
.long MULTIBOOT_HEADER_MAGIC
.long MYMULTIBOOT_FLAGS
.long -(MULTIBOOT_HEADER_MAGIC+MYMULTIBOOT_FLAGS) /* checksum */
.long _multiboot_header                           /* header addr */
.long 0x100000                                    /* load addr */
.long _edata                                      /* load end addr */
.long _end                                        /* bss end addr */
.long _libkvmplat_start32                         /* entry addr */

.section .bss

.space 4096
bootstack:

/*
 * Bootloader entry point.
 *
 * Bootstrap is slightly different from i386.  Multiboot puts us only
 * in 32bit mode, so it's our responsibility to install a page table
 * and switch to long mode.  Notably, we can't call C code until
 * we've switched to long mode.
 */
.code32
.section .text.boot

ENTRY(_libkvmplat_start32)
	cld
	movl $bootstack, %esp

	/* save multiboot info pointer at top of stack, we pop it in 64bit */
	pushl $0
	pushl %ebx

	/* only multiboot is supported for now */
	cmpl $MULTIBOOT_BOOTLOADER_MAGIC, %eax
	jne nomultiboot

	lgdt (gdt64_ptr)
	pushl $0x0
	pushw $0x10
	pushl $1f
	lret

1:	movl $0x18, %eax
	movl %eax, %ds
	movl %eax, %es
	movl %eax, %ss

	xorl %eax, %eax
	movl %eax, %fs
	movl %eax, %gs

	/*
	 * x86_64 switch to long mode
	 */

	/* 1: enable pae */
	movl %cr4, %eax
	orl $X86_CR4_PAE, %eax
	movl %eax, %cr4

	/* 2: enable long mode */
	movl $0xc0000080, %ecx
	rdmsr
	orl $X86_EFER_LME, %eax
	wrmsr

	/* 3: load pml4 pointer */
	movl $cpu_pml4, %eax
	movl %eax, %cr3

	/* 4: enable paging */
	movl %cr0, %eax
	orl $X86_CR0_PG, %eax
	movl %eax, %cr0

	/* 5: poetically longjump to longmode */
	pushw $0x08
	pushl $_libkvmplat_start64
	lret

	/* NOTREACHED */
	jmp haltme

nomultiboot:

haltme:
	cli
	hlt
	jmp haltme
END(_libkvmplat_start32)

/*
 * amd64 programmer's manual:
 *
 * "In long mode, segmentation is not used ... except for a few exceptions."
 *
 * Uuuyea, exceptions.
 */

.section .data.boot
.align 64
gdt64:
	.quad 0x0000000000000000
	.quad GDT_DESC_CODE_VAL		/* 64bit CS		*/
	.quad 0x00cf9b000000ffff	/* 32bit CS		*/
	.quad GDT_DESC_DATA_VAL		/* DS			*/
	.quad 0x0000000000000000	/* TSS part 1 (via C)	*/
	.quad 0x0000000000000000	/* TSS part 2 (via C)	*/
gdt64_end:
.align 64

.type gdt64_ptr, @object
gdt64_ptr:
	.word gdt64_end-gdt64-1
	.quad gdt64

.type mxcsr_ptr, @object
mxcsr_ptr:
	.long 0x1f80			/* Intel SDM power-on default */


#include "pagetable.S"

.code64
.section .text.boot

ENTRY(_libkvmplat_start64)
	movq $bootstack, %rsp
	xorq %rbp, %rbp

	/* We will work on cr0 and cr4 multiple times.
	 * We put cr0 into rsi and cr4 into rdi, because cpuid and
	 * xgetbv/xsetbv work on eax/ebx/ecx/edx. */
	movq %cr0, %rsi
	movq %cr4, %rdi
	/* FPU and SSE are part of base x86-64, so no need to check for their
	 * availability before enabling and initializing. */
	andl $(~(X86_CR0_EM | X86_CR0_TS)), %esi
	orl $(X86_CR0_MP | X86_CR0_NE), %esi
	movq %rsi, %cr0
	fninit
#if __SSE__
	orl $(X86_CR4_OSFXSR | X86_CR4_OSXMMEXCPT), %edi
	movq %rdi, %cr4
	ldmxcsr (mxcsr_ptr)
#endif /* __SSE__ */
	/* Check capabilities subject to availability as indicated by cpuid.
	 * First, start off with "standard features" */
	movl $0x1, %eax
	cpuid
#if __AVX__
	/* ecx and edx now contain capability information, so we can now
	 * enable capabilities based on the indicated features */
	/* OSXSAVE needs to be enabled before AVX */
	testl $(X86_CPUID1_ECX_XSAVE), %ecx
	jz noxsave
	orl $(X86_CR4_OSXSAVE), %edi
	movq %rdi, %cr4
	/* now enable AVX. This needs to be last checking cpuid features from
	 * the eax=1 cpuid call, because it clobbers ecx */
	testl $(X86_CPUID1_ECX_AVX), %ecx
	jz noavx
	xorl %ecx, %ecx
	xgetbv
	orl $(X86_XCR0_SSE | X86_XCR0_AVX), %eax
	xsetbv
noavx:
noxsave:
#endif /* __AVX__ */
	/* Now, check for extended features. */
	movl $0x7, %eax
	movl $0x0, %ecx
	cpuid
	/* ebx, ecx, edx now contain extended capabilities information. */
	/* check for and enable FS/GSBASE */
	testl $(X86_CPUID7_EBX_FSGSBASE), %ebx
	jz nofsgsbase
	orl $(X86_CR4_FSGSBASE), %edi
	movq %rdi, %cr4
nofsgsbase:
	/* done setting up CPU capabilities */

	/* read multiboot info pointer */
	movq -8(%rsp), %rdi

	pushq $0x0
	pushq $0x0

	call _libkvmplat_entry

	cli
	hlt
END(_libkvmplat_start64)

.text
ENTRY(_libkvmplat_newstack)
	movq %rdi, %rsp
	movq %rdx, %rdi

	pushq $0x0
	pushq $0x0

	call *%rsi

	cli
	hlt
END(_libkvmplat_newstack)
