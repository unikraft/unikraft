/* SPDX-License-Identifier: BSD-3-Clause */
/*
 *
 * ARM Generic Interrupt Controller support v3 version
 * based on plat/drivers/gic/gic-v2.c:
 *
 * Authors: Wei Chen <Wei.Chen@arm.com>
 *          Jianyong Wu <Jianyong.Wu@arm.com>
 *
 * Copyright (c) 2018, Arm Ltd. All rights reserved.
 * Copyright (c) 2020, OpenSynergy GmbH. All rights reserved.
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
#include <string.h>
#include <libfdt.h>
#include <uk/config.h>
#include <uk/essentials.h>
#include <uk/print.h>
#include <uk/assert.h>
#include <uk/bitops.h>
#include <uk/asm.h>
#include <uk/plat/lcpu.h>
#include <uk/plat/common/irq.h>
#if defined(CONFIG_PLAT_KVM)
#include <kvm/irq.h>
#endif
#include <uk/plat/spinlock.h>
#include <arm/cpu.h>
#include <gic/gic.h>
#include <gic/gic-v3.h>
#include <ofw/fdt.h>

#define GIC_RDIST_REG(gdev, r) ((void *)(gdev.rdist_mem_addr + (r)))

#ifdef UKPLAT_LCPU_MULTICORE
DEFINE_SPINLOCK(gicv3_dist_lock);
#endif

/** GICv3 driver */
struct _gic_dev gicv3_drv = {
	.version        = GIC_V3,
	.is_present     = 0,
	.is_probed      = 0,
	.is_initialized = 0,
	.dist_mem_addr  = 0,
	.dist_mem_size  = 0,
	.rdist_mem_addr = 0,
	.rdist_mem_size = 0,
#ifdef UKPLAT_LCPU_MULTICORE
	.dist_lock      = &gicv3_dist_lock,
#endif
};

static const char * const gic_device_list[] = {
	"arm,gic-v3",
	NULL
};

/* inline functions to access GICD & GICR registers */
static inline void write_gicd8(uint64_t offset, uint8_t val)
{
	ioreg_write8(GIC_DIST_REG(gicv3_drv, offset), val);
}

static inline void write_gicrd8(uint64_t offset, uint8_t val)
{
	ioreg_write8(GIC_RDIST_REG(gicv3_drv, offset), val);
}

static inline void write_gicd32(uint64_t offset, uint32_t val)
{
	ioreg_write32(GIC_DIST_REG(gicv3_drv, offset), val);
}

static inline void write_gicd64(uint64_t offset, uint64_t val)
{
	ioreg_write64(GIC_DIST_REG(gicv3_drv, offset), val);
}

static inline uint32_t read_gicd32(uint64_t offset)
{
	return ioreg_read32(GIC_DIST_REG(gicv3_drv, offset));
}

static inline void write_gicrd32(uint64_t offset, uint32_t val)
{
	ioreg_write32(GIC_RDIST_REG(gicv3_drv, offset), val);
}

static inline uint32_t read_gicrd32(uint64_t offset)
{
	return ioreg_read32(GIC_RDIST_REG(gicv3_drv, offset));
}

/**
 * \brief Wait for a write completion in [re]distributor
 * \param [in] offset Memory address (Distributor or Redistributor)
 */
static void wait_for_rwp(uint64_t offset)
{
	uint32_t val;

	do {
		val = ioreg_read32((void *)(offset + GICD_CTLR));
	} while ((val & GICD_CTLR_RWP));
}

#ifdef UKPLAT_LCPU_MULTICORE
/**
 * \brief Get AFFINITY values for the calling [v]CPU
 * \return uint32_t (AFF3|AFF2|AFF1|AFF0)
 */
static uint32_t get_cpu_affinity(void)
{
	uint64_t aff;
	uint64_t mpidr = SYSREG_READ64(MPIDR_EL1);

	aff = ((mpidr & MPIDR_AFF3_MASK) >> 8) |
		(mpidr & MPIDR_AFF2_MASK) |
		(mpidr & MPIDR_AFF1_MASK) |
		(mpidr & MPIDR_AFF0_MASK);

	return (uint32_t)aff;
}
#endif

/**
 * \brief Acknowledging IRQ
 */
static uint32_t gic_ack_irq(void)
{
	uint32_t irq;

	/* Read ICC_IAR1_EL1 */
	irq = SYSREG_READ32(ICC_IAR1_EL1);
	dsb(sy);

	return irq;
}

/**
 * \brief Finish interrupt handling: Drop priority and deactivate the interrupt
 * \param [in] irq IRQ number
 */
static void gic_eoi_irq(uint32_t irq)
{
	/* Lower the priority */
	SYSREG_WRITE32(ICC_EOIR1_EL1, irq);
	isb();

	/* Deactivate */
	SYSREG_WRITE32(ICC_DIR_EL1, irq);
	isb();
}

