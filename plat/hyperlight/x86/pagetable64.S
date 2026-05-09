/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Based on KVM platform code
 *
 * Copyright (c) 2024 Microsoft Corporation
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

/*
 * Hyperlight Page Tables
 *
 * Hyperlight host sets up identity-mapped page tables, but if the paging API
 * is enabled, we need to add the direct-map region at high addresses.
 * This is needed by Unikraft's paging code which uses the direct map to
 * access page table metadata.
 *
 * Memory layout:
 * 0x0000000000000000 - 0x00000000ffffffff  Identity map (first 4GB)
 * 0xffffff8000000000 - 0xffffffffffffffff  Direct map of first 512GB
 */

#include <uk/config.h>
#include <uk/reloc.h>

/* x86-64 PTE flags (hardcoded since the macro header is C-only) */
#define X86_PTE_PRESENT  0x001
#define X86_PTE_RW       0x002
#define X86_PTE_PSE      0x080

#define PTE_RO          X86_PTE_PRESENT
#define PTE_RW          (X86_PTE_PRESENT | X86_PTE_RW)

#define PT_LVL          0
#define PD_LVL          1
#define PDPT_LVL        2
#define PML4_LVL        3

/* Page level macros (previously from uk/arch/paging.h) */
#define PAGE_HUGE_LEVEL   2   /* 1 GiB pages (PDPT level) */
#define PAGE_LARGE_LEVEL  1   /* 2 MiB pages (PD level) */

/* Page size per level */
#define PAGE_Lx_SIZE(lvl) (1 << (12 + 9 * (lvl)))   /* 4K, 2M, 1G */

/**
 * Outputs a single page table entry
 */
.macro mk_pte paddr, flags
        .quad \paddr + \flags
.endm

/**
 * Outputs a page table entry with large/huge page flag
 */
.macro pte_page paddr, level, flags
.ifeq (\level - 2)
        mk_pte \paddr, (\flags + 0x080)
        .exitm
.endif
.ifeq (\level - 1)
        mk_pte \paddr, (\flags + 0x080)
.else
        mk_pte \paddr, \flags
.endif
.endm

/**
 * Outputs multiple page table entries for continuous mapping
 */
.macro pte_fill paddr, pages, lvl, pte
.ifle \pages
        .exitm
.endif
        pte_page (\paddr + PAGE_Lx_SIZE(\lvl) * 0), \lvl, \pte
.ifgt \pages - 8
        pte_page (\paddr + PAGE_Lx_SIZE(\lvl) * 1), \lvl, \pte
        pte_page (\paddr + PAGE_Lx_SIZE(\lvl) * 2), \lvl, \pte
        pte_page (\paddr + PAGE_Lx_SIZE(\lvl) * 3), \lvl, \pte
        pte_page (\paddr + PAGE_Lx_SIZE(\lvl) * 4), \lvl, \pte
        pte_page (\paddr + PAGE_Lx_SIZE(\lvl) * 5), \lvl, \pte
        pte_page (\paddr + PAGE_Lx_SIZE(\lvl) * 6), \lvl, \pte
        pte_page (\paddr + PAGE_Lx_SIZE(\lvl) * 7), \lvl, \pte
        pte_fill (\paddr + PAGE_Lx_SIZE(\lvl) * 8), (\pages - 8), \lvl, \pte
.else
        pte_fill (\paddr + PAGE_Lx_SIZE(\lvl) * 1), (\pages - 1), \lvl, \pte
.endif
.endm

/**
 * Outputs non-present page table entries
 */
.macro pte_zero paddr=0, pages
        .fill \pages, 0x8, 0
.endm

.align 4
.global bpt_unmap_mrd
bpt_unmap_mrd:
        .quad   0x0000000000000000              /* pbase: 0 */
        .quad   0x0000000000000000              /* vbase: 0 */
        .quad   0x0000000000000000              /* pg_off: 0 */
        .quad   0x0000000100000000              /* len: 4 GiB */
        .quad   0x0000000000100000              /* pg_count */
        .short  0x0000000000000000              /* type */
        .short  0x0000000000000010              /* flags: UKPLAT_MEMRF_UNMAP */
        .fill   36, 1, 0

/* x86-64 Boot Page Table for Hyperlight
 *
 * We map the first 4GB using 2MB pages for identity mapping.
 * If paging API is enabled, we also map the first 512GB at the
 * direct-map region (0xffffff8000000000) using 1GB pages.
 */

.align 0x1000
x86_bpt_pt0_0_0: /* 4K pages for first 2MB (first page inaccessible) */
        pte_zero 0x0000000000000000, 0x001
        pte_fill 0x0000000000001000, 0x1ff, PT_LVL, PTE_RW

.align 0x1000
x86_bpt_pd0_0: /* 2M pages for 0-1GB */
        ur_pte x86_bpt_pt0_0_0, PTE_RW
        pte_fill 0x0000000000200000, 0x1ff, PD_LVL, PTE_RW

x86_bpt_pd0_1: /* 2M pages for 1-2GB */
        pte_fill 0x0000000040000000, 0x200, PD_LVL, PTE_RW

x86_bpt_pd0_2: /* 2M pages for 2-3GB */
        pte_fill 0x0000000080000000, 0x200, PD_LVL, PTE_RW

x86_bpt_pd0_3: /* 2M pages for 3-4GB */
        pte_fill 0x00000000c0000000, 0x200, PD_LVL, PTE_RW

.align 0x1000
x86_bpt_pdpt0: /* PDPT for identity map (first 4GB) */
        ur_pte  x86_bpt_pd0_0, PTE_RW
        ur_pte  x86_bpt_pd0_1, PTE_RW
        ur_pte  x86_bpt_pd0_2, PTE_RW
        ur_pte  x86_bpt_pd0_3, PTE_RW
        pte_zero , 0x1fc

/* Page table for 512 GiB direct-mapped physical memory at 0xffffff8000000000 */
#ifdef CONFIG_PAGING
.align 0x1000
x86_bpt_pdpt511: /* 1G pages for direct map */
        pte_fill 0x0000000000000000, 0x200, PDPT_LVL, PTE_RW
#endif /* CONFIG_PAGING */

.align 0x1000
.global x86_bpt_pml4
x86_bpt_pml4:
        ur_pte  x86_bpt_pdpt0, PTE_RW           /* Entry 0: identity map */
        pte_zero , 0x1fe                        /* Entries 1-510: empty */
#ifdef CONFIG_PAGING
        ur_pte  x86_bpt_pdpt511, PTE_RW         /* Entry 511: direct map */
#else
        pte_zero , 0x001
#endif /* CONFIG_PAGING */
