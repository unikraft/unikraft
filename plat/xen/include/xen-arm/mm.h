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

#include <uk/plat/common/sections.h>
#include <uk/arch/limits.h>
#include <uk/arch/paging.h>

#ifndef __ASSEMBLY__
typedef uint64_t paddr_t;
typedef uint64_t lpae_t;

#if defined(__arm__)
extern int _boot_stack[];
extern int _boot_stack_end[];
#endif
/* Add this to a virtual address to get the physical address (wraps at 4GB) */
extern paddr_t _libxenplat_paddr_offset;
#endif

#if defined(__aarch64__)
/*
 * TCR flags
 */
#define TCR_TxSZ(x)     ((((64) - (x)) << 16) | (((64) - (x)) << 0))
#define TCR_IRGN_WBWA   (((1) << 8) | ((1) << 24))
#define TCR_ORGN_WBWA   (((1) << 10) | ((1) << 26))
#define TCR_SHARED      (((3) << 12) | ((3) << 28))
#define TCR_TBI0        ((1) << 37)
#define TCR_TBI1        ((1) << 38)
#define TCR_ASID16      ((1) << 36)
#define TCR_IPS_40BIT   ((2) << 32)
#define TCR_TG1_4K      ((2) << 30)

/* Number of virtual address bits for 4KB page */
#define VA_BITS         39

#define SZ_2M           0x00200000

#define PAGE_OFFSET     ((0xffffffffffffffff << (VA_BITS - 1))\
						 & 0xffffffffffffffff)
#define FIX_FDT_TOP     (PAGE_OFFSET)
#define FIX_FDT_START   (FIX_FDT_TOP - SZ_2M)
#define FIX_CON_TOP     (FIX_FDT_START)
#define FIX_CON_START   (FIX_CON_TOP - SZ_2M)
#define FIX_GIC_TOP     (FIX_CON_START)
#define FIX_GIC_START   (FIX_GIC_TOP - SZ_2M)
#define FIX_XS_TOP      (FIX_GIC_START)
#define FIX_XS_START    (FIX_XS_TOP - SZ_2M)

/*
 * Memory types available.
 */
#define MT_DEVICE_nGnRnE    0
#define MT_DEVICE_nGnRE     1
#define MT_DEVICE_GRE       2
#define MT_NORMAL_NC        3
#define MT_NORMAL           4

/* SCTLR_EL1 - System Control Register */
#define	SCTLR_RES0          0xc8222400  /* Reserved, write 0 */
#define	SCTLR_RES1          0x30d00800  /* Reserved, write 1 */

#define	SCTLR_M             0x00000001
#define	SCTLR_A             0x00000002
#define	SCTLR_C             0x00000004
#define	SCTLR_SA            0x00000008
#define	SCTLR_SA0           0x00000010
#define	SCTLR_CP15BEN       0x00000020
#define	SCTLR_THEE          0x00000040
#define	SCTLR_ITD           0x00000080
#define	SCTLR_SED           0x00000100
#define	SCTLR_UMA           0x00000200
#define	SCTLR_I             0x00001000
#define	SCTLR_DZE           0x00004000
#define	SCTLR_UCT           0x00008000
#define	SCTLR_nTWI          0x00010000
#define	SCTLR_nTWE          0x00040000
#define	SCTLR_WXN           0x00080000
#define	SCTLR_EOE           0x01000000
#define	SCTLR_EE            0x02000000
#define	SCTLR_UCI           0x04000000

/* Level 0 table, 512GiB per entry */
#define L0_SHIFT            39
#define L0_INVAL            0x0 /* An invalid address */
#define L0_BLOCK 0x1		/* A block */
/* 0x2 also marks an invalid address */
#define L0_TABLE            0x3 /* A next-level table */

/* Level 1 table, 1GiB per entry */
#define L1_SHIFT            30
#define L1_SIZE             (1 << L1_SHIFT)
#define L1_OFFSET           (L1_SIZE - 1)
#define L1_INVAL            L0_INVAL
#define L1_BLOCK            L0_BLOCK
#define L1_TABLE            L0_TABLE
#define L1_MASK             (~(L1_SIZE-1))

/* Level 2 table, 2MiB per entry */
#define L2_SHIFT            21
#define L2_SIZE             (1 << L2_SHIFT)
#define L2_OFFSET           (L2_SIZE - 1)
#define L2_INVAL            L0_INVAL
#define L2_BLOCK            L0_BLOCK
#define L2_TABLE            L0_TABLE
#define L2_MASK             (~(L2_SIZE-1))

