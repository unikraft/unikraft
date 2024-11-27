/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Michalis Pappas <michalis.pappas@opensynergy.com>
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

#ifndef __UKARCH_LCPU_H__
#error Do not include this header directly
#endif

#include <uk/asm.h>

/* PSTATE */
#define PSTATE_V_BIT		(_AC(1, UL) << 31)
#define PSTATE_C_BIT		(_AC(1, UL) << 30)
#define PSTATE_Z_BIT		(_AC(1, UL) << 29)
#define PSTATE_N_BIT		(_AC(1, UL) << 28)
#define PSTATE_TCO_BIT		(_AC(1, UL) << 25)
#define PSTATE_DIT_BIT		(_AC(1, UL) << 24)
#define PSTATE_UAO_BIT		(_AC(1, UL) << 23)
#define PSTATE_PAN_BIT		(_AC(1, UL) << 22)
#define PSTATE_ALLINT_BIT	(_AC(1, UL) << 13)
#define PSTATE_SBSS_BIT		(_AC(1, UL) << 12)
#define PSTATE_F_BIT		(_AC(1, UL) << 9)
#define PSTATE_I_BIT		(_AC(1, UL) << 8)
#define PSTATE_A_BIT		(_AC(1, UL) << 7)
#define PSTATE_D_BIT		(_AC(1, UL) << 6)
#define PSTATE_EL_BIT		(_AC(1, UL) << 2)
#define PSTATE_SPSEL_BIT	(_AC(1, UL))

/**************************************************************************
 * AArch64 System Register Definitions
 *************************************************************************/

/* CTR_EL0 - Cache Type Register */
#define CTR_DMINLINE_SHIFT	16
#define CTR_DMINLINE_WIDTH	4
#define CTR_IMINLINE_MASK	0xf
#define CTR_BYTES_PER_WORD	4

/* ESR_EL1 - Exception Syndrome Register */
#define ESR_ISS_SHIFT		0
#define ESR_ISS(x)		((x) << ESR_ISS_SHIFT)
#define ESR_ISS_MASK		_AC(0x0000000000ffffff, UL)
#define ESR_ISS_FROM(x)		(((x) & ESR_ISS_MASK) >> ESR_ISS_SHIFT)

#define ESR_IL			(1 << 25)

#define ESR_EC_SHIFT		26
#define ESR_EC(x)		((x) << ESR_EC_SHIFT)
#define ESR_EC_MASK		_AC(0x00000000fc000000, UL)
#define ESR_EC_FROM(x)		(((x) & ESR_EC_MASK) >> ESR_EC_SHIFT)

#define ESR_ISS2_SHIFT		32
#define ESR_ISS2(x)		(_AC(x, UL) << ESR_ISS2_SHIFT)
#define ESR_ISS2_MASK		_AC(0x0000001f00000000, UL)
#define ESR_ISS2_FROM(x)	(((x) & ESR_ISS2_MASK) >> ESR_ISS2_SHIFT)

/* Exception classes */
#define ESR_EL1_EC_UNKNOWN			0x00
#define ESR_EL1_EC_WFx				0x01
#define ESR_EL1_EC_COPROC_15_32			0x03
#define ESR_EL1_EC_COPROC_15_64			0x04
#define ESR_EL1_EC_COPROC_14_32			0x05
#define ESR_EL1_EC_COPROC_14_LS			0x06
#define ESR_EL1_EC_SVE_ASIMD_FP_ACC		0x07
#define ESR_EL1_EC_LS64				0x0a
#define ESR_EL1_EC_COPROC_14_64			0x0c
#define ESR_EL1_EC_BTI				0x0d
#define ESR_EL1_EC_ILL				0x0e
#define ESR_EL1_EC_SVC32			0x11
#define ESR_EL1_EC_SVC64			0x15
#define ESR_EL1_EC_SYS64			0x18
#define ESR_EL1_EC_SVE_ACC			0x19
#define ESR_EL1_EC_FPAC				0x1c
#define ESR_EL1_EC_MMU_IABRT_EL0		0x20
#define ESR_EL1_EC_MMU_IABRT_EL1		0x21
#define ESR_EL1_EC_PC_ALIGN			0x22
#define ESR_EL1_EC_MMU_DABRT_EL0		0x24
#define ESR_EL1_EC_MMU_DABRT_EL1		0x25
#define ESR_EL1_EC_SP_ALIGN			0x26
#define ESR_EL1_EC_FP32				0x28
#define ESR_EL1_EC_FP64				0x2c
#define ESR_EL1_EC_SERROR			0x2f
#define ESR_EL1_EC_BRK_EL0			0x30
#define ESR_EL1_EC_BRK_EL1			0x31
#define ESR_EL1_EC_STEP_EL0			0x32
#define ESR_EL1_EC_STEP_EL1			0x33
#define ESR_EL1_EC_WATCHP_EL0			0x34
#define ESR_EL1_EC_WATCHP_EL1			0x35
#define ESR_EL1_EC_BKPT32			0x38
#define ESR_EL1_EC_BRK64			0x3c

