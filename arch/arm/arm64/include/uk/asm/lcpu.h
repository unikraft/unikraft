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

#ifndef __UKARCH_LCPU_H__
#error Do not include this header directly
#endif

#include <uk/asm.h>
#include <uk/asm/arch.h>
#include <uk/config.h>
#include <uk/essentials.h>

#define CACHE_LINE_SIZE		64

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
	(PTE_ATTR_AF | PTE_ATTR_SH(PTE_ATTR_SH_IS))

#define PTE_ATTR_DEVICE_nGnRE					\
	(PTE_ATTR_DEFAULT | PTE_ATTR_XN | PTE_ATTR_IDX(DEVICE_nGnRE))

#define PTE_ATTR_DEVICE_nGnRnE					\
	(PTE_ATTR_DEFAULT | PTE_ATTR_XN | PTE_ATTR_IDX(DEVICE_nGnRnE))

#ifdef CONFIG_ARM64_FEAT_MTE
#define PTE_ATTR_NORMAL_RW					\
	(PTE_ATTR_DEFAULT | PTE_ATTR_XN | PTE_ATTR_IDX(NORMAL_WB_TAGGED))
#else
#define PTE_ATTR_NORMAL_RW					\
	(PTE_ATTR_DEFAULT | PTE_ATTR_XN | PTE_ATTR_IDX(NORMAL_WB))
#endif /* CONFIG_ARM64_FEAT_MTE */

#define PTE_ATTR_NORMAL_RO					\
	(PTE_ATTR_DEFAULT | PTE_ATTR_XN |			\
	 PTE_ATTR_IDX(NORMAL_WB) | PTE_ATTR_AP_RW_BIT)

#ifdef CONFIG_ARM64_FEAT_BTI
#define PTE_ATTR_NORMAL_RWX					\
	(PTE_ATTR_DEFAULT | PTE_ATTR_UXN |			\
	 PTE_ATTR_IDX(NORMAL_WB) | PTE_ATTR_GP)
#define PTE_ATTR_NORMAL_RX					\
	(PTE_ATTR_DEFAULT | PTE_ATTR_UXN |			\
	 PTE_ATTR_IDX(NORMAL_WB) | PTE_ATTR_AP_RW_BIT |		\
	 PTE_ATTR_GP)
#else
#define PTE_ATTR_NORMAL_RWX					\
	(PTE_ATTR_DEFAULT | PTE_ATTR_UXN | PTE_ATTR_IDX(NORMAL_WB))
#define PTE_ATTR_NORMAL_RX					\
	(PTE_ATTR_DEFAULT | PTE_ATTR_UXN |			\
	 PTE_ATTR_IDX(NORMAL_WB) | PTE_ATTR_AP_RW_BIT)
#endif /* CONFIG_ARM64_FEAT_BTI */

/* Default SCTLR_EL1 configuration */

#define SCTLR_SET_BITS						\
	(SCTLR_EL1_UCI_BIT | SCTLR_EL1_nTWE_BIT |		\
	 SCTLR_EL1_nTWI_BIT | SCTLR_EL1_UCT_BIT |		\
	 SCTLR_EL1_DZE_BIT | SCTLR_EL1_I_BIT |			\
	 SCTLR_EL1_SED_BIT | SCTLR_EL1_SA0_BIT |		\
	 SCTLR_EL1_SA_BIT | SCTLR_EL1_C_BIT |			\
	 SCTLR_EL1_M_BIT | SCTLR_EL1_CP15BEN_BIT |		\
	 SCTLR_EL1_EOS_BIT | SCTLR_EL1_UWXN_BIT |		\
	 SCTLR_EL1_EIS_BIT | SCTLR_EL1_SPAN_BIT |		\
	 SCTLR_EL1_nTLSMD_BIT |	SCTLR_EL1_LSMAOE_BIT)

#define SCTLR_CLEAR_BITS \
	(SCTLR_EL1_EE_BIT | SCTLR_EL1_E0E_BIT |			\
	 SCTLR_EL1_WXN_BIT | SCTLR_EL1_UMA_BIT |		\
	 SCTLR_EL1_ITD_BIT | SCTLR_EL1_A_BIT |			\
	 SCTLR_EL1_nAA_BIT | SCTLR_EL1_EnRCTX_BIT |		\
	 SCTLR_EL1_EnDB_BIT | SCTLR_EL1_RES0_27_BIT |		\
	 SCTLR_EL1_EnDA_BIT | SCTLR_EL1_IESB_BIT |		\
	 SCTLR_EL1_EnIB_BIT | SCTLR_EL1_EnIA_BIT)

