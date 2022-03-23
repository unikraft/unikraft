/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *          Stefan Teodorescu <stefanl.teodorescu@gmail.com>
 *
 * Copyright (c) 2021, Karlsruhe Institute of Technology (KIT).
 *                     All rights reserved.
 * Copyright (c) 2021, University Politehnica of Bucharest.
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
#include <uk/arch/types.h>
#ifdef CONFIG_LIBUKDEBUG
#include <uk/assert.h>
#endif /* CONFIG_LIBUKDEBUG */

typedef __u64 __pte_t;		/* page table entry */

/* architecture-dependent extension for page table */
struct ukarch_pagetable {
	/* nothing */
};
#endif /* !__ASSEMBLY__ */

#ifdef CONFIG_PAGING_5LEVEL
#define PT_LEVELS			5
#else
#define PT_LEVELS			4
#endif /* !CONFIG_PAGING_5LEVEL */

#define X86_PT_LEVEL_SHIFT		9
#define X86_PT_PTES_PER_LEVEL		(1UL << X86_PT_LEVEL_SHIFT)

#define X86_PT_L0_SHIFT			12
#define X86_PT_Lx_SHIFT(lvl)					\
	(X86_PT_L0_SHIFT + (X86_PT_LEVEL_SHIFT * lvl))

#define PT_Lx_IDX(vaddr, lvl)					\
	(((vaddr) >> X86_PT_Lx_SHIFT(lvl)) & (X86_PT_PTES_PER_LEVEL - 1))

#define PT_Lx_PTES(lvl)			X86_PT_PTES_PER_LEVEL

#define PAGE_Lx_SHIFT(lvl)		X86_PT_Lx_SHIFT(lvl)

/* We use plain values here so we do not create dependencies on external helper
 * macros, which would forbit us to use the macros in functions defined further
 * down in this header.
 */
#define PAGE_LEVEL			0
#define PAGE_SHIFT			12
#define PAGE_SIZE			0x1000UL
#define PAGE_MASK			(~(PAGE_SIZE - 1))

#define PAGE_LARGE_LEVEL		1
#define PAGE_LARGE_SHIFT		21
#define PAGE_LARGE_SIZE			0x200000UL
#define PAGE_LARGE_MASK			(~(PAGE_LARGE_SIZE - 1))

#define PAGE_HUGE_LEVEL			2
#define PAGE_HUGE_SHIFT			30
#define PAGE_HUGE_SIZE			0x40000000UL
#define PAGE_HUGE_MASK			(~(PAGE_HUGE_SIZE - 1))

#define PAGE_Lx_HAS(lvl)		((lvl) <= PAGE_HUGE_LEVEL)

#define PAGE_LARGE_ALIGN_UP(addr)				\
	PAGE_Lx_ALIGN_UP(addr, PAGE_LARGE_LEVEL)
#define PAGE_LARGE_ALIGN_DOWN(addr)				\
	PAGE_Lx_ALIGN_DOWN(addr, PAGE_LARGE_LEVEL)
#define PAGE_LARGE_ALIGNED(addr)				\
	PAGE_Lx_ALIGNED(addr, PAGE_LARGE_LEVEL)

#define PAGE_HUGE_ALIGN_UP(addr)				\
	PAGE_Lx_ALIGN_UP(addr, PAGE_HUGE_LEVEL)
#define PAGE_HUGE_ALIGN_DOWN(addr)				\
	PAGE_Lx_ALIGN_DOWN(addr, PAGE_HUGE_LEVEL)
#define PAGE_HUGE_ALIGNED(addr)					\
	PAGE_Lx_ALIGNED(addr, PAGE_HUGE_LEVEL)

#if PT_LEVELS == 5
#define X86_VADDR_BITS			57
#else
#define X86_VADDR_BITS			48
#endif

#define X86_PTE_PADDR_BITS		52
#define X86_PTE_PADDR_MASK		((1UL << X86_PTE_PADDR_BITS) - 1)

#ifndef __ASSEMBLY__
/* In canonical form bit 47 is replicated into the bits [48..63]. We compute
 * this via sign extension
 */
#define X86_VADDR_CANONICALIZE(vaddr)					\
	((__vaddr_t)(((__ssz)(vaddr) << (64 - X86_VADDR_BITS)) >>	\
		(64 - X86_VADDR_BITS)))

