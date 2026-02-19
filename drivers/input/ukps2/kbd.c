/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/bitops.h>
#include <uk/event.h>
#include <uk/init.h>
#include <uk/intctlr.h>
#include <x86/delay.h>

#include "ps2.h"

/* Default legacy IRQ for the PS/2 keyboard */
#define PIC1_IRQ_KBD				0x1

static __u8 lctrl_pressed;
static __u8 lalt_pressed;

UK_EVENT(UKPLAT_SHUTDOWN_EVENT);

static int kbd_ps2_irq_handler(void *arg __unused)
{
	/* Read received scan code (ACPI scan code set) */
	__u16 sc;

	/* If not a key pressed, probably another event on first PS/2 port */
	if (!(uk_arch_inb(PS2_STATUS_REG) & PS2_STATUS_RECV_FULL))
		return 0;

	sc = (uk_arch_inb(PS2_DATA_REG) << 8) + uk_arch_inb(PS2_DATA_REG);

	switch (sc) {
	/* TODO: This is dumbed down, enough for Firecracker shutdown. In
	 * reality the scan codes will most likely not conveniently come one
	 * after another.
	 */
	case ((PS2_KEY_CTRL << 8) | PS2_KEY_ALT):
		lctrl_pressed = 1;
		lalt_pressed = 1;

		break;
	case PS2_KEY_DEL:
		if (!(lctrl_pressed && lalt_pressed))
			break;

		uk_raise_event(UKPLAT_SHUTDOWN_EVENT, (void *)UKPLAT_HALT);

		break;
	}

	return 0;
}

/* Quick, dumbed down, initialization for PS/2 keyboard */
static int kbd_ps2_probe(struct uk_init_ctx *ictx __unused)
{
	int counter = 0;
	__u8 cfg;
	int rc;

	/* In Firecracker's words:
	 * "A i8042 PS/2 controller that emulates just enough to shutdown the
	 * machine."
	 * See Firecracker's 80128ea61b30 ("New API action: SendCtrlAltDel").
	 */

	/* PS/2 Keyboard is on first port. Just enable it. */
	uk_arch_outb(PS2_CMD_REG, PS2_CMD_EN_KBD);

	/* Send read current configuration register command */
	uk_arch_outb(PS2_CMD_REG, PS2_CMD_READ_CFG);

	/* Wait for response byte by checking status bit of receive buffer.
	 * Try for 5 times, but it should work the first time usually.
	 */
	while (!(uk_arch_inb(PS2_STATUS_REG) & PS2_STATUS_RECV_FULL)) {
		if (unlikely(counter >= 5)) {
			uk_arch_outb(PS2_CMD_REG, PS2_CMD_DIS_KBD);
			uk_pr_err("PS/2 Controller unresponsie\n");
			return -ENODEV;
		}

		/* We are advised to wait 50ms if the controller hasn't
		 * responded yet.
		 */
		udelay(50);
		counter++;
	}

	/* Read the configuration register */
	cfg = uk_arch_inb(PS2_DATA_REG);

	/* This shouldn't even be needed in a virtualized environment, e.g.
	 * on Firecracker the above sending of PS2_CMD_EN_KBD would be enough.
	 */
	cfg |= PS2_CFG_REG_EN_KBD_IRQ;
	cfg &= ~PS2_CFG_REG_DIS_KBD_CLK;

	/* Send write current configuration register command */
	uk_arch_outb(PS2_CMD_REG, PS2_CMD_WRITE_CFG);

	/* Send the new configuration register value */
	uk_arch_outb(PS2_DATA_REG, cfg);

	/* TODO: Legacy wired to Master PIC IRQ 1, with I/O-APIC this is likely
	 * rewired so check ACPI MADT Interrupt Source Override.
	 */
	rc = uk_intctlr_irq_register(PIC1_IRQ_KBD, kbd_ps2_irq_handler, NULL);
	if (unlikely(rc)) {
		uk_pr_err("Failed to register PS/2 Keryboard IRQ\n");
		return rc;
	}

	return 0;
}

uk_early_initcall(kbd_ps2_probe, 0x0);
