/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/**
 * Null/minimal interrupt controller for Hyperlight.
 * 
 * Hyperlight is a minimal VMM that doesn't emulate legacy hardware
 * like the 8259 PIC. This driver provides a no-op implementation
 * that doesn't touch any I/O ports.
 */

#include <uk/intctlr.h>
#include <uk/assert.h>

static struct uk_intctlr_desc intctlr;

static int configure_irq(struct uk_intctlr_irq *irq __unused)
{
	return 0;
}

static void hyperlight_mask_irq(unsigned int irq __unused)
{
	/* No-op - Hyperlight doesn't have a PIC to mask */
}

static void hyperlight_unmask_irq(unsigned int irq __unused)
{
/* No-op - Hyperlight doesn't have a PIC to unmask */
}

static struct uk_intctlr_driver_ops hyperlight_ops = {
	.fdt_xlat = __NULL,
	.mask_irq = hyperlight_mask_irq,
	.unmask_irq = hyperlight_unmask_irq,
	.configure_irq = configure_irq,
};

void uk_intctlr_hyperlight_handle_irq(struct uk_lcpu_except_irq_ctx *ctx)
{
	uk_intctlr_irq_handle(ctx);
	/* No ACK needed - Hyperlight handles this at VMM level */
}

int uk_intctlr_probe(void)
{
	intctlr.name = "Hyperlight";
	intctlr.ops = &hyperlight_ops;

	return uk_intctlr_register(&intctlr);
}
