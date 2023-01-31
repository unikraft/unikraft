/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *          Eduard Vintila <eduard.vintila47@gmail.com>
 *
 * Copyright (c) 2021, Karlsruhe Institute of Technology (KIT).
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

#ifndef __PLAT_CMN_ARCH_PAGING_H__
#error Do not include this header directly
#endif

#include <uk/arch/limits.h>
#include <uk/essentials.h>
#include <uk/plat/paging.h>
#include <uk/fallocbuddy.h>
#include <uk/print.h>

#include <errno.h>

/*
 * Only the Sv39 virtual addressing mode is supported for the moment.
 */
#if PT_LEVELS != 3
#error "Unsupported number of page table levels"
#endif

/*
 * We expect the physical memory to be mapped 1:1 into the following area in
 * every virtual address space. This allows us to easily translate from virtual
 * to physical page table addresses and vice versa. We also access the metadata
 * for the frame allocators through this area.
 */
#define DIRECTMAP_AREA_START   \
	RISCV64_VADDR_CANONICALIZE(0x4000000000) /* 256 GiB */
#define DIRECTMAP_AREA_END	   \
	RISCV64_VADDR_CANONICALIZE(0x7fffffffff) /* 512 GiB */
#define DIRECTMAP_AREA_SIZE	(DIRECTMAP_AREA_END - DIRECTMAP_AREA_START + 1)

static inline __vaddr_t
riscv64_directmap_paddr_to_vaddr(__paddr_t paddr)
{
	UK_ASSERT(paddr < DIRECTMAP_AREA_SIZE);
	return (__vaddr_t)paddr + DIRECTMAP_AREA_START;
}

static inline __paddr_t
riscv64_directmap_vaddr_to_paddr(__vaddr_t vaddr)
{
	UK_ASSERT(vaddr >= DIRECTMAP_AREA_START);
	UK_ASSERT(vaddr < DIRECTMAP_AREA_END);
	return (__paddr_t)(vaddr - DIRECTMAP_AREA_START);
}

static inline __pte_t
pgarch_pte_create(__paddr_t paddr, unsigned long attr,
	unsigned int level __unused,  __pte_t template,
	unsigned int template_level __unused)
{
	__pte_t pte;

	UK_ASSERT(PAGE_ALIGNED(paddr));

	pte = RISCV64_PADDR_TO_PTE_PPN(paddr);
	pte |= (RISCV64_PTE_VALID | RISCV64_PTE_GLOBAL);

	if (attr & PAGE_ATTR_PROT_READ)
		pte |= RISCV64_PTE_READ;

	if (attr & PAGE_ATTR_PROT_WRITE)
		pte |= RISCV64_PTE_WRITE;

	if (attr & PAGE_ATTR_PROT_EXEC)
		pte |= RISCV64_PTE_EXEC;

	/* Take all other bits from template */
	pte |= template & (RISCV64_PTE_USER |
			   RISCV64_PTE_ACCESSED |
			   RISCV64_PTE_DIRTY |
			   RISCV64_PTE_RSW_MASK);

	return pte;
}

static inline __pte_t
pgarch_pte_change_attr(__pte_t pte, unsigned long new_attr,
		       unsigned int level __unused)
{
	pte &= ~(RISCV64_PTE_RW | RISCV64_PTE_EXEC);

	if (new_attr & PAGE_ATTR_PROT_READ)
		pte |= RISCV64_PTE_READ;

	if (new_attr & PAGE_ATTR_PROT_WRITE)
		pte |= RISCV64_PTE_WRITE;

	if (new_attr & PAGE_ATTR_PROT_EXEC)
		pte |= RISCV64_PTE_EXEC;

	return pte;
}

static inline unsigned long
pgarch_attr_from_pte(__pte_t pte, unsigned int level __unused)
{
	unsigned long attr = PAGE_ATTR_PROT_READ;

	if (pte & RISCV64_PTE_READ)
		attr |= PAGE_ATTR_PROT_READ;

	if (pte & RISCV64_PTE_WRITE)
		attr |= PAGE_ATTR_PROT_WRITE;

	if (pte & RISCV64_PTE_EXEC)
		attr |= PAGE_ATTR_PROT_EXEC;

	return attr;
}

/*
 * Page tables
 */
static inline __vaddr_t
pgarch_pt_pte_to_vaddr(struct uk_pagetable *pt __unused, __pte_t pte,
		       unsigned int level __maybe_unused)
{
	return riscv64_directmap_paddr_to_vaddr(PT_Lx_PTE_PADDR(pte, level));
}

static inline __pte_t
pgarch_pt_pte_create(struct uk_pagetable *pt __unused, __paddr_t pt_paddr,
		     unsigned int level __unused, __pte_t template,
		     unsigned int template_level __unused)
{
	__pte_t pt_pte;

	UK_ASSERT(PAGE_ALIGNED(pt_paddr));

	pt_pte = RISCV64_PADDR_TO_PTE_PPN(pt_paddr);

	/* We do not apply any restrictive protections in PT PTEs but control
	 * protections in the PTEs mapping pages only
	 */
	pt_pte |= RISCV64_PTE_VALID_LINK;

	/* Take all other bits from template. We also keep the flags that are
	 * ignored by the architecture. The caller might have stored custom
	 * data in these fields
	 */
	pt_pte |= template & (RISCV64_PTE_USER |
			RISCV64_PTE_ACCESSED |
			RISCV64_PTE_DIRTY |
			RISCV64_PTE_RSW_MASK);

	return pt_pte;
}

static inline __vaddr_t
pgarch_pt_map(struct uk_pagetable *pt __unused, __paddr_t pt_paddr,
	      unsigned int level __unused)
{
	return riscv64_directmap_paddr_to_vaddr(pt_paddr);
}

static inline __paddr_t
pgarch_pt_unmap(struct uk_pagetable *pt __unused, __vaddr_t pt_vaddr,
		unsigned int level __unused)
{
	return riscv64_directmap_vaddr_to_paddr(pt_vaddr);
}

static __paddr_t riscv64_pg_maxphysaddr;

#define RISCV64_PG_VALID_PADDR(paddr)	((paddr) < riscv64_pg_maxphysaddr)

int ukarch_paddr_range_isvalid(__paddr_t start, __paddr_t end)
{
	UK_ASSERT(start <= end);
	return (RISCV64_PG_VALID_PADDR(start) && RISCV64_PG_VALID_PADDR(end));
}

static inline int
pgarch_init(void)
{
	__u32 max_addr_bit = 39;

	riscv64_pg_maxphysaddr = (1UL << max_addr_bit);

	/*
	 * The satp register was previously written at boot with the address
	 * of the static page table and the SV39_MODE value. We can verify
	 * if this mode is supported by the MMU by simply checking that the
	 * write actually modified the satp register, as per the RISC-V
	 * privileged specification:
	 *
	 * "[...] if satp is written with an unsupported MODE, the entire write
	 * has no effect; no fields in satp are modified."
	 */
	if (!_csr_read(CSR_SATP))
		UK_CRASH("Sv39 is not supported by the MMU, crashing...\n");

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
	fa_meta = (void *)riscv64_directmap_paddr_to_vaddr(start);
	fa_meta_size = uk_fallocbuddy_metadata_size(pages);

	if (unlikely(fa_meta_size >= len))
		return -ENOMEM;

	start = PAGE_ALIGN_UP(start + fa_meta_size);
	pages = (len - fa_meta_size) >> PAGE_SHIFT;

	dm_off = riscv64_directmap_paddr_to_vaddr(start);

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
	pt->fa = (struct uk_falloc *)riscv64_directmap_paddr_to_vaddr(start);

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
