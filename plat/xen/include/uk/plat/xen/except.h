/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_XEN_EXCEPT_H__
#define __UK_PLAT_XEN_EXCEPT_H__

/**
 * Exception and Interrupt Handling - Architecture-Independent Interface
 *
 * Provides exception (traps, faults) and interrupt (IRQ) handling facilities
 * including context management, IRQ control, and nested exception support.
 * Architecture-specific implementations define event identifiers and handlers.
 */

#include <uk/plat/xen/arch/except.h>

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Verify that architecture-specific header defined required event identifiers.
 * These are used with the uk_event system to register exception/interrupt
 * handlers.
 */

#ifndef UK_PLAT_XEN_EXCEPT_EVENT_DEBUG
#error "UK_PLAT_XEN_EXCEPT_EVENT_DEBUG undefined"
#endif

#ifndef UK_PLAT_XEN_EXCEPT_EVENT_ERR_INVALID_OP
#error "UK_PLAT_XEN_EXCEPT_EVENT_ERR_INVALID_OP undefined"
#endif

#ifndef UK_PLAT_XEN_EXCEPT_EVENT_ERR_PAGE_FAULT
#error "UK_PLAT_XEN_EXCEPT_EVENT_ERR_PAGE_FAULT undefined"
#endif

#ifndef UK_PLAT_XEN_EXCEPT_EVENT_ERR_BUS_ERROR
#error "UK_PLAT_XEN_EXCEPT_EVENT_ERR_BUS_ERROR undefined"
#endif

#ifndef UK_PLAT_XEN_EXCEPT_EVENT_ERR_MATH
#error "UK_PLAT_XEN_EXCEPT_EVENT_ERR_MATH undefined"
#endif

#ifndef UK_PLAT_XEN_EXCEPT_EVENT_ERR_SECURITY
#error "UK_PLAT_XEN_EXCEPT_EVENT_ERR_SECURITY undefined"
#endif

#ifndef UK_PLAT_XEN_EXCEPT_EVENT_SYSCALL
#error "UK_PLAT_XEN_EXCEPT_EVENT_SYSCALL undefined"
#endif

#ifndef UK_PLAT_XEN_EXCEPT_EVENT_IRQ
#error "UK_PLAT_XEN_EXCEPT_EVENT_IRQ undefined"
#endif

#ifndef UK_PLAT_XEN_EXCEPT_EVENT_UNHANDLED
#error "UK_PLAT_XEN_EXCEPT_EVENT_UNHANDLED undefined"
#endif

/* Exception error context accessors */

const char *uk_plat_xen_except_err_ctx_get_str(
	const struct uk_plat_xen_except_err_ctx *ctx);

void uk_plat_xen_except_err_ctx_set_str(
	struct uk_plat_xen_except_err_ctx *ctx,
	const char *str);

int uk_plat_xen_except_err_ctx_get_handler_err(
	const struct uk_plat_xen_except_err_ctx *ctx);

void uk_plat_xen_except_err_ctx_set_handler_err(
	struct uk_plat_xen_except_err_ctx *ctx,
	int handler_err);

struct uk_plat_native_regs *uk_plat_xen_except_err_ctx_get_regs(
	const struct uk_plat_xen_except_err_ctx *ctx);

void uk_plat_xen_except_err_ctx_set_regs(
	struct uk_plat_xen_except_err_ctx *ctx,
	struct uk_plat_native_regs *regs);

__u64 uk_plat_xen_except_err_ctx_get_fault_addr(
	const struct uk_plat_xen_except_err_ctx *ctx);

void uk_plat_xen_except_err_ctx_set_fault_addr(
	struct uk_plat_xen_except_err_ctx *ctx,
	__u64 fault_addr);

/* IRQ context accessors */

struct uk_plat_native_regs *uk_plat_xen_except_irq_ctx_get_regs(
	const struct uk_plat_xen_except_irq_ctx *ctx);

void uk_plat_xen_except_irq_ctx_set_regs(
	struct uk_plat_xen_except_irq_ctx *ctx,
	struct uk_plat_native_regs *regs);

__u64 uk_plat_xen_except_irq_ctx_get_irq(
	const struct uk_plat_xen_except_irq_ctx *ctx);

void uk_plat_xen_except_irq_ctx_set_irq(
	struct uk_plat_xen_except_irq_ctx *ctx,
	__u32 irq);

/* IRQ control */

void uk_plat_xen_enable_irq(void);
void uk_plat_xen_disable_irq(void);
unsigned long uk_plat_xen_save_irqf(void);
void uk_plat_xen_restore_irqf(unsigned long flags);
int uk_plat_xen_irqs_disabled(void);

/* CPU halt operations */

void uk_plat_xen_halt(void);
void uk_plat_xen_halt_irq(void);

/* Platform-specific IRQ handling */

void uk_plat_xen_irqs_handle_pending(void);

/* Auxiliary stack management for exception handling */

static inline __uptr uk_plat_xen_except_get_except_stack_base(void)
{
	/* no exception stack */
	return 0;
}

/* Nested exception support */

static inline void uk_plat_xen_except_push_nested(void)
{
	/* no exception stack */
}

static inline void uk_plat_xen_except_pop_nested(void)
{
	/* no exception stack */
}

static inline int uk_plat_xen_except_init(void)
{
	/* Stub */
	return 0;
}

#endif /* !__ASSEMBLY__ */

#endif /* __UK_PLAT_XEN_EXCEPT_H__ */
