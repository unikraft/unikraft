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

#ifndef __UKARCH_X86_64_MM_NATIVE__
#define __UKARCH_X86_64_MM_NATIVE__

#include "mm.h"
#include <uk/bitmap.h>
#include <uk/assert.h>
#include <uk/print.h>

#define pt_pte_to_virt(pt, pte) (pte_to_mframe(pte) + pt->virt_offset)
#define pt_virt_to_mframe(pt, vaddr) (vaddr - pt->virt_offset)
#define pfn_to_mfn(pfn) (pfn)

#define pte_to_pfn(pte) (pte_to_mframe(pte) >> PAGE_SHIFT)
#define pfn_to_mframe(pfn) (pfn << PAGE_SHIFT)
#define mframe_to_pframe(mframe) (mframe)

static inline __paddr_t ukarch_read_pt_base(void)
{
	__paddr_t cr3;

	__asm__ __volatile__("movq %%cr3, %0" : "=r"(cr3)::);

	return pte_to_mframe(cr3);
}

static inline void ukarch_write_pt_base(__paddr_t cr3)
{
	__asm__ __volatile__("movq %0, %%cr3" :: "r"(cr3) : );
}

static inline int ukarch_flush_tlb_entry(__vaddr_t vaddr)
{
	__asm__ __volatile__("invlpg (%0)" ::"r" (vaddr) : "memory");

	return 0;
}

static inline int ukarch_pte_write(__vaddr_t pt, size_t offset, __pte_t pte,
		size_t level)
{
	return _ukarch_pte_write_raw(pt, offset, pte, level);
}

#endif	/* __UKARCH_X86_64_MM_NATIVE__ */