/**
 * Enable IRQ in distributor (SPIs) or redistributor (SGIs and PPIS)
 */
static void gic_enable_irq(uint32_t irq)
{
	dist_lock(gicv3_drv);

#ifdef UKPLAT_LCPU_MULTICORE
	/* Route this IRQ to the running core, i.e., route to the CPU interface
	 * of the core calling this function
	 */
	if (irq >= GIC_SPI_BASE) {
		uint64_t irouter_val = 0; /* Interrupt_Routing_Mode = 0 */
		uint64_t aff = (uint64_t)get_cpu_affinity();

		irouter_val = ((aff << 8) & MPIDR_AFF3_MASK) | (aff & 0xffffff);
		write_gicd64(GICD_IROUTER(irq), irouter_val);
		uk_pr_debug("IRQ %d routed to 0x%lx (REG: 0x%lx)\n",
				irq, aff, irouter_val);
	}
#endif

	if (irq >= GIC_SPI_BASE) {
		/* Write to distributor register */
		write_gicd32(GICD_ISENABLER(irq),
				UK_BIT(irq % GICD_I_PER_ISENABLERn));
	} else {
		/* Write to redistributor register */
		write_gicrd32(GICR_ISENABLER0,
				UK_BIT(irq % GICR_I_PER_ISENABLERn));
	}

	dist_unlock(gicv3_drv);
}

/**
 * Disable an IRQ in distributor (SPIs) or in redistributor (SGIs and PPIs)
 */
static void gic_disable_irq(uint32_t irq)
{
	dist_lock(gicv3_drv);

	if (irq >= GIC_SPI_BASE) {
		/* Write to distributor register */
		write_gicd32(GICD_ICENABLER(irq),
				UK_BIT(irq % GICD_I_PER_ICENABLERn));
	} else {
		/* Write to redistributor register */
		write_gicrd32(GICR_ICENABLER0,
				UK_BIT(irq % GICR_I_PER_ICENABLERn));
	}

	dist_unlock(gicv3_drv);
}

/**
 * \brief Set IRQ affinity routing
 * \param [in] irq IRQ number
 * \param [in] affinity
 */
static void gic_set_irq_affinity(uint32_t irq, uint8_t affinity)
{
	uint64_t irouter_val = 0; /* Interrupt_Routing_Mode = 0 */

	if (irq < GIC_SPI_BASE)
		UK_CRASH("Bad irq number: should not less than %u",
				GIC_SPI_BASE);

	irouter_val = ((affinity << 8) & MPIDR_AFF3_MASK) |
		(affinity & 0xffffff);

	dist_lock(gicv3_drv);
	write_gicd64(GICD_IROUTER(irq), irouter_val);
	dist_unlock(gicv3_drv);
}

/**
 * \brief Set priority for IRQ in [re]distributor
 * \param [in] irq IRQ number
 * \param [in] priority Priority
 */
static void gic_set_irq_prio(uint32_t irq, uint8_t priority)
{
	dist_lock(gicv3_drv);

	if (irq < GIC_SPI_BASE) {
		/* Change in redistributor */
		write_gicrd8(GICR_IPRIORITYR(irq), priority);
	} else {
		write_gicd8(GICD_IPRIORITYR(irq), priority);
	}

	dist_unlock(gicv3_drv);
}

/**
 * \brief Enable distributor
 */
static void gic_enable_dist(void)
{
	dist_lock(gicv3_drv);
	write_gicd32(GICD_CTLR, GICD_CTLR_ARE_NS |
			GICD_CTLR_ENABLE_G1A | GICD_CTLR_ENABLE_G1);
	wait_for_rwp(gicv3_drv.dist_mem_addr);
	dist_unlock(gicv3_drv);
}

/**
 * \brief Disable distributor
 */
static void gic_disable_dist(void)
{
	/* Write 0 to disable distributor */
	dist_lock(gicv3_drv);
	write_gicd32(GICD_CTLR, 0);
	wait_for_rwp(gicv3_drv.dist_mem_addr);
	dist_unlock(gicv3_drv);
}

/**
 * \brief Enable the redistributor
 */
