/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2020, OpenSynergy GmbH. All rights reserved.
 *
 * ARM Generic Interrupt Controller support v3 version
 * based on plat/drivers/gic/gic-v2.c:
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
#include <uk/plat/spinlock.h>
#include <arm/cpu.h>
#include <gic/gic.h>
#include <gic/gic-v3.h>
#include <ofw/fdt.h>

#define GIC_RDIST_REG(gdev, r)					\
	((void *)(gdev.rdist_mem_addr + (r) +			\
	lcpu_get_current()->idx * GICR_STRIDE))

#define GIC_AFF_TO_ROUTER(aff, mode)				\
	((((uint64_t)(aff) << 8) & MPIDR_AFF3_MASK) | ((aff) & 0xffffff) | \
	 ((uint64_t)(mode) << 31))

#ifdef CONFIG_HAVE_SMP
__spinlock gicv3_dist_lock;
#endif /* CONFIG_HAVE_SMP */

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
#ifdef CONFIG_HAVE_SMP
	.dist_lock      = &gicv3_dist_lock,
#endif /* CONFIG_HAVE_SMP */
};

static const char * const gic_device_list[] = {
	"arm,gic-v3",
	NULL
};

/* Inline functions to access GICD & GICR registers */
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
 * Wait for a write completion in [re]distributor
 *
 * @param offset Memory address of distributor or redistributor
 */
static void wait_for_rwp(uint64_t offset)
{
	uint32_t val;

	do {
		val = ioreg_read32((void *)(offset + GICD_CTLR));
	} while ((val & GICD_CTLR_RWP));
}

#ifdef CONFIG_HAVE_SMP
/**
 * Get affinity value for the calling CPU
 *
 * @return uint32_t (AFF3|AFF2|AFF1|AFF0)
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
#endif /* CONFIG_HAVE_SMP */

/**
 * Acknowledge IRQ and retrieve highest priority pending interrupt
 *
 * @return the ID of the signaled interrupt
 */
static uint32_t gicv3_ack_irq(void)
{
	uint32_t irq;

	irq = SYSREG_READ32(ICC_IAR1_EL1);
	dsb(sy);

	return irq;
}

/**
 * Signal completion of interrupt processing
 *
 * @param irq the ID of the interrupt to complete. Must be from a corresponding
 *    call to gicv3_ack_irq()
 */
static void gicv3_eoi_irq(uint32_t irq)
{
	/* Lower the priority */
	SYSREG_WRITE32(ICC_EOIR1_EL1, irq);
	isb();

	/* Deactivate */
	SYSREG_WRITE32(ICC_DIR_EL1, irq);
	isb();
}

/**
 * Enable an interrupt
 *
 * @param irq interrupt number [0..GIC_MAX_IRQ-1]
 */
static void gicv3_enable_irq(uint32_t irq)
{
	UK_ASSERT(irq < GIC_MAX_IRQ);

	dist_lock(gicv3_drv);

#ifdef CONFIG_HAVE_SMP
	/* Route this IRQ to the running core, i.e., route to the CPU interface
	 * of the core calling this function
	 */
	if (irq >= GIC_SPI_BASE) {
		uint64_t aff = (uint64_t)get_cpu_affinity();
		uint64_t irouter_val = GIC_AFF_TO_ROUTER(aff, 0);

		write_gicd64(GICD_IROUTER(irq), irouter_val);
		uk_pr_debug("IRQ %d routed to 0x%lx (REG: 0x%lx)\n",
				irq, aff, irouter_val);
	}
#endif /* CONFIG_HAVE_SMP */

	if (irq >= GIC_SPI_BASE)
		write_gicd32(GICD_ISENABLER(irq),
				UK_BIT(irq % GICD_I_PER_ISENABLERn));
	else
		write_gicrd32(GICR_ISENABLER0,
				UK_BIT(irq % GICR_I_PER_ISENABLERn));

	dist_unlock(gicv3_drv);
}

/**
 * Send a software generated interrupt to the specified core.
 *
 * @sgintid the software generated interrupt id
 * @cpuid the id of the targeted cpu
 */
