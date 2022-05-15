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
#include <string.h>
#include <libfdt.h>
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
#include <gic/gic-v2.h>
#include <ofw/fdt.h>

/* Max CPU interface for GICv2 */
#define GIC_MAX_CPUIF		8

#define GIC_CPU_REG(gdev, r)	((void *)(gdev.cpuif_mem_addr + (r)))

#ifdef CONFIG_HAVE_SMP
__spinlock gicv2_dist_lock;
#endif /* CONFIG_HAVE_SMP */

/** GICv2 driver */
struct _gic_dev gicv2_drv = {
	.version        = GIC_V2,
	.is_present     = 0,
	.is_probed      = 0,
	.is_initialized = 0,
	.dist_mem_addr  = 0,
	.dist_mem_size  = 0,
	.cpuif_mem_addr = 0,
	.cpuif_mem_size = 0,
#ifdef CONFIG_HAVE_SMP
	.dist_lock      = &gicv2_dist_lock,
#endif /* CONFIG_HAVE_SMP */
};

static const char * const gic_device_list[] = {
	"arm,cortex-a15-gic",
	NULL
};

/* Inline functions to access GICC & GICD registers */
static inline void write_gicd8(uint64_t offset, uint8_t val)
{
	ioreg_write8(GIC_DIST_REG(gicv2_drv, offset), val);
}

static inline void write_gicd32(uint64_t offset, uint32_t val)
{
	ioreg_write32(GIC_DIST_REG(gicv2_drv, offset), val);
}

static inline uint32_t read_gicd32(uint64_t offset)
{
	return ioreg_read32(GIC_DIST_REG(gicv2_drv, offset));
}

static inline void write_gicc32(uint64_t offset, uint32_t val)
{
	ioreg_write32(GIC_CPU_REG(gicv2_drv, offset), val);
}

static inline uint32_t read_gicc32(uint64_t offset)
{
	return ioreg_read32(GIC_CPU_REG(gicv2_drv, offset));
}

/* Functions of GIC CPU interface */

/* Enable GIC cpu interface */
static void gic_enable_cpuif(void)
{
	/* just set bit 0 to 1 to enable cpu interface */
	write_gicc32(GICC_CTLR, GICC_CTLR_ENABLE);
}

/* Set priority threshold for processor */
static void gic_set_threshold_priority(uint32_t threshold_prio)
{
	/* GICC_PMR allocate 1 byte for each irq */
	UK_ASSERT(threshold_prio <= GICC_PMR_PRIO_MAX);
	write_gicc32(GICC_PMR, threshold_prio);
}

/*
 * Acknowledging irq equals reading GICC_IAR also
 * get the interrupt ID as the side effect.
 */
static uint32_t gic_ack_irq(void)
{
	return read_gicc32(GICC_IAR);
}

/*
 * write to GICC_EOIR to inform cpu interface completion
 * of interrupt processing. If GICC_CTLR.EOImode sets to 1
 * this func just gets priority drop.
 */
static void gic_eoi_irq(uint32_t irq)
{
	write_gicc32(GICC_EOIR, irq);
}

/* Functions of GIC Distributor */

/*
 * @sgintid denotes the sgi ID;
 * @targetfilter : this term is TargetListFilter
 * @targetlist is bitmask value, A bit set to '1' indicated
 * the interrupt is wired to that CPU.
 */
static void gic_sgi_gen(uint32_t sgintid, enum sgi_filter targetfilter,
			uint8_t targetlist)
{
	uint32_t val;

	/* Only INTID 0-15 allocated to sgi */
	UK_ASSERT(sgintid <= GICD_SGI_MAX_INITID);

	/* Set SGI tagetfileter field */
	val = (targetfilter & GICD_SGI_FILTER_MASK) << GICD_SGI_FILTER_SHIFT;

	/* Set SGI targetlist field */
	val |= (targetlist & GICD_SGI_TARGET_MASK) << GICD_SGI_TARGET_SHIFT;

	/* Set SGI INITID field */
	val |= sgintid;

	/* Generate SGI - spin lock here is needed when smp is supported */
	dist_lock(gicv2_drv);
	write_gicd32(GICD_SGIR, val);
	dist_unlock(gicv2_drv);
}

