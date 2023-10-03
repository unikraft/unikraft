/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *
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

#ifndef __PLAT_CMN_ARCH_PAGING_H__
#error Do not include this header directly
#endif

#include <uk/arch/limits.h>
#include <uk/essentials.h>
#include <uk/plat/paging.h>
#include <uk/plat/common/cpu.h>
#include <uk/fallocbuddy.h>
#include <uk/print.h>

#include <errno.h>

#include "cpu_defs.h"
#include "cpu.h"

/* We do not support 32-bit virtual address spaces as they do not provide
 * enough space for the direct mapping of page tables. In this case, we would
 * have to use temporary mappings or recursive page table mapping
 */
#if PT_LEVELS < 4
#error "Unsupported number of page table levels"
#endif

/*
 * We expect the physical memory to be mapped 1:1 into the following area in
 * every virtual address space. This allows us to easily translate from virtual
 * to physical page table addresses and vice versa. We also access the metadata
 * for the frame allocators through this area.
 */
#define DIRECTMAP_AREA_START	0xffffff8000000000 /* -512 GiB */
#define DIRECTMAP_AREA_END	0xffffffffffffffff
#define DIRECTMAP_AREA_SIZE	(DIRECTMAP_AREA_END - DIRECTMAP_AREA_START + 1)

static inline __vaddr_t
x86_directmap_paddr_to_vaddr(__paddr_t paddr)
{
	UK_ASSERT(paddr < DIRECTMAP_AREA_SIZE);
	return (__vaddr_t)paddr + DIRECTMAP_AREA_START;
}

static inline __paddr_t
x86_directmap_vaddr_to_paddr(__vaddr_t vaddr)
{
	UK_ASSERT(vaddr >= DIRECTMAP_AREA_START);
	UK_ASSERT(vaddr < DIRECTMAP_AREA_END);
	return (__paddr_t)(vaddr - DIRECTMAP_AREA_START);
}

static inline __pte_t
pgarch_pte_create(__paddr_t paddr, unsigned long attr, unsigned int level,
		  __pte_t template, unsigned int template_level __unused)
{
	__pte_t pte;

	UK_ASSERT(PAGE_ALIGNED(paddr));

	pte = paddr & X86_PTE_PADDR_MASK;
	pte |= X86_PTE_PRESENT;

	if (level > PAGE_LEVEL) {
		UK_ASSERT(level <= PAGE_HUGE_LEVEL);
		pte |= X86_PTE_PSE;
	}

	if (attr & PAGE_ATTR_PROT_WRITE)
		pte |= X86_PTE_RW;

	if (!(attr & PAGE_ATTR_PROT_EXEC))
		pte |= X86_PTE_NX;

	if (attr & PAGE_ATTR_WRITECOMBINE) {
		pte |= X86_PTE_PCD;
		pte |= X86_PTE_PWT;
	}

	/* Take all other bits from template */
	pte |= template & (X86_PTE_US |
			   X86_PTE_ACCESSED |
			   X86_PTE_DIRTY |
			   X86_PTE_GLOBAL |
			   X86_PTE_USER1_MASK |
			   X86_PTE_USER2_MASK |
			   X86_PTE_MPK_MASK);

	return pte;
}

static inline __pte_t
pgarch_pte_change_attr(__pte_t pte, unsigned long new_attr,
		       unsigned int level __unused)
{
	pte &= ~(X86_PTE_RW | X86_PTE_NX | X86_PTE_PCD | X86_PTE_PWT);

	if (new_attr & PAGE_ATTR_PROT_WRITE)
		pte |= X86_PTE_RW;

	if (!(new_attr & PAGE_ATTR_PROT_EXEC))
		pte |= X86_PTE_NX;

	if (new_attr & PAGE_ATTR_WRITECOMBINE) {
		pte |= X86_PTE_PCD;
		pte |= X86_PTE_PWT;
	}

	return pte;
}

static inline unsigned long
pgarch_attr_from_pte(__pte_t pte, unsigned int level __unused)
{
	unsigned long attr = PAGE_ATTR_PROT_READ;

	if (pte & X86_PTE_RW)
		attr |= PAGE_ATTR_PROT_WRITE;

	if (!(pte & X86_PTE_NX))
		attr |= PAGE_ATTR_PROT_EXEC;

	if ((pte & X86_PTE_PWT) && (pte & X86_PTE_PCD))
		attr |= PAGE_ATTR_WRITECOMBINE;

	return attr;
}

