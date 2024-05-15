/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#include <kvm/efi.h>
#include <uk/arch/paging.h>
#include <uk/plat/common/bootinfo.h>
#include <uk/plat/lcpu.h>
#include <x86/cpu.h>

/* Slave Controller Edge/Level Triggered Register */
#define PIC2_ELCR2					0x4D1
#define PIC2_ELCR2_IRQ11_ECL				(1 << 3)
#define PIC2_ELCR2_IRQ10_ECL				(1 << 2)

/* Initial Count Register (for Timer) */
#define LAPIC_TMICT					0xFEE00380

/* APIC MSR Register */
/* TODO: This should be removed once our APIC driver is able to handle LAPIC */
#define LAPIC_MSR_BASE					0x01B
#define LAPIC_BASE_EN					(1 << 11)

/* Master/Slave PIC Data Registers */
#define PIC1_DATA					0x21
#define PIC1_DATA_DEFAULT_MASK				0xB8
#define PIC2_DATA					0xA1
#define PIC2_DATA_DEFAULT_MASK				0x8E

void lcpu_start64(void *, void *) __noreturn;
void _ukplat_entry(void *, void *);

extern void *x86_bpt_pml4;

static __u8 __align(16) uk_efi_bootstack[__PAGE_SIZE];

static struct {
	void *entry_fn;
	void *bootstack;
} uk_efi_boot_startup_args;

/* Unless UEFI CSM (now dropped from the specification) is activated, our PIC's
 * are masked
 */
static inline void unmask_8259_pic(void)
{
	outb(PIC1_DATA, PIC1_DATA_DEFAULT_MASK);
	outb(PIC2_DATA, PIC2_DATA_DEFAULT_MASK);
}

/* UEFI enables the LAPIC Timer to run periodic routines, usually at 10KHz */
static inline void lapic_timer_disable(void)
{
	volatile __u32 *lapic_tmict = (volatile __u32 *)LAPIC_TMICT;
	__u32 eax, edx;

	/* Check if APIC is active */
	rdmsr(LAPIC_MSR_BASE, &eax, &edx);
	if (unlikely(!(eax & LAPIC_BASE_EN)))
		return;

	/* Zero-ing out this register disables the LAPIC Timer */
	*lapic_tmict = 0x0;
}

/* Unless UEFI CSM (now dropped from the specification) is activated, our PIC's
 * are masked and their interrupts mode are not configured.
 * TODO: Until we have a proper IRQ subsystem to transparently set IRQ type when
 * registering an IRQ, set the known PIIX/PIIX3 shared PCI IRQ's as
 * level-triggered
 */
static inline void pic_8259_elcr2_level_irq10_11(void)
{
	outb(PIC2_ELCR2, PIC2_ELCR2_IRQ11_ECL | PIC2_ELCR2_IRQ10_ECL);
}

void __noreturn uk_efi_jmp_to_kern()
{
	struct ukplat_bootinfo *bi = ukplat_bootinfo_get();

	if (unlikely(!bi))
		ukplat_crash();

	uk_efi_boot_startup_args.entry_fn = &_ukplat_entry;
	uk_efi_boot_startup_args.bootstack = uk_efi_bootstack + __PAGE_SIZE;

	ukplat_lcpu_disable_irq();
	ukarch_pt_write_base((__paddr_t)&x86_bpt_pml4);
	unmask_8259_pic();
	lapic_timer_disable();
	pic_8259_elcr2_level_irq10_11();
	lcpu_start64(&uk_efi_boot_startup_args, bi);
}