static inline int ukarch_vaddr_range_isvalid(__vaddr_t start, __vaddr_t end)
{
#ifdef CONFIG_LIBUKDEBUG
	UK_ASSERT(start <= end);
#endif /* CONFIG_LIBUKDEBUG */

	if (X86_VADDR_CANONICALIZE(start) != start)
		return 0;

	if (X86_VADDR_CANONICALIZE(end) != end)
		return 0;

	/* Check if start and end have both the last valid bit set or unset. As
	 * both addresses are in canonical form this ensures that the range
	 * does not cross the non-canonical range
	 */
	return (((start ^ end) & (1UL << (X86_VADDR_BITS - 1))) == 0);
}
#endif /* !__ASSEMBLY__ */

/* We use a non-canonical address larger than 52 bits (max phys) */
#define X86_INVALID_ADDR		0xBAADBAADBAADBAADUL

#define __VADDR_INV			PAGE_HUGE_ALIGN_DOWN(X86_INVALID_ADDR)
#define __PADDR_INV			PAGE_HUGE_ALIGN_DOWN(X86_INVALID_ADDR)

#define PT_Lx_PTE_PADDR(pte, lvl)				\
	(((__paddr_t)(pte) & X86_PTE_PADDR_MASK) & PAGE_MASK)

#define PT_Lx_PTE_INVALID(lvl)		0x0UL

/* Note: Some flags only apply to page PTEs, not page table PTEs */
#define X86_PTE_PRESENT			0x001UL
#define X86_PTE_RW			0x002UL
#define X86_PTE_US			0x004UL
#define X86_PTE_PWT			0x008UL
#define X86_PTE_PCD			0x010UL
#define X86_PTE_ACCESSED		0x020UL
#define X86_PTE_DIRTY			0x040UL
#define X86_PTE_PAT(lvl)		((lvl) > PAGE_LEVEL ? 0x1000 : 0x80)
#define X86_PTE_PSE			0x080UL
#define X86_PTE_GLOBAL			0x100UL
#define X86_PTE_USER1_MASK		0xE00UL
#define X86_PTE_USER2_MASK		(0x7FUL << 52)
#define X86_PTE_MPK_MASK		(0xFUL << 59)
#define X86_PTE_NX			(1UL << 63)

/* For lvl > PAGE_HUGE_LEVEL the X86_PTE_PSE bit must always be 0 (resv.) */
#define PAGE_Lx_IS(pte, lvl)					\
	((lvl == PAGE_LEVEL) || ((pte) & X86_PTE_PSE))

#define PT_Lx_PTE_PRESENT(pte, lvl)				\
	((pte) & X86_PTE_PRESENT)
#define PT_Lx_PTE_CLEAR_PRESENT(pte, lvl)			\
	(pte & ~X86_PTE_PRESENT)

/* Page attributes */
#define PAGE_ATTR_PROT_NONE		0x00 /* Page is not accessible */
#define PAGE_ATTR_PROT_READ		0x01 /* Page is readable */
#define PAGE_ATTR_PROT_WRITE		0x02 /* Page is writeable */
#define PAGE_ATTR_PROT_EXEC		0x04 /* Page is executable */

#ifndef CONFIG_PARAVIRT
#ifndef __ASSEMBLY__
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

	return 0;
}

static inline __paddr_t ukarch_pt_read_base(void)
{
	__pte_t cr3;

	__asm__ __volatile__("movq %%cr3, %0" : "=r"(cr3));

	return PT_Lx_PTE_PADDR(cr3, PT_LEVELS);
}

static inline int ukarch_pt_write_base(__paddr_t pt_paddr)
{
	__asm__ __volatile__("movq %0, %%cr3" :: "r"(pt_paddr));

	return 0;
}

static inline void ukarch_tlb_flush_entry(__vaddr_t vaddr)
{
	__asm__ __volatile__("invlpg (%0)" :: "r"(vaddr) : "memory");
}

static inline void ukarch_tlb_flush(void)
{
	/* Overwriting CR3 will flush the TLB for all non-global PTEs */
	ukarch_pt_write_base(ukarch_pt_read_base());
}
#endif /* !__ASSEMBLY__ */
#endif /* !CONFIG_PARAVIRT */
