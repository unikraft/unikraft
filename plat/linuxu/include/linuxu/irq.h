/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_LINUXU_IRQ_H__
#define __UK_PLAT_LINUXU_IRQ_H__

#include <linuxu/signal.h>
#include <uk/alloc.h>
#include <uk/plat/lcpu.h>

#define __MAX_IRQ 16

typedef int (*irq_handler_func_t)(void *);

struct ukplat_event_irq_data {
	struct __regs *regs;
	unsigned int irq;
};

/* IRQ handlers declarations */
struct irq_handler {
	/* func() special values:
	 *   NULL: free,
	 *   HANDLER_RESERVED: reserved
	 */
	irq_handler_func_t func;
	void *arg;

	struct uk_sigaction oldaction;
};

int ukplat_irq_register(unsigned long irq, irq_handler_func_t func, void *arg);

int ukplat_irq_init(void);

#endif /* __UK_PLAT_LINUXU_IRQ_H__ */
