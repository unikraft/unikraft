/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>

#include <uk/assert.h>
#include <uk/config.h>
#include <uk/intctlr.h>
#include <uk/intctlr/gic-v2.h>
#include <uk/intctlr/gic-v3.h>
#include <uk/intctlr/limits.h>
#include <uk/pcpuvar.h>
#include <uk/print.h>

#if CONFIG_LIBUKACPI
#include <uk/acpi.h>
#endif /* CONFIG_LIBUKACPI */

#if CONFIG_LIBUKOFW
#include <libfdt.h>
#include <uk/ofw/fdt.h>
#include <uk/plat/common/bootinfo.h>
#endif /* CONFIG_LIBUKOFW */

struct _gic_dev *gic;
struct uk_intctlr_desc intctlr;
struct uk_intctlr_driver_ops ops;

#if CONFIG_HAVE_SMP

#if CONFIG_LIBUKACPI

#define CPU_ID_MASK					\
	(UK_ARCH_ARM64_MPIDR_EL1_AFF0_MASK |		\
	 UK_ARCH_ARM64_MPIDR_EL1_AFF1_MASK |		\
	 UK_ARCH_ARM64_MPIDR_EL1_AFF2_MASK |		\
	 UK_ARCH_ARM64_MPIDR_EL1_AFF3_MASK)


static int enumerate_cpus(void)
{
	int bsp_found __maybe_unused = 0;
	struct uk_acpi_madt *madt;
	__u64 bsp_cpu_id;
	__u64 cpu_id;
	union {
		struct uk_acpi_madt_gicc *gicc;
		struct uk_acpi_subsdt_hdr *h;
	} m;
	__sz idx, off, len;

	bsp_cpu_id = uk_pcpuvar_lval(0, uk_pcpuvar_cpu_id);
	uk_pr_info("Bootstrapping processor has the ID %ld\n", bsp_cpu_id);

	/* Enumerate all other CPUs */
	madt = uk_acpi_get_madt();
	UK_ASSERT(madt);

	len = madt->hdr.tab_len - sizeof(*madt);
	idx = 1;
	for (off = 0; off < len; off += m.h->len) {
		m.h = (struct uk_acpi_subsdt_hdr *)(madt->entries + off);

		if (m.h->type != UK_ACPI_MADT_GICC ||
		    (!(m.gicc->flags & UK_ACPI_MADT_GICC_FLAGS_EN) &&
		     !(m.gicc->flags & UK_ACPI_MADT_GICC_FLAGS_ON_CAP)))
			continue;

		cpu_id = m.gicc->mpidr & CPU_ID_MASK;

		if (bsp_cpu_id == cpu_id) {
			UK_ASSERT(!bsp_found);
			bsp_found = 1;
			continue;
		}

		uk_pcpuvar_lval(idx, uk_pcpuvar_cpu_id) = cpu_id;
		uk_pcpuvar_lval(idx, uk_pcpuvar_cpu_idx) = idx;
		idx++;

		/* Ignore cores that exceed max configured value */
		if (unlikely(idx == CONFIG_UKPLAT_CPU_MAXCOUNT)) {
			uk_pr_warn("Maximum number of cores exceeded.\n");
			return 0;
		}
	}
	UK_ASSERT(bsp_found);

	return 0;
}

#elif CONFIG_LIBUKOFW

/**
 * The number of cells for the size field should be 0 in cpu nodes.
 * The number of cells in the address field is set by default to 2 in cpu
 * nodes. This is also the maximum number of cells for the address field that
 * we can currently support.
 */
#define FDT_CPU_SIZE_CELLS_DEFAULT 0
#define FDT_CPU_ADDR_CELLS_DEFAULT 2

