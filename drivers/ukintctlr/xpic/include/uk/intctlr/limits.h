/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_INTCTLR_XPIC_LIMITS_H__
#define __UK_INTCTLR_XPIC_LIMITS_H__

#define UK_INTCTLR_FIRST_ALLOCABLE_IRQ			16
#define UK_INTCTLR_LAST_ALLOCABLE_IRQ			224
#define UK_INTCTLR_MAX_IRQ				(256 - 32)

#define UK_INTCTLR_ALLOCABLE_IRQ_COUNT			\
	(UK_INTCTLR_LAST_ALLOCABLE_IRQ - UK_INTCTLR_FIRST_ALLOCABLE_IRQ)

#endif /* __UK_INTCTLR_XPIC_LIMITS_H__ */
