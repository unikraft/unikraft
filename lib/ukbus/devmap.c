/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Jia He <justin.he@arm.com>
 *
 * Copyright (c) 2020, Arm Ltd. All rights reserved.
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

#include <uk/bus.h>
#include <uk/print.h>
#include <uk/errptr.h>
#include <uk/paging.h>
#include <uk/plat/native/paging.h>

__vaddr_t uk_bus_devmap(__paddr_t base, __sz size)
{
	struct uk_pagetable *pt;
	unsigned long pages;
	unsigned long attr;
	__vaddr_t vaddr;
	__paddr_t paddr;
	int rc;

	attr = UK_PAGING_PAGE_ATTR_PROT_RW;
#ifdef CONFIG_ARCH_ARM_64
	attr |= UK_PLAT_NATIVE_PAGE_ATTR_TYPE_DEVICE_nGnRnE;
#endif /* CONFIG_ARCH_ARM_64 */

	pages = ALIGN_UP(size, __PAGE_SIZE) >> UK_PAGING_PAGE_SHIFT;

	pt = uk_paging_pt_get_active();

	paddr = ALIGN_DOWN(base, __PAGE_SIZE);
	vaddr = ALIGN_DOWN(base, __PAGE_SIZE);

	/* We can't use uk_vma_map_dma here as the vmem API is initialized
	 * way too late for the interrupt controller which is required for
	 * bringing up secondary cores.
	 *
	 * Notice that updating the attributes of an already mapped region
	 * can potentialy fail, as uk_paging_page_map() will also return EEXIST
	 * for a partially mapped region too. Addressing this problem by
	 * remapping a region page-by-page is too slow for devices with large
	 * regions like PCI.
	 */
	rc = uk_paging_page_map(pt, vaddr, paddr, pages, attr, 0);
	if (unlikely(rc && rc != -EEXIST)) {
		uk_pr_err("Could not map MMIO Region at 0x%lx - 0x%lx (%d)\n",
			  base, base + size, rc);
		return rc;
	}

	if (rc == -EEXIST) {
		rc = uk_paging_page_unmap(pt, vaddr, pages, 0);
		if (unlikely(rc)) {
			uk_pr_err("Could not unmap MMIO Region at 0x%lx - 0x%lx (%d)\n",
				  base, base + size, rc);
			return rc;
		}

		rc = uk_paging_page_map(pt, vaddr, paddr, pages, attr, 0);
		if (unlikely(rc)) {
			uk_pr_err("Could not map MMIO Region at 0x%lx - 0x%lx (%d)\n",
				  base, base + size, rc);
			return rc;
		}
	}

	return (__vaddr_t)base;
}
