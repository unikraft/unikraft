/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Wei Chen <wei.chen@arm.com>
 *
 * Copyright (c) 2018, Arm Ltd. All rights reserved.
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
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */
#ifndef __CPU_ARM_64_DEFS_H__
#define __CPU_ARM_64_DEFS_H__

/*
 * Power State Coordination Interface (PSCI v0.2) function codes
 */
#define PSCI_FNID_VERSION		0x84000000
#define PSCI_FNID_CPU_SUSPEND		0xc4000001
#define PSCI_FNID_CPU_OFF		0x84000002
#define PSCI_FNID_CPU_ON		0xc4000003
#define PSCI_FNID_AFFINITY_INFO		0xc4000004
#define PSCI_FNID_MIGRATE		0xc4000005
#define PSCI_FNID_MIGRATE_INFO_TYPE	0x84000006
#define PSCI_FNID_MIGRATE_INFO_UP_CPU	0xc4000007
#define PSCI_FNID_SYSTEM_OFF		0x84000008
#define PSCI_FNID_SYSTEM_RESET		0x84000009

/*
 * CTR_EL0, Cache Type Register
 * Provides information about the architecture of the caches.
 */
#define CTR_DMINLINE_SHIFT	16
#define CTR_DMINLINE_WIDTH	4
#define CTR_IMINLINE_MASK	0xf
#define CTR_BYTES_PER_WORD	4

/* Registers and Bits definitions for MMU */
/* MAIR_EL1 - Memory Attribute Indirection Register */
#define MAIR_ATTR_MASK(idx)	(0xff << ((n)* 8))
#define MAIR_ATTR(attr, idx)	((attr) << ((idx) * 8))

/* Device-nGnRnE memory */
#define MAIR_DEVICE_nGnRnE	0x00
/* Device-nGnRE memory */
#define MAIR_DEVICE_nGnRE	0x04
/* Device-GRE memory */
#define MAIR_DEVICE_GRE		0x0C
/* Outer Non-cacheable + Inner Non-cacheable */
#define MAIR_NORMAL_NC		0x44
/* Outer + Inner Write-through non-transient */
#define MAIR_NORMAL_WT		0xbb
/* Outer + Inner Write-back non-transient */
#define MAIR_NORMAL_WB		0xff

/*
 * Memory types, these values are the indexs of the attributes
 * that defined in MAIR_EL1.
 */
#define DEVICE_nGnRnE	0
#define DEVICE_nGnRE	1
#define DEVICE_GRE	2
#define NORMAL_NC	3
#define NORMAL_WT	4
#define NORMAL_WB	5

#define MAIR_INIT_ATTR	\
		(MAIR_ATTR(MAIR_DEVICE_nGnRnE, DEVICE_nGnRnE) | \
		MAIR_ATTR(MAIR_DEVICE_nGnRE, DEVICE_nGnRE) |   \
		MAIR_ATTR(MAIR_DEVICE_GRE, DEVICE_GRE) |       \
		MAIR_ATTR(MAIR_NORMAL_NC, NORMAL_NC) |         \
		MAIR_ATTR(MAIR_NORMAL_WB, NORMAL_WT) |         \
		MAIR_ATTR(MAIR_NORMAL_WT, NORMAL_WB))

/* TCR_EL1 - Translation Control Register */
#define TCR_ASID_16	(1 << 36)

#define TCR_IPS_SHIFT	32
#define TCR_IPS_32BIT	(0 << TCR_IPS_SHIFT)
#define TCR_IPS_36BIT	(1 << TCR_IPS_SHIFT)
#define TCR_IPS_40BIT	(2 << TCR_IPS_SHIFT)
#define TCR_IPS_42BIT	(3 << TCR_IPS_SHIFT)
#define TCR_IPS_44BIT	(4 << TCR_IPS_SHIFT)
#define TCR_IPS_48BIT	(5 << TCR_IPS_SHIFT)

