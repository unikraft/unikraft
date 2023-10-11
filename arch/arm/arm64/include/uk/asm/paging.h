/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Michalis Pappas <michalis.pappas@opensynergy.com>
 *          Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *
 * Copyright (c) 2022, OpenSynergy GmbH. All rights reserved.
 * Copyright (c) 2021, Karlsruhe Institute of Technology (KIT).
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

#ifndef __UKARCH_PAGING_H__
#error Do not include this header directly
#endif

#ifndef __ASSEMBLY__
#include <uk/config.h>
#include <uk/arch/paging.h>
#include <uk/arch/types.h>

#ifdef CONFIG_LIBUKDEBUG
#include <uk/assert.h>
#endif /* CONFIG_LIBUKDEBUG */

/* The implementation for arm64 supports a 48-bit virtual address space
 * with 4KiB translation granule, as defined by VMSAv8.
 *
 * Notice:
 * The Unikraft paging API uses the x86_64 convention, where pages are
 * defined at L0, and the top level table is at L3. This is the opposite
 * of the convention use by VMSAv8-A, where pages are defined at L3 and
 * the top level table is defined at L0 (48-bit) / L-1 (52-bit). This
 * implementation uses the Unikraft convention.
 */

typedef __u64 __pte_t;		/* page table entry */

/* architecture-dependent extension for page table */
struct ukarch_pagetable {
	/* nothing */
};
#endif /* !__ASSEMBLY__ */

#define PT_LEVELS			4
#define PT_PTES_PER_LEVEL		512
#define PT_LEVEL_SHIFT			9

/* We use plain values here so we do not create dependencies on external helper
 * macros, which would forbid us to use the macros in functions defined further
 * down in this header.
 */
#define PAGE_LEVEL			0
#define PAGE_SHIFT			12
#define PAGE_SIZE			0x1000UL
#define PAGE_MASK			(~(PAGE_SIZE - 1))

#define PAGE_ATTR_PROT_MASK		0x0f
#define PAGE_ATTR_PROT_SHIFT		0

#define PAGE_ATTR_PROT_NONE		0x00
#define PAGE_ATTR_PROT_READ		0x01
#define PAGE_ATTR_PROT_WRITE		0x02
#define PAGE_ATTR_PROT_EXEC		0x04

#define PAGE_ATTR_TYPE_MASK		0x07
#define PAGE_ATTR_TYPE_SHIFT		5

#define PAGE_ATTR_TYPE_NORMAL_WB	(0 << PAGE_ATTR_TYPE_SHIFT)
#define PAGE_ATTR_TYPE_NORMAL_WT	(1 << PAGE_ATTR_TYPE_SHIFT)
#define PAGE_ATTR_TYPE_NORMAL_NC	(2 << PAGE_ATTR_TYPE_SHIFT)
#define PAGE_ATTR_TYPE_DEVICE_nGnRnE	(3 << PAGE_ATTR_TYPE_SHIFT)
#define PAGE_ATTR_TYPE_DEVICE_nGnRE	(4 << PAGE_ATTR_TYPE_SHIFT)
#define PAGE_ATTR_TYPE_DEVICE_GRE	(5 << PAGE_ATTR_TYPE_SHIFT)
#define PAGE_ATTR_TYPE_NORMAL_WB_TAGGED	(6 << PAGE_ATTR_TYPE_SHIFT)

#define PAGE_ATTR_SHAREABLE_MASK	0x03
#define PAGE_ATTR_SHAREABLE_SHIFT	8

#define PAGE_ATTR_SHAREABLE_NS		(0 << PAGE_ATTR_SHAREABLE_SHIFT)
#define PAGE_ATTR_SHAREABLE_IS		(1 << PAGE_ATTR_SHAREABLE_SHIFT)
#define PAGE_ATTR_SHAREABLE_OS		(2 << PAGE_ATTR_SHAREABLE_SHIFT)

/* Page fault error code bits */
#define ARM64_PF_ESR_WnR		0x0000040UL
#define ARM64_PF_ESR_ISV		0x1000000UL

