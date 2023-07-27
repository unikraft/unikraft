/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <libfdt.h>
#include <uk/console.h>
#include <uk/print.h>

extern struct uk_console_ops *uk_console_ops;
extern struct uk_console_ops uk_console_pl011_ops;
extern struct uk_console_ops uk_console_ns16550_ops;

void uk_console_init(struct ukplat_bootinfo *bi)
{
	void *dtb = (void *)bi->dtb;

	/* find the device by name */
	if (fdt_node_offset_by_compatible(dtb, -1, "arm,pl011") >= 0)
		uk_console_ops = &uk_console_pl011_ops;

	if (fdt_node_offset_by_compatible(dtb, -1, "ns16550") >= 0 ||
	    fdt_node_offset_by_compatible(dtb, -1, "ns16550a") >= 0) {
		uk_console_ops = &uk_console_ns16550_ops;
	}

	if (!uk_console_ops)
		UK_CRASH("No console UART found!\n");

	/* Initialize the corresponding console */
	uk_console_ops->init(bi);

	uk_pr_info("Console init finished\n");
}
