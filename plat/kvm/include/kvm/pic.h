#ifndef __PLAT_KVM_X86_PIC_H
#define __PLAT_KVM_X86_PIC_H

#include <stdint.h>
#include <kvm/intctrl.h>
#include <x86/acpi/acpi.h>
#include <x86/cpu.h>

#define IMCR_ADDR        0x22
#define IMCR_DATA		 0x23

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

#define PIC_LEVEL_TRIGGER_MODE 0
#define PIC_EDGE_TRIGGER_MODE 1


#define MAX_PIC_INTR 15

void pic_mask_irq(uint32_t irq);

static inline
void disable_pic(void)
{
	__u16 port;

	port = PIC1_DATA;
	outb(port, 0xff);
	port = PIC2_DATA;
	outb(port, 0xff);
}

/* Probe the APIC using ACPI MADT 
 * 
 * @param madt
 *  The ACPI madt structure
 *
 * @param dev
 *  The interrupt controller device, which is to be initialized
 */
int pic_probe(const struct MADT *madt, struct _pic_dev **dev);

#endif