#define ARM64_PADDR_BITS		48
#define ARM64_VADDR_BITS		48

#define PT_Lx_PTES(lvl)			PT_PTES_PER_LEVEL

#define PT_Lx_PTE_PRESENT(pte, lvl)	((pte & PTE_VALID_BIT))

#define PT_Lx_PTE_CLEAR_PRESENT(pte, lvl)			\
	(pte & ~PTE_VALID_BIT)

#define PT_MAP_LEVEL_MAX		(PT_LEVELS - 2)

#define PAGE_Lx_HAS(lvl)		((lvl) <= PT_MAP_LEVEL_MAX)

#define PAGE_Lx_IS(pte, lvl)					\
	((lvl == PAGE_LEVEL) || ((pte) & PTE_TYPE_MASK) == PTE_TYPE_BLOCK)

#define PAGE_Lx_SHIFT(lvl)					\
	(PAGE_SHIFT + (PT_LEVEL_SHIFT * lvl))

#define PAGE_SHIFT_Lx(shift)					\
	(((shift) - PAGE_SHIFT) / PT_LEVEL_SHIFT)

#define PT_Lx_IDX(vaddr, lvl)					\
	(((vaddr) >> PAGE_Lx_SHIFT(lvl)) & (PT_PTES_PER_LEVEL - 1))

/* Any address controlled by TTBR1, ie bit[55] == 1 */
#define ARM64_INVALID_ADDR		0xBAADBAADBAADBAADUL

#define __VADDR_INV						\
	PAGE_Lx_ALIGN_DOWN(ARM64_INVALID_ADDR, PT_MAP_LEVEL_MAX)

#define __PADDR_INV						\
	PAGE_Lx_ALIGN_DOWN(ARM64_INVALID_ADDR, PT_MAP_LEVEL_MAX)

/* Any PTE with bit[0] == 0 is invalid */
#define PT_Lx_PTE_INVALID(lvl)		0x0UL

#define ARM64_PADDR_MAX			((1ULL << ARM64_PADDR_BITS) - 1)
#define ARM64_VADDR_MAX			((1ULL << ARM64_VADDR_BITS) - 1)

#define ARM64_PADDR_VALID(paddr)	(paddr <= (__paddr_t)ARM64_PADDR_MAX)
#define ARM64_VADDR_VALID(vaddr)				\
	((vaddr & ARM64_VADDR_MAX) <= (__vaddr_t)ARM64_VADDR_MAX)

#ifndef __ASSEMBLY__
static inline __paddr_t PT_Lx_PTE_PADDR(__pte_t pte, unsigned int lvl)
{
	__paddr_t paddr;
	static __u64 pte_lx_map_paddr_mask[] = {
		PTE_L0_PAGE_PADDR_MASK,
		PTE_L1_BLOCK_PADDR_MASK,
		PTE_L2_BLOCK_PADDR_MASK
	};

	if (PAGE_Lx_IS(pte, lvl)) {
#ifdef CONFIG_LIBUKDEBUG
		UK_ASSERT(lvl <= PT_MAP_LEVEL_MAX);
#endif /* CONFIG_LIBUKDEBUG */
		paddr = pte & pte_lx_map_paddr_mask[lvl];
	} else {
#ifdef CONFIG_LIBUKDEBUG
		UK_ASSERT(lvl > PAGE_LEVEL && lvl < PT_LEVELS);
#endif /* CONFIG_LIBUKDEBUG */
		paddr = pte & PTE_Lx_TABLE_PADDR_MASK;
	}
	return paddr;
}