static void gicv3_sgi_gen(uint8_t sgintid, uint32_t cpuid)
{
	uint64_t sgi_register = 0, control_register_rss, type_register_rss;
	uint64_t range_selector = 0, extended_cpuid;
	uint32_t aff0;

	extended_cpuid = (uint64_t) cpuid;

	/* Only INTID 0-15 allocated to sgi */
	UK_ASSERT(sgintid <= GICD_SGI_MAX_INITID);
	sgi_register |= (sgintid << 24);

	/* Set affinity fields and optional range selector */
	sgi_register |= (extended_cpuid & MPIDR_AFF3_MASK) << 48;
	sgi_register |= (extended_cpuid & MPIDR_AFF2_MASK) << 32;
	sgi_register |= (extended_cpuid & MPIDR_AFF1_MASK) << 16;
	/**
	 ** For affinity 0, we need to find which group of 16 values is
	 ** represented by the TargetList field in ICC_SGI1R_EL1.
	 **/
	aff0 = extended_cpuid & MPIDR_AFF0_MASK;
	if (aff0 >= 16) {
		control_register_rss = SYSREG_READ64(ICC_CTLR_EL1) & (1 << 18);
		type_register_rss =  read_gicd32(GICD_TYPER)  & (1 << 26);
		if (control_register_rss == 1 && type_register_rss == 1) {
			range_selector = aff0 / 16;
			sgi_register |= (range_selector << 44);
		} else {
			uk_pr_err("Can't generate interrupt!\n");
			return;
		}
	}

	sgi_register |= (1 << (aff0 % 16));

	/* Generate interrupt */
	dist_lock(gicv3_drv);
	SYSREG_WRITE64(ICC_SGI1R_EL1, sgi_register);
	dist_unlock(gicv3_drv);
}

/**
 * Disable an interrupt
 *
 * @param irq interrupt number [0..GIC_MAX_IRQ-1]
 */
static void gicv3_disable_irq(uint32_t irq)
{
	UK_ASSERT(irq < GIC_MAX_IRQ);

	dist_lock(gicv3_drv);

	if (irq >= GIC_SPI_BASE)
		write_gicd32(GICD_ICENABLER(irq),
				UK_BIT(irq % GICD_I_PER_ICENABLERn));
	else
		write_gicrd32(GICR_ICENABLER0,
				UK_BIT(irq % GICR_I_PER_ICENABLERn));

	dist_unlock(gicv3_drv);
}

/**
 * Set interrupt affinity
 *
 * @param irq interrupt number [GIC_SPI_BASE..GIC_MAX_IRQ-1]
 * @param affinity target CPU affinity in 32 bits format
 * (AFF3|AFF2|AFF1|AFF0), as returned by get_cpu_affinity()
 */
static void gicv3_set_irq_affinity(uint32_t irq, uint32_t affinity)
{
	UK_ASSERT(irq >= GIC_SPI_BASE && irq < GIC_MAX_IRQ);

	dist_lock(gicv3_drv);
	write_gicd64(GICD_IROUTER(irq), GIC_AFF_TO_ROUTER(affinity, 0));
	dist_unlock(gicv3_drv);
}

/**
 * Set priority for an interrupt
 *
 * @param irq interrupt number [0..GIC_MAX_IRQ-1]
 * @param priority priority [0..255]. The GIC implementation may not support
 *    all levels. For example, if only 128 levels are supported every two levels
 *    (e.g., 0 and 1) map to the same effective value. Lower values correspond
 *    to higher priority
 */
static void gicv3_set_irq_prio(uint32_t irq, uint8_t priority)
{
	dist_lock(gicv3_drv);

	if (irq < GIC_SPI_BASE) /* Change in redistributor */
		write_gicrd8(GICR_IPRIORITYR(irq), priority);
	else
		write_gicd8(GICD_IPRIORITYR(irq), priority);

	dist_unlock(gicv3_drv);
}

/**
 * Configure trigger type for an interrupt
 *
 * @param irq interrupt number [GIC_PPI_BASE..GIC_MAX_IRQ-1]
 * @param trigger trigger type (UK_IRQ_TRIGGER_*)
 */
