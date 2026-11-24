/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_INTCTLR_XPIC_LIMITS_H__
#define __UK_INTCTLR_XPIC_LIMITS_H__

#define UK_INTCTLR_XPIC_LEGACY_IRQ_COUNT			16
#define UK_INTCTLR_XPIC_VECTOR_BASE			32
#define UK_INTCTLR_FIRST_ALLOCABLE_IRQ			\
	UK_INTCTLR_XPIC_LEGACY_IRQ_COUNT
#define UK_INTCTLR_MAX_IRQ				\
	(256 - UK_INTCTLR_XPIC_VECTOR_BASE)
/* The local APIC uses vector 0xff for spurious interrupts. */
#define UK_INTCTLR_LAST_ALLOCABLE_IRQ			(UK_INTCTLR_MAX_IRQ - 2)

#define UK_INTCTLR_ALLOCABLE_IRQ_COUNT			\
	(UK_INTCTLR_LAST_ALLOCABLE_IRQ - UK_INTCTLR_FIRST_ALLOCABLE_IRQ + 1)

#endif /* __UK_INTCTLR_XPIC_LIMITS_H__ */
