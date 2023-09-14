/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_INTCTLR_H__
#define __UK_INTCTLR_H__

#ifdef __cplusplus
export "C" {
#endif

#ifndef __ASSEMBLY__

#include <uk/alloc.h>
#include <uk/asm/irq.h>
#include <uk/essentials.h>
#include <uk/plat/lcpu.h>

/**
 * This event is raised before the platform code handles an IRQ. The normal
 * IRQ handling will continue or stop according to the returned `UK_EVENT_*`
 * value.
 * Note: this event is usually raised in an interrupt context.
 */
#define UK_INTCTLR_EVENT_IRQ uk_intctlr_event_irq

/** The event payload for the #UK_INTCTLR_EVENT_IRQ event */
struct uk_intctlr_event_irq_data {
	/** The registers of the interrupted code */
	struct __regs *regs;
	/** The platform specific interrupt vector number */
	unsigned long irq;
};

enum uk_intctlr_irq_trigger {
	UK_INTCTLR_IRQ_TRIGGER_NONE, /* interpreted as "do not change" */
	UK_INTCTLR_IRQ_TRIGGER_EDGE,
	UK_INTCTLR_IRQ_TRIGGER_LEVEL,
};

/** IRQ descriptor */
struct uk_intctlr_irq {
	unsigned int id;
	unsigned int trigger;
};

/**
 * Interrupt controller driver ops
 *
 * These must be implemented by the interrupt controller
 */
struct uk_intctlr_driver_ops {
	int (*configure_irq)(struct uk_intctlr_irq *irq);
	int (*fdt_xlat)(const void *fdt, int nodeoffset, __u32 index,
			struct uk_intctlr_irq *irq);
	void (*mask_irq)(unsigned int irq);
	void (*unmask_irq)(unsigned int irq);
};

/** Interrupt controller descriptor */
struct uk_intctlr_desc {
	char *name;
	struct uk_intctlr_driver_ops *ops;
};

/** Interrupt handler function */
typedef int (*uk_intctlr_irq_handler_func_t)(void *);

/**
 * Probe the interrupt controller
 *
 * This function must be implemented by the driver
 *
 * @return zero on success or negative value on error
 */
int uk_intctlr_probe(void);

/**
 * Handle an interrupt
 *
 * This function provides a unified interrupt handling implementation.
 * Must be called by the driver's interrupt handling routine.
 *
 * @param regs Register context at the time the interrupt was raised
 * @param irq  Interrupt to handle
 * @return zero on success or negative value on error
 */
void uk_intctlr_irq_handle(struct __regs *regs, unsigned int irq);

/**
 * Configure an interrupt
 *
 * @param irq Interrupt configuration
 * @return zero on success or negative value on error
 */
int uk_intctlr_irq_configure(struct uk_intctlr_irq *irq);

/**
 * Register interrupt controller driver with the uk_intctlr subsystem
 *
 * This function must be called by the driver during probe
 *
 * @param intctlr populated interrupt controller descriptor
 * @return zero on success or negative value on error
 */
int uk_intctlr_register(struct uk_intctlr_desc *intctlr);

/**
 * Initialize the uk_intctlr subsystem
 *
 * Must be called after probing the device via uk_intctlr_probe
 *
 * @param alloc The allocator to use for internal allocations
 * @return zero on success, negative value on failure
 */
int uk_intctlr_init(struct uk_alloc *alloc);

/**
 * Register an interrupt handler
 *
 * @param irq     Interrupt to register handler for
 * @param handler Handler function
 * @param arg     Caller data to be passed to the handler
 */
int uk_intctlr_irq_register(unsigned int irq,
			    uk_intctlr_irq_handler_func_t handler,
			    void *arg);

/**
 * Unregister a previously registered interrupt handler
 *
 * @param irq     Interrupt to register handler for
 * @param handler Handler function
 */
int uk_intctlr_irq_unregister(unsigned int irq,
			      uk_intctlr_irq_handler_func_t handler);

/**
 *  Mask an interrupt
 *
 *  @param irq Interrupt to mask
 */
void uk_intctlr_irq_mask(unsigned int irq);

/**
 *  Unmask an interrupt
 *
 *  @param irq Interrupt to unmask
 */
void uk_intctlr_irq_unmask(unsigned int irq);

/**
 * Allocate IRQs from available pool
 *
 * @param irqs pointer to array of irqs
 * @param sz   number of array elements
 * @return zero on success, or negative value on error
 */
int uk_intctlr_irq_alloc(unsigned int *irqs, __sz count);

/**
 * Free previously allocated IRQs
 *
 * @param irqs pointer to array of irqs
 * @param sz   number of array elements
 * @return zero on success, or negative value on error
 */
int uk_intctlr_irq_free(unsigned int *irqs, __sz count);

/**
 * Translate from `interrupts` fdt node to IRQ descriptor
 *
 * This function is only available for devices that are discoverable
 * via fdt
 *
 * @param fdt pointer to the device tree blob
 * @param nodeoffset offset of `interrupts` node to parse in the fdt
 * @param index the index of the interrupt to retrieve within the node
 * @param irq interrupt descriptor to populate
 * @return zero on success or libfdt error on failure
 */
int uk_intctlr_irq_fdt_xlat(const void *fdt, int nodeoffset, __u32 index,
			    struct uk_intctlr_irq *irq);

#endif /* __ASSEMBLY__ */

#ifdef __cplusplus
}
#endif

#endif /* __UK_INTCTLR_H__ */
