/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_NATIVE_ARCH_EXCEPT_H__
#define __UK_PLAT_NATIVE_ARCH_EXCEPT_H__

#include <uk/arch/types.h>
#include <uk/arch/util.h>
#include <uk/config.h>
#include <uk/plat/native/arch/regs.h>
#include <uk/lcpu/core.h>

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

/* Exception event identifiers for uk_event handlers */
#define UK_PLAT_NATIVE_X86_64_EXCEPT_EVENT_ERR_GP_FAULT			\
	native_x86_64_except_event_err_gp_fault
#define UK_PLAT_NATIVE_X86_64_EXCEPT_EVENT_NMI				\
	native_x86_64_except_event_err_nmi
#define UK_PLAT_NATIVE_EXCEPT_EVENT_DEBUG				\
	native_except_event_debug
#define UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_INVALID_OP			\
	native_except_event_err_invalid_op
#define UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_PAGE_FAULT			\
	native_except_event_err_page_fault
#define UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_BUS_ERROR			\
	native_except_event_err_bus_error
#define UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_MATH				\
	native_except_event_err_math
#define UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_SECURITY			\
	native_except_event_err_security
#define UK_PLAT_NATIVE_EXCEPT_EVENT_SYSCALL				\
	native_except_event_syscall
#define UK_PLAT_NATIVE_EXCEPT_EVENT_IRQ					\
	native_except_event_irq
#define	UK_PLAT_NATIVE_EXCEPT_EVENT_UNHANDLED			\
	native_except_unhandled_except

/**
 * x86_64 exception context passed to error event handlers.
 * Contains CPU state at exception time plus exception-specific metadata.
 */
struct uk_plat_native_except_err_ctx {
	/* Exception description string */
	const char *str;
	/* x86 exception vector number (0-31) */
	__u32 trapnr;
	/* Hardware error code (only valid for #PF, #GP, #TS, etc.) */
	int error_code;
	/* Set by handler if exception unrecoverable */
	int handler_err;
	/* CPU registers at exception time */
	struct uk_plat_native_regs *regs;
	/* Faulting virtual address (page faults only, from CR2) */
	__u64 cr2;
};

/* Exception context accessors */
__isr static inline const char *
uk_plat_native_except_err_ctx_get_str(
	const struct uk_plat_native_except_err_ctx *ctx)
{
	return ctx->str;
}

__isr static inline void
uk_plat_native_except_err_ctx_set_str(
	struct uk_plat_native_except_err_ctx *ctx,
	const char *str)
{
	ctx->str = str;
}

__isr static inline __u32
uk_plat_native_x86_64_except_err_ctx_get_trapnr(
	const struct uk_plat_native_except_err_ctx *ctx)
{
	return ctx->trapnr;
}

__isr static inline void
uk_plat_native_x86_64_except_err_ctx_set_trapnr(
	struct uk_plat_native_except_err_ctx *ctx,
	__u32 trapnr)
{
	ctx->trapnr = trapnr;
}

__isr static inline int
uk_plat_native_x86_64_except_err_ctx_get_error_code(
	const struct uk_plat_native_except_err_ctx *ctx)
{
	return ctx->error_code;
}

__isr static inline void
uk_plat_native_x86_64_except_err_ctx_set_error_code(
	struct uk_plat_native_except_err_ctx *ctx,
	int error_code)
{
	ctx->error_code = error_code;
}

__isr static inline int
uk_plat_native_except_err_ctx_get_handler_err(
	const struct uk_plat_native_except_err_ctx *ctx)
{
	return ctx->handler_err;
}

__isr static inline void
uk_plat_native_except_err_ctx_set_handler_err(
	struct uk_plat_native_except_err_ctx *ctx,
	int handler_err)
{
	ctx->handler_err = handler_err;
}

__isr static inline struct uk_plat_native_regs *
uk_plat_native_except_err_ctx_get_regs(
	const struct uk_plat_native_except_err_ctx *ctx)
{
	return ctx->regs;
}

__isr static inline void
uk_plat_native_except_err_ctx_set_regs(
	struct uk_plat_native_except_err_ctx *ctx,
	struct uk_plat_native_regs *regs)
{
	ctx->regs = regs;
}

__isr static inline __u64
uk_plat_native_except_err_ctx_get_fault_addr(
	const struct uk_plat_native_except_err_ctx *ctx)
{
	return ctx->cr2;
}

