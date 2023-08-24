/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2020, OpenSynergy GmbH. All rights reserved.
 *
 * ARM Generic Interrupt Controller support v3 version
 * based on drivers/ukintctlr/gic/include/uk/intctlr/gic-v2.h:
 *
 * Authors: Wei Chen <Wei.Chen@arm.com>
 *          Jianyong Wu <Jianyong.Wu@arm.com>
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
 */
#ifndef __UK_INTCTLR_GICV3_H__
#define __UK_INTCTLR_GICV3_H__

#include <uk/intctlr/gic.h>

/** Affinity AFF3 bit mask */
#define MPIDR_AFF3_MASK			0xff00000000
/** Affinity AFF2 bit mask */
#define MPIDR_AFF2_MASK			0x0000ff0000
/** Affinity AFF1 bit mask */
#define MPIDR_AFF1_MASK			0x000000ff00
/** Affinity AFF0 bit mask */
#define MPIDR_AFF0_MASK			0x00000000ff

/*
 * GIC System register assembly aliases
 */
#define ICC_PMR_EL1			S3_0_C4_C6_0
#define ICC_DIR_EL1			S3_0_C12_C11_1
#define ICC_SGI1R_EL1			S3_0_C12_C11_5
#define ICC_EOIR1_EL1			S3_0_C12_C12_1
#define ICC_IAR1_EL1			S3_0_C12_C12_0
#define ICC_BPR1_EL1			S3_0_C12_C12_3
#define ICC_CTLR_EL1			S3_0_C12_C12_4
#define ICC_SRE_EL1			S3_0_C12_C12_5
#define ICC_IGRPEN1_EL1			S3_0_C12_C12_7

/*
 * Distributor and Redistributor registers
 */
#define GICC_CTLR_EL1_EOImode_drop	(1U << 1)

/* Default size according to ARM Generic Interrupt Controller Architecture
 * Specification GIC Architecture version 3 and version 4 Issue H.
 */
#define GICD_V3_MEM_SZ			0x10000

#define GICD_STATUSR			(0x010)
#define GICD_SETSPI_NSR			(0x040)
#define GICD_CLRSPI_NSR			(0x048)
#define GICD_SETSPI_SR			(0x050)
#define GICD_CLRSPI_SR			(0x058)
#define GICD_IROUTER_BASE		(0x6000)
#define GICD_IROUTER32			(0x6100)
#define GICD_IROUTER1019		(0x7FD8)
#define GICD_PIDR2			(0xFFE8)

#define GICD_CTLR_RWP			(1UL << 31)
#define GICD_CTLR_ARE_NS		(1U << 4)
#define GICD_CTLR_ENABLE_G1NS		(1U << 1)
#define GICD_CTLR_ENABLE_G0		(1U << 0)

/* Common between GICD_PIDR2 and GICR_PIDR2 */
#define GIC_PIDR2_ARCH_MASK		(0xf0)
#define GIC_PIDR2_ARCH_GICv3		(0x30)
#define GIC_PIDR2_ARCH_GICv4		(0x40)

/* Additional bits in GICD_TYPER defined by GICv3 */
#define GICD_TYPE_ID_BITS_SHIFT 19
#define GICD_TYPE_ID_BITS(r)					\
	((((r) >> GICD_TYPE_ID_BITS_SHIFT) & 0x1f) + 1)

#define GICD_TYPE_LPIS			(1U << 17)

#define GICR_WAKER_ProcessorSleep	(1U << 1)
#define GICR_WAKER_ChildrenAsleep	(1U << 2)

#define GICR_CTLR			(0x0000)
#define GICR_IIDR			(0x0004)
#define GICR_TYPER			(0x0008)
#define GICR_STATUSR			(0x0010)
#define GICR_WAKER			(0x0014)
#define GICR_SETLPIR			(0x0040)
#define GICR_CLRLPIR			(0x0048)
#define GICR_PROPBASER			(0x0070)
#define GICR_PENDBASER			(0x0078)
#define GICR_INVLPIR			(0x00A0)
#define GICR_INVALLR			(0x00B0)
#define GICR_SYNCR			(0x00C0)