/*
 * Forward the SGI to the CPU interfaces specified in the
 * targetlist. Targetlist is a 8-bit bitmap for 0~7 CPU.
 */
void gic_sgi_gen_to_list(uint32_t sgintid, uint8_t targetlist)
{
	unsigned long irqf;

	irqf = ukplat_lcpu_save_irqf();
	gic_sgi_gen(sgintid, GICD_SGI_FILTER_TO_LIST, targetlist);
	ukplat_lcpu_restore_irqf(irqf);
}

/*
 * Forward the SGI to the CPU specified by cpuid.
 */
static void gic_sgi_gen_to_cpu(uint8_t sgintid, uint32_t cpuid)
{
	gic_sgi_gen_to_list((uint32_t) sgintid, (uint8_t) (1 << (cpuid % 8)));
}

/*
 * Forward the SGI to all CPU interfaces except that of the
 * processor that requested the interrupt.
 */
void gic_sgi_gen_to_others(uint32_t sgintid)
{
	unsigned long irqf;

	irqf = ukplat_lcpu_save_irqf();
	gic_sgi_gen(sgintid, GICD_SGI_FILTER_TO_OTHERS, 0);
	ukplat_lcpu_restore_irqf(irqf);
}

/*
 * Forward the SGI only to the CPU interface of the processor
 * that requested the interrupt.
 */
void gic_sgi_gen_to_self(uint32_t sgintid)
{
	gic_sgi_gen(sgintid, GICD_SGI_FILTER_TO_SELF, 0);
}

/*
 * set target cpu for irq in distributor,
 * @target: bitmask value, bit 1 indicates target to
 * corresponding cpu interface
 */
static void gic_set_irq_target(uint32_t irq, uint32_t target)
{
	if (irq < GIC_SPI_BASE)
		UK_CRASH("Bad irq number: should not less than %u",
			GIC_SPI_BASE);

	dist_lock(gicv2_drv);
	write_gicd8(GICD_ITARGETSR(irq), (uint8_t)target);
	dist_unlock(gicv2_drv);
}

/* set priority for irq in distributor */
static void gic_set_irq_prio(uint32_t irq, uint8_t priority)
{
	dist_lock(gicv2_drv);
	write_gicd8(GICD_IPRIORITYR(irq), priority);
	dist_unlock(gicv2_drv);
}

/*
 * Enable an irq in distributor, each irq occupies one bit
 * to configure in corresponding register
 */
static void gic_enable_irq(uint32_t irq)
{
	dist_lock(gicv2_drv);

	write_gicd32(GICD_ISENABLER(irq),
		UK_BIT(irq % GICD_I_PER_ISENABLERn));

	dist_unlock(gicv2_drv);
}

/*
 * Disable an irq in distributor, one bit reserved for an irq
 * to configure in corresponding register
 */
static void gic_disable_irq(uint32_t irq)
{
	dist_lock(gicv2_drv);
	write_gicd32(GICD_ICENABLER(irq),
		UK_BIT(irq % GICD_I_PER_ICENABLERn));
	dist_unlock(gicv2_drv);
}

/* Enable distributor */
static void gic_enable_dist(void)
{
	/* just set bit 0 to 1 to enable distributor */
	dist_lock(gicv2_drv);
	write_gicd32(GICD_CTLR, read_gicd32(GICD_CTLR) | GICD_CTLR_ENABLE);
	dist_unlock(gicv2_drv);
}

/* disable distributor */
static void gic_disable_dist(void)
{
	/* just clear bit 0 to 0 to disable distributor */
	dist_lock(gicv2_drv);
	write_gicd32(GICD_CTLR, read_gicd32(GICD_CTLR) & (~GICD_CTLR_ENABLE));
	dist_unlock(gicv2_drv);
}


