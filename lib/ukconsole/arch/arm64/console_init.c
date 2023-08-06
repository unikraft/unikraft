/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <libfdt.h>
#include <uk/console.h>
#include <uk/print.h>

/* uk_console_ops definitions */
#ifdef CONFIG_UART_PL011
extern struct uk_console_ops uk_console_pl011_ops;
#endif
#ifdef CONFIG_UART_NS16550
extern struct uk_console_ops uk_console_ns16550_ops;
#endif
extern struct uk_console_ops uk_console_null_ops;

/* setup early debug */
#if CONFIG_EARLY_UART_PL011
struct uk_console_ops *uk_console_ops = &uk_console_pl011_ops;
#elif CONFIG_EARLY_UART_NS16550
struct uk_console_ops *uk_console_ops = &uk_console_ns16550_ops;
#else
struct uk_console_ops *uk_console_ops = &uk_console_null_ops;
#endif

void uk_console_init(struct ukplat_bootinfo *bi)
{
	void *dtb = (void *)bi->dtb;

	/* find the device by name */
#ifdef CONFIG_UART_PL011
	if (fdt_node_offset_by_compatible(dtb, -1, "arm,pl011") >= 0)
		uk_console_ops = &uk_console_pl011_ops;
#endif
#ifdef CONFIG_UART_NS16550
	if (fdt_node_offset_by_compatible(dtb, -1, "ns16550") >= 0 ||
	    fdt_node_offset_by_compatible(dtb, -1, "ns16550a") >= 0) {
		uk_console_ops = &uk_console_ns16550_ops;
	}
#endif

	if (uk_console_ops == &uk_console_null_ops)
		UK_CRASH("No console UART found!\n");

	/* Initialize the corresponding console */
	uk_console_ops->init(bi);

	uk_pr_info("Console init finished\n");
}