static int enumerate_cpus(void)
{
	int fdt_cpu;
	const fdt32_t *naddr_prop, *nsize_prop, *id_reg;
	const struct fdt_property *cpu_prop;
	int naddr, nsize = 0, cell;
	int subnode;
	__u64 bsp_cpu_id;
	__u64 cpu_id;
	char bsp_found __maybe_unused = 0;
	void *dtb;
	__sz idx;

	/* MP support is dependent on an initialized GIC */
	UK_ASSERT(gic);

	dtb = (void *)ukplat_bootinfo_get()->dtb;
	UK_ASSERT(dtb);

	bsp_cpu_id = uk_pcpuvar_lval(0, uk_pcpuvar_cpu_id);
	uk_pr_info("Bootstrapping processor has the ID %ld\n",
		   bsp_cpu_id);

	/* Search for CPUS node in DTB */
	fdt_cpu = fdt_path_offset(dtb, "/cpus");
	if (unlikely(fdt_cpu < 0)) {
		uk_pr_err("cpus node is not found in device tree\n");
		return -EINVAL;
	}

	/* Get address cells */
	naddr_prop = fdt_getprop(dtb, fdt_cpu, "#address-cells", &cell);
	if (!naddr_prop) {
		naddr = FDT_CPU_ADDR_CELLS_DEFAULT;
		uk_pr_warn("Using default value 2 for cpu address-cells.\n");
	} else {
		naddr = fdt32_to_cpu(naddr_prop[0]);
	}
	if (unlikely(naddr < 0 || naddr >= FDT_MAX_NCELLS)) {
		uk_pr_err("Invalid address-cells size.\n");
		return -EINVAL;
	}
	if (unlikely(naddr > FDT_CPU_ADDR_CELLS_DEFAULT)) {
		uk_pr_err("Address-cells size greater than 2 not supported yet.\n");
		return -EINVAL;
	}

	/* Get size cells */
	nsize_prop = fdt_getprop(dtb, fdt_cpu, "#size-cells", &cell);
	if (!nsize_prop) {
		nsize = FDT_CPU_SIZE_CELLS_DEFAULT;
		uk_pr_warn("Using default value of 0 for cpu size-cells.\n");
	} else {
		nsize = fdt32_to_cpu(nsize_prop[0]);
	}
	/**
	 * According to the device-tree v0.3 specification,
	 * for the `size-cells` property of the `/cpus` node:
	 * "Value shall be 0. Specifies that no size is
	 * required in the reg property in children of this node."
	 */
	if (unlikely(nsize != 0)) {
		uk_pr_err("Invalid size-cells value for cpu node.\n");
		return -EINVAL;
	}

	/* Search all the CPU nodes in DTB */
	idx = 1;
	fdt_for_each_subnode(subnode, dtb, fdt_cpu) {
		int prop_len = 0;

		cpu_prop = fdt_get_property(dtb, subnode,
					    "device_type", &prop_len);
		if (!cpu_prop)
			continue;
		if (strcmp(cpu_prop->data, "cpu"))
			continue;

		cpu_prop = fdt_get_property(dtb, subnode, "reg", &prop_len);
		if (unlikely(!cpu_prop || prop_len <= 0)) {
			uk_pr_err("CPU node does not contain reg field (cpu_id), this core will not be enabled.\n");
			continue;
		}
		id_reg = (const fdt32_t *)cpu_prop->data;
		cpu_id = fdt_reg_read_number(id_reg, naddr);

		cpu_prop = fdt_get_property(dtb, subnode,
					    "enable-method", NULL);
		if (!cpu_prop) {
			if (cpu_id != bsp_cpu_id) {
				uk_pr_err("enable-method for core 0x%lx not set, core will not be enabled.",
					  cpu_id);
				continue;
			} else {
				uk_pr_warn("enable-method for core 0x%lx not set\n",
					   cpu_id);
				bsp_found = 1;
				continue;
			}
		} else if (strcmp(cpu_prop->data, "psci")) {
			uk_pr_err("enable-method for core 0x%lx is not PSCI : (%s), core will not be enabled.\n",
				  cpu_id, cpu_prop->data);
			continue;
		}

		if (cpu_id == bsp_cpu_id) {
			UK_ASSERT(!bsp_found);
			bsp_found = 1;
			continue;
		}

		uk_pcpuvar_lval(idx, uk_pcpuvar_cpu_id) = cpu_id;
		uk_pcpuvar_lval(idx, uk_pcpuvar_cpu_idx) = idx;
		idx++;

		if (unlikely(idx == CONFIG_UKPLAT_CPU_MAXCOUNT)) {
			/* Handle gracefully */
			uk_pr_warn("Maximum number of cores exceeded.\n");
			return 0;
		}
	}
	UK_ASSERT(bsp_found);

	return 0;
}

#else /* !CONFIG_LIBUKACPI && !CONFIG_LIBUKOFW */
#error "Invalid config: Either CONFIG_LIBUKACPI or CONFIG_LIBUKOFW must be selected"
#endif /* !CONFIG_LIBUKACPI && !CONFIG_LIBUKOFW */