#define TCR_TG1_SHIFT	30
#define TCR_TG1_16K	(1 << TCR_TG1_SHIFT)
#define TCR_TG1_4K	(2 << TCR_TG1_SHIFT)
#define TCR_TG1_64K	(3 << TCR_TG1_SHIFT)

#define TCR_TG0_SHIFT	14
#define TCR_TG0_4K	(0 << TCR_TG0_SHIFT)
#define TCR_TG0_64K	(1 << TCR_TG0_SHIFT)
#define TCR_TG0_16K	(2 << TCR_TG0_SHIFT)

#define TCR_SH1_SHIFT	28
#define TCR_SH1_IS	(0x3 << TCR_SH1_SHIFT)
#define TCR_ORGN1_SHIFT	26
#define TCR_ORGN1_WBWA	(0x1 << TCR_ORGN1_SHIFT)
#define TCR_IRGN1_SHIFT	24
#define TCR_IRGN1_WBWA	(0x1 << TCR_IRGN1_SHIFT)
#define TCR_SH0_SHIFT	12
#define TCR_SH0_IS	(0x3 << TCR_SH0_SHIFT)
#define TCR_ORGN0_SHIFT	10
#define TCR_ORGN0_WBWA	(0x1 << TCR_ORGN0_SHIFT)
#define TCR_IRGN0_SHIFT	8
#define TCR_IRGN0_WBWA	(0x1 << TCR_IRGN0_SHIFT)

#define TCR_CACHE_ATTRS ((TCR_IRGN0_WBWA | TCR_IRGN1_WBWA) | \
			(TCR_ORGN0_WBWA | TCR_ORGN1_WBWA))

#define TCR_SMP_ATTRS	(TCR_SH0_IS | TCR_SH1_IS)

#define TCR_T1SZ_SHIFT	16
#define TCR_T0SZ_SHIFT	0
#define TCR_T1SZ(x)	((x) << TCR_T1SZ_SHIFT)
#define TCR_T0SZ(x)	((x) << TCR_T0SZ_SHIFT)
#define TCR_TxSZ(x)	(TCR_T1SZ(x) | TCR_T0SZ(x))

/*
 * As we use VA == PA mapping, so the VIRT_BITS must be the same
 * as PA_BITS. We can get PA_BITS from ID_AA64MMFR0_EL1.PARange.
 * So the TxSZ will be calculate dynamically.
 */
#define TCR_INIT_FLAGS	(TCR_ASID_16 | TCR_TG0_4K | \
			TCR_CACHE_ATTRS | TCR_SMP_ATTRS)

/* SCTLR_EL1 - System Control Register */
#define SCTLR_M		(_AC(1, UL) << 0)	/* MMU enable */
#define SCTLR_A		(_AC(1, UL) << 1)	/* Alignment check enable */
#define SCTLR_C		(_AC(1, UL) << 2)	/* Data/unified cache enable */
#define SCTLR_SA	(_AC(1, UL) << 3)	/* Stack alignment check enable */
#define SCTLR_SA0	(_AC(1, UL) << 4)	/* Stack Alignment Check Enable for EL0 */
#define SCTLR_CP15BEN	(_AC(1, UL) << 5)	/* System instruction memory barrier enable */
#define SCTLR_ITD	(_AC(1, UL) << 7)	/* IT disable */
#define SCTLR_SED	(_AC(1, UL) << 8)	/* SETEND instruction disable */
#define SCTLR_UMA	(_AC(1, UL) << 9)	/* User mask access */
#define SCTLR_I		(_AC(1, UL) << 12)	/* Instruction access Cacheability control */
#define SCTLR_DZE	(_AC(1, UL) << 14)	/* Traps EL0 DC ZVA instructions to EL1 */
#define SCTLR_UCT	(_AC(1, UL) << 15)	/* Traps EL0 accesses to the CTR_EL0 to EL1 */
#define SCTLR_nTWI	(_AC(1, UL) << 16)	/* Don't trap EL0 WFI to EL1 */
#define SCTLR_nTWE	(_AC(1, UL) << 18)	/* Don't trap EL0 WFE to EL1 */
#define SCTLR_WXN	(_AC(1, UL) << 19)	/* Write permission implies XN */
#define SCTLR_EOE	(_AC(1, UL) << 24)	/* Endianness of data accesses at EL0 */
#define SCTLR_EE	(_AC(1, UL) << 25)	/* Endianness of data accesses at EL1 */
#define SCTLR_UCI	(_AC(1, UL) << 26)	/* Traps EL0 cache instructions to EL1 */

