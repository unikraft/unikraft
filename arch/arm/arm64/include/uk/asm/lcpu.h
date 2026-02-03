/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2009, Citrix Systems, Inc.
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation.
 * Copyright (c) 2022, OpenSynergy GmbH.
 * Copyright (c) 2018, Arm Ltd.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef __UK_ASM_LCPU_H__
#define __UK_ASM_LCPU_H__

#include <uk/asm.h>
#include <uk/arch/arm64.h>
#include <uk/config.h>
#include <uk/essentials.h>

/* Device-nGnRnE memory */
#define MAIR_DEVICE_nGnRnE	0x00
/* Device-nGnRE memory */
#define MAIR_DEVICE_nGnRE	0x04
/* Device-GRE memory */
#define MAIR_DEVICE_GRE		0x0C
/* Outer Non-cacheable + Inner Non-cacheable */
#define MAIR_NORMAL_NC		0x44
/* Outer + Inner Write-back non-transient */
#define MAIR_NORMAL_WB		0xff
/* Tagged Outer + Inner Write-back non-transient */
#define	MAIR_NORMAL_WB_TAGGED	0xf0
/* Outer + Inner Write-through non-transient */
#define MAIR_NORMAL_WT		0xbb

/* Memory attributes */
#define PTE_ATTR_DEFAULT					\
	(UK_ARCH_ARM64_PTE_ATTR_AF | UK_ARCH_ARM64_PTE_ATTR_SH(UK_ARCH_ARM64_PTE_ATTR_SH_IS))

#define PTE_ATTR_DEVICE_nGnRE					\
	(PTE_ATTR_DEFAULT | UK_ARCH_ARM64_PTE_ATTR_XN | UK_ARCH_ARM64_PTE_ATTR_IDX(DEVICE_nGnRE))

#define PTE_ATTR_DEVICE_nGnRnE					\
	(PTE_ATTR_DEFAULT | UK_ARCH_ARM64_PTE_ATTR_XN | UK_ARCH_ARM64_PTE_ATTR_IDX(DEVICE_nGnRnE))

#ifdef CONFIG_ARM64_FEAT_MTE
#define PTE_ATTR_NORMAL_RW					\
	(PTE_ATTR_DEFAULT | UK_ARCH_ARM64_PTE_ATTR_XN | UK_ARCH_ARM64_PTE_ATTR_IDX(NORMAL_WB_TAGGED))
#else
#define PTE_ATTR_NORMAL_RW					\
	(PTE_ATTR_DEFAULT | UK_ARCH_ARM64_PTE_ATTR_XN | UK_ARCH_ARM64_PTE_ATTR_IDX(NORMAL_WB))
#endif /* CONFIG_ARM64_FEAT_MTE */

#define PTE_ATTR_NORMAL_RO					\
	(PTE_ATTR_DEFAULT | UK_ARCH_ARM64_PTE_ATTR_XN |			\
	 UK_ARCH_ARM64_PTE_ATTR_IDX(NORMAL_WB) | UK_ARCH_ARM64_PTE_ATTR_AP_RW_BIT)

#ifdef CONFIG_ARM64_FEAT_BTI
#define PTE_ATTR_NORMAL_RWX					\
	(PTE_ATTR_DEFAULT | UK_ARCH_ARM64_PTE_ATTR_UXN |			\
	 UK_ARCH_ARM64_PTE_ATTR_IDX(NORMAL_WB) | UK_ARCH_ARM64_PTE_ATTR_GP)
#define PTE_ATTR_NORMAL_RX					\
	(PTE_ATTR_DEFAULT | UK_ARCH_ARM64_PTE_ATTR_UXN |			\
	 UK_ARCH_ARM64_PTE_ATTR_IDX(NORMAL_WB) | UK_ARCH_ARM64_PTE_ATTR_AP_RW_BIT |		\
	 UK_ARCH_ARM64_PTE_ATTR_GP)
#else
#define PTE_ATTR_NORMAL_RWX					\
	(PTE_ATTR_DEFAULT | UK_ARCH_ARM64_PTE_ATTR_UXN | UK_ARCH_ARM64_PTE_ATTR_IDX(NORMAL_WB))
#define PTE_ATTR_NORMAL_RX					\
	(PTE_ATTR_DEFAULT | UK_ARCH_ARM64_PTE_ATTR_UXN |			\
	 UK_ARCH_ARM64_PTE_ATTR_IDX(NORMAL_WB) | UK_ARCH_ARM64_PTE_ATTR_AP_RW_BIT)
#endif /* CONFIG_ARM64_FEAT_BTI */

/* Default SCTLR_EL1 configuration */

