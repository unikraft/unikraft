/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#include <errno.h>
#include <uk/config.h>
#include <uk/assert.h>
#include <uk/intctlr.h>
#include <uk/intctlr/gic-v2.h>
#include <uk/intctlr/gic-v3.h>

struct _gic_dev *gic;
struct uk_intctlr_desc intctlr;
struct uk_intctlr_driver_ops ops;

#if CONFIG_LIBUKOFW

#include <libfdt.h>
#include <uk/ofw/fdt.h>

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
		UK_ASSERT((irq_num + GIC_SPI_BASE) <= UK_INTCTLR_GIC_MAX_IRQ);
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

	return uk_intctlr_register(&intctlr);
}
