/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *          Stefan Teodorescu <stefanl.teodorescu@gmail.com>
 *          Eduard Vintila <eduard.vintila47@gmail.com>
 *
 * Copyright (c) 2021, Karlsruhe Institute of Technology (KIT).
 *                     All rights reserved.
 * Copyright (c) 2021, University Politehnica of Bucharest.
 *                     All rights reserved.
 * Copyright (c) 2022, University of Bucharest. All rights reserved.
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
#include <riscv/cpu.h>
#include <riscv/cpu_defs.h>
#ifdef CONFIG_LIBUKDEBUG
#include <uk/assert.h>
#endif /* CONFIG_LIBUKDEBUG */

typedef __u64 __pte_t;		/* page table entry */

/* architecture-dependent extension for page table */
struct ukarch_pagetable {
	/* nothing */
};
#endif /* !__ASSEMBLY__ */

/* Sv39 mode */
#define PT_LEVELS			3

#define RISCV64_PT_LEVEL_SHIFT		    9
#define RISCV64_PT_PTES_PER_LEVEL		(1UL << RISCV64_PT_LEVEL_SHIFT)

#define RISCV64_PT_L0_SHIFT			12
#define RISCV64_PT_Lx_SHIFT(lvl)	\
	(RISCV64_PT_L0_SHIFT + (RISCV64_PT_LEVEL_SHIFT * (lvl)))
#define RISCV64_PT_SHIFT_Lx(shift)					\
	(((shift) - RISCV64_PT_L0_SHIFT) / RISCV64_PT_LEVEL_SHIFT)

#define PT_Lx_IDX(vaddr, lvl)					\
	(((vaddr) >> RISCV64_PT_Lx_SHIFT(lvl)) &		\
	 (RISCV64_PT_PTES_PER_LEVEL - 1))

#define PT_Lx_PTES(lvl)			RISCV64_PT_PTES_PER_LEVEL

#define PAGE_Lx_SHIFT(lvl)		RISCV64_PT_Lx_SHIFT(lvl)
#define PAGE_SHIFT_Lx(shift)

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

#define RISCV64_VADDR_BITS			39
#define RISCV64_PTE_PPN_MASK		0x3ffffffffffc00UL
#define RISCV64_PTE_PPN_SHIFT		10

#ifndef __ASSEMBLY__
/* In canonical form bit 38 is replicated into the bits [39..63]. We compute
 * this via sign extension
 */
#define RISCV64_VADDR_CANONICALIZE(vaddr)	\
	((__vaddr_t)(((__ssz)(vaddr) << (64 - RISCV64_VADDR_BITS)) >>	\
		(64 - RISCV64_VADDR_BITS)))

static inline int ukarch_vaddr_range_isvalid(__vaddr_t start, __vaddr_t end)
{
#ifdef CONFIG_LIBUKDEBUG
	UK_ASSERT(start <= end);
#endif /* CONFIG_LIBUKDEBUG */

	if (RISCV64_VADDR_CANONICALIZE(start) != start)
		return 0;

	if (RISCV64_VADDR_CANONICALIZE(end) != end)
		return 0;

	/* Check if start and end have both the last valid bit set or unset. As
	 * both addresses are in canonical form this ensures that the range
	 * does not cross the non-canonical range
	 */
	return (((start ^ end) & (1UL << (RISCV64_VADDR_BITS - 1))) == 0);
}
#endif /* !__ASSEMBLY__ */

#define RISCV64_PADDR_TO_PPN(paddr) ((__paddr_t)(paddr) >> PAGE_SHIFT)
#define RISCV64_PPN_TO_PADDR(ppn) ((__paddr_t)(ppn) << PAGE_SHIFT)
#define RISCV64_PTE_PPN(pte)	\
	(((pte)&RISCV64_PTE_PPN_MASK) >> RISCV64_PTE_PPN_SHIFT)
#define RISCV64_PTE_PADDR(pte) RISCV64_PPN_TO_PADDR(RISCV64_PTE_PPN(pte))
#define RISCV64_PPN_TO_PTE_PPN(ppn)	\
	((__pte_t)((ppn << RISCV64_PTE_PPN_SHIFT) & RISCV64_PTE_PPN_MASK))
