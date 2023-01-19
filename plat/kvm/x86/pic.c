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

#include <kvm/pic.h>
#include <uk/arch/types.h>
#include <x86/cpu.h>
/*
 * arguments:
 * offset1 - vector offset for master PIC vectors on the master become
 *           offset1..offset1+7
 * offset2 - same for slave PIC: offset2..offset2+7
 */
static void PIC_remap(int offset1, int offset2)
{
	uk_pr_debug(">>>>>>>>>>>>>>>>>>>> PIC remapped\n");
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
	/* ICW3: tell Slave PIC its cascade identity (0000 0010) */ outb(PIC2_DATA, 2); outb(PIC1_DATA, ICW4_8086);
	outb(PIC2_DATA, ICW4_8086);

	outb(PIC1_DATA, a1); /* restore saved masks. */
	outb(PIC2_DATA, a2);
}

int pic_init(void)
{
	PIC_remap(32, 40);
	return 0;
}

void pic_ack_irq(unsigned int irq)
{
	if (!IRQ_ON_MASTER(irq))
		outb(PIC2_COMMAND, PIC_EOI);

	outb(PIC1_COMMAND, PIC_EOI);
}

void pic_mask_irq(unsigned int irq)
{
	__u16 port;

	port = IRQ_PORT(irq);
	outb(port, inb(port) | (1 << IRQ_OFFSET(irq)));
}

void pic_clear_irq(unsigned int irq)
{
	__u16 port;

	port = IRQ_PORT(irq);
	outb(port, inb(port) & ~(1 << IRQ_OFFSET(irq)));
}

void pic_set_trigger_type(unsigned int irq, __u8 trigger)
{
	/* TODO: yet to be implemented */
}

uint32_t pic_get_max_irqs(void)
{
	return MAX_PIC_INTR;
}

/* Dummy functions */
void pic_set_irq_prio(unsigned int irq __unused, __u8 priority __unused)
{
	return;
}

void __unused pic_set_irq_affinity(unsigned int irq __unused, __u8 affinity __unused)
{
	return;
}

void pic_handle_irq(void)
{
	return;
}
