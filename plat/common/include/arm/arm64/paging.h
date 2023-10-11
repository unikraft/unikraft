/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Michalis Pappas <michalis.pappas@opensynergy.com>
 *
 * Based on plat/common/include/x86/paging.h by Marc Rittinghaus.
 *
 * Copyright (c) 2022, OpenSynergy GmbH. All rights reserved.
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

#include <arm/arm64/cpu.h>

#include <uk/arch/lcpu.h>
#include <uk/arch/limits.h>
#include <uk/essentials.h>
#include <uk/plat/paging.h>
#include <uk/fallocbuddy.h>
#include <uk/print.h>

#include <errno.h>

/* The implementation for arm64 supports a 48-bit virtual address space
 * with 4KiB translation granule, as defined by VMSAv8.
 *
 * Notice:
 * The Unikraft paging API uses the x86_64 convention, where pages are
 * defined at L0, and the top level table is at L3. This is the opposite
 * of the convention used by VMSAv8-A, where pages are defined at L3 and
 * the top level table is defined at L0 (48-bit) / L-1 (52-bit). This
 * implementation uses the Unikraft convention.
 */

/*
 * We expect the physical memory to be mapped 1:1 into the following area in
 * every virtual address space. This allows us to easily translate from virtual
 * to physical page table addresses and vice versa. We also access the metadata
 * for the frame allocators through this area.
 */
#define DIRECTMAP_AREA_START	0x0000ff8000000000
#define DIRECTMAP_AREA_END	0xffffffffffffffff
#define DIRECTMAP_AREA_SIZE	(DIRECTMAP_AREA_END - DIRECTMAP_AREA_START + 1)

static inline __vaddr_t
arm64_directmap_paddr_to_vaddr(__paddr_t paddr)
{
	UK_ASSERT(paddr < DIRECTMAP_AREA_SIZE);
	return (__vaddr_t)paddr + DIRECTMAP_AREA_START;
}

static inline __paddr_t
arm64_directmap_vaddr_to_paddr(__vaddr_t vaddr)
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

	static unsigned long pte_lx_map_paddr_mask[] = {
		PTE_L0_PAGE_PADDR_MASK,
		PTE_L1_BLOCK_PADDR_MASK,
		PTE_L2_BLOCK_PADDR_MASK,
	};

	UK_ASSERT(PAGE_ALIGNED(paddr));
	UK_ASSERT(level <= PT_MAP_LEVEL_MAX);

	pte = paddr & pte_lx_map_paddr_mask[level];

	if (!template)
		pte |= PTE_ATTR_AF;
	else
		pte |= template & (PTE_ATTR_CONTIGUOUS | PTE_ATTR_DBM |
				   PTE_ATTR_nG | PTE_ATTR_SH_MASK |
				   PTE_ATTR_AF | PTE_ATTR_IDX_MASK);

	if (level == PAGE_LEVEL)
		pte |= PTE_TYPE_PAGE;
	else
		pte |= PTE_TYPE_BLOCK;

	if (!(attr & PAGE_ATTR_PROT_WRITE))
		pte |= PTE_ATTR_AP(PTE_ATTR_AP_RO);

	if (!(attr & PAGE_ATTR_PROT_EXEC))
		pte |= PTE_ATTR_XN;

	switch (attr & (PAGE_ATTR_SHAREABLE_MASK <<
		PAGE_ATTR_SHAREABLE_SHIFT)) {
	case PAGE_ATTR_SHAREABLE_IS:
		pte |= PTE_ATTR_SH(PTE_ATTR_SH_IS);
		break;
	case PAGE_ATTR_SHAREABLE_OS:
		pte |= PTE_ATTR_SH(PTE_ATTR_SH_OS);
		break;
	case PAGE_ATTR_SHAREABLE_NS:
		pte |= PTE_ATTR_SH(PTE_ATTR_SH_NS);
		break;
	default:
		UK_ASSERT(0 && "Invalid shareability type\n");
	}

	switch (attr & (PAGE_ATTR_TYPE_MASK << PAGE_ATTR_TYPE_SHIFT)) {
	case PAGE_ATTR_TYPE_NORMAL_WB:
		pte |= PTE_ATTR_IDX(NORMAL_WB);
		break;
	case PAGE_ATTR_TYPE_NORMAL_WT:
		pte |= PTE_ATTR_IDX(NORMAL_WT);
		break;
	case PAGE_ATTR_TYPE_NORMAL_NC:
		pte |= PTE_ATTR_IDX(NORMAL_NC);
		break;
	case PAGE_ATTR_TYPE_DEVICE_nGnRnE:
		pte |= PTE_ATTR_IDX(DEVICE_nGnRnE);
		break;
	case PAGE_ATTR_TYPE_DEVICE_nGnRE:
		pte |= PTE_ATTR_IDX(DEVICE_nGnRE);
		break;
	case PAGE_ATTR_TYPE_DEVICE_GRE:
		pte |= PTE_ATTR_IDX(DEVICE_GRE);
		break;
	case PAGE_ATTR_TYPE_NORMAL_WB_TAGGED:
		pte |= PTE_ATTR_IDX(NORMAL_WB_TAGGED);
		break;
	default:
		UK_ASSERT(0 && "Invalid memory type\n");
	}

	return pte;
}

