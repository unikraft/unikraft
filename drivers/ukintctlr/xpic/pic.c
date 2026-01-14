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

#include <uk/asm/pic.h>
#include <uk/arch.h>
#include <uk/arch/util.h>
#include <uk/essentials.h>
#include <uk/intctlr.h>

static void pic_mask_irq(unsigned int irq);
static void pic_clear_irq(unsigned int irq);

static struct uk_intctlr_driver_ops pic_ops = {
	.fdt_xlat = __NULL,
	.mask_irq = pic_mask_irq,
	.unmask_irq = pic_clear_irq,
};

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
	a1 = uk_arch_inb(PIC1_DATA);
	a2 = uk_arch_inb(PIC2_DATA);

	/* start init seq (cascade) */
	uk_arch_outb(PIC1_COMMAND, ICW1_INIT + ICW1_ICW4);
	uk_arch_outb(PIC2_COMMAND, ICW1_INIT + ICW1_ICW4);
	/* ICW2: Master PIC vector off */
	uk_arch_outb(PIC1_DATA, offset1);
	/* ICW2: Slave PIC vector off */
	uk_arch_outb(PIC2_DATA, offset2);
	/* ICW3: tell Master PIC there is a slave PIC at IRQ2 (0000 0100) */
	uk_arch_outb(PIC1_DATA, 4);
	/* ICW3: tell Slave PIC its cascade identity (0000 0010) */
	uk_arch_outb(PIC2_DATA, 2);

	uk_arch_outb(PIC1_DATA, ICW4_8086);
	uk_arch_outb(PIC2_DATA, ICW4_8086);

	uk_arch_outb(PIC1_DATA, a1); /* restore saved masks. */
	uk_arch_outb(PIC2_DATA, a2);
}

int pic_init(struct uk_intctlr_driver_ops **ops)
{
	PIC_remap(32, 40);

	*ops = &pic_ops;

	return 0;
}

void pic_ack_irq(unsigned int irq)
{
	if (!IRQ_ON_MASTER(irq))
		uk_arch_outb(PIC2_COMMAND, PIC_EOI);

	uk_arch_outb(PIC1_COMMAND, PIC_EOI);
}

static void pic_mask_irq(unsigned int irq)
{
	__u16 port;

	port = IRQ_PORT(irq);
	uk_arch_outb(port, uk_arch_inb(port) | (1 << IRQ_OFFSET(irq)));
}

static void pic_clear_irq(unsigned int irq)
{
	__u16 port;

	port = IRQ_PORT(irq);
	uk_arch_outb(port, uk_arch_inb(port) & ~(1 << IRQ_OFFSET(irq)));
}

