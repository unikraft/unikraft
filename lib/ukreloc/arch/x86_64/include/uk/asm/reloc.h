/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_RELOC_H__
#error Do not include this header directly
#endif

#ifdef __ASSEMBLY__

/* Relocation-friendly ur_mov to replace mov instructions incompatible with
 * a PIE binary. Usage example:
 * ```
 * ur_mov	gdt64_ptr, %eax, 4
 * ```
 * The above will get mkukreloc.py to process the symbol gdt64_ptr_ukeloc_imm4
 * representing in memory where this data is placed and the following entry:
 * struct uk_reloc {
 *        __u64 r_mem_off = gdt64_ptr_ukeloc_imm4 - __BASE_ADDR
 *        __u64 r_addr = gdt64_ptr - __BASE_ADDR
 *        __u32 r_sz = 4 from gdt64_ptr_ukeloc_imm[4]
 *        __u32 flags = 0
 * } __packed;
 *
 * If CONFIG_LIBUKRELOC is not enabled then it will be simply resolved to
 * ```
 * mov	$gdt64_ptr, %eax
 * ```
 *
 * @param sym The symbol to relocate
 * @param req The register into which to place the value
 * @param bytes The size in bytes of the relocation
 * @param flags Optional, if value is _phys, UKRELOC_FLAGS_PHYS_REL is set
 */
.macro	ur_mov	sym:req, reg:req, bytes:req, flags
#if CONFIG_LIBUKRELOC
/* UKRELOC_PLACEHODER is 16 bits, so in 64-bit code we must force a `movabs`
 * to ensure that the last amount of opcodes are meant for the immediate
 */
.ifeq  (8 - \bytes)
	movabs	$UKRELOC_PLACEHOLDER, \reg
.endif
.ifgt  (8 - \bytes)
	mov	$UKRELOC_PLACEHOLDER, \reg
.endif
	ur_sym	\sym\()_uk_reloc_imm\bytes\()\flags\(), (. - \bytes)
	nop
	ur_sec_updt	\sym
#else  /* CONFIG_LIBUKRELOC */
	mov	$\sym, \reg
#endif /* !CONFIG_LIBUKRELOC */
.endm

#define SAVE_REG32_COUNT				6

/* Expects the lower 32 bits of the base virtual address in %edi
 * and the higher 32 bt in %esi (hopefully a KASLR randomized value).
 * Also place the base load address in %edx. The macro can either create its
 * own scratch stack to save register state or use the already existing one,
 * if there.
 *
 * NOTE: Unlike its C-variant, do_uk_reloc(), this does not relocate
 * the kernel's static, build-time memory region descriptors!
 * Therefore, these memory regions must be manually relocated somewhere in the
 * early code before the list of memory regions is modified (insertions,
 * deletions, sorting operations, modification of any sort, etc.)!
 * See reloc.h for more details. ONLY USE THESE FUNCTIONS IF YOU KNOW WHAT
 * YOU ARE DOING!
 *
 * @param have_stack Boolean value to tell whether the caller already has a
 *                   stack available or not
 */
.macro	do_uk_reloc32	have_stack:req
#if CONFIG_LIBUKRELOC
.code32
.align	8
.if !\have_stack
	/* Setup uk_reloc32 scratch stack of size SAVE_REG32_COUNT */
	movl	%edx, %esp
	subl	$(_base_addr - start_uk_reloc32), %esp

	jmp	start_uk_reloc32

.align	8
uk_reloc32_stack:
/* Force the linker to statically resolve these relocations, thus handing us
 * the static linking absolute address of _uk_reloc_start (beginning of .uk_reloc)
 * and of _base_addr, because the final image always contains the relocations
 * resolved with the absolute addresses, except for those resulted from the
 * `ur_*` macro's. Thus, here we basically insert an entry into .rela.dyn with
 * the values below, and it is left to us, at runtime, to subtract from them
 * the reference static absolute base address (_base_addr) and add the runtime
 * base address (our passed %edx) to obtain the runtime address of a symbol.
 */