#define GICR_TYPER_PLPIS		(1U << 0)
#define GICR_TYPER_VLPIS		(1U << 1)
#define GICR_TYPER_LAST			(1U << 4)
#define GICR_TYPER_PROC_NUM_SHIFT	8
#define GICR_TYPER_PROC_NUM_MASK	(0xffff << GICR_TYPER_PROC_NUM_SHIFT)

/* GICR frames offset */
#define GICR_STRIDE			(0x20000)
#define GICR_RD_BASE			(0)
#define GICR_SGI_BASE			(0x10000)

/* GICR for SGI's & PPI's */
#define GICR_IGROUPR0			(GICR_SGI_BASE + 0x0080)
#define GICR_ISENABLER0			(GICR_SGI_BASE + 0x0100)
#define GICR_ICENABLER0			(GICR_SGI_BASE + 0x0180)
#define GICR_ISPENDR0			(GICR_SGI_BASE + 0x0200)
#define GICR_ICPENDR0			(GICR_SGI_BASE + 0x0280)
#define GICR_ISACTIVER0			(GICR_SGI_BASE + 0x0300)
#define GICR_ICACTIVER0			(GICR_SGI_BASE + 0x0380)
#define GICR_IPRIORITYR0		(GICR_SGI_BASE + 0x0400)
#define GICR_IPRIORITYR7		(GICR_SGI_BASE + 0x041C)
#define GICR_ICFGR0			(GICR_SGI_BASE + 0x0C00)
#define GICR_ICFGR1			(GICR_SGI_BASE + 0x0C04)
#define GICR_IGRPMODR0			(GICR_SGI_BASE + 0x0D00)
#define GICR_NSACR			(GICR_SGI_BASE + 0x0E00)

/*
 * Distributor registers. Unikraft only support run on non-secure
 * so we just describe non-secure registers.
 * Redistributor registers are also described when necessary
 */
#define GICD_TYPE_LPIS		(1U << 17)

/* Register bits */
#define GICD_CTL_ENABLE		0x1

#define GICD_TYPE_LINES		0x01f
#define GICD_TYPE_CPUS_SHIFT	5
#define GICD_TYPE_CPUS		0x0e0
#define GICD_TYPE_SEC		0x400
#define GICD_TYPER_DVIS		(1U << 18)

#define GICD_IROUTER(n)		(GICD_IROUTER_BASE + (n * 8))

/*
 * Distributor Control Register, GICD_CTLR.
 * Enables the forwarding of pending interrupts from the
 * Distributor to the CPU interfaces
 */
#define GICD_CTLR		0x0000
#define GICD_CTLR_ENABLE	0x1

/*
 * Interrupt Controller Type Register, GICD_TYPER.
 * Provides information about the configuration of the GIC.
 */
#define GICD_TYPER		0x0004
#define GICD_TYPER_LINE_NUM(r)	((((r) & 0x1f) + 1) << 5)
#define GICD_TYPER_CPUI_NUM(r)	((((r) >> 5) & 0x3) + 1)

/*
 * Distributor Implementer Identification Register, GICD_IIDR.
 * Provides information about the implementer and revision of the Distributor.
 */
#define GICD_IIDR		0x0008
#define GICD_IIDR_PROD(r)	(((r) >> 24) & 0xff)
#define GICD_IIDR_VAR(r)	(((r) >> 16) & 0xf)
#define GICD_IIDR_REV(r)	(((r) >> 12) & 0xf)
#define GICD_IIDR_IMPL(r)	((r) & 0xfff)

/*
 * Interrupt Group Registers, GICD_IGROUPRn
 * These registers provide a status bit for each interrupt supported by
 * the GIC. Each bit controls whether the corresponding interrupt is in
 * Group 0 or Group 1
 */
#define GICD_IGROUPR(n)		(0x0080 + 4 * ((n) >> 5))
#define GICD_I_PER_IGROUPRn	32
#define GICD_DEF_IGROUPRn	0xffffffff

/*
 * Interrupt Set-Enable Registers, GICD_ISENABLERn.
 * These registers provide a Set-enable bit for each interrupt supported
 * by the GIC. Writing 1 to a Set-enable bit enables forwarding of the
 * corresponding interrupt from the Distributor to the CPU interfaces.
 * Reading a bit identifies whether the interrupt is enabled.
 */