/* Reserve to 1 */
#define SCTLR_RES1_B11	(_AC(1, UL) << 11)
#define SCTLR_RES1_B20	(_AC(1, UL) << 20)
#define SCTLR_RES1_B22	(_AC(1, UL) << 22)
#define SCTLR_RES1_B23	(_AC(1, UL) << 23)
#define SCTLR_RES1_B28	(_AC(1, UL) << 28)
#define SCTLR_RES1_B29	(_AC(1, UL) << 29)

/* Reserve to 0 */
#define SCTLR_RES0_B6	(_AC(1, UL) << 6)
#define SCTLR_RES0_B10	(_AC(1, UL) << 10)
#define SCTLR_RES0_B13	(_AC(1, UL) << 13)
#define SCTLR_RES0_B17	(_AC(1, UL) << 17)
#define SCTLR_RES0_B21	(_AC(1, UL) << 21)
#define SCTLR_RES0_B27	(_AC(1, UL) << 27)
#define SCTLR_RES0_B30	(_AC(1, UL) << 30)
#define SCTLR_RES0_B31	(_AC(1, UL) << 31)

/* Bits to set */
#define SCTLR_SET_BITS	\
		(SCTLR_UCI | SCTLR_nTWE | SCTLR_nTWI | SCTLR_UCT | \
		SCTLR_DZE | SCTLR_I | SCTLR_SED | SCTLR_SA0 | SCTLR_SA | \
		SCTLR_C | SCTLR_M | SCTLR_CP15BEN | SCTLR_RES1_B11 | \
		SCTLR_RES1_B20 | SCTLR_RES1_B22 | SCTLR_RES1_B23 | \
		SCTLR_RES1_B28 | SCTLR_RES1_B29)

/* Bits to clear */
#define SCTLR_CLEAR_BITS \
		(SCTLR_EE | SCTLR_EOE | SCTLR_WXN | SCTLR_UMA | \
		SCTLR_ITD | SCTLR_A | SCTLR_RES0_B6 | SCTLR_RES0_B10 | \
		SCTLR_RES0_B13 | SCTLR_RES0_B17 | SCTLR_RES0_B21 | \
		SCTLR_RES0_B27 | SCTLR_RES0_B30 | SCTLR_RES0_B31)

/*
 * Definitions for Block and Page descriptor attributes
 */
/* Level 0 table, 512GiB per entry */
#define L0_SHIFT	39
#define L0_SIZE		(1ul << L0_SHIFT)
#define L0_OFFSET	(L0_SIZE - 1ul)
#define L0_INVAL	0x0 /* An invalid address */
	/* 0x1 Level 0 doesn't support block translation */
	/* 0x2 also marks an invalid address */
#define L0_TABLE	0x3 /* A next-level table */

/* Level 1 table, 1GiB per entry */
#define L1_SHIFT	30
#define L1_SIZE 	(1 << L1_SHIFT)
#define L1_OFFSET 	(L1_SIZE - 1)
#define L1_INVAL	L0_INVAL
#define L1_BLOCK	0x1
#define L1_TABLE	L0_TABLE

/* Level 2 table, 2MiB per entry */
#define L2_SHIFT	21
#define L2_SIZE 	(1 << L2_SHIFT)
#define L2_OFFSET 	(L2_SIZE - 1)
#define L2_INVAL	L1_INVAL
#define L2_BLOCK	L1_BLOCK
#define L2_TABLE	L1_TABLE

