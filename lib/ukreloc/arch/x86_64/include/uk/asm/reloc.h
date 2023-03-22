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

#endif /* __ASSEMBLY__ */
