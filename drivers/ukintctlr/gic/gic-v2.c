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
#include <uk/arch/limits.h>
#include <uk/plat/lcpu.h>
#ifdef CONFIG_UKPLAT_ACPI
#include <uk/plat/common/acpi.h>
#endif /* CONFIG_UKPLAT_ACPI */
#include <uk/plat/common/bootinfo.h>
#include <uk/plat/spinlock.h>
#include <arm/cpu.h>
#include <uk/intctlr.h>
#include <uk/intctlr/gic-v2.h>
#include <uk/intctlr/limits.h>
#include <uk/ofw/fdt.h>

#if CONFIG_PAGING
#include <uk/bus/platform.h>
#include <uk/errptr.h>
#endif /* CONFIG_PAGING */

/* Max CPU interface for GICv2 */
#define GIC_MAX_CPUIF		8

#define GIC_MAX_IRQ		UK_INTCTLR_MAX_IRQ

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

static const char * const gic_device_list[] __maybe_unused = {
	"arm,cortex-a15-gic",
	"arm,gic-400",
	NULL
};

/* Inline functions to access GICC & GICD registers */
static inline void write_gicd8(__u64 offset, __u8 val)
{
	ioreg_write8(GIC_DIST_REG(gicv2_drv, offset), val);
}

static inline void write_gicd32(__u64 offset, __u32 val)
{
	ioreg_write32(GIC_DIST_REG(gicv2_drv, offset), val);
}

static inline __u32 read_gicd32(__u64 offset)
{
	return ioreg_read32(GIC_DIST_REG(gicv2_drv, offset));
}

static inline void write_gicc32(__u64 offset, __u32 val)
{
	ioreg_write32(GIC_CPU_REG(gicv2_drv, offset), val);
}

static inline __u32 read_gicc32(__u64 offset)
{
	return ioreg_read32(GIC_CPU_REG(gicv2_drv, offset));
}

/* Functions of GIC CPU interface */

/** Enable GIC CPU interface */
static void gicv2_enable_cpuif(void)
{
	/* just set bit 0 to 1 to enable CPU interface */
	write_gicc32(GICC_CTLR, GICC_CTLR_ENABLE);
}

/**
 * Set priority threshold for processor. Only interrupts with higher priority
 * than this threshold are signaled to the processor
 *
 * @param priority priority threshold [0..255]. The GIC implementation
 *    may not support all levels. For example, if only 128 levels are supported
 *    every two levels (e.g., 0 and 1) map to the same effective value
 */
static void gicv2_set_threshold_priority(__u32 priority)
{
	/* GICC_PMR allocate 1 byte for each IRQ */
	UK_ASSERT(priority <= GICC_PMR_PRIO_MAX);
	write_gicc32(GICC_PMR, priority);
}

/**
 * Acknowledge IRQ and retrieve highest priority pending interrupt
 *
 * @return the ID of the signaled interrupt in bits [0..9] and for SGIs in
 *    a multiprocessor system the originating CPU's ID in bits [10..12]
 */
static __u32 gicv2_ack_irq(void)
{
	return read_gicc32(GICC_IAR);
}

/**
 * Signal completion of interrupt processing.
 *
 * NOTE: If GICC_CTLR.EOImode is set to 1 this performs a priority drop for
 *    the specified interrupt.
 *
 * @param eoir acknowledge register value with bits [0..9] indicating the
 *    ID of the interrupt to complete and for SGIs in a multiprocessor system
 *    the ID of the CPU that requested the interrupt in bits [10..12]. Must
 *    correspond to the last value read with ack_irq()
 */
static void gicv2_eoi_irq(__u32 eoir)
{
	write_gicc32(GICC_EOIR, eoir);
}

/* Functions of GIC Distributor */

/**
 * Generate a Software Generated Interrupt (SGI)
 *
 * @param sgintid the SGI ID [0-15]
 * @param targetfilter filter for the target list (GICD_SGI_FILTER_*)
 * @param targetlist an 8-bit bitmap with 1 bit per CPU 0-7. A `1` bit
 *    indicates that the SGI should be forwarded to the respective CPU
 */