/**
 * Config trigger type for an interrupt
 *
 * @param irq interrupt number [GIC_PPI_BASE..GIC_MAX_IRQ-1]
 * @param trigger trigger type (UK_IRQ_TRIGGER_*)
 */
static void gic_set_irq_type(uint32_t irq, enum uk_irq_trigger trigger)
{
	uint32_t val, mask, oldmask;

	UK_ASSERT(irq >= GIC_PPI_BASE && irq < GIC_MAX_IRQ);
	UK_ASSERT(trigger == UK_IRQ_TRIGGER_EDGE ||
		  trigger == UK_IRQ_TRIGGER_LEVEL);

	dist_lock(gicv2_drv);

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
	dist_unlock(gicv2_drv);
}

static void gic_handle_irq(void)
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

static void gic_init_dist(void)
{
	uint32_t val, cpuif_number, irq_number;
	uint32_t i;

	/* Turn off distributor */
	gic_disable_dist();

	/* Get GIC CPU interface */
	val = read_gicd32(GICD_TYPER);
	cpuif_number = GICD_TYPER_CPUI_NUM(val);
	if (cpuif_number > GIC_MAX_CPUIF)
		cpuif_number = GIC_MAX_CPUIF;
	uk_pr_info("GICv2 Max CPU interface:%d\n", cpuif_number);

	/* Get the maximum number of interrupts that the GIC supports */
	irq_number = GICD_TYPER_LINE_NUM(val);
	if (irq_number > GIC_MAX_IRQ)
		irq_number = GIC_MAX_IRQ;
	uk_pr_info("GICv2 Max interrupt lines:%d\n", irq_number);

	/* Set all SPI's interrupt target to all CPUs */
	for (i = GIC_SPI_BASE; i < irq_number; i += GICD_I_PER_ITARGETSRn)
		write_gicd32(GICD_ITARGETSR(i), GICD_ITARGETSR_DEF);

	/* Set all SPI's interrupt type to be level-sensitive */
	for (i = GIC_SPI_BASE; i < irq_number; i += GICD_I_PER_ICFGRn)
		write_gicd32(GICD_ICFGR(i), GICD_ICFGR_DEF_TYPE);

	/* Set all SPI's priority to a default value */
	for (i = GIC_SPI_BASE; i < irq_number; i += GICD_I_PER_IPRIORITYn)
		write_gicd32(GICD_IPRIORITYR(i), GICD_IPRIORITY_DEF);

	/* Deactivate and disable all SPIs */
	for (i = GIC_SPI_BASE; i < irq_number; i += GICD_I_PER_ICACTIVERn) {
		write_gicd32(GICD_ICACTIVER(i), GICD_DEF_ICACTIVERn);
		write_gicd32(GICD_ICENABLER(i), GICD_DEF_ICENABLERn);
	}

	/* Turn on distributor */
	gic_enable_dist();

	uk_pr_info("GICv2 distributor initialized.\n");
}

static void gic_init_cpuif(void)
{
	uint32_t i;

	/* Set priority threshold to the lowest to make all IRQs visible to
	 * the CPU interface. Note: Higher priority corresponds to a lower
	 * priority field value.
	 */
	gic_set_threshold_priority(GICC_PMR_PRIO_MAX);

	/* Set PPI and SGI to the default value */
	for (i = 0; i < GIC_SPI_BASE; i += GICD_I_PER_IPRIORITYn)
		write_gicd32(GICD_IPRIORITYR(i), GICD_IPRIORITY_DEF);

	/* Deactivate SGIs and PPIs as the state is unknown at boot */
	write_gicd32(GICD_ICACTIVER(0), GICD_DEF_ICACTIVERn);
	write_gicd32(GICD_ICENABLER(0), GICD_DEF_PPI_ICENABLERn);

	/* Enable all SGIs */
	write_gicd32(GICD_ISENABLER(0), GICD_DEF_SGI_ISENABLERn);

	/* Enable CPU interface */
	gic_enable_cpuif();

	isb();

	uk_pr_info("GICv2 CPU interface initialized.\n");
}


