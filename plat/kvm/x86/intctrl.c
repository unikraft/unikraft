/* SPDX-License-Identifier: ISC */
/*
 * Authors: Dan Williams
 *          Martin Lucina
 *
 * Copyright (c) 2015-2017 IBM
 * Copyright (c) 2016-2017 Docker, Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose with or without fee is hereby granted, provided
 * that the above copyright notice and this permission notice appear
 * in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/* Taken from solo5 platform_intr.c */

#include <stdint.h>
#include <x86/cpu.h>
#include <kvm/intctrl.h>

#define PIC1             0x20    /* IO base address for master PIC */
#define PIC2             0xA0    /* IO base address for slave PIC */
#define PIC1_COMMAND     PIC1
#define PIC1_DATA        (PIC1 + 1)
#define PIC2_COMMAND     PIC2
#define PIC2_DATA        (PIC2 + 1)
#define IRQ_ON_MASTER(n) ((n) < 8)
#define IRQ_PORT(n)      (IRQ_ON_MASTER(n) ? PIC1_DATA : PIC2_DATA)
#define IRQ_OFFSET(n)    (IRQ_ON_MASTER(n) ? (n) : ((n) - 8))

#define PIC_EOI          0x20 /* End-of-interrupt command code */
#define ICW1_ICW4        0x01 /* ICW4 (not) needed */
#define ICW1_SINGLE      0x02 /* Single (cascade) mode */
#define ICW1_INTERVAL    0x04 /* Call address interval 4 (8) */
#define ICW1_LEVEL       0x08 /* Level triggered (edge) mode */
#define ICW1_INIT        0x10 /* Initialization - required! */
#define ICW4_8086        0x01 /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO        0x02 /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE   0x08 /* Buffered mode/slave */
#define ICW4_BUF_MASTER  0x0C /* Buffered mode/master */
#define ICW4_SFN         0x10 /* Special fully nested (not) */

/*
 * arguments:
 * offset1 - vector offset for master PIC vectors on the master become
 *           offset1..offset1+7
 * offset2 - same for slave PIC: offset2..offset2+7
 */
static void PIC_remap(int offset1, int offset2)
{
	unsigned char a1, a2;

	/* save masks */
	a1 = inb(PIC1_DATA);
	a2 = inb(PIC2_DATA);

	/* start init seq (cascade) */
	outb(PIC1_COMMAND, ICW1_INIT + ICW1_ICW4);
	outb(PIC2_COMMAND, ICW1_INIT + ICW1_ICW4);
	/* ICW2: Master PIC vector off */
	outb(PIC1_DATA, offset1);
	/* ICW2: Slave PIC vector off */
	outb(PIC2_DATA, offset2);
	/* ICW3: tell Master PIC there is a slave PIC at IRQ2 (0000 0100) */
	outb(PIC1_DATA, 4);
	/* ICW3: tell Slave PIC its cascade identity (0000 0010) */
	outb(PIC2_DATA, 2);

	outb(PIC1_DATA, ICW4_8086);
	outb(PIC2_DATA, ICW4_8086);

	outb(PIC1_DATA, a1); /* restore saved masks. */
	outb(PIC2_DATA, a2);
}

void intctrl_init(void)
{
	PIC_remap(32, 40);
}

void intctrl_ack_irq(unsigned int irq)
{
	if (!IRQ_ON_MASTER(irq))
		outb(PIC2_COMMAND, PIC_EOI);

	outb(PIC1_COMMAND, PIC_EOI);
}

void intctrl_mask_irq(unsigned int irq)
{
	__u16 port;

	port = IRQ_PORT(irq);
	outb(port, inb(port) | (1 << IRQ_OFFSET(irq)));
}

void intctrl_clear_irq(unsigned int irq)
{
	__u16 port;

	port = IRQ_PORT(irq);
	outb(port, inb(port) & ~(1 << IRQ_OFFSET(irq)));
}