/* Default TCR_EL1 configuration */

#define TCR_CACHE_ATTRS						\
	(TCR_EL1_IRGN0_WBWA | TCR_EL1_IRGN1_WBWA |		\
	 TCR_EL1_ORGN0_WBWA | TCR_EL1_ORGN1_WBWA)

#define TCR_SMP_ATTRS						\
	(TCR_EL1_SH0_IS | TCR_EL1_SH1_IS)

#ifdef CONFIG_PAGING
/* Set TCR attributes as required by the arm64 paging implementation:
 * 48-bit IA, 48-bit OA, 4KiB granule, TTBR0_EL1 walks enabled,
 * TTBR1_EL1 walks disabled.
 */
#define TCR_INIT_FLAGS						\
	(TCR_EL1_ASID_16 | TCR_CACHE_ATTRS | TCR_SMP_ATTRS |	\
	 (TCR_EL1_TG0_4K << TCR_EL1_TG0_SHIFT) |		\
	 TCR_EL1_EPD1_BIT | TCR_EL1_T0SZ(TCR_EL1_T0SZ_48) |	\
	 TCR_EL1_IPS(TCR_EL1_IPS_48))
#else
#define TCR_INIT_FLAGS						\
	(TCR_EL1_ASID_16 | TCR_CACHE_ATTRS | TCR_SMP_ATTRS |	\
	 (TCR_EL1_TG0_4K << TCR_EL1_TG0_SHIFT))
#endif /* CONFIG_PAGING */

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
	(MAIR_EL1_ATTR(MAIR_DEVICE_nGnRnE, DEVICE_nGnRnE) |	\
	 MAIR_EL1_ATTR(MAIR_DEVICE_nGnRE, DEVICE_nGnRE) |	\
	 MAIR_EL1_ATTR(MAIR_DEVICE_GRE, DEVICE_GRE) |		\
	 MAIR_EL1_ATTR(MAIR_NORMAL_NC, NORMAL_NC) |		\
	 MAIR_EL1_ATTR(MAIR_NORMAL_WT, NORMAL_WT) |		\
	 MAIR_EL1_ATTR(MAIR_NORMAL_WB, NORMAL_WB) |		\
	 MAIR_EL1_ATTR(MAIR_NORMAL_WB_TAGGED, NORMAL_WB_TAGGED))

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
 * From exceptions come from EL0, we have to save sp_el0. So the
 * TRAP_STACK_SIZE should be 272 + 8 = 280. But we enable the stack
 * alignment check, we will force align the stack for EL1 exceptions,
 * so we add a sp to save original stack pointer: 280 + 8 = 288
 *
 * TODO: We'd better to calculate this size automatically later.
 */
#define __TRAP_STACK_SIZE	288
#define __SP_OFFSET		272
#define __SP_EL0_OFFSET		280

/*
 * In thread context switch, we will save the callee-saved registers
 * (x19 ~ x28) and Frame Point Register and Link Register to prev's
 * thread stack:
 * http://infocenter.arm.com/help/topic/com.arm.doc.ihi0055b/IHI0055B_aapcs64.pdf
 */
#define __CALLEE_SAVED_SIZE    96

#if !__ASSEMBLY__

#include <stdint.h>

/*
 * Change this structure must update TRAP_STACK_SIZE at the same time.
 * This data structure must be 16-byte alignment.
 */
struct __regs {
	/* Generic Purpose registers, from x0 ~ x29 */
	uint64_t x[30];

	/* Link Register (x30) */
	uint64_t lr;

	/* Exception Link Register */
	uint64_t elr_el1;

	/* Processor State Register */
	uint64_t spsr_el1;

	/* Exception Status Register */
	uint64_t esr_el1;

	/* Stack Pointer */
	uint64_t sp;

	/* Stack Pointer from el0 */
	uint64_t sp_el0;
};

/*
 * Change this structure must update __CALLEE_SAVED_SIZE at the
 * same time.
 */
struct __callee_saved_regs {
	/* Callee-saved registers, from x19 ~ x28 */
	uint64_t callee[10];

	/* Frame Point Register (x29) */
	uint64_t fp;

	/* Link Register (x30) */
	uint64_t lr;
};

/*
 * Instruction Synchronization Barrier flushes the pipeline in the
 * processor, so that all instructions following the ISB are fetched
 * from cache or memory, after the instruction has been completed.
 */
