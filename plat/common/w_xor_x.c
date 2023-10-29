/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *          Michalis Pappas <michalis.pappas@opensynergy.com>
 *
 * Copyright (c) 2021, Karlsruhe Institute of Technology (KIT).
 *                     All rights reserved.
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
#include <uk/config.h>
#include <uk/plat/memory.h>
#include <uk/plat/paging.h>
#include <uk/print.h>

#ifdef CONFIG_ARCH_ARM_64
#include <arm/arm64/cpu.h>
#endif /* CONFIG_ARCH_ARM_64 */

#ifdef CONFIG_UKPLAT_MEMRNAME
#define WXORX_REGION_NAME d->name
#else /* !CONFIG_UKPLAT_MEMRNAME */
#define WXORX_REGION_NAME "memory range"
#endif /* CONFIG_UKPLAT_MEMRNAME */

#ifdef CONFIG_ARCH_ARM_64
#define enable_wxn() ({				\
	__u64 reg;				\
	reg = SYSREG_READ64(SCTLR_EL1);		\
	reg |= SCTLR_EL1_WXN_BIT;		\
	SYSREG_WRITE64(SCTLR_EL1, reg);		\
	isb();					\
})
#endif /* CONFIG_ARCH_ARM_64 */

void __weak enforce_w_xor_x(void)
{
	struct ukplat_memregion_desc *d;
	__vaddr_t base;
	__vaddr_t end;
	unsigned long prot, pages;
	int rc, i = 0;

	while ((rc = ukplat_memregion_get(i++, &d)) >= 0) {
		if (d->type == UKPLAT_MEMRT_FREE)
			continue;

		UK_ASSERT_VALID_MRD(d);

#ifdef CONFIG_ARCH_ARM_64
		/* Skip RW regions. These will be protected by WXN */
		if (d->flags & UKPLAT_MEMRF_WRITE)
			continue;
#endif /* CONFIG_ARCH_ARM64 */

		/* We expect platforms to provide sane sections,
		 * that is, if multiple sections reside in the same
		 * page, they should have consistent protections.
		 */
		base  = ALIGN_DOWN(d->vbase, PAGE_SIZE);
		end   = ALIGN_UP(d->vbase + d->len, PAGE_SIZE);
		pages = DIV_ROUND_UP(end - base, PAGE_SIZE);
		prot  = PAGE_ATTR_PROT_READ;

		if (d->flags & UKPLAT_MEMRF_EXECUTE)
			prot |= PAGE_ATTR_PROT_EXEC;
		else if (d->flags & UKPLAT_MEMRF_WRITE)
			prot |= PAGE_ATTR_PROT_WRITE;

		uk_pr_debug("Setting protections for %s: %"
			    __PRIvaddr " - %" __PRIvaddr " [R%c%c]\n",
			    WXORX_REGION_NAME,
			    base, base + pages * PAGE_SIZE,
			    (prot & PAGE_ATTR_PROT_WRITE) ? 'W' : '-',
			    (prot & PAGE_ATTR_PROT_EXEC) ? 'X' : '-');

		rc = ukplat_page_set_attr(ukplat_pt_get_active(),
					  base, pages, prot, 0);

		if (unlikely(rc)) {
			uk_pr_err("Failed to set protections for %s: %"
				  __PRIvaddr " - %" __PRIvaddr " [R%c%c]: %d\n",
				  WXORX_REGION_NAME,
				  base, base + pages * PAGE_SIZE,
				  (prot & PAGE_ATTR_PROT_WRITE) ? 'W' : '-',
				  (prot & PAGE_ATTR_PROT_EXEC) ? 'X' : '-',
				  rc);
		}
	}
#ifdef CONFIG_ARCH_ARM_64
	/* Enable WXN to protect RW regions.
	 * This saves us from manually updating the PTEs.
	 */
	uk_pr_debug("Enabling WXN\n");
	enable_wxn();
	ukarch_tlb_flush();
#endif /* CONFIG_ARCH_ARM64 */
}
#undef WXORX_REGION_NAME