static void gicv2_sgi_gen(__u32 sgintid, enum sgi_filter targetfilter,
			  __u8 targetlist)
{
	__u32 val;

	UK_ASSERT(sgintid <= GICD_SGI_MAX_INITID);
	UK_ASSERT(targetfilter < GICD_SGI_FILTER_MAX);

	/* Set SGI targetfilter field */
	val = (targetfilter & GICD_SGI_FILTER_MASK) << GICD_SGI_FILTER_SHIFT;

	/* Set SGI targetlist field */
	val |= (targetlist & GICD_SGI_TARGET_MASK) << GICD_SGI_TARGET_SHIFT;

	/* Set SGI INITID field */
	val |= sgintid;

	/* Generate SGI */
	dist_lock(gicv2_drv);
	write_gicd32(GICD_SGIR, val);
	dist_unlock(gicv2_drv);
}

/**
 * Forward the SGI to the CPU interfaces specified in the target list
 *
 * @param sgintid the SGI ID [0-15]
 * @param targetlist an 8-bit bitmap with 1 bit per CPU 0-7. A `1` bit
 *    indicates that the SGI should be forwarded to the respective CPU
 */
void gicv2_sgi_gen_to_list(__u32 sgintid, __u8 targetlist)
{
	unsigned long irqf;

	irqf = ukplat_lcpu_save_irqf();
	gicv2_sgi_gen(sgintid, GICD_SGI_FILTER_TO_LIST, targetlist);
	ukplat_lcpu_restore_irqf(irqf);
}

/**
 * Forward the SGI to the CPU specified by cpuid.
 */
static void gicv2_sgi_gen_to_cpu(__u8 sgintid, __u32 cpuid)
{
	gicv2_sgi_gen_to_list((__u32) sgintid, (__u8) (1 << (cpuid % 8)));
}

/**
 * Forward the SGI to all CPU interfaces except the one of the processor that
 * requested the interrupt.
 */
void gicv2_sgi_gen_to_others(__u32 sgintid)
{
	unsigned long irqf;

	irqf = ukplat_lcpu_save_irqf();
	gicv2_sgi_gen(sgintid, GICD_SGI_FILTER_TO_OTHERS, 0);
	ukplat_lcpu_restore_irqf(irqf);
}

/**
 * Forward the SGI only to the CPU interface of the processor that requested
 * the interrupt.
 */
void gicv2_sgi_gen_to_self(__u32 sgintid)
{
	gicv2_sgi_gen(sgintid, GICD_SGI_FILTER_TO_SELF, 0);
}

/**
 * Set target CPU for an interrupt
 *
 * @param irq interrupt number [GIC_SPI_BASE..GIC_MAX_IRQ]
 * @param targetlist an 8-bit bitmap with 1 bit per CPU 0-7. A `1` bit
 *    indicates that the SGI should be forwarded to the respective CPU
 */
static void gicv2_set_irq_target(__u32 irq, __u32 targetlist)
{
	UK_ASSERT(irq >= GIC_SPI_BASE && irq <= GIC_MAX_IRQ);
	UK_ASSERT(targetlist <= __U8_MAX);

	dist_lock(gicv2_drv);
	write_gicd8(GICD_ITARGETSR(irq), (__u8)targetlist);
	dist_unlock(gicv2_drv);
}

/**
 * Set priority for an interrupt
 *
 * @param irq interrupt number [0..GIC_MAX_IRQ]
 * @param priority priority [0..255]. The GIC implementation may not support
 *    all levels. For example, if only 128 levels are supported every two levels
 *    (e.g., 0 and 1) map to the same effective value. Lower values correspond
 *    to higher priority
 */
static void gicv2_set_irq_prio(__u32 irq, __u8 priority)
{
	UK_ASSERT(irq <= GIC_MAX_IRQ);

	dist_lock(gicv2_drv);
	write_gicd8(GICD_IPRIORITYR(irq), priority);
	dist_unlock(gicv2_drv);
}