.reloc	., R_X86_64_64, _uk_reloc_start
.quad	0x0  /* Make room for the relocation to not overlap with the stack */
.reloc	., R_X86_64_64, _base_addr
.quad	0x0  /* Make room for the relocation to not overlap with the stack */
.rept	SAVE_REG32_COUNT
.long	0x0
.endr

.endif

.align	8
start_uk_reloc32:
	/* Preserve caller's registers.
	 * Place the final 8-byte relocation base virtual address
	 * at +8(%esp) and the base physical address at +12(%esp).
	 * Since we are in Protected Mode, it is safe to assume that
	 * our physical load base address can fix into four bytes.
	 */
	pushl	%eax
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%edi
	pushl	%esi

	/* Place load paddr into %esi, assuming _base_addr as first
	 * loaded symbol
	 */
	movl	%edx, %esi

	/* Access resolved _base_addr and _uk_reloc_start */
	subl	$16, %esp
	/* Statically resolved _uk_reloc_start */
	popl	%ebx
	/* Subtract statically resolved _base_addr */
	subl	4(%esp), %ebx
	/* Add the runtime base address to obtain final runtime address
	 * of .uk_reloc
	 */
	addl	%esi, %ebx
	/* Return stack to backed up registers */
	addl	$12, %esp

	cmpl	$UKRELOC_SIGNATURE, (%ebx)
	jne	.finish_uk_reloc32

	/* Skip UKRELOC_SIGNATURE and go to uk_reloc entries */
	addl	$4, %ebx
.foreach_uk_reloc32:
	xorl	%ecx, %ecx
	/* Store r_sz in %ecx */
	movb	16(%ebx), %cl
	/* Check whether we reached sentinel or not */
	test	%ecx, %ecx
	jz	.finish_uk_reloc32
	movl	%esi, %edx
	/* Add m_off to load vaddr */
	addl	0(%ebx), %edx
	/* Check for relocation relative to physical load base address */
	xorl	%eax, %eax
	movb	20(%ebx), %al
	test	%eax, UKRELOC_FLAGS_PHYS_REL
	jnz	.uk_reloc32_phys

	/* Store lower 32 bits of r_val_off in %eax */
	movl	8(%ebx), %eax
	/* Store higher 32 bits of r_val_off in %edi */
	movl	12(%ebx), %edi
	/* Add lower 32 bits load vaddr to r_val_off */
	addl	4(%esp), %eax
	/* If the offset is so big that adding the two lower 32-bits values
	 * results in a CF flag being set (highly unlikely, but still)
	 * add the carry to %edi
	 */
	jnc	.uk_reloc32_r_val_off_no_carry
	inc	%edi

.uk_reloc32_r_val_off_no_carry:
	/* Add higher 32 bits load vaddr to r_val_off */
	addl	0(%esp), %edi
	jmp	.foreach_r_val_off_32
.uk_reloc32_phys:
	/* For a physical relocation, since we are in 32-bit mode with no MMU,
	 * the higher 32 bits of the relocation should be 0, otherwise you
	 * must have done something wrong :).
	 */
	/* Store lower 32 bits of r_val_off in %eax */
	movl	8(%ebx), %eax
	/* Add load paddr to r_val_off */
	addl	8(%esp), %eax
	/* Zero-out the supposed higher 32 bits value */
	xorl	%edi, %edi

/* We now have in %eax the relocation value, in %ecx the byte count and in %edx
 * the place in memory where we have to place the relocation
 */
.foreach_r_val_off_32:
	movb	%al, 0(%edx)
	inc	%edx
	shr	$8, %eax
	jnz	.foreach_r_val_off_32

	xchg	%edi, %eax
	test	%edi, %edi
	jnz	.foreach_r_val_off_32

	addl	$0x18, %ebx
	jmp	.foreach_uk_reloc32

.finish_uk_reloc32:
	/* Restore caller's registers */
	popl	%esi
	popl	%edi
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%eax
#endif /* CONFIG_LIBUKRELOC */
.endm

#endif /* __ASSEMBLY__ */
