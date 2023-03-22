/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_RELOC_H__
#define __UK_RELOC_H__

#if CONFIG_LIBUKRELOC
#define UKRELOC_FLAGS_PHYS_REL		(1 << 0)
#define UKRELOC_PLACEHOLDER		0xB0B0
#define UKRELOC_SIGNATURE		0x0BADB0B0
#define UKRELOC_ALIGNMENT		0x1000
#endif /* CONFIG_LIBUKRELOC */

#ifdef __ASSEMBLY__

#if CONFIG_LIBUKRELOC
/* Used to append an entry to the initial .uk_reloc section, before mkukreloc.py
 * adds the .rela.dyn entries. Furthermore, ensure the symbol is not discarded
 * and is seen as a relocation by the linker.
 *
 * @param sym
 *   The symbol to use for the relocation.
 */
#if defined(__X86_64__)
#define RELA_DYN_ENTRY_TYPE R_X86_64_64
#elif defined(__ARM_64__)
#define RELA_DYN_ENTRY_TYPE R_AARCH64_ABS64
#else
#error Selected architecture specific absolute relocations are not defined
#endif
.macro	ur_sec_updt	sym:req
.pushsection	.uk_reloc
	.quad	0x0			/* r_mem_off */
	.quad	0x0			/* r_addr */
	.reloc	., RELA_DYN_ENTRY_TYPE, \sym
	.long	0x0			/* r_sz */
	.long	0x0			/* flags */
.popsection
.endm

/*
 * Generate a unique uk_reloc symbol.
 *
 * @param sym
 *   The base symbol off which we generate the uk_reloc symbol.
 * @param val
 *   The value of the symbol to generate.
 */
.macro	ur_sym	sym:req, val:req
.globl	\sym
.set	\sym\()_\@\(), \val
.endm
#endif /* CONFIG_LIBUKRELOC */

/* If CONFIG_LIBUKRELOC is enabled, this will create a uk_reloc symbol that
 * mkukreloc.py will process. Example usage:
 * ```
 * ur_data	quad, gdt64, 8, _phys
 * ```
 * The above will make mkukreloc.py process the symbol gdt64_ukeloc_data8_phys
 * representing in memory where this data is placed and the following entry:
 * struct uk_reloc {
 *        __u64 r_mem_off = gdt64_ukeloc_data8_phys - __BASE_ADDR
 *        __u64 r_addr = gdt64 - __BASE_ADDR
 *        __u32 r_sz = 8 from gdt64_ukeloc_data[8]_phys
 *        __u32 flags = UKRELOC_FLAGS_PHYS_REL from gdt64_uk_reloc_data8[_phys]
 * } __packed;
 *
 * If CONFIG_LIBUKRELOC is not enabled then it will be simply resolved to
 * ```
 * .quad	gdt64
 * ```
 * @param type The type GAS directive, i.e. quad, long, short, etc.
 * @param sym The symbol to relocate
 * @param bytes The size in bytes of the relocation
 * @param flags Optional, if value is _phys, UKRELOC_FLAGS_PHYS_REL is set
 */
.macro	ur_data	type:req, sym:req, bytes:req, flags
#if CONFIG_LIBUKRELOC
	ur_sym	\sym\()_uk_reloc_data\bytes\()\flags\(), .
	.\type	UKRELOC_PLACEHOLDER
	ur_sec_updt	\sym
#else /* CONFIG_LIBUKRELOC */
	.\type	\sym
#endif /* !CONFIG_LIBUKRELOC */
.endm

#else  /* __ASSEMBLY__ */

#include <uk/arch/types.h>
#include <uk/essentials.h>

struct uk_reloc {
	/* Offset relative to runtime base address where to apply relocation */
	__u64 r_mem_off;
	/* Relative address value of the relocation */
	__u64 r_addr;
	/* Size of the relocation */
	__u32 r_sz;
	/* Relocation flags to change relocator behavior for this entry */
	__u32 flags;
} __packed;

UK_CTASSERT(sizeof(struct uk_reloc) == 24);

#define UKRELOC_ENTRY(ur_r_mem_off, ur_r_addr, ur_r_sz, ur_flags)	\
	{								\
		.r_mem_off	= (ur_r_mem_off),			\
		.r_addr		= (ur_r_addr),				\
		.r_sz		= (ur_r_sz),				\
		.flags		= (ur_flags),				\
	}

struct uk_reloc_hdr {
	/* Signature of the .uk_reloc section */
	__u32 signature;
	/* The uk_reloc entries to be iterated upon by the relocator */
	struct uk_reloc urs[];
} __packed;

UK_CTASSERT(sizeof(struct uk_reloc_hdr) == 4);

#endif /* !__ASSEMBLY__ */

#endif /* __UK_RELOC_H__ */