/**
 * Initialize GICv2
 * NOTE: First time must not be called from multiple CPUs in parallel
 *
 * @return 0 on success, a non-zero error otherwise
 */
static int gic_initialize(void)
{
#ifdef CONFIG_HAVE_SMP
	if (gicv2_drv.is_initialized) {
		/* GIC is already initialized, we just need to initialize
		 * the CPU interface
		 */
		gic_init_cpuif();
		return 0;
	}
#endif /* CONFIG_HAVE_SMP */

	gicv2_drv.is_initialized = 1;

	/* Initialize GICv2 distributor */
	gic_init_dist();

	/* Initialize GICv2 CPU interface */
	gic_init_cpuif();

	return 0;
}

static int gicv2_do_probe(const void *fdt)
{
	int fdt_gic, r;
	struct _gic_operations drv_ops = {
		.initialize        = gic_initialize,
		.ack_irq           = gic_ack_irq,
		.eoi_irq           = gic_eoi_irq,
		.enable_irq        = gic_enable_irq,
		.disable_irq       = gic_disable_irq,
		.set_irq_type      = gic_set_irq_type,
		.set_irq_prio      = gic_set_irq_prio,
		.set_irq_affinity  = gic_set_irq_target,
		.irq_translate     = gic_irq_translate,
		.handle_irq        = gic_handle_irq,
		.gic_sgi_gen = gic_sgi_gen_to_cpu,
	};

	/* Set driver functions */
	gicv2_drv.ops = drv_ops;

	/* Currently, we only support 1 GIC per system */
	fdt_gic = fdt_node_offset_by_compatible_list(fdt, -1, gic_device_list);
	if (fdt_gic < 0)
		return -FDT_ERR_NOTFOUND; /* GICv2 not present */

	/* Get address and size of the GIC's register regions */
	r = fdt_get_address(fdt, fdt_gic, 0, &gicv2_drv.dist_mem_addr,
			    &gicv2_drv.dist_mem_size);
	if (unlikely(r < 0)) {
		uk_pr_err("Could not find GICv2 distributor region!\n");
		return r;
	}

	r = fdt_get_address(fdt, fdt_gic, 1, &gicv2_drv.cpuif_mem_addr,
			    &gicv2_drv.cpuif_mem_size);
	if (unlikely(r < 0)) {
		uk_pr_err("Could not find GICv2 cpuif region!\n");
		return r;
	}

	uk_pr_info("Found GICv2 on:\n");
	uk_pr_info("\tDistributor  : 0x%lx - 0x%lx\n",
		   gicv2_drv.dist_mem_addr,
		   gicv2_drv.dist_mem_addr + gicv2_drv.dist_mem_size - 1);
	uk_pr_info("\tCPU interface: 0x%lx - 0x%lx\n",
		   gicv2_drv.cpuif_mem_addr,
		   gicv2_drv.cpuif_mem_addr + gicv2_drv.cpuif_mem_size - 1);

	/* GICv2 is present */
	gicv2_drv.is_present = 1;

	return 0;
}

/**
 * Probe device tree for GICv2
 * NOTE: First time must not be called from multiple CPUs in parallel
 *
 * @param [in] fdt pointer to device tree
 * @param [out] dev receives pointer to GICv2 if available, NULL otherwise
 *
 * @return 0 if device is available, an FDT (FDT_ERR_*) error otherwise
 */
int gicv2_probe(const void *fdt, struct _gic_dev **dev)
{
	int rc;

#ifdef CONFIG_HAVE_SMP
	if (gicv2_drv.is_probed) {
		/* GIC is already probed, we don't need to probe again */
		if (gicv2_drv.is_present) {
			*dev = &gicv2_drv;
			return 0;
		}

		*dev = NULL;
		return -FDT_ERR_NOTFOUND;
	}
#endif /* CONFIG_HAVE_SMP */

	gicv2_drv.is_probed = 1;

	rc = gicv2_do_probe(fdt);
	if (rc) {
		*dev = NULL;
		return rc;
	}

	*dev = &gicv2_drv;
	return 0;
}