#define GICD_ISENABLER(n)	(0x0100 + 4 * ((n) >> 5))
#define GICD_I_PER_ISENABLERn	32
#define GICD_DEF_SGI_ISENABLERn	0xffff
#define GICR_I_PER_ISENABLERn	32

/*
 * Interrupt Clear-Enable Registers, GICD_ICENABLERn.
 * Provide a Clear-enable bit for each interrupt supported by the GIC.
 * Writing 1 to a Clear-enable bit disables forwarding of the
 * corresponding interrupt from the Distributor to the CPU interfaces.
 * Reading a bit identifies whether the interrupt is enabled.
 */
#define GICD_ICENABLER(n)	(0x0180 + 4 * ((n) >> 5))
#define GICD_I_PER_ICENABLERn	32
#define GICD_DEF_ICENABLERn	0xffffffff
#define GICD_DEF_PPI_ICENABLERn	0xffff0000
#define GICR_I_PER_ICENABLERn	32

/*
 * Interrupt Set-Pending Registers, GICD_ISPENDRn.
 * Provide a Set-pending bit for each interrupt supported by the GIC.
 * Writing 1 to a Set-pending bit sets the status of the corresponding
 * peripheral interrupt to pending. Reading a bit identifies whether
 * the interrupt is pending.
 */
#define GICD_ISPENDR(n)		(0x0200 + 4 * ((n) >> 5))
#define GICD_I_PER_ISPENDRn	32
/*
 * Interrupt Clear-Pending Registers, GICD_ICPENDRn
 * Provide a Clear-pending bit for each interrupt supported by the GIC.
 * Writing 1 to a Clear-pending bit clears the pending state of the
 * corresponding peripheral interrupt. Reading a bit identifies whether
 * the interrupt is pending.
 */
#define GICD_ICPENDR(n)		(0x0280 + 4 * ((n) >> 5))
#define GICD_I_PER_ICPENDRn	32

/*
 * Interrupt Set-Active Registers, GICD_ISACTIVERn
 * Provide a Set-active bit for each interrupt that the GIC supports.
 * Writing to a Set-active bit Activates the corresponding interrupt.
 * These registers are used when preserving and restoring GIC state.
 */
#define GICD_ISACTIVER(n)	(0x0300 + 4 * ((n) >> 5))
#define GICD_I_PER_ISACTIVERn	32
/*
 * Interrupt Clear-Active Registers, GICD_ICACTIVERn
 * Provide a Clear-active bit for each interrupt that the GIC supports.
 * Writing to a Clear-active bit Deactivates the corresponding interrupt.
 * These registers are used when preserving and restoring GIC state.
 */
#define GICD_ICACTIVER(n)	(0x0380 + 4 * ((n) >> 5))
#define GICD_I_PER_ICACTIVERn	32
#define GICD_DEF_ICACTIVERn	0xffffffff

/*
 * Interrupt ID mask for GICD_ISENABLER, GICD_ICENABLER, GICD_ISPENDR,
 * GICD_ICPENDR, GICD_ISACTIVER and GICD_ICACTIVER
 */
#define GICD_I_MASK(n)		(1ul << ((n) & 0x1f))

/*
 * Interrupt Priority Registers, GICD_IPRIORITYRn
 * Provide an 8-bit priority field for each interrupt supported by the
 * GIC.
 *
 * These registers are byte-accessible, so we define this macro
 * for byte-access.
 */
#define GICD_IPRIORITYR(n)	(0x0400 + (n))
#define GICR_IPRIORITYR(n)	(GICR_SGI_BASE + 0x0400 + (n))

/*
 * Interrupt Priority Registers, GICD_IPRIORITYRn
 * Provide a macro to access the offset of GICD_IPRIORITYRn register
 * for a given interrupt
 *
 * These registers are 32 bits and contains the priority for 4 interrupts,
 * so this macro can be used to set the priority of many interrupts at the
 * same time.
 */
#define GICD_IPRIORITYR4(n)	(0x0400 + 4 * ((n) >> 2))
#define GICR_IPRIORITYR4(n)	(GICR_SGI_BASE + 0x0400 + 4 * ((n) >> 2))
#define GICD_I_PER_IPRIORITYn	4
#define GICD_IPRIORITY_DEF	0x80808080