/*
 * Page tables
 */
static inline __vaddr_t
pgarch_pt_pte_to_vaddr(struct uk_pagetable *pt __unused, __pte_t pte,
		       unsigned int level __maybe_unused)
{
	return x86_directmap_paddr_to_vaddr(PT_Lx_PTE_PADDR(pte, level));
}

static inline __pte_t
pgarch_pt_pte_create(struct uk_pagetable *pt __unused, __paddr_t pt_paddr,
		     unsigned int level __unused, __pte_t template,
		     unsigned int template_level __unused)
{
	__pte_t pt_pte;

	UK_ASSERT(PAGE_ALIGNED(pt_paddr));

	pt_pte = pt_paddr & X86_PTE_PADDR_MASK;

	/* We do not apply any restrictive protections in PT PTEs but control
	 * protections in the PTEs mapping pages only
	 */
	pt_pte |= (X86_PTE_PRESENT | X86_PTE_RW);

	/* Do not use the PWT/PCD bits for the PT PTEs. We only use them for
	 * page PTEs
	 */
	pt_pte &= ~(X86_PTE_PWT | X86_PTE_PCD);

	/* Take all other bits from template. We also keep the flags that are
	 * ignored by the architecture. The caller might have stored custom
	 * data in these fields
	 */
	pt_pte |= template & (X86_PTE_US |
			      X86_PTE_ACCESSED |
			      X86_PTE_DIRTY | /* ignored */
			      X86_PTE_GLOBAL | /* ignored */
			      X86_PTE_USER1_MASK |
			      X86_PTE_USER2_MASK |
			      X86_PTE_MPK_MASK /* ignored */
			      );

	return pt_pte;
}

static inline __vaddr_t
pgarch_pt_map(struct uk_pagetable *pt __unused, __paddr_t pt_paddr,
	      unsigned int level __unused)
{
	return x86_directmap_paddr_to_vaddr(pt_paddr);
}

static inline __paddr_t
pgarch_pt_unmap(struct uk_pagetable *pt __unused, __vaddr_t pt_vaddr,
		unsigned int level __unused)
{
	return x86_directmap_vaddr_to_paddr(pt_vaddr);
}

/* Temporary kernel mapping */
static inline __vaddr_t
pgarch_kmap(struct uk_pagetable *pt __unused, __paddr_t paddr,
	    __sz len __unused)
{
	return x86_directmap_paddr_to_vaddr(paddr);
}

static inline void
pgarch_kunmap(struct uk_pagetable *pt __unused, __vaddr_t vaddr __unused,
	      __sz len __unused)
{
	/* nothing to do */
}

#define X86_PG_VADDR_SHIFT		8
#define X86_PG_VADDR_BITS		16
#define X86_PG_VADDR_MASK					\
	(((1UL << X86_PG_VADDR_BITS) - 1) << X86_PG_VADDR_SHIFT)

#define X86_PG_PADDR_SHIFT		0
#define X86_PG_PADDR_BITS		8
#define X86_PG_PADDR_MASK					\
	(((1UL << X86_PG_PADDR_BITS) - 1) << X86_PG_PADDR_SHIFT)

static __paddr_t x86_pg_maxphysaddr;

#define X86_PG_VALID_PADDR(paddr)	((paddr) <= x86_pg_maxphysaddr)

int ukarch_paddr_range_isvalid(__paddr_t start, __paddr_t end)
{
	UK_ASSERT(start <= end);
	return (X86_PG_VALID_PADDR(start) && X86_PG_VALID_PADDR(end));
}

