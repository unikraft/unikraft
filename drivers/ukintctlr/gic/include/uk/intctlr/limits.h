/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_INTCTLR_GIC_LIMITS__H__
#define __UK_INTCTLR_GIC_LIMITS__H__

/**
 * Max usable INTID for GIC. INTIDs 1020 - 1023 are reseverd.
 */
#define UK_INTCTLR_MAX_IRQ			1019

/**
 * INTIDs that can be allocated via the uk_intctlr subsystem.
 *
 * On GIC we assign the range of SPIs.
 */
#define UK_INTCTLR_FIRST_ALLOCABLE_IRQ		32
#define UK_INTCTLR_LAST_ALLOCABLE_IRQ		1019
#define UK_INTCTLR_ALLOCABLE_IRQ_COUNT		\
	(UK_INTCTLR_LAST_ALLOCABLE_IRQ - UK_INTCTLR_FIRST_ALLOCABLE_IRQ)

#endif /* __UK_INTCTLR_LIMITS_H__ */