/* ISS for instruction and data aborts */
#define ESR_ISS_ABRT_FSC_SHIFT			0
#define ESR_ISS_ABRT_FSC(x)			((x) << ESR_ISS_ABRT_FSC_SHIFT)
#define ESR_ISS_ABRT_FSC_MASK			_AC(0x000000000000003f, UL)
#define ESR_ISS_ABRT_FSC_FROM(x) \
	(((x) & ESR_ISS_ABRT_FSC_MASK) >> ESR_ISS_ABRT_FSC_SHIFT)

#define ESR_ISS_ABRT_FSC_ADDR_L0		0x00
#define ESR_ISS_ABRT_FSC_ADDR_L1		0x01
#define ESR_ISS_ABRT_FSC_ADDR_L2		0x02
#define ESR_ISS_ABRT_FSC_ADDR_L3		0x03
#define ESR_ISS_ABRT_FSC_TRANS_L0		0x04
#define ESR_ISS_ABRT_FSC_TRANS_L1		0x05
#define ESR_ISS_ABRT_FSC_TRANS_L2		0x06
#define ESR_ISS_ABRT_FSC_TRANS_L3		0x07
#define ESR_ISS_ABRT_FSC_ACCF_L0		0x08
#define ESR_ISS_ABRT_FSC_ACCF_L1		0x09
#define ESR_ISS_ABRT_FSC_ACCF_L2		0x0a
#define ESR_ISS_ABRT_FSC_ACCF_L3		0x0b
#define ESR_ISS_ABRT_FSC_PERM_L0		0x0c
#define ESR_ISS_ABRT_FSC_PERM_L1		0x0d
#define ESR_ISS_ABRT_FSC_PERM_L2		0x0e
#define ESR_ISS_ABRT_FSC_PERM_L3		0x0f
#define ESR_ISS_ABRT_FSC_SYNC			0x10
#define ESR_ISS_ABRT_FSC_TAG			0x11
#define ESR_ISS_ABRT_FSC_SYNC_PT_LM1		0x13
#define ESR_ISS_ABRT_FSC_SYNC_PT_L0		0x14
#define ESR_ISS_ABRT_FSC_SYNC_PT_L1		0x15
#define ESR_ISS_ABRT_FSC_SYNC_PT_L2		0x16
#define ESR_ISS_ABRT_FSC_SYNC_PT_L3		0x17
#define ESR_ISS_ABRT_FSC_SYNC_ECC		0x18
#define ESR_ISS_ABRT_FSC_SYNC_ECC_PT_LM1	0x1b
#define ESR_ISS_ABRT_FSC_SYNC_ECC_PT_L0		0x1c
#define ESR_ISS_ABRT_FSC_SYNC_ECC_PT_L1		0x1d
#define ESR_ISS_ABRT_FSC_SYNC_ECC_PT_L2		0x1e
#define ESR_ISS_ABRT_FSC_SYNC_ECC_PT_L3		0x1f
#define ESR_ISS_ABRT_FSC_ALIGN			0x21
#define ESR_ISS_ABRT_FSC_ADDR_LM1		0x29
#define ESR_ISS_ABRT_FSC_TRANS_LM1		0x2b
#define ESR_ISS_ABRT_FSC_TLB			0x30
#define ESR_ISS_ABRT_FSC_UNSUP_ATOM		0x31
#define ESR_ISS_ABRT_FSC_LOCKDOWN		0x34
#define ESR_ISS_ABRT_FSC_UNSUP_EXCL		0x35