/*
 * Interrupt Processor Targets Registers, GICD_ITARGETSRn
 * Provide an 8-bit CPU targets field for each interrupt supported by
 * the GIC.
 *
 * These registers are byte-accessible, so we define this macro
 * for byte-access.
 */
#define GICD_ITARGETSR(n)	(0x0800 + (n))

/*
 * Interrupt Processor Targets Registers, GICD_ITARGETSRn
 * Provide an 8-bit CPU targets field for each interrupt supported by
 * the GIC.
 *
 * These registers are 32 bits and contains the CPU targets for 4 interrupts,
 * so this macro can be used to set the CPU targets of many interrupts at the
 * same time.
 */
#define GICD_ITARGETSR4(n)	(0x0800 + 4 * ((n) >> 2))
#define GICD_I_PER_ITARGETSRn	4
#define GICD_ITARGETSR_DEF	0xffffffff

/*
 * Interrupt Configuration Registers, GICD_ICFGRn
 * The GICD_ICFGRs provide a 2-bit Int_config field for each interrupt
 * supported by the GIC. This field identifies whether the corresponding
 * interrupt is edge-triggered or level-sensitive.
 */
#define GICD_ICFGR(n)		(0x0C00 + 4 * ((n) >> 4))
#define GICD_I_PER_ICFGRn	16
#define GICD_ICFGR_DEF_TYPE	0
#define GICD_ICFGR_MASK		0x3
/* First bit is a polarity bit (0 - low, 1 - high) */
#define GICD_ICFGR_POL_LOW	(0 << 0)
#define GICD_ICFGR_POL_HIGH	(1 << 0)
#define GICD_ICFGR_POL_MASK	0x1
/* Second bit is a trigger bit (0 - level, 1 - edge) */
#define GICD_ICFGR_TRIG_LVL	(0 << 1)
#define GICD_ICFGR_TRIG_EDGE	(1 << 1)
#define GICD_ICFGR_TRIG_MASK	0x2

/*
 * Software Generated Interrupt Register, GICD_SGIR
 */
#define GICD_SGIR		0x0F00
#define GICD_SGI_TARGET_SHIFT	16
#define GICD_SGI_TARGET_MASK	0xff
#define GICD_SGI_FILTER_SHIFT	24
#define GICD_SGI_FILTER_MASK	0x3
#define GICD_SGI_MAX_INITID	15
#define GICD_PPI_START

/*
 * SGI Clear-Pending Registers, GICD_CPENDSGIRn
 * Provide a clear-pending bit for each supported SGI and source
 * processor combination. When a processor writes a 1 to a clear-pending
 * bit, the pending state of the corresponding SGI for the corresponding
 * source processor is removed, and no longer targets the processor
 * performing the write. Writing a 0 has no effect. Reading a bit identifies
 * whether the SGI is pending, from the corresponding source processor, on
 * the reading processor.
 */
#define GICD_CPENDSGIRn		(0x0F10 + 4 * ((n) >> 2))
#define GICD_I_PER_CPENDSGIRn	4

/*
 * SGI Set-Pending Registers, GICD_SPENDSGIRn
 * Provide a set-pending bit for each supported SGI and source processor
 * combination. When a processor writes a 1 to a set-pending bit, the pending
 * state is applied to the corresponding SGI for the corresponding source
 * processor. Writing a 0 has no effect. Reading a bit identifies whether
 * the SGI is pending, from the corresponding source processor, on the
 * reading processor.
 */
#define GICD_SPENDSGIRn		(0x0F20 + 4 * ((n) >> 2))
#define GICD_I_PER_SPENDSGIRn	4

/* Interrupt Acknowledge Register */
#define GICC_IAR_INTID_MASK		0x3FF
#define GICC_IAR_INTID_SPURIOUS	1023

/**
 * Probe device tree or ACPI for GICv3
 * NOTE: First time must not be called from multiple CPUs in parallel
 *
 * @param [out] dev receives pointer to GICv3 if available, NULL otherwise
 * @return 0 if device is available, < 0 otherwise
 */
int gicv3_probe(struct _gic_dev **dev);

#endif /* __UK_INTCTLR_GICV3_H__ */