#define isb()   __asm__ __volatile("isb" ::: "memory")

/*
 * Options for DMB and DSB:
 *	oshld	Outer Shareable, load
 *	oshst	Outer Shareable, store
 *	osh	Outer Shareable, all
 *	nshld	Non-shareable, load
 *	nshst	Non-shareable, store
 *	nsh	Non-shareable, all
 *	ishld	Inner Shareable, load
 *	ishst	Inner Shareable, store
 *	ish	Inner Shareable, all
 *	ld	Full system, load
 *	st	Full system, store
 *	sy	Full system, all
 */
#define dmb(opt)    __asm__ __volatile("dmb " #opt ::: "memory")
#define dsb(opt)    __asm__ __volatile("dsb " #opt ::: "memory")

/* We probably only need "dmb" here, but we'll start by being paranoid. */
#ifndef mb
#define mb()    dsb(sy) /* Full system memory barrier all */
#endif

#ifndef rmb
#define rmb()   dsb(ld) /* Full system memory barrier load */
#endif

#ifndef wmb
#define wmb()   dsb(st) /* Full system memory barrier store */
#endif

/* Macros to access system registers */
#define SYSREG_READ(reg)					\
({	uint64_t val;						\
	__asm__ __volatile__("mrs %0, " __STRINGIFY(reg)	\
			: "=r" (val));				\
	val;							\
})

#define SYSREG_WRITE(reg, val)					\
({	__asm__ __volatile__("msr " __STRINGIFY(reg) ", %0"	\
			: : "r" ((uint64_t)(val)));		\
})

#define SYSREG_READ32(reg)					\
({	uint32_t val;						\
	__asm__ __volatile__("mrs %0, " __STRINGIFY(reg)	\
			: "=r" (val));				\
	val;							\
})

#define SYSREG_WRITE32(reg, val)				\
({	__asm__ __volatile__("msr " __STRINGIFY(reg) ", %0"	\
			: : "r" ((uint32_t)(val)));		\
})

#define SYSREG_READ64(reg)			SYSREG_READ(reg)
#define SYSREG_WRITE64(reg, val)		SYSREG_WRITE(reg, val)

/*
 * we should use inline assembly with volatile constraint to access mmio
 * device memory to avoid compiler use load/store instructions of writeback
 * addressing mode which will cause crash when running in hyper mode
 * unless they will be decoded by hypervisor.
 */
static inline uint8_t ioreg_read8(const volatile uint8_t *address)
{
	uint8_t value;

	__asm__ __volatile__("ldrb %w0, [%1]" : "=r"(value) : "r"(address));
	return value;
}

static inline uint16_t ioreg_read16(const volatile uint16_t *address)
{
	uint16_t value;

	__asm__ __volatile__("ldrh %w0, [%1]" : "=r"(value) : "r"(address));
	return value;
}

static inline uint32_t ioreg_read32(const volatile uint32_t *address)
{
	uint32_t value;

	__asm__ __volatile__("ldr %w0, [%1]" : "=r"(value) : "r"(address));
	return value;
}

static inline uint64_t ioreg_read64(const volatile uint64_t *address)
{
	uint64_t value;

	__asm__ __volatile__("ldr %0, [%1]" : "=r"(value) : "r"(address));
	return value;
}

static inline void ioreg_write8(const volatile uint8_t *address, uint8_t value)
{
	__asm__ __volatile__("strb %w0, [%1]" : : "rZ"(value), "r"(address));
}

static inline void ioreg_write16(const volatile uint16_t *address,
				 uint16_t value)
{
	__asm__ __volatile__("strh %w0, [%1]" : : "rZ"(value), "r"(address));
}

static inline void ioreg_write32(const volatile uint32_t *address,
				 uint32_t value)
{
	__asm__ __volatile__("str %w0, [%1]" : : "rZ"(value), "r"(address));
}

static inline void ioreg_write64(const volatile uint64_t *address,
				 uint64_t value)
{
	__asm__ __volatile__("str %0, [%1]" : : "rZ"(value), "r"(address));
}

static inline unsigned long ukarch_read_sp(void)
{
	unsigned long sp;

	__asm__ __volatile("mov %0, sp": "=&r"(sp));

	return sp;
}

static inline void ukarch_spinwait(void)
{
	/* Intelligent busy wait not supported on arm64. */
}

#endif /* !__ASSEMBLY__ */
