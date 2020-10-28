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
#ifdef CONFIG_PLAT_KVM
#include <kvm/irq.h>
#endif
#include <uk/plat/spinlock.h>
#include <arm/cpu.h>
#include <gic/gic.h>
#include <gic/gic-v3.h>
#include <ofw/fdt.h>

#define GIC_DIST_REG(r)	 ((void *)(gic_dist_addr + (r)))
#define GIC_RDIST_REG(r) ((void *)(gic_rdist_addr + (r)))
#define IRQ_TYPE_MASK	0x0000000f
#define GIC_DIST_ADDR  (gic_dist_addr)
#define GIC_RDIST_ADDR (gic_rdist_addr)

#define GIC_AFF_TO_ROUTER(aff, mode)				\
	((((uint64_t)(aff) << 8) & MPIDR_AFF3_MASK) | ((aff) & 0xffffff) | \
	 ((uint64_t)(mode) << 31))

static uint64_t gic_dist_addr, gic_rdist_addr;
static uint64_t gic_dist_size, gic_rdist_size;
#ifdef CONFIG_HAVE_SMP
static char gic_is_initialized;
__spinlock gic_dist_lock;
inline void dist_lock(void) { ukarch_spin_lock(&gic_dist_lock); };
inline void dist_unlock(void) { ukarch_spin_unlock(&gic_dist_lock); };
#else
inline void dist_lock(void) {};
inline void dist_unlock(void) {};
#endif /* CONFIG_HAVE_SMP */

#ifdef CONFIG_HAVE_SMP
__spinlock gicv3_dist_lock;
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
#ifdef CONFIG_HAVE_SMP
	.dist_lock      = &gicv3_dist_lock,
#endif
};

static const char * const gic_device_list[] = {
	"arm,gic-v3",
	NULL
};

/* Inline functions to access GICD & GICR registers */
static inline void write_gicd8(uint64_t offset, uint8_t val)
{
	ioreg_write8(GIC_DIST_REG(offset), val);
}

static inline void write_gicrd8(uint64_t offset, uint8_t val)
{
	ioreg_write8(GIC_RDIST_REG(offset), val);
}

static inline void write_gicd32(uint64_t offset, uint32_t val)
{
	ioreg_write32(GIC_DIST_REG(offset), val);
}

static inline void write_gicd64(uint64_t offset, uint64_t val)
{
	ioreg_write64(GIC_DIST_REG(offset), val);
}

static inline uint32_t read_gicd32(uint64_t offset)
{
	return ioreg_read32(GIC_DIST_REG(offset));
}

static inline void write_gicrd32(uint64_t offset, uint32_t val)
{
	ioreg_write32(GIC_RDIST_REG(offset), val);
}