#define L2_BLOCK_MASK	_AC(0xffffffe00000, UL)

/* Level 3 table, 4KiB per entry */
#define L3_SHIFT	12
#define L3_SIZE 	(1 << L3_SHIFT)
#define L3_OFFSET 	(L3_SIZE - 1)
#define L3_SHIFT	12
#define L3_INVAL	0x0
	/* 0x1 is reserved */
	/* 0x2 also marks an invalid address */
#define L3_PAGE		0x3

#define L0_ENTRIES_SHIFT 9
#define L0_ENTRIES	(1 << L0_ENTRIES_SHIFT)
#define L0_ADDR_MASK	(L0_ENTRIES - 1)

#define Ln_ENTRIES_SHIFT 9
#define Ln_ENTRIES	(1 << Ln_ENTRIES_SHIFT)
#define Ln_ADDR_MASK	(Ln_ENTRIES - 1)
#define Ln_TABLE_MASK	((1 << 12) - 1)
#define Ln_TABLE	0x3
#define Ln_BLOCK	0x1

/*
 * Hardware page table definitions.
 */
/* TODO: Add the upper attributes */
#define ATTR_MASK_H	_AC(0xfff0000000000000, UL)
#define ATTR_MASK_L	_AC(0x0000000000000fff, UL)
#define ATTR_MASK	(ATTR_MASK_H | ATTR_MASK_L)
/* Bits 58:55 are reserved for software */
#define ATTR_SW_MANAGED	(_AC(1, UL) << 56)
#define ATTR_SW_WIRED	(_AC(1, UL) << 55)
#define ATTR_UXN	(_AC(1, UL) << 54)
#define ATTR_PXN	(_AC(1, UL) << 53)
#define ATTR_XN		(ATTR_PXN | ATTR_UXN)
#define ATTR_CONTIGUOUS	(_AC(1, UL) << 52)
#define ATTR_DBM	(_AC(1, UL) << 51)
#define ATTR_nG		(1 << 11)
#define ATTR_AF		(1 << 10)
#define ATTR_SH(x)	((x) << 8)
#define ATTR_SH_MASK	ATTR_SH(3)
#define ATTR_SH_NS	0		/* Non-shareable */
#define ATTR_SH_OS	2		/* Outer-shareable */
#define ATTR_SH_IS	3		/* Inner-shareable */
#define ATTR_AP_RW_BIT	(1 << 7)
#define ATTR_AP(x)	((x) << 6)
#define ATTR_AP_MASK	ATTR_AP(3)
#define ATTR_AP_RW	(0 << 1)
#define ATTR_AP_RO	(1 << 1)
#define ATTR_AP_USER	(1 << 0)
#define ATTR_NS		(1 << 5)
#define ATTR_IDX(x)	((x) << 2)
#define ATTR_IDX_MASK	(7 << 2)

#define ATTR_DEFAULT	(ATTR_AF | ATTR_SH(ATTR_SH_IS))

#define ATTR_DESCR_MASK	3

/*
 * Define the attributes of pagetable descriptors
 */
#define SECT_ATTR_DEFAULT	\
		(Ln_BLOCK | ATTR_DEFAULT)
#define SECT_ATTR_NORMAL	\
		(SECT_ATTR_DEFAULT | ATTR_XN | \
		ATTR_IDX(NORMAL_WB))
#define SECT_ATTR_NORMAL_RO	\
		(SECT_ATTR_DEFAULT | ATTR_XN | \
		ATTR_AP_RW_BIT | ATTR_IDX(NORMAL_WB))
#define SECT_ATTR_NORMAL_EXEC	\
		(SECT_ATTR_DEFAULT | ATTR_UXN | \
		ATTR_AP_RW_BIT | ATTR_IDX(NORMAL_WB))
#define SECT_ATTR_DEVICE_nGnRE	\
		(SECT_ATTR_DEFAULT | ATTR_XN | \
		ATTR_IDX(DEVICE_nGnRnE))

#endif /* __CPU_ARM_64_DEFS_H__ */