static inline __pte_t
pgarch_pte_change_attr(__pte_t pte __unused, unsigned long new_attr __unused,
		       unsigned int level __unused)
{
	UK_CRASH("%s: Not implemented", __func__);

	return 0;
}

static inline unsigned long
pgarch_attr_from_pte(__pte_t pte, unsigned int level __unused)
{
	unsigned long attr = PAGE_ATTR_PROT_READ;

	if (!(pte & PTE_ATTR_AP(PTE_ATTR_AP_RO)))
		attr |= PAGE_ATTR_PROT_WRITE;

	if (!(pte & PTE_ATTR_PXN))
		attr |= PAGE_ATTR_PROT_EXEC;

	switch (pte & PTE_ATTR_SH_MASK) {
	case (PTE_ATTR_SH(PTE_ATTR_SH_NS)):
		attr |= PAGE_ATTR_SHAREABLE_NS;
		break;
	case (PTE_ATTR_SH(PTE_ATTR_SH_IS)):
		attr |= PAGE_ATTR_SHAREABLE_IS;
		break;
	case (PTE_ATTR_SH(PTE_ATTR_SH_OS)):
		attr |= PAGE_ATTR_SHAREABLE_OS;
		break;
	default:
		UK_ASSERT(0 && "Invalid shareability type\n");
	};

	switch (pte & PTE_ATTR_IDX_MASK) {
	case PTE_ATTR_IDX(NORMAL_WB):
		attr |= PAGE_ATTR_TYPE_NORMAL_WB;
		break;
	case PTE_ATTR_IDX(NORMAL_WT):
		attr |= PAGE_ATTR_TYPE_NORMAL_WT;
		break;
	case PTE_ATTR_IDX(NORMAL_NC):
		attr |= PAGE_ATTR_TYPE_NORMAL_NC;
		break;
	case PTE_ATTR_IDX(DEVICE_nGnRnE):
		attr |= PAGE_ATTR_TYPE_DEVICE_nGnRnE;
		break;
	case PTE_ATTR_IDX(DEVICE_nGnRE):
		attr |= PAGE_ATTR_TYPE_DEVICE_nGnRE;
		break;
	case PTE_ATTR_IDX(DEVICE_GRE):
		attr |= PAGE_ATTR_TYPE_DEVICE_GRE;
		break;
	case PAGE_ATTR_TYPE_NORMAL_WB_TAGGED:
		attr |= PAGE_ATTR_TYPE_NORMAL_WB_TAGGED;
		break;
	default:
		UK_ASSERT(0 && "Invalid memory type\n");
	}

	return attr;
}

/*
 * Page tables
 */
static inline __vaddr_t
pgarch_pt_pte_to_vaddr(struct uk_pagetable *pt __unused, __pte_t pte,
		       unsigned int level __maybe_unused)
{
	return arm64_directmap_paddr_to_vaddr(PT_Lx_PTE_PADDR(pte, level));
}

/* Create a table descriptor */
static inline __pte_t
pgarch_pt_pte_create(struct uk_pagetable *pt __unused, __paddr_t pt_paddr,
		     unsigned int level __unused, __pte_t template __unused,
		     unsigned int template_level __unused)
{
	__pte_t pt_pte;

	UK_ASSERT(PAGE_ALIGNED(pt_paddr));

	pt_pte = pt_paddr & PTE_Lx_TABLE_PADDR_MASK;

	pt_pte |= PTE_TYPE_TABLE;

	return pt_pte;
}

