/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Stefan Teodorescu <stefanl.teodorescu@gmail.com>
 *
 * Copyright (c) 2021, University Politehnica of Bucharest. All rights reserved.
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
 *
 * Some of these macros here were inspired from Xen code.
 * For example, from "xen/include/asm-x86/x86_64/page.h" file.
 */

#ifndef __UKARCH_X86_64_MM_PV__
#define __UKARCH_X86_64_MM_PV__

#include "mm.h"

#include <stdint.h>
#include <common/hypervisor.h>
#include <uk/bitmap.h>
#include <uk/assert.h>
#include <uk/print.h>
#include <xen/xen.h>
#include <xen-x86/setup.h>
#include <xen-x86/mm.h>
#include <xen-x86/mm_pv.h>

extern unsigned long *phys_to_machine_mapping;
extern struct uk_pagetable ukplat_default_pt;

#define pte_to_pfn(pte) (mfn_to_pfn(pte_to_mfn(pte)))
#define pfn_to_mframe(pfn) (pfn_to_mfn(pfn) << PAGE_SHIFT)
#define mframe_to_pframe(mframe) (mfn_to_pfn(mframe >> PAGE_SHIFT) << PAGE_SHIFT)
#define pt_pte_to_virt(pt, pte) ((pte_to_pfn(pte) << PAGE_SHIFT) + pt->virt_offset)
#define pt_virt_to_mframe(pt, vaddr) (pfn_to_mfn((vaddr - pt->virt_offset) >> PAGE_SHIFT) << PAGE_SHIFT)

/* TODO: revisit implementation when using multiple page tables */
static inline __paddr_t ukarch_read_pt_base(void)
{
	if (!ukplat_default_pt.pt_base_paddr) {
		ukplat_default_pt.pt_base = HYPERVISOR_start_info->pt_base;
		ukplat_default_pt.pt_base_paddr = ukplat_default_pt.pt_base;
	}
	return ukplat_default_pt.pt_base_paddr;
}

static inline void ukarch_write_pt_base(__paddr_t cr3)
{
	mmuext_op_t uops[1];
	int rc;

	uops[0].cmd = MMUEXT_UNPIN_TABLE;
	/*
	 * TODO: check if this new (uncommented) code is correct. Commented
	 * is the old code that works only for the first page table (because
	 * the vaddr == paddr.
	 * uops[0].arg1.mfn = pfn_to_mfn(ukarch_read_pt_base() >> PAGE_SHIFT);
	 */
	uops[0].arg1.mfn = pfn_to_mfn(ukarch_read_pt_base() >> PAGE_SHIFT);
	rc = HYPERVISOR_mmuext_op(uops, 1, NULL, DOMID_SELF);
	if (rc < 0) {
		uk_pr_err("Could not unpin old PT base:"
				"mmuext_op failed with rc=%d\n", rc);
		return;
	}

	uops[0].cmd = MMUEXT_PIN_L4_TABLE;
	uops[0].arg1.mfn = pfn_to_mfn(cr3 >> PAGE_SHIFT);
	rc = HYPERVISOR_mmuext_op(uops, 1, NULL, DOMID_SELF);
	if (rc < 0) {
		uk_pr_err("Could not pin new PT base:"
				"mmuext_op failed with rc=%d\n", rc);
		return;
	}

	uops[0].cmd = MMUEXT_NEW_BASEPTR;
	uops[0].arg1.mfn = pfn_to_mfn(cr3 >> PAGE_SHIFT);
	rc = HYPERVISOR_mmuext_op(uops, 1, NULL, DOMID_SELF);
	if (rc < 0) {
		uk_pr_err("Could not set new PT base:"
				"mmuext_op failed with rc=%d\n", rc);
		return;
	}
}

static inline int ukarch_flush_tlb_entry(__vaddr_t vaddr)
{
	/*
	 * XXX(optimization): use HYPERVISOR_update_va_mapping for L1 to update
	 * and flush at the same time.
	 */
	mmuext_op_t uops[1];
	int rc;

	uops[0].cmd = MMUEXT_INVLPG_ALL;
	uops[0].arg1.linear_addr = vaddr;
	rc = HYPERVISOR_mmuext_op(uops, 1, NULL, DOMID_SELF);
	if (rc < 0) {
		uk_pr_err("Could not flush TLB entry for 0x%016lx:"
				"mmuext_op failed with rc=%d\n", vaddr, rc);
		return rc;
	}

	return 0;
}

static inline int ukarch_pte_write(__vaddr_t pt_vaddr, size_t offset,
		__pte_t pte, size_t level)
{
	mmu_update_t mmu_updates[1];
	int rc;

	UK_ASSERT(level >= 1 && level <= PAGETABLE_LEVELS);
	UK_ASSERT(PAGE_ALIGNED(pt_vaddr));
	UK_ASSERT(offset < pagetable_entries[level - 1]);

	mmu_updates[0].ptr = pt_virt_to_mframe(pt_vaddr)
			     + sizeof(unsigned long) * offset;
	mmu_updates[0].val = pte;
	rc = HYPERVISOR_mmu_update(mmu_updates, 1, NULL, DOMID_SELF);
	if (rc < 0) {
		uk_pr_err("Could not write PTE: mmu_update failed with rc=%d\n",
			rc);
		return rc;
	}

	return 0;
}

#endif	/* __UKARCH_X86_64_MM_PV__ */