static void gicv3_set_irq_type(uint32_t irq, enum uk_irq_trigger trigger)
{
	uint32_t val, mask, oldmask;

	UK_ASSERT(irq >= GIC_PPI_BASE && irq < GIC_MAX_IRQ);
	UK_ASSERT(trigger == UK_IRQ_TRIGGER_EDGE ||
			trigger == UK_IRQ_TRIGGER_LEVEL);

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
		goto EXIT_UNLOCK;

	/* Update new interrupt type */
	val &= (~(GICD_ICFGR_MASK << (irq % GICD_I_PER_ICFGRn) * 2));
	val |= (mask << (irq % GICD_I_PER_ICFGRn) * 2);
	write_gicd32(GICD_ICFGR(irq), val);

EXIT_UNLOCK:
	dist_unlock(gicv3_drv);
}

/** Enable distributor */
static void gicv3_enable_dist(void)
{
	dist_lock(gicv3_drv);
	write_gicd32(GICD_CTLR, GICD_CTLR_ARE_NS |
			GICD_CTLR_ENABLE_G0 | GICD_CTLR_ENABLE_G1NS);
	wait_for_rwp(gicv3_drv.dist_mem_addr);
	dist_unlock(gicv3_drv);
}

/** Disable distributor */
static void gicv3_disable_dist(void)
{
	/* Write 0 to disable distributor */
	dist_lock(gicv3_drv);
	write_gicd32(GICD_CTLR, 0);
	wait_for_rwp(gicv3_drv.dist_mem_addr);
	dist_unlock(gicv3_drv);
}

/** Enable the redistributor */
static void gicv3_init_redist(void)
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

	/* Deactivate SGIs and PPIs as the state is unknown at boot */
	write_gicrd32(GICR_ICACTIVER0, GICD_DEF_ICACTIVERn);

	/* Disable all PPIs */
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

	uk_pr_info("GICv3 redistributor initialized.\n");
}

/** Initialize the distributor */
static void gicv3_init_dist(void)
{
	uint32_t val, irq_number;
	uint32_t i;

	/* Disable the distributor */
	gicv3_disable_dist();

	/* Get GIC redistributor interface */
	val = read_gicd32(GICD_TYPER);
	irq_number = GICD_TYPER_LINE_NUM(val);
	if (irq_number > GIC_MAX_IRQ)
		irq_number = GIC_MAX_IRQ;
	uk_pr_info("GICv3 Max interrupt lines: %d\n", irq_number);

	/* Check for LPI support */
	if (val & GICD_TYPE_LPIS)
		uk_pr_warn("LPI support is not implemented by this driver!\n");

	/* Configure all SPIs as non-secure Group 1 */
	for (i = GIC_SPI_BASE; i < irq_number; i += GICD_I_PER_IGROUPRn)
		write_gicd32(GICD_IGROUPR(i), GICD_DEF_IGROUPRn);

#ifdef CONFIG_HAVE_SMP
	/* Route all global SPIs to this CPU */
	uint64_t aff = (uint64_t)get_cpu_affinity();
	uint64_t irouter_val = GIC_AFF_TO_ROUTER(aff, 0);

	for (i = GIC_SPI_BASE; i < irq_number; i++)
		write_gicd64(GICD_IROUTER(i), irouter_val);
#endif /* CONFIG_HAVE_SMP */

	/* Set all SPI's interrupt type to be level-sensitive */
	for (i = GIC_SPI_BASE; i < irq_number; i += GICD_I_PER_ICFGRn)
		write_gicd32(GICD_ICFGR(i), GICD_ICFGR_DEF_TYPE);

	/* Set all SPI's priority to a default value */
	for (i = GIC_SPI_BASE; i < irq_number; i += GICD_I_PER_IPRIORITYn)
		write_gicd32(GICD_IPRIORITYR4(i), GICD_IPRIORITY_DEF);

	/* Deactivate and disable all SPIs */
	for (i = GIC_SPI_BASE; i < irq_number; i += GICD_I_PER_ICACTIVERn) {
		write_gicd32(GICD_ICACTIVER(i), GICD_DEF_ICACTIVERn);
		write_gicd32(GICD_ICENABLER(i), GICD_DEF_ICENABLERn);
	}

	/* Wait for completion */
	wait_for_rwp(gicv3_drv.dist_mem_addr);

	/* Enable the distributor */
	gicv3_enable_dist();

	uk_pr_info("GICv3 distributor initialized.\n");
}

