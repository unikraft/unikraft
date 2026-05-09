/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/config.h>
#include <uk/console/driver.h>
#include <uk/arch/types.h>

/**
 * Hyperlight debug print port.
 * Each byte written to this port is sent to the host for printing.
 */
#define HYPERLIGHT_PORT_DEBUG_PRINT  103

/**
 * Write a byte to an I/O port using x86 OUT instruction.
 */
static inline void outb(__u16 port, __u8 value)
{
	__asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * Console output function - sends each byte to Hyperlight host
 * via the debug print port.
 */
static __ssz hyperlight_console_out(struct uk_console *dev __unused,
				    const char *buf, __sz len)
{
	for (__sz i = 0; i < len; i++)
		outb(HYPERLIGHT_PORT_DEBUG_PRINT, buf[i]);
	return len;
}

/**
 * Console input function - not supported in Hyperlight
 */
static __ssz hyperlight_console_in(struct uk_console *dev __unused,
				   char *buf __unused, __sz len __unused)
{
	/* Input not supported */
	return 0;
}

static struct uk_console_ops hyperlight_console_ops = {
	.out = hyperlight_console_out,
	.in = hyperlight_console_in,
};

static struct uk_console hyperlight_console;

/**
 * Initialize and register the Hyperlight console as early console.
 * Called from setup.c during early boot.
 */
void _ukplat_init_console(void)
{
	uk_console_init(&hyperlight_console, "Hyperlight",
			&hyperlight_console_ops,
			UK_CONSOLE_FLAG_STDOUT | UK_CONSOLE_FLAG_STDIN);

	uk_console_register(&hyperlight_console);
}