static inline __paddr_t PT_Lx_PTE_SET_PADDR(__pte_t pte, unsigned int lvl,
					     __paddr_t paddr)
{
	static __u64 pte_lx_map_paddr_mask[] = {
		PTE_L0_PAGE_PADDR_MASK,
		PTE_L1_BLOCK_PADDR_MASK,
		PTE_L2_BLOCK_PADDR_MASK
	};

	if (PAGE_Lx_IS(pte, lvl)) {
#ifdef CONFIG_LIBUKDEBUG
		UK_ASSERT(lvl <= PT_MAP_LEVEL_MAX);
#endif /* CONFIG_LIBUKDEBUG */
		paddr &= pte_lx_map_paddr_mask[lvl];
		pte &= ~pte_lx_map_paddr_mask[lvl];
	} else {
#ifdef CONFIG_LIBUKDEBUG
		UK_ASSERT(lvl > PAGE_LEVEL && lvl < PT_LEVELS);
#endif /* CONFIG_LIBUKDEBUG */
		paddr &= PTE_Lx_TABLE_PADDR_MASK;
		pte &= ~PTE_Lx_TABLE_PADDR_MASK;
	}
	return pte | paddr;
}

static inline int ukarch_paddr_isvalid(__paddr_t addr)
{
	return ARM64_PADDR_VALID(addr);
}

static inline int ukarch_paddr_range_isvalid(__paddr_t start, __sz len)
{
	__paddr_t end = start + len - 1;

#ifdef CONFIG_LIBUKDEBUG
	UK_ASSERT(start <= end);
#endif /* CONFIG_LIBUKDEBUG */

	return (ARM64_PADDR_VALID(end)) && (ARM64_PADDR_VALID(start));
}

static inline int ukarch_vaddr_isvalid(__vaddr_t addr)
{
	return ARM64_VADDR_VALID(addr);
}

static inline int ukarch_vaddr_range_isvalid(__vaddr_t start, __sz len)
{
	__vaddr_t end = start + len - 1;

#ifdef CONFIG_LIBUKDEBUG
	UK_ASSERT(start <= end);
#endif /* CONFIG_LIBUKDEBUG */

	return (ARM64_VADDR_VALID(end)) && (ARM64_VADDR_VALID(start));
}

static inline int ukarch_pte_read(__vaddr_t pt_vaddr, unsigned int lvl,
				  unsigned int idx, __pte_t *pte)
{
	(void)lvl;

#ifdef CONFIG_LIBUKDEBUG
	UK_ASSERT(idx < PT_Lx_PTES(lvl));
#endif /* CONFIG_LIBUKDEBUG */

	*pte = *((__pte_t *)pt_vaddr + idx);

	return 0;
}

static inline int ukarch_pte_write(__vaddr_t pt_vaddr, unsigned int lvl,
				   unsigned int idx, __pte_t pte)
{
	(void)lvl;

#ifdef CONFIG_LIBUKDEBUG
	UK_ASSERT(idx < PT_Lx_PTES(lvl));
#endif /* CONFIG_LIBUKDEBUG */

	*((__pte_t *)pt_vaddr + idx) = pte;
	dsb(ishst);
	isb();

	return 0;
}

static inline __paddr_t ukarch_pt_read_base(void)
{
	__paddr_t reg;

	__asm__ __volatile__("mrs %x0, ttbr0_el1\n" : "=r" (reg));

	return (reg & TTBR0_EL1_BADDR_MASK);
}

static inline int ukarch_pt_write_base(__paddr_t pt_paddr)
{
	__paddr_t reg = (pt_paddr & TTBR0_EL1_BADDR_MASK);

	__asm__ __volatile__("msr ttbr0_el1, %x0\n"
			     "isb\n"
			     :: "r" (reg));
	return 0;
}

static inline void ukarch_tlb_flush_entry(__vaddr_t vaddr)
{
	__asm__ __volatile__(
		"	dsb	ishst\n"        /* wait for write complete */
		"	tlbi	vaae1is, %x0\n" /* invalidate by vaddr */
		"	dsb	ish\n"          /* wait for invalidate compl */
		"	isb\n"                  /* sync context */
		:: "r" (vaddr) : "memory");
}

static inline void ukarch_tlb_flush(void)
{
	__asm__ __volatile__(
		"	dsb	ishst\n"     /* wait for write complete */
		"	tlbi	vmalle1is\n" /* invalidate all */
		"	dsb	ish\n"       /* wait for invalidate complete */
		"	isb\n"               /* sync context */
		::: "memory");
}
#endif /* !__ASSEMBLY__ */