#define SCTLR_SET_BITS						\
	(UK_ARCH_ARM64_SCTLR_EL1_UCI_BIT | UK_ARCH_ARM64_SCTLR_EL1_nTWE_BIT |		\
	 UK_ARCH_ARM64_SCTLR_EL1_nTWI_BIT | UK_ARCH_ARM64_SCTLR_EL1_UCT_BIT |		\
	 UK_ARCH_ARM64_SCTLR_EL1_DZE_BIT | UK_ARCH_ARM64_SCTLR_EL1_I_BIT |			\
	 UK_ARCH_ARM64_SCTLR_EL1_SED_BIT | UK_ARCH_ARM64_SCTLR_EL1_SA0_BIT |		\
	 UK_ARCH_ARM64_SCTLR_EL1_SA_BIT | UK_ARCH_ARM64_SCTLR_EL1_C_BIT |			\
	 UK_ARCH_ARM64_SCTLR_EL1_M_BIT | UK_ARCH_ARM64_SCTLR_EL1_CP15BEN_BIT |		\
	 UK_ARCH_ARM64_SCTLR_EL1_EOS_BIT | UK_ARCH_ARM64_SCTLR_EL1_UWXN_BIT |		\
	 UK_ARCH_ARM64_SCTLR_EL1_EIS_BIT | UK_ARCH_ARM64_SCTLR_EL1_SPAN_BIT |		\
	 UK_ARCH_ARM64_SCTLR_EL1_nTLSMD_BIT |	UK_ARCH_ARM64_SCTLR_EL1_LSMAOE_BIT)

#define SCTLR_CLEAR_BITS \
	(UK_ARCH_ARM64_SCTLR_EL1_EE_BIT | UK_ARCH_ARM64_SCTLR_EL1_E0E_BIT |			\
	 UK_ARCH_ARM64_SCTLR_EL1_WXN_BIT | UK_ARCH_ARM64_SCTLR_EL1_UMA_BIT |		\
	 UK_ARCH_ARM64_SCTLR_EL1_ITD_BIT | UK_ARCH_ARM64_SCTLR_EL1_A_BIT |			\
	 UK_ARCH_ARM64_SCTLR_EL1_nAA_BIT | UK_ARCH_ARM64_SCTLR_EL1_EnRCTX_BIT |		\
	 UK_ARCH_ARM64_SCTLR_EL1_EnDB_BIT | UK_ARCH_ARM64_SCTLR_EL1_RES0_27_BIT |		\
	 UK_ARCH_ARM64_SCTLR_EL1_EnDA_BIT | UK_ARCH_ARM64_SCTLR_EL1_IESB_BIT |		\
	 UK_ARCH_ARM64_SCTLR_EL1_EnIB_BIT | UK_ARCH_ARM64_SCTLR_EL1_EnIA_BIT)

/* Default TCR_EL1 configuration */

#define TCR_CACHE_ATTRS						\
	(UK_ARCH_ARM64_TCR_EL1_IRGN0_WBWA | UK_ARCH_ARM64_TCR_EL1_IRGN1_WBWA |		\
	 UK_ARCH_ARM64_TCR_EL1_ORGN0_WBWA | UK_ARCH_ARM64_TCR_EL1_ORGN1_WBWA)

#define TCR_SMP_ATTRS						\
	(UK_ARCH_ARM64_TCR_EL1_SH0_IS | UK_ARCH_ARM64_TCR_EL1_SH1_IS)

#if CONFIG_HAVE_PAGING
/* Set TCR attributes as required by the arm64 paging implementation:
 * 48-bit IA, 48-bit OA, 4KiB granule, TTBR0_EL1 walks enabled,
 * TTBR1_EL1 walks disabled.
 */
#define TCR_INIT_FLAGS						\
	(UK_ARCH_ARM64_TCR_EL1_ASID_16 | TCR_CACHE_ATTRS | TCR_SMP_ATTRS |	\
	 (UK_ARCH_ARM64_TCR_EL1_TG0_4K << UK_ARCH_ARM64_TCR_EL1_TG0_SHIFT) |		\
	 UK_ARCH_ARM64_TCR_EL1_EPD1_BIT | UK_ARCH_ARM64_TCR_EL1_T0SZ(UK_ARCH_ARM64_TCR_EL1_T0SZ_48) |	\
	 UK_ARCH_ARM64_TCR_EL1_IPS(UK_ARCH_ARM64_TCR_EL1_IPS_48))
#else /* ! CONFIG_HAVE_PAGING */
#define TCR_INIT_FLAGS						\
	(UK_ARCH_ARM64_TCR_EL1_ASID_16 | TCR_CACHE_ATTRS | TCR_SMP_ATTRS |	\
	 (UK_ARCH_ARM64_TCR_EL1_TG0_4K << UK_ARCH_ARM64_TCR_EL1_TG0_SHIFT))
#endif /* !CONFIG_HAVE_PAGING */

/* Default MAIR_EL1 configuration */

/* These are the indexes in MAIR_EL1 */
#define DEVICE_nGnRnE		0
#define DEVICE_nGnRE		1
#define DEVICE_GRE		2
#define NORMAL_NC		3
#define NORMAL_WT		4
#define NORMAL_WB		5
#define NORMAL_WB_TAGGED	6

