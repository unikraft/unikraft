/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef __UK_INTCTLR_XPIC_MSI_H__
#define __UK_INTCTLR_XPIC_MSI_H__

#include <uk/arch/types.h>

/* Compose a physical edge-triggered local APIC message for an allocated IRQ. */
int uk_intctlr_xpic_msi_message(unsigned int irq, __u32 *address, __u32 *data);

#endif