static void gic_enable_redist(void)
{
	uint32_t i, val;

	/* Wake up CPU redistributor */
	val  = read_gicrd32(GICR_WAKER);
	val &= ~GICR_WAKER_ProcessorSleep;
	write_gicrd32(GICR_WAKER, val);

	/* Poll GICR_WAKER.ChildrenAsleep */
	do {
		val = read_gicrd32(GICR_WAKER);
	} while ((val & GICR_WAKER_ChildrenAsleep));

	/* Set PPI and SGI to a default value */
	for (i = 0; i < GIC_SPI_BASE; i += GICD_I_PER_IPRIORITYn)
		write_gicrd32(GICR_IPRIORITYR4(i), GICD_IPRIORITY_DEF);

	/* Deactivate SGIs and PPIs since the activate state is
	 * unknown at boot
	 */
	write_gicrd32(GICR_ICACTIVER0, GICD_DEF_ICACTIVERn);

	/* Disable all PPI interrupts */
	write_gicrd32(GICR_ICENABLER0, GICD_DEF_PPI_ICENABLERn);

	/* Configure SGIs and PPIs as non-secure Group 1 */
	write_gicrd32(GICR_IGROUPR0, GICD_DEF_IGROUPRn);

	/* Enable all SGIs */
	write_gicrd32(GICR_ISENABLER0, GICD_DEF_SGI_ISENABLERn);

	/* Wait for completion */
	wait_for_rwp(gicv3_drv.rdist_mem_addr);

	/* Enable system register access */
	val  = SYSREG_READ32(ICC_SRE_EL1);
	val |= 0x7;
	SYSREG_WRITE32(ICC_SRE_EL1, val);
	isb();

	/* No priority grouping */
	SYSREG_WRITE32(ICC_BPR1_EL1, 0);

	/* Set priority mask register */
	SYSREG_WRITE32(ICC_PMR_EL1, 0xff);

	/* EOI drops priority, DIR deactivates the interrupt (mode 1) */
	SYSREG_WRITE32(ICC_CTLR_EL1, GICC_CTLR_EL1_EOImode_drop);

	/* Enable Group 1 interrupts */
	SYSREG_WRITE32(ICC_IGRPEN1_EL1, 1);

	isb();

	uk_pr_info("GICv3 redistributor initialized!\n");
}

/* Config interrupt trigger type */
static void gic_set_irq_type(uint32_t irq, int trigger)
{
	uint32_t val, mask, oldmask;

	if (irq < GIC_PPI_BASE)
		UK_CRASH("Bad irq number: should not less than %u",
			GIC_PPI_BASE);
	if (trigger >= UK_IRQ_TRIGGER_MAX)
		return;

	dist_lock(gicv3_drv);

	val = read_gicd32(GICD_ICFGR(irq));
	mask = oldmask = (val >> ((irq % GICD_I_PER_ICFGRn) * 2)) &
			GICD_ICFGR_MASK;

	if (trigger == UK_IRQ_TRIGGER_LEVEL) {
		mask &= ~GICD_ICFGR_TRIG_MASK;
		mask |= GICD_ICFGR_TRIG_LVL;
	} else if (trigger == UK_IRQ_TRIGGER_EDGE) {
		mask &= ~GICD_ICFGR_TRIG_MASK;
		mask |= GICD_ICFGR_TRIG_EDGE;
	}

	/* Check if nothing changed */
	if (mask == oldmask)
		return;

	/* Update new interrupt type */
	val &= (~(GICD_ICFGR_MASK << (irq % GICD_I_PER_ICFGRn) * 2));
	val |= (mask << (irq % GICD_I_PER_ICFGRn) * 2);
	write_gicd32(GICD_ICFGR(irq), val);

	dist_unlock(gicv3_drv);
}

static void gic_handle_irq(void)
{
	uint32_t stat, irq;

	do {
		stat = gic_ack_irq();
		irq = stat & GICC_IAR_INTID_MASK;

#ifndef UKPLAT_LCPU_MULTICORE
		uk_pr_debug("Unikraft: EL1 IRQ#%d trap caught\n", irq);
#else
		uk_pr_debug("Unikraft (Core %d): EL1 IRQ#%d trap caught\n",
				ukplat_lcpu_index(), irq);
#endif

		/*
		 * TODO: Handle IPI&SGI interrupts here
		 */
		if (irq < GIC_MAX_IRQ) {
			isb();
			_ukplat_irq_handle((unsigned long)irq);
			gic_eoi_irq(stat);
			continue;
		}

		break;
	} while (1);
}

/**
 * \brief Enable the distributor
 */