static inline int
pgarch_init(void)
{
	__u32 eax, ebx, ecx, edx;
	__u32 max_addr_bit;
	__u64 efer;

	/* Check for availability of extended features */
	ukarch_x86_cpuid(0x80000000, 0, &eax, &ebx, &ecx, &edx);
	if (eax < 0x80000008)
		return -ENOTSUP;

	/* Check for 1GiB page support. We assume 64-bit and NX support */
	ukarch_x86_cpuid(0x80000001, 0, &eax, &ebx, &ecx, &edx);

	UK_ASSERT(edx & X86_CPUID81_LM);
	UK_ASSERT(edx & X86_CPUID81_NX);

	if (unlikely(!(edx & X86_CPUID81_PAGE1GB))) {
		uk_pr_crit("%s not supported.\n", "1GiB pages");
		return -ENOTSUP;
	}

	/* Enable the NX bit */
	efer = rdmsrl(X86_MSR_EFER);
	efer |= X86_EFER_NXE;
	wrmsrl(X86_MSR_EFER, efer);

#if PT_LEVELS == 5
	ukarch_x86_cpuid(0x7, 0, &eax, &ebx, &ecx, &edx);

	if (unlikely(!(ecx & __X86_CPUID7_ECX_LA57))) {
		uk_pr_crit("%s not supported.\n", "5-level paging");
		return -ENOTSUP;
	}
#endif /* PT_LEVELS == 5 */

	/* Check for PAT support */
	ukarch_x86_cpuid(0x1, 0, &eax, &ebx, &ecx, &edx);
	if (unlikely(!(edx & X86_CPUID1_EDX_PAT))) {
		uk_pr_crit("Page table attributes are not supported.\n");
		return -ENOTSUP;
	}
	/* Reset PAT to default value */
	wrmsrl(X86_MSR_PAT, X86_PAT_DEFAULT);

	ukarch_x86_cpuid(0x80000008, 0, &eax, &ebx, &ecx, &edx);

	max_addr_bit = (eax & X86_PG_VADDR_MASK) >> X86_PG_VADDR_SHIFT;
	if (unlikely(max_addr_bit < X86_VADDR_BITS)) {
		uk_pr_crit("%d-bit %s addresses not supported.\n",
			   X86_VADDR_BITS, "virtual");
		return -ENOTSUP;
	}

	max_addr_bit = (eax & X86_PG_PADDR_MASK) >> X86_PG_PADDR_SHIFT;
	x86_pg_maxphysaddr = (1UL << max_addr_bit) - 1;

	return 0;
}

static inline int
pgarch_pt_add_mem(struct uk_pagetable *pt, __paddr_t start, __sz len)
{
	unsigned long pages = len >> PAGE_SHIFT;
	void *fa_meta;
	__sz fa_meta_size;
	__vaddr_t dm_off;
	int rc;

	UK_ASSERT(start <= __PADDR_MAX - len);
	UK_ASSERT(ukarch_paddr_range_isvalid(start, start + len));

	/* Reserve space for the metadata at the beginning of the area. Note
	 * that the metadata area will be a bit too large because we eat away
	 * from the frames by placing the metadata in the first frames.
	 */
	fa_meta = (void *)x86_directmap_paddr_to_vaddr(start);
	fa_meta_size = uk_fallocbuddy_metadata_size(pages);

	if (unlikely(fa_meta_size >= len))
		return -ENOMEM;

	start = PAGE_ALIGN_UP(start + fa_meta_size);
	pages = (len - fa_meta_size) >> PAGE_SHIFT;

	dm_off = x86_directmap_paddr_to_vaddr(start);

	UK_ASSERT(pt->fa->addmem);
	rc = pt->fa->addmem(pt->fa, fa_meta, start, pages, dm_off);
	if (unlikely(rc))
		return rc;

	return 0;
}

static inline int
pgarch_pt_init(struct uk_pagetable *pt, __paddr_t start, __sz len)
{
	__sz fa_size;
	int rc;

	/* Reserve space for the allocator at the beginning of the area. */
	pt->fa = (struct uk_falloc *)x86_directmap_paddr_to_vaddr(start);

	fa_size = ALIGN_UP(uk_fallocbuddy_size(), 8);
	if (unlikely(fa_size >= len))
		return -ENOMEM;

	rc = uk_fallocbuddy_init(pt->fa);
	if (unlikely(rc))
		return rc;

	rc = pgarch_pt_add_mem(pt, start + fa_size, len - fa_size);
	if (unlikely(rc))
		return rc;

	return 0;
}
