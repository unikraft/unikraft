/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>
#include <uk/arch/util.h>
#include <uk/arch/x86_64.h>
#include <uk/assert.h>
#include <uk/event.h>
#include <uk/config.h>
#include <uk/intctlr.h>
#include <uk/intctlr/limits.h>
#include <uk/lcpu.h>

#if CONFIG_LIBUKINTCTLR_APIC
#include <uk/arch/x86_64.h>
#include <uk/arch/util.h>
#include <uk/intctlr/msi.h>

#define APIC_MSI_BASE			0xfee00000U
#define APIC_MSI_DEST_SHIFT		12U

static inline int x2apic_enable(void)
{
	__u32 eax, ebx, ecx, edx;

	/* Check for x2APIC support */
	uk_arch_x86_64_cpuid(1, 0, &eax, &ebx, &ecx, &edx);
	if (!(ecx & UK_ARCH_X86_64_CPUID1_ECX_X2APIC))
		return -ENOTSUP;

	/* Check if APIC is active */
	uk_arch_x86_64_rdmsr(UK_ARCH_X86_64_APIC_MSR_BASE,
					    &eax, &edx);
	if (!(eax & UK_ARCH_X86_64_APIC_BASE_EN))
		return -ENOTSUP;

	/* Switch to x2APIC mode */
	eax |= UK_ARCH_X86_64_APIC_BASE_EXTD;
	uk_arch_x86_64_wrmsr(UK_ARCH_X86_64_APIC_MSR_BASE,
					    eax, edx);

	/* Set APIC software enable flag if necessary */
	uk_arch_x86_64_rdmsr(UK_ARCH_X86_64_APIC_MSR_SVR,
					    &eax, &edx);
	if ((eax & UK_ARCH_X86_64_APIC_SVR_EN) == 0) {
		eax |= UK_ARCH_X86_64_APIC_SVR_EN;
		uk_arch_x86_64_wrmsr(UK_ARCH_X86_64_APIC_MSR_SVR,
						    eax, edx);
	}

	/*
	 * TODO: Configure spurious interrupt vector number
	 * After power-up or reset this is 0xff, which might not be
	 * configured in the trap table
	 */

	return 0;
}

static inline void x2apic_ack_interrupt(void)
{
	uk_arch_x86_64_wrmsr(UK_ARCH_X86_64_APIC_MSR_EOI, 0, 0);
}

int uk_intctlr_xpic_msi_message(unsigned int irq, __u32 *address, __u32 *data)
{
	const __u32 x2apic = UK_ARCH_X86_64_APIC_BASE_EN |
			    UK_ARCH_X86_64_APIC_BASE_EXTD;
	__u32 local_id, base, high;

	if (irq < UK_INTCTLR_FIRST_ALLOCABLE_IRQ ||
	    irq > UK_INTCTLR_LAST_ALLOCABLE_IRQ)
		return -EINVAL;
	uk_arch_x86_64_rdmsr(UK_ARCH_X86_64_APIC_MSR_BASE, &base, &high);
	if ((base & x2apic) != x2apic)
		return -ENOTSUP;
	uk_arch_x86_64_rdmsr(UK_ARCH_X86_64_APIC_MSR_ID, &local_id, &high);
	if (local_id > 0xff)
		return -ENOTSUP;
	*address = APIC_MSI_BASE | (local_id << APIC_MSI_DEST_SHIFT);
	*data = irq + UK_INTCTLR_XPIC_VECTOR_BASE;
	return 0;
}

#define apic_ack_interrupt	x2apic_ack_interrupt
#define apic_enable		x2apic_enable
#endif /* CONFIG_LIBUKINTCTLR_APIC */

#include "pic.h"

static struct uk_intctlr_desc intctlr;

static int configure_irq(struct uk_intctlr_irq *irq __unused)
{
	return 0;
}

static int uk_intctlr_xpic_handle_irq(void *data)
{
	struct uk_lcpu_except_irq_ctx *ctx;
	__u32 irq;

	ctx = data;
	uk_intctlr_irq_handle(ctx);

	irq = uk_lcpu_except_irq_ctx_get_irq(ctx);

#if CONFIG_LIBUKINTCTLR_APIC
	apic_ack_interrupt();

	/* The 8259A only owns the legacy lines. */
	if (irq < UK_INTCTLR_XPIC_LEGACY_IRQ_COUNT)
		pic_ack_irq(irq);
#else   /* !CONFIG_LIBUKINTCTLR_APIC */
	pic_ack_irq(irq);
#endif /* !CONFIG_LIBUKINTCTLR_APIC */

	return UK_EVENT_HANDLED;
}

UK_EVENT_HANDLER(UK_LCPU_EXCEPT_EVENT_IRQ, uk_intctlr_xpic_handle_irq);

int uk_intctlr_probe(void)
{
	int rc = -ENODEV;
	struct uk_intctlr_driver_ops *ops;

	rc = pic_init(&ops);
	if (unlikely(rc))
		return rc;

#if CONFIG_LIBUKINTCTLR_APIC
	apic_enable();
	intctlr.name = "APIC";
#else /* ! CONFIG_LIBUKINTCTLR_APIC */
	intctlr.name = "PIC";
#endif /* CONFIG_LIBUKINTCTLR_APIC */

	intctlr.ops = ops;
	intctlr.ops->configure_irq = configure_irq;

	return uk_intctlr_register(&intctlr);
}