__isr static inline void
uk_plat_native_except_err_ctx_set_fault_addr(
	struct uk_plat_native_except_err_ctx *ctx,
	__u64 fault_addr)
{
	ctx->cr2 = fault_addr;
}

/**
 * x86_64 interrupt context passed to IRQ event handlers.
 * Contains CPU state at interrupt time plus interrupt vector number.
 */
struct uk_plat_native_except_irq_ctx {
	/* CPU registers at IRQ time */
	struct uk_plat_native_regs *regs;
	/* x86 interrupt vector number */
	__u64 irq;
};

/* IRQ context accessors */
__isr static inline struct uk_plat_native_regs *
uk_plat_native_except_irq_ctx_get_regs(
	const struct uk_plat_native_except_irq_ctx *ctx)
{
	return ctx->regs;
}

__isr static inline void
uk_plat_native_except_irq_ctx_set_regs(
	struct uk_plat_native_except_irq_ctx *ctx,
	struct uk_plat_native_regs *regs)
{
	ctx->regs = regs;
}

__isr static inline __u64
uk_plat_native_except_irq_ctx_get_irq(
	const struct uk_plat_native_except_irq_ctx *ctx)
{
	return ctx->irq;
}

__isr static inline void
uk_plat_native_except_irq_ctx_set_irq(
	struct uk_plat_native_except_irq_ctx *ctx,
	__u64 irq)
{
	ctx->irq = irq;
}

#if CONFIG_LIBUKPLAT_NATIVE_EXCEPT
/**
 * Enable x86_64 interrupts (STI instruction).
 * Sets RFLAGS.IF, allowing maskable hardware interrupts.
 */
__isr static inline void uk_plat_native_enable_irq(void)
{
	uk_arch_enable_irq();
}

/**
 * Disable x86_64 interrupts (CLI instruction).
 * Clears RFLAGS.IF, blocking maskable hardware interrupts.
 * NMI and exceptions still occur.
 */
__isr static inline void uk_plat_native_disable_irq(void)
{
	uk_arch_disable_irq();
}

/**
 * Save current IRQ state and disable interrupts.
 * Returns 1 if interrupts were enabled, 0 if already disabled.
 * Use with uk_plat_native_restore_irqf() for critical sections.
 *
 * @return 1 if RFLAGS.IF was set, 0 otherwise
 */
__isr static inline unsigned long uk_plat_native_save_irqf(void)
{
	unsigned long flags;

	flags = uk_arch_rflags_get();
	uk_arch_disable_irq();

	return !!(flags & UK_ARCH_EFLAGS_IF);
}

/**
 * Restore IRQ state from uk_plat_native_save_irqf().
 * Re-enables interrupts if flags=1, keeps disabled if flags=0.
 *
 * @param flags  Return value from uk_plat_native_save_irqf()
 */
__isr static inline void uk_plat_native_restore_irqf(unsigned long flags)
{
	if (flags)
		uk_arch_enable_irq();
	else
		uk_arch_disable_irq();
}

/**
 * Check if x86_64 interrupts are currently disabled.
 *
 * @return Non-zero if RFLAGS.IF is clear (IRQs disabled), 0 otherwise
 */
__isr static inline int uk_plat_native_irqs_disabled(void)
{
	return !(uk_arch_rflags_get() & UK_ARCH_EFLAGS_IF);
}

/**
 * Handle pending interrupts (no-op on x86_64).
 * x86_64 delivers pending interrupts automatically when RFLAGS.IF is set.
 */
static inline void uk_plat_native_irqs_handle_pending(void) { }

/**
 * Get base of exception stack for current CPU.
 * ISR-safe, can be called from exception handlers.
 *
 * @return Base address of exception stack
 */
__isr __uptr uk_plat_native_except_get_except_stack_base(void);

/**
 * Enable nested exception handling.
 * Allows exceptions to be taken while handling an exception.
 * Must be called with interrupts disabled.
 */
__isr void uk_plat_native_except_push_nested(void);

/**
 * Disable nested exception handling.
 * Returns to non-nested exception mode.
 * Must be called with interrupts disabled.
 */
__isr void uk_plat_native_except_pop_nested(void);
#endif /* CONFIG_LIBUKPLAT_NATIVE_EXCEPT */

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PLAT_NATIVE_ARCH_EXCEPT_H__ */