static inline uint32_t read_gicrd32(uint64_t offset)
{
	return ioreg_read32(GIC_RDIST_REG(offset));
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
uint32_t gic_ack_irq(void)
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
void gic_eoi_irq(uint32_t irq)
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
void gic_enable_irq(uint32_t irq)
{
	UK_ASSERT(irq < GIC_MAX_IRQ);

	dist_lock();

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
#endif

	if (irq >= GIC_SPI_BASE)
		write_gicd32(GICD_ISENABLER(irq),
				UK_BIT(irq % GICD_I_PER_ISENABLERn));
	else
		write_gicrd32(GICR_ISENABLER0,
				UK_BIT(irq % GICR_I_PER_ISENABLERn));

	dist_unlock();
}

/**
 * Disable an interrupt
 *
 * @param irq interrupt number [0..GIC_MAX_IRQ-1]
 */
void gic_disable_irq(uint32_t irq)
{
	UK_ASSERT(irq < GIC_MAX_IRQ);

	dist_lock();

	if (irq >= GIC_SPI_BASE)
		write_gicd32(GICD_ICENABLER(irq),
				UK_BIT(irq % GICD_I_PER_ICENABLERn));
	else
		write_gicrd32(GICR_ICENABLER0,
				UK_BIT(irq % GICR_I_PER_ICENABLERn));

	dist_unlock();
}

/**
 * Set interrupt affinity
 *
 * @param irq interrupt number [GIC_SPI_BASE..GIC_MAX_IRQ-1]
 * @param affinity target CPU affinity in 32 bits format
 * (AFF3|AFF2|AFF1|AFF0), as returned by get_cpu_affinity()
 */
void gic_set_irq_affinity(uint32_t irq, uint32_t affinity)
{
	UK_ASSERT(irq >= GIC_SPI_BASE && irq < GIC_MAX_IRQ);

	dist_lock();
	write_gicd64(GICD_IROUTER(irq), GIC_AFF_TO_ROUTER(affinity, 0));
	dist_unlock();
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
void gic_set_irq_prio(uint32_t irq, uint8_t priority)
{
	dist_lock();

	if (irq < GIC_SPI_BASE) /* Change in redistributor */
		write_gicrd8(GICR_IPRIORITYR(irq), priority);
	else
		write_gicd8(GICD_IPRIORITYR(irq), priority);

	dist_unlock();
}

/**
 * Config trigger type for an interrupt
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

	dist_lock();

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
	dist_unlock();
}

/** Enable distributor */
static void gic_enable_dist(void)
{
	dist_lock();
	write_gicd32(GICD_CTLR, GICD_CTLR_ARE_NS |
			GICD_CTLR_ENABLE_G0 | GICD_CTLR_ENABLE_G1NS);
	wait_for_rwp(GIC_DIST_ADDR);
	dist_unlock();
}

/** Disable distributor */
static void gic_disable_dist(void)
{
	/* Write 0 to disable distributor */
	dist_lock();
	write_gicd32(GICD_CTLR, 0);
	wait_for_rwp(GIC_DIST_ADDR);
	dist_unlock();
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

	/* Disable all PPI interrupts */
	write_gicrd32(GICR_ICENABLER0, GICD_DEF_PPI_ICENABLERn);

	/* Configure SGIs and PPIs as non-secure Group 1 */
	write_gicrd32(GICR_IGROUPR0, GICD_DEF_IGROUPRn);

	/* Enable all SGIs */
	write_gicrd32(GICR_ISENABLER0, GICD_DEF_SGI_ISENABLERn);

	/* Wait for completion */
	wait_for_rwp(GIC_RDIST_ADDR);

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

int32_t gic_irq_translate(uint32_t type, uint32_t hw_irq)
{
	uint32_t irq;

	switch (type) {
	case GIC_SPI_TYPE:
		irq = hw_irq + GIC_SPI_BASE;
		if (irq >= GIC_SPI_BASE && irq < __MAX_IRQ)
			return irq;
		break;
	case GIC_PPI_TYPE:
		irq = hw_irq + GIC_PPI_BASE;
		if (irq >= GIC_PPI_BASE && irq < GIC_SPI_BASE)
			return irq;
		break;
	default:
		uk_pr_warn("Invalid IRQ type [%d]\n", type);
	}

	uk_pr_err("irq is out of range\n");
	return -EINVAL;
}

void gic_handle_irq(void)
{
	uint32_t stat, irq;

	do {
		stat = gic_ack_irq();
		irq = stat & GICC_IAR_INTID_MASK;

#ifndef CONFIG_HAVE_SMP
		uk_pr_debug("EL1 IRQ#%d trap caught\n", irq);
#else /* !CONFIG_HAVE_SMP */
		uk_pr_debug("Core %d: EL1 IRQ#%d trap caught\n",
				ukplat_lcpu_id(), irq);
#endif /* CONFIG_HAVE_SMP */

		/* Ensure interrupt processing starts only after ACK */
		isb();

		if (irq < GIC_MAX_IRQ) {
			_ukplat_irq_handle((unsigned long)irq);
			gic_eoi_irq(stat);
			continue;
		}

		/* EoI should only be signaled for non-spurious interrupts */
		if (irq != GICC_IAR_INTID_SPURIOUS)
			gic_eoi_irq(stat);

		break;
	} while (1);
}

/** Enable the distributor */
static void gic_init_dist(void)
{
	uint32_t val, irq_number;
	uint32_t i;

	/* Disable the distributor */
	gic_disable_dist();

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
	wait_for_rwp(GIC_DIST_ADDR);

	/* Enable the distributor */
	gic_enable_dist();
}

int _dtb_init_gic(const void *fdt)
{
	int fdt_gic, ret;

#ifdef CONFIG_HAVE_SMP
	if (gic_is_initialized) {
		/* Distributor is already initialized, we just need to
		 * initialize the CPU redistributor interface
		 */
		gic_init_redist();
		return 0;
	}
	gic_is_initialized = 1;
#endif /* CONFIG_HAVE_SMP */

	uk_pr_info("Probing GICv3...\n");

	/* Currently, we only support 1 GIC per system */
	fdt_gic = fdt_node_offset_by_compatible_list(fdt, -1,
				gic_device_list);
	if (fdt_gic < 0)
		UK_CRASH("Could not find GICv3 Interrupt Controller!\n");

	/* Get device address and size at regs region */
	ret = fdt_get_address(fdt, fdt_gic, 0,
			&gic_dist_addr, &gic_dist_size);
	if (ret < 0)
		UK_CRASH("Could not find GICv3 distributor region!\n");

	ret = fdt_get_address(fdt, fdt_gic, 1,
			&gic_rdist_addr, &gic_rdist_size);
	if (ret < 0)
		UK_CRASH("Could not find GICv3 redistributor region!\n");

	uk_pr_info("Found GICv3 on:\n");
	uk_pr_info("\tDistributor  : 0x%lx - 0x%lx\n",
		gicv3_drv.dist_mem_addr,
		gicv3_drv.dist_mem_addr + gicv3_drv.dist_mem_size - 1);
	uk_pr_info("\tRedistributor: 0x%lx - 0x%lx\n",
		gicv3_drv.rdist_mem_addr,
		gicv3_drv.rdist_mem_addr + gicv3_drv.rdist_mem_size - 1);

	/* GICv3 is present */
	gicv3_drv.is_present = 1;

	/* Initialize GICv3 distributor */
	gic_init_dist();

	/* Initialize GICv3 CPU redistributor */
	gic_enable_redist();

	return 0;
}