/**
 * Enable an interrupt
 *
 * @param irq interrupt number [0..GIC_MAX_IRQ]
 */
static void gicv2_enable_irq(__u32 irq)
{
	UK_ASSERT(irq <= GIC_MAX_IRQ);

	dist_lock(gicv2_drv);
	write_gicd32(GICD_ISENABLER(irq), UK_BIT(irq % GICD_I_PER_ISENABLERn));
	dist_unlock(gicv2_drv);
}

/**
 * Disable an interrupt
 *
 * @param irq interrupt number [0..GIC_MAX_IRQ]
 */
static void gicv2_disable_irq(__u32 irq)
{
	UK_ASSERT(irq <= GIC_MAX_IRQ);

	dist_lock(gicv2_drv);
	write_gicd32(GICD_ICENABLER(irq), UK_BIT(irq % GICD_I_PER_ICENABLERn));
	dist_unlock(gicv2_drv);
}

/** Enable distributor */
static void gicv2_enable_dist(void)
{
	/* just set bit 0 to 1 to enable distributor */
	dist_lock(gicv2_drv);
	write_gicd32(GICD_CTLR, read_gicd32(GICD_CTLR) | GICD_CTLR_ENABLE);
	dist_unlock(gicv2_drv);
}

/** Disable distributor */
static void gicv2_disable_dist(void)
{
	/* just clear bit 0 to disable distributor */
	dist_lock(gicv2_drv);
	write_gicd32(GICD_CTLR, read_gicd32(GICD_CTLR) & (~GICD_CTLR_ENABLE));
	dist_unlock(gicv2_drv);
}


/**
 * Config trigger type for an interrupt
 *
 * @param irq interrupt number [GIC_PPI_BASE..GIC_MAX_IRQ]
 * @param trigger trigger type (UK_INTCTLR_IRQ_TRIGGER_*)
 */
