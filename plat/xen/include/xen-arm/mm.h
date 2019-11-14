/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/* Taken from Mini-OS */

#ifndef _ARCH_MM_H_
#define _ARCH_MM_H_

#include <stdint.h>
#include <uk/plat/common/sections.h>
#include <uk/arch/limits.h>

typedef uint64_t paddr_t;
extern int _boot_stack[];
extern int _boot_stack_end[];
/* Add this to a virtual address to get the physical address (wraps at 4GB) */
extern uint32_t _libxenplat_paddr_offset;

#define L1_PAGETABLE_SHIFT      12

#define L1_PROT          0

#define to_phys(x)  (((uintptr_t)(x)+_libxenplat_paddr_offset) & 0xffffffff)
#define to_virt(x)  ((void *)(((uintptr_t)(x)-_libxenplat_paddr_offset) & 0xffffffff))

#define PFN_UP(x)   (unsigned long)(((x) + __PAGE_SIZE-1) >> L1_PAGETABLE_SHIFT)
#define PFN_DOWN(x) (unsigned long)((x) >> L1_PAGETABLE_SHIFT)
#define PFN_PHYS(x) ((uint64_t)(x) << L1_PAGETABLE_SHIFT)
#define PHYS_PFN(x) (unsigned long)((x) >> L1_PAGETABLE_SHIFT)

#define virt_to_pfn(_virt)     (PFN_DOWN(to_phys(_virt)))
#define virt_to_mfn(_virt)     (PFN_DOWN(to_phys(_virt)))
#define mfn_to_virt(_mfn)      (to_virt(PFN_PHYS(_mfn)))
#define pfn_to_virt(_pfn)      (to_virt(PFN_PHYS(_pfn)))

#define virtual_to_mfn(_virt)  virt_to_mfn(_virt)

#define arch_mm_init(a)

#endif