#define RISCV64_PADDR_TO_PTE_PPN(paddr)	\
	RISCV64_PPN_TO_PTE_PPN(RISCV64_PADDR_TO_PPN(paddr))

#define RISCV64_SET_SATP_MODE(mode)	\
	((((__u64)(mode)) << SATP64_MODE_SHIFT) & SATP64_MODE)
#define RISCV64_SET_SATP_ASID(asid)	\
	((((__u64)(asid)) << SATP64_ASID_SHIFT) & SATP64_ASID)
#define RISCV64_SET_SATP_PPN(ppn) ((__u64)(ppn)&SATP64_PPN)

/* Non-canonical address larger than 56 bits (max phys) */
#define RISCV64_INVALID_ADDR		0xBAADBAADBAADBAADUL

#define __VADDR_INV		PAGE_HUGE_ALIGN_DOWN(RISCV64_INVALID_ADDR)
#define __PADDR_INV		PAGE_HUGE_ALIGN_DOWN(RISCV64_INVALID_ADDR)

#define PT_Lx_PTE_PADDR(pte, lvl)	RISCV64_PTE_PADDR(pte)

#define PT_Lx_PTE_INVALID(lvl)		0x0UL

#define RISCV64_PTE_VALID (1UL << 0)
#define RISCV64_PTE_INVALID 0x0UL
#define RISCV64_PTE_GLOBAL (1UL << 5)
#define RISCV64_PTE_READ (1UL << 1)
#define RISCV64_PTE_WRITE (1UL << 2)
#define RISCV64_PTE_EXEC (1UL << 3)
#define RISCV64_PTE_RW (RISCV64_PTE_READ | RISCV64_PTE_WRITE)
#define RISCV64_PTE_RX (RISCV64_PTE_READ | RISCV64_PTE_EXEC)
#define RISCV64_PTE_LINK 0x0UL
#define RISCV64_PTE_VALID_LINK (RISCV64_PTE_VALID | RISCV64_PTE_LINK)
#define RISCV64_PTE_XWR_MASK 0x0EUL
#define RISCV64_PTE_USER (1UL << 4)
#define RISCV64_PTE_ACCESSED (1UL << 6)
#define RISCV64_PTE_DIRTY (1UL << 7)
#define RISCV64_PTE_RSW_MASK (3UL << 8)


/* If one of the X, W, R bits is set, then the pte describes a page. */
#define PAGE_Lx_IS(pte, lvl)					\
	((lvl == PAGE_LEVEL) || ((pte) & RISCV64_PTE_XWR_MASK))

#define PT_Lx_PTE_PRESENT(pte, lvl)				\
	((pte) & RISCV64_PTE_VALID)
#define PT_Lx_PTE_CLEAR_PRESENT(pte, lvl)			\
	(pte & ~RISCV64_PTE_VALID)

/* Page attributes */
#define PAGE_ATTR_PROT_NONE		0x00 /* Page is not accessible */
#define PAGE_ATTR_PROT_READ		0x01 /* Page is readable */
#define PAGE_ATTR_PROT_WRITE	0x02 /* Page is writeable */
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

static inline void ukarch_tlb_flush(void)
{
	__asm__ __volatile__("sfence.vma x0, x0" ::: "memory");
}

static inline void ukarch_tlb_flush_entry(__vaddr_t vaddr)
{
	__asm__ __volatile__("sfence.vma %0, x0" : : "r" (vaddr) : "memory");
}

static inline __paddr_t ukarch_pt_read_base(void)
{
	__pte_t satp;

	satp = _csr_read(CSR_SATP);

	return RISCV64_PPN_TO_PADDR(satp & SATP64_PPN);
}

static inline int ukarch_pt_write_base(__paddr_t pt_paddr)
{
	__pte_t satp = RISCV64_SET_SATP_MODE(SATP_MODE_SV39)
		       | RISCV64_SET_SATP_ASID(0)
		       | RISCV64_SET_SATP_PPN(RISCV64_PADDR_TO_PPN(pt_paddr));

	_csr_write(CSR_SATP, satp);
	ukarch_tlb_flush();

	return 0;
}

#endif /* !__ASSEMBLY__ */
#endif /* !CONFIG_PARAVIRT */