static
void gicv2_set_irq_trigger(__u32 irq, enum uk_intctlr_irq_trigger trigger)
{
	__u32 val, mask, oldmask;

	UK_ASSERT(irq >= GIC_PPI_BASE && irq <= GIC_MAX_IRQ);
	UK_ASSERT(trigger == UK_INTCTLR_IRQ_TRIGGER_EDGE ||
		  trigger == UK_INTCTLR_IRQ_TRIGGER_LEVEL);

	dist_lock(gicv2_drv);

	val = read_gicd32(GICD_ICFGR(irq));
	mask = oldmask = (val >> ((irq % GICD_I_PER_ICFGRn) * 2)) &
			GICD_ICFGR_MASK;

	if (trigger == UK_INTCTLR_IRQ_TRIGGER_LEVEL) {
		mask &= ~GICD_ICFGR_TRIG_MASK;
		mask |= GICD_ICFGR_TRIG_LVL;
	} else if (trigger == UK_INTCTLR_IRQ_TRIGGER_EDGE) {
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

static void gicv2_handle_irq(struct __regs *regs)
{
	__u32 stat, irq;

	do {
		stat = gicv2_ack_irq();
		irq = stat & GICC_IAR_INTID_MASK;

#ifndef CONFIG_HAVE_SMP
		uk_pr_debug("EL1 IRQ#%"__PRIu32" caught\n", irq);
#else /* !CONFIG_HAVE_SMP */
		uk_pr_debug("Core %"__PRIu64": EL1 IRQ#%"__PRIu32" caught\n",
			    ukplat_lcpu_id(), irq);
#endif /* CONFIG_HAVE_SMP */

		/* Ensure interrupt processing starts only after ACK */
		isb();

		if (irq <= GIC_MAX_IRQ) {
			uk_intctlr_irq_handle(regs, irq);
			gicv2_eoi_irq(stat);
			continue;
		}

		/* EoI should only be signaled for non-spurious interrupts */
		if (irq != GICC_IAR_INTID_SPURIOUS)
			gicv2_eoi_irq(stat);

		break;
	} while (1);
}

static void gicv2_init_dist(void)
{
	__u32 val, cpuif_number, irq_number;
	__u32 i;

	/* Turn off distributor */
	gicv2_disable_dist();

	/* Get GIC CPU interface */
	val = read_gicd32(GICD_TYPER);
	cpuif_number = GICD_TYPER_CPUI_NUM(val);
	if (cpuif_number > GIC_MAX_CPUIF)
		cpuif_number = GIC_MAX_CPUIF;
	uk_pr_info("GICv2 Max CPU interface:%d\n", cpuif_number);

	/* Get the maximum number of interrupts that the GIC supports */
	irq_number = GICD_TYPER_LINE_NUM(val);
	if (irq_number > GIC_MAX_IRQ)
		irq_number = GIC_MAX_IRQ + 1;
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
	gicv2_enable_dist();

	uk_pr_info("GICv2 distributor initialized.\n");
}

static void gicv2_init_cpuif(void)
{
	__u32 i;

	/* Set priority threshold to the lowest to make all IRQs visible to
	 * the CPU interface. Note: Higher priority corresponds to a lower
	 * priority field value.
	 */
	gicv2_set_threshold_priority(GICC_PMR_PRIO_MAX);

	/* Set PPI and SGI to the default value */
	for (i = 0; i < GIC_SPI_BASE; i += GICD_I_PER_IPRIORITYn)
		write_gicd32(GICD_IPRIORITYR(i), GICD_IPRIORITY_DEF);

	/* Deactivate SGIs and PPIs as the state is unknown at boot */
	write_gicd32(GICD_ICACTIVER(0), GICD_DEF_ICACTIVERn);
	write_gicd32(GICD_ICENABLER(0), GICD_DEF_PPI_ICENABLERn);

	/* Enable all SGIs */
	write_gicd32(GICD_ISENABLER(0), GICD_DEF_SGI_ISENABLERn);

	/* Enable CPU interface */
	gicv2_enable_cpuif();

	isb();

	uk_pr_info("GICv2 CPU interface initialized.\n");
}


/**
 * Initialize GICv2
 * NOTE: First time must not be called from multiple CPUs in parallel
 *
 * @return 0 on success, a non-zero error otherwise
 */
static int gicv2_initialize(void)
{
#ifdef CONFIG_HAVE_SMP
	if (gicv2_drv.is_initialized) {
		/* GIC is already initialized, we just need to initialize
		 * the CPU interface
		 */
		gicv2_init_cpuif();
		return 0;
	}
#endif /* CONFIG_HAVE_SMP */

	gicv2_drv.is_initialized = 1;

	/* Initialize GICv2 distributor */
	gicv2_init_dist();

	/* Initialize GICv2 CPU interface */
	gicv2_init_cpuif();

	return 0;
}

static void gicv2_set_ops(void)
{
	struct _gic_operations drv_ops = {
		.initialize        = gicv2_initialize,
		.ack_irq           = gicv2_ack_irq,
		.eoi_irq           = gicv2_eoi_irq,
		.enable_irq        = gicv2_enable_irq,
		.disable_irq       = gicv2_disable_irq,
		.set_irq_trigger   = gicv2_set_irq_trigger,
		.set_irq_prio      = gicv2_set_irq_prio,
		.set_irq_affinity  = gicv2_set_irq_target,
		.handle_irq        = gicv2_handle_irq,
		.gic_sgi_gen       = gicv2_sgi_gen_to_cpu,
	};

	/* Set driver functions */
	gicv2_drv.ops = drv_ops;
}

#if defined(CONFIG_UKPLAT_ACPI)
static int acpi_get_gicc(struct _gic_dev *g)
{
	union {
		struct acpi_madt_gicc *gicc;
		struct acpi_subsdt_hdr *h;
	} m;
	struct acpi_madt *madt;
	__sz off, len;

	madt = acpi_get_madt();
	UK_ASSERT(madt);

	/* In ACPI all GICCs' base address must be the same */
	len = madt->hdr.tab_len - sizeof(*madt);
	for (off = 0; off < len; off += m.h->len) {
		m.h = (struct acpi_subsdt_hdr *)(madt->entries + off);

		if (m.h->type != ACPI_MADT_GICC)
			continue;

		/* If GICv3/4 this field is 0 */
		if (!m.gicc->paddr)
			return -ENOTSUP;

		g->cpuif_mem_addr = m.gicc->paddr;
		g->cpuif_mem_size = GICC_MEM_SZ;

		return 0;
	}

	return -ENOENT;
}

static int gicv2_do_probe(void)
{
	int rc;

	rc = acpi_get_gicc(&gicv2_drv);
	if (unlikely(rc < 0))
		return rc;

	rc = acpi_get_gicd(&gicv2_drv);
	if (unlikely(rc < 0))
		return rc;

	if (unlikely(gicv2_drv.dist_mem_size != GICD_V2_MEM_SZ))
		return -ENOTSUP;

	return 0;
}
#else /* CONFIG_UKPLAT_ACPI */
static int gicv2_do_probe(void)
{
	struct ukplat_bootinfo *bi = ukplat_bootinfo_get();
	int fdt_gic, r;
	void *fdt;

	UK_ASSERT(bi);
	fdt = (void *)bi->dtb;

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

	r = uk_intctlr_plat_probe(&gicv2_drv);
	if (unlikely(r)) {
		uk_pr_err("GICv2 platform probe failed\n");
		return r;
	}

	return 0;
}
#endif /* !CONFIG_UKPLAT_ACPI */

#if CONFIG_PAGING
static int gicv2_map(void)
{
	__vaddr_t vbase;

	vbase = uk_bus_pf_devmap(gicv2_drv.dist_mem_addr,
				 gicv2_drv.dist_mem_size);
	if (unlikely(PTRISERR(vbase))) {
		uk_pr_err("Could not map GIC dist (%d)\n", PTR2ERR(vbase));
		return PTR2ERR(vbase);
	}

	gicv2_drv.dist_mem_addr = vbase;

	vbase = uk_bus_pf_devmap(gicv2_drv.cpuif_mem_addr,
				 gicv2_drv.cpuif_mem_size);
	if (unlikely(PTRISERR(vbase))) {
		uk_pr_err("Could not map GIC cpuif (%d)\n", PTR2ERR(vbase));
		return PTR2ERR(vbase);
	}

	gicv2_drv.cpuif_mem_addr = vbase;

	return 0;
}
#endif /* CONFIG_PAGING */

/**
 * Probe device tree or ACPI for GICv2
 * NOTE: First time must not be called from multiple CPUs in parallel
 *
 * @param [out] dev receives pointer to GICv2 if available, NULL otherwise
 *
 * @return 0 if device is available, an FDT (FDT_ERR_*) error otherwise
 */
int gicv2_probe(struct _gic_dev **dev)
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

	rc = gicv2_do_probe();
	if (rc) {
		*dev = NULL;
		return rc;
	}

#if CONFIG_PAGING
	rc = gicv2_map();
	if (unlikely(rc)) {
		uk_pr_err("Could not map device (%d)\n", rc);
		return rc;
	}
#endif /* CONFIG_PAGING */

	uk_pr_info("Found GICv2 on:\n");
	uk_pr_info("\tDistributor  : 0x%lx - 0x%lx\n",
		   gicv2_drv.dist_mem_addr,
		   gicv2_drv.dist_mem_addr + gicv2_drv.dist_mem_size - 1);
	uk_pr_info("\tCPU interface: 0x%lx - 0x%lx\n",
		   gicv2_drv.cpuif_mem_addr,
		   gicv2_drv.cpuif_mem_addr + gicv2_drv.cpuif_mem_size - 1);

	/* GICv2 is present */
	gicv2_drv.is_present = 1;
	gicv2_set_ops();

	*dev = &gicv2_drv;
	return 0;
}