/* GCR_EL1: Tag Control Register */
#define GCR_EL1_RRND_BIT			(_AC(1, UL) << 16)
#define GCR_EL1_EXCLUDE_MASK			_AC(0xffff, UL)

/* ID_AA64ISAR0_EL1: AArch64 Instruction Set Attributes Register 0 */
#define ID_AA64ISAR0_EL1_RNDR_SHIFT		_AC(60, ULL)
#define ID_AA64ISAR0_EL1_RNDR_MASK		_AC(0xf, UL)

/* ID_AA64ISAR1_EL1: AArch64 Instruction Set Attributes Register 1 */
#define ID_AA64ISAR1_EL1_GPI_SHIFT		28
#define ID_AA64ISAR1_EL1_GPI_MASK		0xf
#define ID_AA64ISAR1_EL1_GPA_SHIFT		24
#define ID_AA64ISAR1_EL1_GPA_MASK		0xf
#define ID_AA64ISAR1_EL1_API_SHIFT		8
#define ID_AA64ISAR1_EL1_API_MASK		0xf
#define ID_AA64ISAR1_EL1_APA_SHIFT		4
#define ID_AA64ISAR1_EL1_APA_MASK		0xf

/* ID_AA64ISAR2_EL1: AArch64 Instruction Set Attributes Register 2 */
#define ID_AA64ISAR2_EL1_APA3_SHIFT		12
#define ID_AA64ISAR2_EL1_APA3_MASK		0xf

/* ID_AA64PFR1_EL1: AArch64 Processor Feature Register 1 */
#define ID_AA64PFR1_EL1_NMI_SHIFT		_AC(32, UL)
#define ID_AA64PFR1_EL1_NMI_MASK		_AC(0xf, UL)
#define ID_AA64PFR1_EL1_CSV2_SHIFT		_AC(28, UL)
#define ID_AA64PFR1_EL1_CSV2_MASK		_AC(0xf, UL)
#define ID_AA64PFR1_EL1_RNDR_SHIFT		_AC(24, UL)
#define ID_AA64PFR1_EL1_RNDR_MASK		_AC(0xf, UL)
#define ID_AA64PFR1_EL1_SME_SHIFT		_AC(20, UL)
#define ID_AA64PFR1_EL1_SME_MASK		_AC(0xf, UL)
#define ID_AA64PFR1_EL1_MPAM_SHIFT		_AC(16, UL)
#define ID_AA64PFR1_EL1_MPAM_MASK		_AC(0xf, UL)
#define ID_AA64PFR1_EL1_RAS_SHIFT		_AC(12, UL)
#define ID_AA64PFR1_EL1_RAS_MASK		_AC(0xf, UL)
#define ID_AA64PFR1_EL1_MTE_SHIFT		_AC(8, UL)
#define ID_AA64PFR1_EL1_MTE_MASK		_AC(0xf, UL)
#define ID_AA64PFR1_EL1_SSBS_SHIFT		_AC(4, UL)
#define ID_AA64PFR1_EL1_SSBS_MASK		_AC(0xf, UL)
#define ID_AA64PFR1_EL1_BT_SHIFT		_AC(0, UL)
#define ID_AA64PFR1_EL1_BT_MASK			_AC(0xf, UL)

/* MAIR_EL1: Memory Attribute Indirection Register */
#define MAIR_EL1_ATTR_MASK(idx)			(_AC(0xff, UL) << ((idx) * 8))
#define MAIR_EL1_ATTR(attr, idx)		(_AC(attr, UL) << ((idx) * 8))

/* ID_AA64MMFR0_EL1 - Memory Model Feature Register 1 */
#define ID_AA64MMFR0_EL1_PARANGE_SHIFT		_AC(0, U)
#define ID_AA64MMFR0_EL1_PARANGE_MASK		_AC(0xf, ULL)

