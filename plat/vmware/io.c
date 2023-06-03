/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Sharan Santhanam <sharan.santhanam@neclab.eu>
 *
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
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

#include <uk/plat/io.h>
#include <uk/config.h>

#ifdef CONFIG_PAGING
#include <uk/plat/paging.h>
#include <uk/assert.h>
#endif

__paddr_t ukplat_virt_to_phys(const volatile void *address)
{
#ifdef CONFIG_PAGING
	struct uk_pagetable *pt = ukplat_pt_get_active();
	__vaddr_t vaddr = (__vaddr_t) address;
	__pte_t pte;
	unsigned int level = PAGE_LEVEL;
	unsigned long offset;
	int rc __maybe_unused;

	rc = ukplat_pt_walk(pt, PAGE_ALIGN_DOWN(vaddr), &level, __NULL, &pte);
	UK_ASSERT(rc == 0);

	UK_ASSERT(PT_Lx_PTE_PRESENT(pte, level));
	UK_ASSERT(PAGE_Lx_IS(pte, level));

	offset = vaddr - PAGE_Lx_ALIGN_DOWN(vaddr, level);

	return PT_Lx_PTE_PADDR(pte, level) + offset;
#else /* CONFIG_PAGING */
	return (__paddr_t)address;
#endif /* CONFIG_PAGING */
}
