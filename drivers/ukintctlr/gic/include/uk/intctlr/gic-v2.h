/* SPDX-License-Identifier: BSD-3-Clause */
/*
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
#ifndef __UK_INTCTLR_GICV2_H__
#define __UK_INTCTLR_GICV2_H__

#include <uk/intctlr/gic.h>

/* GICv2 GICD register map size page aligned up according to
 * ARM Generic Interrupt Controller Architecture version 2.0 Issue B.b.
 */
#define GICD_V2_MEM_SZ					0x01000

/*
 * Distributor registers. Unikraft only supports running on non-secure
 * so we just describe non-secure registers.
 */

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

enum sgi_filter {
	/*
	 * Forward the interrupt to the CPU interfaces specified in the
	 * CPUTargetList field
	 */
	GICD_SGI_FILTER_TO_LIST = 0,
	/*
	 * Forward the interrupt to all CPU interfaces except that of the
	 * processor that requested the interrupt.
	 */
	GICD_SGI_FILTER_TO_OTHERS,
	/*
	 * Forward the interrupt only to the CPU interface of the processor
	 * that requested the interrupt.
	 */
	GICD_SGI_FILTER_TO_SELF,

	GICD_SGI_FILTER_MAX
};

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


/*
 * CPU interface registers. Unikraft only supports running on non-secure
 * so we just describe non-secure registers.
 */

/* Default page-aligned up size for GICv2 CPU interface according to
 * ARM Generic Interrupt Controller Architecture version 2.0 Issue B.b.
 */
#define GICC_MEM_SZ	0x2000

/* CPU Interface Control Register */
#define GICC_CTLR		0x0000
#define GICC_CTLR_ENABLE	0x1

/* Interrupt Priority Mask Register */
#define GICC_PMR		0x0004
#define GICC_PMR_PRIO_MAX	255

/* Binary Point Register */
#define GICC_BPR		0x0008

/* Interrupt Acknowledge Register */
#define GICC_IAR		0x000C
#define GICC_IAR_INTID_MASK	0x3FF
#define GICC_IAR_INTID_SPURIOUS	1023

/* End of Interrupt Register */
#define GICC_EOIR		0x0010

/* Running Priority Register */
#define GICC_RPR		0x0014

/* Highest Priority Pending Interrupt Register */
#define GICC_HPPIR		0x0018

/* Aliased Binary Point Register */
#define GICC_ABPR		0x001C

/* CPU Interface Identification Register */
#define GICC_IIDR		0x00FC

/* Deactivate Interrupt Register */
#define GICC_DIR		0x1000

/**
 * Forward the SGI to the CPU interfaces specified in the target.
 *
 * @param sgintid the SGI ID [0-15]
 * @param target an 8-bit bitmap with 1 bit per CPU 0-7. A `1` bit
 *    indicates that the SGI should be forwarded to the respective CPU
 */
void gicv2_sgi_gen_to_list(uint32_t sgintid, uint8_t target);

/**
 * Forward the SGI to all CPU interfaces except that of the processor
 * that requested the interrupt.
 *
 * @param sgintid the SGI ID [0-15]
 */
void gicv2_sgi_gen_to_others(uint32_t sgintid);

/**
 * Forward the SGI only to the CPU interface of the processor
 * that requested the interrupt.
 *
 * @param sgintid the SGI ID [0-15]
 */
void gicv2_sgi_gen_to_self(uint32_t sgintid);

/**
 * Probe device tree or ACPI for GICv2
 * NOTE: First time must not be called from multiple CPUs in parallel
 *
 * @param [out] dev receives pointer to GICv2 if available, NULL otherwise
 * @return 0 if device is available, < 0 otherwise
 */
int gicv2_probe(struct _gic_dev **dev);

#endif /* __UK_INTCTLR_GICV2_H__ */