#define PARANGE_0000				_AC(32, U)
#define PARANGE_0001				_AC(36, U)
#define PARANGE_0010				_AC(40, U)
#define PARANGE_0011				_AC(42, U)
#define PARANGE_0100				_AC(44, U)
#define PARANGE_0101				_AC(48, U)
#define PARANGE_0110				_AC(52, U)

#define ID_AA64MMFR0_EL1_ECV_SHIFT		_AC(60, U)
#define ID_AA64MMFR0_EL1_ECV_MASK		_AC(0xf, ULL)
#define ID_AA64MMFR0_EL1_ECV_NOT_SUPPORTED	_AC(0x0, ULL)
#define ID_AA64MMFR0_EL1_ECV_SUPPORTED		_AC(0x1, ULL)
#define ID_AA64MMFR0_EL1_ECV_SELF_SYNCH		_AC(0x2, ULL)

#define ID_AA64MMFR0_EL1_FGT_SHIFT		_AC(56, U)
#define ID_AA64MMFR0_EL1_FGT_MASK		_AC(0xf, ULL)
#define ID_AA64MMFR0_EL1_FGT_SUPPORTED		_AC(0x1, ULL)
#define ID_AA64MMFR0_EL1_FGT_NOT_SUPPORTED	_AC(0x0, ULL)

#define ID_AA64MMFR0_EL1_TGRAN4_SHIFT		_AC(28, U)
#define ID_AA64MMFR0_EL1_TGRAN4_MASK		_AC(0xf, ULL)
#define ID_AA64MMFR0_EL1_TGRAN4_SUPPORTED	_AC(0x0, ULL)
#define ID_AA64MMFR0_EL1_TGRAN4_NOSUPPORTED	_AC(0xf, ULL)

#define ID_AA64MMFR0_EL1_TGRAN64_SHIFT		_AC(24, U)
#define ID_AA64MMFR0_EL1_TGRAN64_MASK		_AC(0xf, ULL)
#define ID_AA64MMFR0_EL1_TGRAN64_SUPPORTED	_AC(0x0, ULL)
#define ID_AA64MMFR0_EL1_TGRAN64_NOT_SUPPORTED	_AC(0xf, ULL)

#define ID_AA64MMFR0_EL1_TGRAN16_SHIFT		_AC(20, U)
#define ID_AA64MMFR0_EL1_TGRAN16_MASK		_AC(0xf, ULL)
#define ID_AA64MMFR0_EL1_TGRAN16_SUPPORTED	_AC(0x1, ULL)
#define ID_AA64MMFR0_EL1_TGRAN16_NOT_SUPPORTED	_AC(0x0, ULL)

/* ID_AA64MMFR2_EL1 - Memory Model Feature Register 2 */
#define ID_AA64MMFR2_EL1			S3_0_C0_C7_2

#define ID_AA64MMFR2_EL1_ST_SHIFT		_AC(28, U)
#define ID_AA64MMFR2_EL1_ST_MASK		_AC(0xf, ULL)

#define ID_AA64MMFR2_EL1_CCIDX_SHIFT		_AC(20, U)
#define ID_AA64MMFR2_EL1_CCIDX_MASK		_AC(0xf, ULL)
#define ID_AA64MMFR2_EL1_CCIDX_LENGTH		_AC(4, U)

#define ID_AA64MMFR2_EL1_CNP_SHIFT		_AC(0, U)
#define ID_AA64MMFR2_EL1_CNP_MASK		_AC(0xf, ULL)

/* MDSCR_EL1: Monitor Debug System Control Register */
#define MDSCR_EL1_SS			UK_BIT(0)
#define MDSCR_EL1_KDE			UK_BIT(13)

/* RGSR_EL1: Random Allocation Tag Seed Register */
#define RGSR_EL1_SEED_SHIFT		_AC(8, U)
#define RGSR_EL1_SEED_MASK		_AC(0xffff, UL)