#endif /* CONFIG_HAVE_SMP */

#if CONFIG_LIBUKOFW
#define FDT_GIC_IRQ_FLAGS_TL_MASK	0xf /* trigger - level */
#define FDT_GIC_IRQ_FLAGS_TL_EDGE_HI	1   /* edge trigerred, low-to-high */
#define FDT_GIC_IRQ_FLAGS_TL_EDGE_LO	2   /* edge trigerred, high-to-low */
#define FDT_GIC_IRQ_FLAGS_TL_LEVEL_HI	4   /* level sensitive, active high */
#define FDT_GIC_IRQ_FLAGS_TL_LEVEL_LO	8   /* level sensitive, active low */

static int fdt_xlat(const void *fdt, int nodeoffset, __u32 index,
		    struct uk_intctlr_irq *irq)
{
	int rc, size;
	fdt32_t *prop;
	__u32 type, irq_num, flags;

	UK_ASSERT(irq);

	rc = fdt_get_interrupt(fdt, nodeoffset, index, &size, &prop);
	if (unlikely(rc < 0))
		return rc;

	/* GIC interrupt-cells is const 3 */
	UK_ASSERT(size == 3);

	type = fdt32_to_cpu(prop[0]);
	irq_num = fdt32_to_cpu(prop[1]);
	flags = fdt32_to_cpu(prop[2]);

	switch (type) {
	case GIC_SPI_TYPE:
		UK_ASSERT((irq_num + GIC_SPI_BASE) <= UK_INTCTLR_MAX_IRQ);
		irq->id = irq_num + GIC_SPI_BASE;
		break;
	case GIC_PPI_TYPE:
		UK_ASSERT((irq_num + GIC_PPI_BASE) < GIC_SPI_BASE);
		irq->id = irq_num + GIC_PPI_BASE;
		break;
	default:
		return -FDT_ERR_BADVALUE;
	}

	switch (flags & FDT_GIC_IRQ_FLAGS_TL_MASK) {
	case FDT_GIC_IRQ_FLAGS_TL_EDGE_HI:
	case FDT_GIC_IRQ_FLAGS_TL_EDGE_LO:
		irq->trigger = UK_INTCTLR_IRQ_TRIGGER_EDGE;
		break;
	case FDT_GIC_IRQ_FLAGS_TL_LEVEL_HI:
	case FDT_GIC_IRQ_FLAGS_TL_LEVEL_LO:
		irq->trigger = UK_INTCTLR_IRQ_TRIGGER_LEVEL;
		break;
	default:
		return -FDT_ERR_BADVALUE;
	}

	return 0;
}
#endif /* CONFIG_LIBUKOFW */

static int configure_irq(struct uk_intctlr_irq *irq)
{
	if (irq->trigger != UK_INTCTLR_IRQ_TRIGGER_NONE)
		gic->ops.set_irq_trigger(irq->id, irq->trigger);

	return 0;
}

int uk_intctlr_probe(void)
{
	int rc = -ENODEV;

#if CONFIG_LIBUKINTCTLR_GICV2
	/* First, try GICv2 */
	rc = gicv2_probe(&gic);
	if (rc == 0) {
		intctlr.name = "GICv2";
		rc = gic->ops.initialize();
		goto init;
	}
#endif /* CONFIG_LIBUKINTCTLR_GICV2 */

#if CONFIG_LIBUKINTCTLR_GICV3
	/* GICv2 is not present, try GICv3 */
	rc = gicv3_probe(&gic);
	if (rc == 0) {
		intctlr.name = "GICv3";
		rc = gic->ops.initialize();
		goto init;
	}
#endif /* CONFIG_LIBUKINTCTLR_GICV3 */

init:
	if (unlikely(rc))
		return rc;

	ops.configure_irq = configure_irq;
	ops.mask_irq = gic->ops.disable_irq;
	ops.unmask_irq = gic->ops.enable_irq;
#if CONFIG_LIBUKOFW
	ops.fdt_xlat = fdt_xlat;
#endif /* CONFIG_LIBUKOFW */

	intctlr.ops = &ops;

	rc = uk_intctlr_register(&intctlr);
	if (unlikely(rc))
		return rc;

#if CONFIG_HAVE_SMP
	rc = enumerate_cpus();
#endif /* CONFIG_HAVE_SMP */

	return rc;
}