static void gic_init_dist(void)
{
	uint32_t val, irq_number;
	uint32_t i;

	/* Disable the distributor */
	gic_disable_dist();

	/* Get GIC redistributor interface */
	val = read_gicd32(GICD_TYPER);
	irq_number = 32 * ((val & GICD_TYPE_LINES) + 1);
	if (irq_number > GIC_MAX_IRQ)
		irq_number = GIC_MAX_IRQ;
	uk_pr_info("GICv3 Max interrupt lines: %d\n", irq_number);

	/* Check for LPI support */
	if (val & GICD_TYPE_LPIS) {
		/* Hardware has LPI support */
		uk_pr_info("LPI support is not implemented by this driver!\n");
	}

	/* Configure all SPIs as non-secure Group 1 */
	for (i = GIC_SPI_BASE; i < irq_number; i += GICD_I_PER_IGROUPRn)
		write_gicd32(GICD_IGROUPR(i), GICD_DEF_IGROUPRn);

#ifdef UKPLAT_LCPU_MULTICORE
	/*
	 * Route all global SPIs to this CPU.
	 */
	uint64_t irouter_val = 0; /* Interrupt_Routing_Mode = 0 */
	uint32_t aff = get_cpu_affinity();

	irouter_val |= ((aff << 8) & MPIDR_AFF3_MASK) | (aff & 0xffffff);
	for (i = GIC_SPI_BASE; i < irq_number; i++)
		write_gicd64(GICD_IROUTER(i), irouter_val);
#endif

	/*
	 * Set all SPI interrupts type to be level-sensitive
	 */
	for (i = GIC_SPI_BASE; i < irq_number; i += GICD_I_PER_ICFGRn)
		write_gicd32(GICD_ICFGR(i), GICD_ICFGR_DEF_TYPE);

	/*
	 * Set all SPI priority to a default value.
	 */
	for (i = GIC_SPI_BASE; i < irq_number; i += GICD_I_PER_IPRIORITYn)
		write_gicd32(GICD_IPRIORITYR4(i), GICD_IPRIORITY_DEF);

	/*
	 * Deactivate and disable all SPIs.
	 */
	for (i = GIC_SPI_BASE; i < irq_number; i += GICD_I_PER_ICACTIVERn) {
		write_gicd32(GICD_ICACTIVER(i), GICD_DEF_ICACTIVERn);
		write_gicd32(GICD_ICENABLER(i), GICD_DEF_ICENABLERn);
	}

	/* Wait for completion */
	wait_for_rwp(gicv3_drv.dist_mem_addr);

	/* Enable the distributor */
	gic_enable_dist();
}

static int gic_initialize(void)
{
#ifdef UKPLAT_LCPU_MULTICORE
	if (gicv3_drv.is_initialized) {
		/* GIC is already initialized, we just need to initialize
		 * the CPU redistributor interface
		 */
		gic_enable_redist();
		return 0;
	}
#endif
	gicv3_drv.is_initialized = 1;

	/* Initialize GICv3 distributor */
	gic_init_dist();

	/* Initialize redistributor */
	gic_enable_redist();

	return 0;
}

struct _gic_dev *gicv3_probe(const void *fdt, int *ret)
{
#ifdef UKPLAT_LCPU_MULTICORE
	if (gicv3_drv.is_probed) {
		/* GIC is already probed, we don't need to probe again */
		if (gicv3_drv.is_present)
			return &gicv3_drv;
		else
			return NULL;
	}
#endif
	gicv3_drv.is_probed = 1;

	int fdt_gic, r;
	struct _gic_operations drv_ops = {
		.initialize        = gic_initialize,
		.ack_irq           = gic_ack_irq,
		.eoi_irq           = gic_eoi_irq,
		.enable_irq        = gic_enable_irq,
		.disable_irq       = gic_disable_irq,
		.set_irq_type      = gic_set_irq_type,
		.set_irq_prio      = gic_set_irq_prio,
		.set_irq_affinity  = gic_set_irq_affinity,
		.irq_translate     = gic_irq_translate,
		.handle_irq        = gic_handle_irq,
	};

	/* Set driver functions */
	gicv3_drv.ops = drv_ops;

	/* Currently, we only support 1 GIC per system */
	fdt_gic = fdt_node_offset_by_compatible_list(fdt, -1,
				gic_device_list);
	if (fdt_gic < 0) {
		/* GICv3 not present */
		*ret = -1;
		return NULL;
	}

	/* Get device address and size at regs region */
	r = fdt_get_address(fdt, fdt_gic, 0,
			&gicv3_drv.dist_mem_addr, &gicv3_drv.dist_mem_size);
	if (r < 0) {
		uk_pr_err("Could not find GICv3 distributor region!\n");
		*ret = r;
		return NULL;
	}

	r = fdt_get_address(fdt, fdt_gic, 1,
			&gicv3_drv.rdist_mem_addr, &gicv3_drv.rdist_mem_size);
	if (r < 0) {
		uk_pr_err("Could not find GICv3 redistributor region!\n");
		*ret = r;
		return NULL;
	}

	uk_pr_info("Found GICv3 on:\n");
	uk_pr_info("\tDistributor  : 0x%lx - 0x%lx\n",
		gicv3_drv.dist_mem_addr,
		gicv3_drv.dist_mem_addr + gicv3_drv.dist_mem_size - 1);
	uk_pr_info("\tRedistributor: 0x%lx - 0x%lx\n",
		gicv3_drv.rdist_mem_addr,
		gicv3_drv.rdist_mem_addr + gicv3_drv.rdist_mem_size - 1);

	/* GICv3 is present */
	gicv3_drv.is_present = 1;

	*ret = 0;
	return &gicv3_drv;
}