/* SCTLR_EL1: System Control Register */
#define SCTLR_EL1_M_BIT			(_AC(1, UL) << 0)
#define SCTLR_EL1_A_BIT			(_AC(1, UL) << 1)
#define SCTLR_EL1_C_BIT			(_AC(1, UL) << 2)
#define SCTLR_EL1_SA_BIT		(_AC(1, UL) << 3)
#define SCTLR_EL1_SA0_BIT		(_AC(1, UL) << 4)
#define SCTLR_EL1_CP15BEN_BIT		(_AC(1, UL) << 5)
#define SCTLR_EL1_nAA_BIT		(_AC(1, UL) << 6)
#define SCTLR_EL1_ITD_BIT		(_AC(1, UL) << 7)
#define SCTLR_EL1_SED_BIT		(_AC(1, UL) << 8)
#define SCTLR_EL1_UMA_BIT		(_AC(1, UL) << 9)
#define SCTLR_EL1_EnRCTX_BIT		(_AC(1, UL) << 10)
#define SCTLR_EL1_EOS_BIT		(_AC(1, UL) << 11)
#define SCTLR_EL1_I_BIT			(_AC(1, UL) << 12)
#define SCTLR_EL1_EnDB_BIT		(_AC(1, UL) << 13)
#define SCTLR_EL1_DZE_BIT		(_AC(1, UL) << 14)
#define SCTLR_EL1_UCT_BIT		(_AC(1, UL) << 15)
#define SCTLR_EL1_nTWI_BIT		(_AC(1, UL) << 16)
#define SCTLR_EL1_RES0_27_BIT		(_AC(1, UL) << 17)
#define SCTLR_EL1_nTWE_BIT		(_AC(1, UL) << 18)
#define SCTLR_EL1_WXN_BIT		(_AC(1, UL) << 19)
#define SCTLR_EL1_UWXN_BIT		(_AC(1, UL) << 20)
#define SCTLR_EL1_IESB_BIT		(_AC(1, UL) << 21)
#define SCTLR_EL1_EIS_BIT		(_AC(1, UL) << 22)
#define SCTLR_EL1_SPAN_BIT		(_AC(1, UL) << 23)
#define SCTLR_EL1_E0E_BIT		(_AC(1, UL) << 24)
#define SCTLR_EL1_EE_BIT		(_AC(1, UL) << 25)
#define SCTLR_EL1_UCI_BIT		(_AC(1, UL) << 26)
#define SCTLR_EL1_EnDA_BIT		(_AC(1, UL) << 27)
#define SCTLR_EL1_nTLSMD_BIT		(_AC(1, UL) << 28)
#define SCTLR_EL1_LSMAOE_BIT		(_AC(1, UL) << 29)
#define SCTLR_EL1_EnIB_BIT		(_AC(1, UL) << 30)
#define SCTLR_EL1_EnIA_BIT		(_AC(1, UL) << 31)
#define SCTLR_EL1_BT0_BIT		(_AC(1, UL) << 35)
#define SCTLR_EL1_BT1_BIT		(_AC(1, UL) << 36)
#define SCTLR_EL1_TCF0_SHIFT		_AC(38, UL)
#define SCTLR_EL1_TCF0_MASK		_AC(2, UL)
#define SCTLR_EL1_TCF_SHIFT		_AC(40, UL)
#define SCTLR_EL1_TCF_MASK		_AC(2, UL)
#define SCTLR_EL1_ATA0_BIT		(_AC(1, UL) << 42)
#define SCTLR_EL1_ATA_BIT		(_AC(1, UL) << 43)
#define SCTLR_EL1_DSSBS_BIT		(_AC(1, UL) << 44)

/* SCTLR_EL1.TCF values */
#define SCTLR_EL1_TCF_IGNORE		_AC(0, UL)
#define SCTLR_EL1_TCF_SYNC		_AC(1, UL)
#define SCTLR_EL1_TCF_ASYNC		_AC(2, UL)
#define SCTLR_EL1_TCF_ASYMMETRIC	_AC(3, UL)

/* SPSR: Saved Program Status Register */
#define SPSR_EL1_SS			UK_BIT(21)
#define SPSR_EL1_D			UK_BIT(9)

