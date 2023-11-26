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

#include <asm/reloc.h>

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

/**
 * For proper positional independence we require that whatever page table
 * related entries in the static page table we may have, they must be
 * relocatable against a dynamic physical address. In addition to what
 * `ur_data` does, `ur_pte` also creates a uk_reloc symbol with `pte_attr0`
 * suffix, containing the value of the PTE attribute. Example usage:
 * ```
 * ur_pte	x86_bpt_pd0_0, PTE_RW
 * ```
 * The above will make mkukreloc.py process the symbol
 * x86_bpt_pd0_0_ukeloc_data8_phys representing in memory where this data is
 * placed, x86_bpt_pd0_0_ukeloc_pte_attr0 representing the value of the
 * page table entry attributes and the following entry:
 * struct uk_reloc {
 *        __u64 r_mem_off = x86_bpt_pd0_0_ukeloc_data8_phys - __BASE_ADDR
 *        __u64 r_addr = x86_bpt_pd0_0 - __BASE_ADDR + PTE_RW
 *        __u32 r_sz = 8 from x86_bpt_pd0_0_ukeloc_data[8]_phys
 *        __u32 flags = UKRELOC_FLAGS_PHYS_REL from x86_bpt_pd0_0_uk_reloc_data8[_phys]
 * } __packed;
 *
 * @param sym The symbol to relocate
 * @param pte The page table entry's attribute flags
 */
.macro	ur_pte	pte_sym:req, pte:req
#if CONFIG_LIBUKRELOC
	ur_data	quad, \pte_sym, 8, _phys
	ur_sym	\pte_sym\()_uk_reloc_pte_attr0, \pte
#else /* CONFIG_LIBUKRELOC */
	ur_data	quad, (\pte_sym + \pte), 8, _phys
#endif /* !CONFIG_LIBUKRELOC */
.endm

#if !CONFIG_LIBUKRELOC
.align 4
do_uk_reloc:
	ret
#endif /* !CONFIG_LIBUKRELOC */

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

/* Misaligned access here is never going to happen for a non-x86 architecture
 * as there are no uk_reloc_imm relocation types defined for them.
 * We need this for x86 to patch early boot code, so it's a false positive.
 * An alignment exception (#AC if CR0.AM=1 and RFLAGS.AC=1) on x86 can only
 * occur in userspace, which Unikraft does not deal with anyway.
 * If someone, in the future, adds a uk_reloc type that allows
 * misalignments on architectures that do not allow this, it's most likely
 * not needed and an alternative solution should be considered.
 *
 * @param ur Pointer to the uk_reloc entry to apply
 * @param val The value of the relocation
 * @param baddr The base address relative to which the relocation is applied
 */
#if defined(__X86_64__)
#define X86_64_NO_SANITIZE_ALIGNMENT __attribute__((no_sanitize("alignment")))
#else
#define X86_64_NO_SANITIZE_ALIGNMENT
#endif
static inline void X86_64_NO_SANITIZE_ALIGNMENT
apply_uk_reloc(struct uk_reloc *ur, __u64 val, void *baddr)
{
	switch (ur->r_sz) {
	case 2:
		*(__u16 *)((__u8 *)baddr + ur->r_mem_off) = (__u16)val;
		break;
	case 4:
		*(__u32 *)((__u8 *)baddr + ur->r_mem_off) = (__u32)val;
		break;
	case 8:
		*(__u64 *)((__u8 *)baddr + ur->r_mem_off) = (__u64)val;
		break;
	}
}

/* Relocates initial mkbootinfo.py memory regions and applies all uk_reloc
 * entries in the current image's .uk_reloc section.
 *
 * @param r_paddr The physical address relative to which we apply a relocation,
 *                used if UKRELOC_FLAGS_PHYS_REL flag is set.
 *		  If 0, the current runtime base address is used.
 * @param r_vaddr The virtual address relative to which we apply a relocation.
 *		  If 0, the current runtime base address is used.
 */
#if CONFIG_LIBUKRELOC
void do_uk_reloc(__paddr_t r_paddr, __vaddr_t r_vaddr);
#else  /* CONFIG_LIBUKRELOC */
static inline void do_uk_reloc(__paddr_t r_paddr __unused,
			       __vaddr_t r_vaddr __unused) { }
#endif /* !CONFIG_LIBUKRELOC */

/* This relocates the UKPLAT_MEMRT_KERNEL memory region descriptors in the same
 * manner do_uk_reloc does for the .uk_reloc ELF section.
 *
 * NOTE: THIS COULD CAUSE THE MEMORY REGION DESCRIPTORS LIST TO BECOME OUT OF
 *       ORDER W.R.T. UKPLAT_MEMRT_KERNEL MEMORY REGION DESCRIPTORS. THIS SHOULD
 *       BE FOLLOWED BY A CALL TO A COALESCING/SORTING METHOD.
 *       ONLY USE THIS IF YOU KNOW WHAT YOU ARE DOING!
 */
#if CONFIG_LIBUKRELOC
void do_uk_reloc_kmrds(__paddr_t r_paddr, __vaddr_t r_vaddr);
#else  /* CONFIG_LIBUKRELOC */
static inline void do_uk_reloc_kmrds(__paddr_t r_paddr __unused,
				     __vaddr_t r_vaddr __unused) { }
#endif /* !CONFIG_LIBUKRELOC */

#endif /* !__ASSEMBLY__ */

#endif /* __UK_RELOC_H__ */
