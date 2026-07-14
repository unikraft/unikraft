/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PAL_EXCEPT_H__
#define __UK_PAL_EXCEPT_H__

/**
 * Exception and Interrupt Handling - Platform Abstraction Layer
 *
 * Provides exception (traps, faults) and interrupt (IRQ) management across
 * all platforms and architectures. Includes context access, IRQ control,
 * and nested exception support.
 */

#include <uk/arch/types.h>
#include <uk/pal/arch/except.h>

/*
 * Platform must define event identifiers for exception and interrupt
 * handlers. These are used with the event system to register callbacks.
 */

#ifndef UK_PAL_EXCEPT_EVENT_DEBUG
#error "UK_PAL_EXCEPT_EVENT_DEBUG undefined"
#endif

#ifndef UK_PAL_EXCEPT_EVENT_ERR_INVALID_OP
#error "UK_PAL_EXCEPT_EVENT_ERR_INVALID_OP undefined"
#endif

#ifndef UK_PAL_EXCEPT_EVENT_ERR_PAGE_FAULT
#error "UK_PAL_EXCEPT_EVENT_ERR_PAGE_FAULT undefined"
#endif

#ifndef UK_PAL_EXCEPT_EVENT_ERR_BUS_ERROR
#error "UK_PAL_EXCEPT_EVENT_ERR_BUS_ERROR undefined"
#endif

#ifndef UK_PAL_EXCEPT_EVENT_ERR_MATH
#error "UK_PAL_EXCEPT_EVENT_ERR_MATH undefined"
#endif

#ifndef UK_PAL_EXCEPT_EVENT_ERR_SECURITY
#error "UK_PAL_EXCEPT_EVENT_ERR_SECURITY undefined"
#endif

#ifndef UK_PAL_EXCEPT_EVENT_SYSCALL
#error "UK_PAL_EXCEPT_EVENT_SYSCALL undefined"
#endif

#ifndef UK_PAL_EXCEPT_EVENT_IRQ
#error "UK_PAL_EXCEPT_EVENT_IRQ undefined"
#endif

#ifndef UK_PAL_EXCEPT_EVENT_UNHANDLED
#error "UK_PAL_EXCEPT_EVENT_UNHANDLED undefined"
#endif

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Exception error context (opaque).
 * Contains CPU state and exception metadata passed to error handlers.
 * Access via accessor functions.
 */
struct uk_pal_except_err_ctx;

/* Exception error context accessors */

__isr const char *uk_pal_except_err_ctx_get_str(
	const struct uk_pal_except_err_ctx *ctx);

__isr void uk_pal_except_err_ctx_set_str(
	struct uk_pal_except_err_ctx *ctx,
	const char *str);

__isr int uk_pal_except_err_ctx_get_handler_err(
	const struct uk_pal_except_err_ctx *ctx);

__isr void uk_pal_except_err_ctx_set_handler_err(
	struct uk_pal_except_err_ctx *ctx,
	int handler_err);

__isr struct uk_pal_regs *uk_pal_except_err_ctx_get_regs(
	const struct uk_pal_except_err_ctx *ctx);

__isr void uk_pal_except_err_ctx_set_regs(
	struct uk_pal_except_err_ctx *ctx,
	struct uk_pal_regs *regs);

__isr __u64 uk_pal_except_err_ctx_get_fault_addr(
	const struct uk_pal_except_err_ctx *ctx);

__isr void uk_pal_except_err_ctx_set_fault_addr(
	struct uk_pal_except_err_ctx *ctx,
	__u64 fault_addr);

/**
 * Interrupt context (opaque).
 * Contains CPU state and interrupt metadata passed to IRQ handlers.
 * Access via accessor functions.
 */
struct uk_pal_except_irq_ctx;

/* Interrupt context accessors */

__isr struct uk_pal_regs *uk_pal_except_irq_ctx_get_regs(
	const struct uk_pal_except_irq_ctx *ctx);

__isr void uk_pal_except_irq_ctx_set_regs(
	struct uk_pal_except_irq_ctx *ctx,
	struct uk_pal_regs *regs);

__isr __u64 uk_pal_except_irq_ctx_get_irq(
	const struct uk_pal_except_irq_ctx *ctx);

__isr void uk_pal_except_irq_ctx_set_irq(
	struct uk_pal_except_irq_ctx *ctx,
	__u32 irq);

/* Nested exception support */

/**
 * Enable nested exception handling.
 * Allows exceptions to occur while handling an exception.
 */
__isr void uk_pal_except_push_nested(void);

/**
 * Disable nested exception handling.
 * Returns to non-nested exception mode.
 */
__isr void uk_pal_except_pop_nested(void);

/* Interrupt control */

/**
 * Disable interrupts.
 * Blocks maskable interrupts from being delivered.
 */
__isr void uk_pal_disable_irq(void);

/**
 * Enable interrupts.
 * Allows maskable interrupts to be delivered.
 */
__isr void uk_pal_enable_irq(void);

/**
 * Check if interrupts are currently disabled.
 *
 * @return Non-zero if interrupts are disabled, 0 otherwise
 */
__isr int uk_pal_irqs_disabled(void);

/**
 * Handle pending interrupts.
 * Process any interrupts that were deferred. May be no-op on some platforms.
 */
__isr void uk_pal_irqs_handle_pending(void);

/**
 * Restore interrupt state.
 * Restores interrupt enable/disable state from saved flags.
 *
 * @param flags  Flags previously returned by uk_pal_save_irqf()
 */
__isr void uk_pal_restore_irqf(unsigned long flags);

/**
 * Save current interrupt state and disable interrupts.
 * Returns state that can be restored with uk_pal_restore_irqf().
 *
 * @return Opaque interrupt state flags
 */
__isr unsigned long uk_pal_save_irqf(void);

/**
 * Get exception stack base address.
 *
 * @return Base of exception stack for current CPU
 */
__isr __uptr uk_pal_except_get_except_stack_base(void);

/**
 * Initialize exception handling.
 *
 * @return 0 on success, !=0 on error
 */
__isr int uk_pal_except_init(void);

#if CONFIG_HAVE_SMP
/**
 * Send an Interprocessor Interrupt.
 *
 * @param id The CPU ID to send the IPI to
 * @param irq The IRQ to send to the CPU
 * @return 0 on success, negative errno on failure
 */
int uk_pal_except_send_ipi(__u64 id, __u32 irq);
#endif /* CONFIG_HAVE_SMP */

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PAL_EXCEPT_H__ */
