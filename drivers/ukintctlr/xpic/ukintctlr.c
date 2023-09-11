/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#include <errno.h>
#include <uk/assert.h>
#include <uk/config.h>
#include <uk/intctlr.h>

#if CONFIG_LIBUKINTCTLR_APIC
#include <uk/intctlr/apic.h>
#endif /* CONFIG_LIBUKINTCTLR_APIC */

#include "pic.h"

static struct uk_intctlr_desc intctlr;

static int configure_irq(struct uk_intctlr_irq *irq __unused)
{
	return 0;
}

void uk_intctlr_xpic_handle_irq(struct __regs *regs, unsigned int irq)
{
	uk_intctlr_irq_handle(regs, irq);

#if CONFIG_LIBUKINTCTLR_APIC
	apic_ack_interrupt();

	/* FIXME This is here because right now we only use
	 * APIC for IPIs on SMP. This should be removed as
	 * soon as we fully implement APIC and get rid of
	 * PIC
	 */
	if (irq <= 16)
		pic_ack_irq(irq);
#else   /* !CONFIG_LIBUKINTCTLR_APIC */
	pic_ack_irq(irq);
#endif /* !CONFIG_LIBUKINTCTLR_APIC */
}

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