#define MAIR_INIT_ATTR						\
	(UK_ARCH_ARM64_MAIR_EL1_ATTR(MAIR_DEVICE_nGnRnE, DEVICE_nGnRnE) |	\
	 UK_ARCH_ARM64_MAIR_EL1_ATTR(MAIR_DEVICE_nGnRE, DEVICE_nGnRE) |	\
	 UK_ARCH_ARM64_MAIR_EL1_ATTR(MAIR_DEVICE_GRE, DEVICE_GRE) |		\
	 UK_ARCH_ARM64_MAIR_EL1_ATTR(MAIR_NORMAL_NC, NORMAL_NC) |		\
	 UK_ARCH_ARM64_MAIR_EL1_ATTR(MAIR_NORMAL_WT, NORMAL_WT) |		\
	 UK_ARCH_ARM64_MAIR_EL1_ATTR(MAIR_NORMAL_WB, NORMAL_WB) |		\
	 UK_ARCH_ARM64_MAIR_EL1_ATTR(MAIR_NORMAL_WB_TAGGED, NORMAL_WB_TAGGED))

/* Mapping of TCR_EL1.IPS to number of bits */
#ifdef __ASSEMBLY__
tcr_ips_bits:
	.byte 32, 36, 40, 42, 44, 48, 52
#else
static __attribute__((unused))
unsigned char tcr_ips_bits[] = {32, 36, 40, 42, 44, 48, 52};
#endif

/*
 * Stack size to save general purpose registers and essential system
 * registers. 8 * (30 + lr + elr_el1 + spsr_el1 + esr_el1) = 272.
 * We enable the stack alignment check, we will force align the stack for
 * EL1 exceptions, so we add a sp to save original stack pointer: 272 + 8 = 280
 * and then a padding of 8 bytes: 280 + 8 = 288 (288 % 16 == 0).
 *
 * TODO: We'd better to calculate this size automatically later.
 */
#define __TRAP_STACK_SIZE	288
#define __SP_OFFSET		272

#define __REGS_PAD_SIZE		8
#define __REGS_SIZEOF		__TRAP_STACK_SIZE

/*
 * In thread context switch, we will save the callee-saved registers
 * (x19 ~ x28) and Frame Point Register and Link Register to prev's
 * thread stack:
 * http://infocenter.arm.com/help/topic/com.arm.doc.ihi0055b/IHI0055B_aapcs64.pdf
 */
#define __CALLEE_SAVED_SIZE    96

#if !__ASSEMBLY__

#include <uk/arch/types.h>

/*
 * Mappings of `struct __regs` register fields
 * according to AAPCS64 definition for function calls
 *  rargX    - Arguments 0..7
 *  rretX    - Function call return value
 */

#define __fn_rarg0		x[0]
#define __fn_rarg1		x[1]
#define __fn_rarg2		x[2]
#define __fn_rarg3		x[3]
#define __fn_rarg4		x[4]
#define __fn_rarg5		x[5]
#define __fn_rarg6		x[6]
#define __fn_rarg7		x[7]

#define __fn_rret0		x[0]
#define __fn_rret1		x[1]

/*
 * Change this structure must update TRAP_STACK_SIZE at the same time.
 * This data structure must be 16-byte alignment.
 */
struct __regs {
	/* Generic Purpose registers, from x0 ~ x29 */
	__u64 x[30];

	/* Link Register (x30) */
	__u64 lr;

	/* Exception Link Register */
	__u64 elr_el1;

	/* Processor State Register */
	__u64 spsr_el1;

	/* Exception Status Register */
	__u64 esr_el1;

	/* Stack Pointer */
	__u64 sp;

	/* Padding to make sure this structure is 16-byte aligned */
	__u8 pad[__REGS_PAD_SIZE];
};

UK_CTASSERT(sizeof(struct __regs) == __REGS_SIZEOF);

static inline __uptr ukarch_regs_get_sp(struct __regs *r)
{
	return r->sp;
}

static inline void ukarch_regs_set_sp(__uptr sp, struct __regs *r)
{
	r->sp = sp;
}

static inline __uptr ukarch_regs_get_pc(struct __regs *r)
{
	return r->elr_el1;
}

static inline void ukarch_regs_set_pc(__uptr pc, struct __regs *r)
{
	r->elr_el1 = pc;
}

/*
 * Change this structure must update __CALLEE_SAVED_SIZE at the
 * same time.
 */
struct __callee_saved_regs {
	/* Callee-saved registers, from x19 ~ x28 */
	__u64 callee[10];

	/* Frame Point Register (x29) */
	__u64 fp;

	/* Link Register (x30) */
	__u64 lr;
};

UK_CTASSERT(sizeof(struct __callee_saved_regs) == __CALLEE_SAVED_SIZE);

#endif /* !__ASSEMBLY__ */
#endif /* __UK_ASM_LCPU_H__ */