/* TCR_EL1 - Translation Control Register */
#define TCR_EL1_DS_SHIFT		59
#define TCR_EL1_DS_BIT			(_AC(1, UL) << TCR_EL1_DS_SHIFT)
#define TCR_EL1_TCMA1_BIT		(_AC(1, UL) << 58)
#define TCR_EL1_TCMA0_BIT		(_AC(1, UL) << 57)
#define TCR_EL1_TBI1_BIT		(_AC(1, UL) << 38)
#define TCR_EL1_TBI0_BIT		(_AC(1, UL) << 37)
#define TCR_EL1_ASID_SHIFT		36
#define TCR_EL1_ASID_8			(_AC(0, UL) << TCR_EL1_ASID_SHIFT)
#define TCR_EL1_ASID_16			(_AC(1, UL) << TCR_EL1_ASID_SHIFT)
#define TCR_EL1_IPS_SHIFT		32
#define TCR_EL1_IPS_MASK		(_AC(0x7, UL))
#define TCR_EL1_TG1_SHIFT		30
#define TCR_EL1_TG1_MASK		_AC(0x3, UL)
#define TCR_EL1_SH1_SHIFT		28
#define TCR_EL1_SH1_IS			(_AC(0x3, UL) << TCR_EL1_SH1_SHIFT)
#define TCR_EL1_TG0_SHIFT		14
#define TCR_EL1_TG0_MASK		_AC(0x3, UL)
#define TCR_EL1_ORGN1_SHIFT		26
#define TCR_EL1_ORGN1_WBWA		(_AC(0x1, UL) << TCR_EL1_ORGN1_SHIFT)
#define TCR_EL1_IRGN1_SHIFT		24
#define TCR_EL1_IRGN1_WBWA		(_AC(0x1, UL) << TCR_EL1_IRGN1_SHIFT)
#define TCR_EL1_EPD1_SHIFT		23
#define TCR_EL1_EPD1_BIT		(_AC(1, UL) << TCR_EL1_EPD1_SHIFT)
#define TCR_EL1_T1SZ_SHIFT		16
#define TCR_EL1_SH0_SHIFT		12
#define TCR_EL1_SH0_IS			(_AC(0x3, UL) << TCR_EL1_SH0_SHIFT)
#define TCR_EL1_ORGN0_SHIFT		10
#define TCR_EL1_ORGN0_WBWA		(_AC(0x1, UL) << TCR_EL1_ORGN0_SHIFT)
#define TCR_EL1_IRGN0_SHIFT		8
#define TCR_EL1_IRGN0_WBWA		(_AC(0x1, UL) << TCR_EL1_IRGN0_SHIFT)
#define TCR_EL1_EPD0_SHIFT		7
#define TCR_EL1_EPD0_BIT		(_AC(1, UL) << TCR_EL1_EPD0_SHIFT)
#define TCR_EL1_T0SZ_MASK		0x3f
#define TCR_EL1_T0SZ_SHIFT		0

#define TCR_EL1_IPS_32			0
#define TCR_EL1_IPS_36			1
#define TCR_EL1_IPS_40			2
#define TCR_EL1_IPS_42			3
#define TCR_EL1_IPS_44			4
#define TCR_EL1_IPS_48			5
#define TCR_EL1_IPS_52			6
#define TCR_EL1_IPS(x)			(_AC(x, UL) << TCR_EL1_IPS_SHIFT)

#define TCR_EL1_T0SZ_52			12
#define TCR_EL1_T0SZ_48			16
#define TCR_EL1_T0SZ_44			20
#define TCR_EL1_T0SZ_42			22
#define TCR_EL1_T0SZ_40			24
#define TCR_EL1_T0SZ_36			28
#define TCR_EL1_T0SZ_32			32
#define TCR_EL1_T0SZ(x)			(_AC(x, UL) << TCR_EL1_T0SZ_SHIFT)
#define TCR_EL1_T1SZ(x)			(_AC(x, UL) << TCR_EL1_T1SZ_SHIFT)

#define TCR_EL1_TG0_4K			0
#define TCR_EL1_TG0_64K			1
#define TCR_EL1_TG0_16K			2