static void gicv3_handle_irq(struct __regs *regs)
{
	uint32_t stat, irq;

	do {
		stat = gicv3_ack_irq();
		irq = stat & GICC_IAR_INTID_MASK;

#ifndef CONFIG_HAVE_SMP
		uk_pr_debug("EL1 IRQ#%"__PRIu32" caught\n", irq);
#else /* !CONFIG_HAVE_SMP */
		uk_pr_debug("Core %"__PRIu64": EL1 IRQ#%"__PRIu32" caught\n",
			    ukplat_lcpu_id(), irq);
#endif /* CONFIG_HAVE_SMP */

		/* Ensure interrupt processing starts only after ACK */
		isb();

		if (irq < GIC_MAX_IRQ) {
			_ukplat_irq_handle(regs, (unsigned long)irq);
			gicv3_eoi_irq(stat);

			continue;
		}

		/* EoI should only be signaled for non-spurious interrupts */
		if (irq != GICC_IAR_INTID_SPURIOUS)
			gicv3_eoi_irq(stat);

		break;
	} while (1);
}

/**
 * Initialize GICv3
 * NOTE: First time must not be called from multiple CPUs in parallel
 *
 * @return 0 on success, a non-zero error otherwise
 */
static int gicv3_initialize(void)
{
#ifdef CONFIG_HAVE_SMP
	if (gicv3_drv.is_initialized) {
		/* Distributor is already initialized, we just need to
		 * initialize the CPU redistributor interface
		 */
		gicv3_init_redist();
		return 0;
	}
#endif /* CONFIG_HAVE_SMP */

	gicv3_drv.is_initialized = 1;

	/* Initialize GICv3 distributor */
	gicv3_init_dist();

	/* Initialize GICv3 CPU redistributor */
	gicv3_init_redist();

	return 0;
}

static int gicv3_do_probe(const void *fdt)
{
	int fdt_gic, r;
	struct _gic_operations drv_ops = {
		.initialize        = gicv3_initialize,
		.ack_irq           = gicv3_ack_irq,
		.eoi_irq           = gicv3_eoi_irq,
		.enable_irq        = gicv3_enable_irq,
		.disable_irq       = gicv3_disable_irq,
		.set_irq_type      = gicv3_set_irq_type,
		.set_irq_prio      = gicv3_set_irq_prio,
		.set_irq_affinity  = gicv3_set_irq_affinity,
		.irq_translate     = gic_irq_translate,
		.handle_irq        = gicv3_handle_irq,
		.gic_sgi_gen	   = gicv3_sgi_gen,
	};

	/* Set driver functions */
	gicv3_drv.ops = drv_ops;

	/* Currently, we only support 1 GIC per system */
	fdt_gic = fdt_node_offset_by_compatible_list(fdt, -1, gic_device_list);
	if (fdt_gic < 0)
		return -FDT_ERR_NOTFOUND; /* GICv3 not present */

	/* Get address and size of the GIC's register regions */
	r = fdt_get_address(fdt, fdt_gic, 0, &gicv3_drv.dist_mem_addr,
				&gicv3_drv.dist_mem_size);
	if (r < 0) {
		uk_pr_err("Could not find GICv3 distributor region!\n");
		return r;
	}

	r = fdt_get_address(fdt, fdt_gic, 1, &gicv3_drv.rdist_mem_addr,
				&gicv3_drv.rdist_mem_size);
	if (r < 0) {
		uk_pr_err("Could not find GICv3 redistributor region!\n");
		return r;
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

	return 0;
}

/**
 * Probe device tree for GICv3
 * NOTE: First time must not be called from multiple CPUs in parallel
 *
 * @param [in] fdt pointer to device tree
 * @param [out] dev receives pointer to GICv3 if available, NULL otherwise
 *
 * @return 0 if device is available, an FDT (FDT_ERR_*) error otherwise
 */
int gicv3_probe(const void *fdt, struct _gic_dev **dev)
{
	int rc;

#ifdef CONFIG_HAVE_SMP
	if (gicv3_drv.is_probed) {
		/* GIC is already probed, we don't need to probe again */
		if (gicv3_drv.is_present) {
			*dev = &gicv3_drv;
			return 0;
		}

		*dev = NULL;
		return -FDT_ERR_NOTFOUND;
	}
#endif /* CONFIG_HAVE_SMP */

	gicv3_drv.is_probed = 1;

	rc = gicv3_do_probe(fdt);
	if (rc) {
		*dev = NULL;
		return rc;
	}

	*dev = &gicv3_drv;
	return 0;

}