static inline __vaddr_t
pgarch_pt_map(struct uk_pagetable *pt __unused, __paddr_t pt_paddr,
	      unsigned int level __unused)
{
	return arm64_directmap_paddr_to_vaddr(pt_paddr);
}

static inline __paddr_t
pgarch_pt_unmap(struct uk_pagetable *pt __unused, __vaddr_t pt_vaddr,
		unsigned int level __unused)
{
	return arm64_directmap_vaddr_to_paddr(pt_vaddr);
}

/* Temporary kernel mapping */
static inline __vaddr_t
pgarch_kmap(struct uk_pagetable *pt __unused, __paddr_t paddr,
	    __sz len __unused)
{
	return arm64_directmap_paddr_to_vaddr(paddr);
}

static inline void
pgarch_kunmap(struct uk_pagetable *pt __unused, __vaddr_t vaddr __unused,
	      __sz len __unused)
{
	/* nothing to do */
}

static inline int pgarch_init(void)
{
	/* Sanity checks to make sure that the PE supports the minimum
	 * requirements and the platform has set up a sane environment.
	 * This is expected to be useful during bring-up, so keep it
	 * conditional to UKDEBUG, to improve boot performance.
	 */
#ifdef CONFIG_UKDEBUG
	unsigned int ia_size;
	unsigned int oa_size;
	unsigned int tgran4;

	__u64 reg = SYSREG_READ64(TCR_EL1);

	/* Check 48-bit addressing mode configured */
	if (unlikely(reg & TCR_EL1_DS_BIT))
		UK_CRASH("48-bit addressing in not enabled\n");

	/* Check 48-bit IA configured. */
	ia_size = 64 - ((reg & TCR_EL1_T0SZ_MASK) >> TCR_EL1_T0SZ_SHIFT);

	if (unlikely(ia_size != ARM64_PADDR_BITS))
		UK_CRASH("Invalid paddr width: %d bits\n", ia_size);

	/* Check 48-bit OA configured */
	oa_size = tcr_ips_bits[(reg >> TCR_EL1_IPS_SHIFT) & TCR_EL1_IPS_MASK];

	if (unlikely(oa_size != ARM64_VADDR_BITS))
		UK_CRASH("Invalid vaddr width: %d bits", oa_size);

	/* Check 4K granule size configured */
	if (unlikely((reg & (TCR_EL1_TG0_MASK << TCR_EL1_TG0_SHIFT)) !=
	    TCR_EL1_TG0_4K))
		UK_CRASH("4KiB granule size is not enabled\n");

	/* Check TTBR0 walks are enabled */
	if (unlikely(reg & TCR_EL1_EPD0_BIT))
		UK_CRASH("TTBR0_EL1 table walks are not enabled\n");

	/* Check TTBR1 walks are disabled */
	if (unlikely(!(reg & TCR_EL1_EPD1_BIT)))
		UK_CRASH("TTBR1_EL1 table walks are not disabled\n");

	/* Check 4K granule size supported */
	reg = SYSREG_READ64(ID_AA64MMFR0_EL1);

	tgran4 = (reg >> ID_AA64MMFR0_EL1_TGRAN4_SHIFT) &
		  ID_AA64MMFR0_EL1_TGRAN4_MASK;

	if (unlikely(tgran4 != ID_AA64MMFR0_EL1_TGRAN4_SUPPORTED))
		UK_CRASH("4KiB granule not supported\n");
#endif /* CONFIG_UKDEBUG */
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
	UK_ASSERT(ukarch_paddr_range_isvalid(start, len));

	/* Reserve space for the metadata at the beginning of the area. Note
	 * that the metadata area will be a bit too large because we eat away
	 * from the frames by placing the metadata in the first frames.
	 */
	fa_meta = (void *)arm64_directmap_paddr_to_vaddr(start);
	fa_meta_size = uk_fallocbuddy_metadata_size(pages);

	if (unlikely(fa_meta_size >= len))
		return -ENOMEM;

	start = PAGE_ALIGN_UP(start + fa_meta_size);
	pages = (len - fa_meta_size) >> PAGE_SHIFT;

	dm_off = arm64_directmap_paddr_to_vaddr(start);

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
	pt->fa = (struct uk_falloc *)arm64_directmap_paddr_to_vaddr(start);

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