/* TTBR0_EL1 - Translation Table Base Register 0 */
#define TTBR0_EL1_ASID_MASK		_AC(0xffff000000000000, UL)
#define TTBR0_EL1_BADDR_MASK		_AC(0x0000fffffffffffe, UL)
#define TTBR0_EL1_CnP_MASK		_AC(0x0000000000000001, UL)

/* TTBR1_EL1 - Translation Table Base Register 1 */
#define TTBR1_EL1_ASID_MASK		_AC(0xffff000000000000, UL)
#define TTBR1_EL1_BADDR_MASK		_AC(0x0000fffffffffffe, UL)
#define TTBR1_EL1_CnP_MASK		_AC(0x0000000000000001, UL)

/* TTBR0_EL1 - Translation Table Base Register 0 */
#define TTBR0_EL1_ASID_MASK		_AC(0xffff000000000000, UL)
#define TTBR0_EL1_BADDR_MASK		_AC(0x0000fffffffffffe, UL)
#define TTBR0_EL1_CnP_MASK		_AC(0x0000000000000001, UL)

/* TTBR1_EL1 - Translation Table Base Register 1 */
#define TTBR1_EL1_ASID_MASK		_AC(0xffff000000000000, UL)
#define TTBR1_EL1_BADDR_MASK		_AC(0x0000fffffffffffe, UL)
#define TTBR1_EL1_CnP_MASK		_AC(0x0000000000000001, UL)

/**************************************************************************
 * VMSAv8-64 Register Definitions
 *************************************************************************/

/* Translation Table Descriptor Format */
#define PTE_VALID_BIT			1
#define PTE_TYPE_MASK			0x3
#define PTE_TYPE_BLOCK			1
#define PTE_TYPE_PAGE			3
#define PTE_TYPE_TABLE			3

#define PTE_L2_BLOCK_PADDR_MASK		_AC(0x0000ffffc0000000, UL)
#define PTE_L1_BLOCK_PADDR_MASK		_AC(0x0000ffffffe00000, UL)
#define PTE_L0_PAGE_PADDR_MASK		_AC(0x0000fffffffff000, UL)
#define PTE_Lx_TABLE_PADDR_MASK		_AC(0x0000fffffffff000, UL)

/* Translation Table Descriptor Attributes */
#define PTE_ATTR_MASK_H			_AC(0xfff0000000000000, UL)
#define PTE_ATTR_MASK_L			_AC(0x0000000000000fff, UL)
#define PTE_ATTR_MASK			(ATTR_MASK_H | ATTR_MASK_L)
#define PTE_ATTR_SW_MANAGED		(_AC(1, UL) << 56)
#define PTE_ATTR_SW_WIRED		(_AC(1, UL) << 55)
#define PTE_ATTR_UXN			(_AC(1, UL) << 54)
#define PTE_ATTR_PXN			(_AC(1, UL) << 53)
#define PTE_ATTR_XN			(PTE_ATTR_PXN | PTE_ATTR_UXN)
#define PTE_ATTR_CONTIGUOUS		(_AC(1, UL) << 52)
#define PTE_ATTR_DBM			(_AC(1, UL) << 51)
#define PTE_ATTR_GP			(_AC(1, UL) << 50)
#define PTE_ATTR_nG			(1 << 11)
#define PTE_ATTR_AF			(1 << 10)
#define PTE_ATTR_SH(x)			((x) << 8)
#define PTE_ATTR_SH_MASK		PTE_ATTR_SH(3)
#define PTE_ATTR_SH_NS			0 /* Non-shareable */
#define PTE_ATTR_SH_OS			2 /* Outer-shareable */
#define PTE_ATTR_SH_IS			3 /* Inner-shareable */
#define PTE_ATTR_AP_RW_BIT		(1 << 7)
#define PTE_ATTR_AP(x)			((x) << 6)
#define PTE_ATTR_AP_MASK		ATTR_AP(3)
#define PTE_ATTR_AP_RW			(0 << 1)
#define PTE_ATTR_AP_RO			(1 << 1)
#define PTE_ATTR_AP_USER		(1 << 0)
#define PTE_ATTR_NS			(1 << 5)
#define PTE_ATTR_IDX(x)			((x) << 2)
#define PTE_ATTR_IDX_MASK		(7 << 2)