/* Level 3 table, 4KiB per entry */
#define L3_SHIFT            12
#define L3_SIZE             (1 << L3_SHIFT)
#define L3_OFFSET           (L3_SIZE - 1)
#define L3_INVAL            0x0
/* 0x1 is reserved */
/* 0x2 also marks an invalid address */
#define L3_PAGE             0x3
#define L3_MASK             (~(L3_SIZE-1))

#define Ln_ENTRIES          (1 << 9)
#define Ln_ADDR_MASK        (Ln_ENTRIES - 1)

#define ATTR_MASK_L         0xfff

#define l1_pgt_idx(va)      (((va) >> L1_SHIFT) & Ln_ADDR_MASK)
#define l2_pgt_idx(va)      (((va) >> L2_SHIFT) & Ln_ADDR_MASK)
#define l3_pgt_idx(va)      (((va) >> L3_SHIFT) & Ln_ADDR_MASK)

/*
 * Lower attributes fields in Stage 1 VMSAv8-A Block and Page descriptor
 */
#define ATTR_nG         (1 << 11)
#define ATTR_AF         (1 << 10)
#define ATTR_SH(x)      ((x) << 8)
#define  ATTR_SH_MASK   ATTR_SH(3)
#define  ATTR_SH_NS     0               /* Non-shareable */
#define  ATTR_SH_OS     2               /* Outer-shareable */
#define  ATTR_SH_IS     3               /* Inner-shareable */
#define ATTR_AP_RW_BIT  (1 << 7)
#define ATTR_AP(x)      ((x) << 6)
#define  ATTR_AP_MASK   ATTR_AP(3)
#define  ATTR_AP_RW     (0 << 1)
#define  ATTR_AP_RO     (1 << 1)
#define  ATTR_AP_USER   (1 << 0)
#define ATTR_NS         (1 << 5)
#define ATTR_IDX(x)     ((x) << 2)
#define ATTR_IDX_MASK   (7 << 2)

#define BLOCK_DEF_ATTR (ATTR_AF|ATTR_SH(ATTR_SH_IS)|ATTR_IDX(MT_NORMAL))
#define BLOCK_DEV_ATTR (ATTR_AF|ATTR_SH(ATTR_SH_IS)|ATTR_IDX(MT_DEVICE_nGnRnE))

#endif

/* to align the pointer to the (next) page boundary */
#define PAGE_ALIGN(addr) (((addr) + PAGE_SIZE - 1) & PAGE_MASK)

#define L1_PAGETABLE_SHIFT      12

#if defined(__aarch64__)
#define to_phys(x) \
	(((paddr_t)(x)-_libxenplat_paddr_offset) & 0xffffffffffffffff)
#define to_virt(x) \
	((void *)(((x)+_libxenplat_paddr_offset) & 0xffffffffffffffff))
#else
#define to_phys(x)  (((paddr_t)(x)+_libxenplat_paddr_offset) & 0xffffffff)
#define to_virt(x)  ((void *)(((x)-_libxenplat_paddr_offset) & 0xffffffff))
#endif

#define PFN_UP(x)    (unsigned long)(((x) + PAGE_SIZE-1) >> PAGE_SHIFT)
#define PFN_DOWN(x)    (unsigned long)((x) >> PAGE_SHIFT)
#define PFN_PHYS(x)    ((uint64_t)(x) << PAGE_SHIFT)
#define PHYS_PFN(x)    (unsigned long)((x) >> PAGE_SHIFT)

#define virt_to_pfn(_virt)         (PFN_DOWN(to_phys(_virt)))
#define virt_to_mfn(_virt)         (PFN_DOWN(to_phys(_virt)))
#define mfn_to_virt(_mfn)          (to_virt(PFN_PHYS(_mfn)))
#define pfn_to_virt(_pfn)          (to_virt(PFN_PHYS(_pfn)))

#define mfn_to_pfn(x) (x)
#define pfn_to_mfn(x) (x)

#define virtual_to_mfn(_virt)	   virt_to_mfn(_virt)

// FIXME
#define map_frames(f, n) (NULL)

#ifndef __ASSEMBLY__
void arch_mm_prepare(unsigned long *start_pfn_p, unsigned long *max_pfn_p);
void set_pgt_entry(lpae_t *ptr, lpae_t val);
#endif

#define arch_mm_init(a)

#endif
