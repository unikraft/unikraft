/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_NATIVE_ARCH_EXCEPT_H__
#define __UK_PLAT_NATIVE_ARCH_EXCEPT_H__

#include <uk/arch/types.h>
#include <uk/arch/util.h>
#include <uk/plat/native/arch/regs.h>
#include <uk/compiler.h>
#include <uk/pcpuvar.h>
#include <uk/config.h>

#if CONFIG_LIBUKPLAT_NATIVE_EXCEPT
#define UK_PLAT_NATIVE_EXCEPT_SWITCH_STACK_SYM				\
	uk_plat_native_except_switch_stack

#define UK_PLAT_NATIVE_EXCEPT_STACK_BASE_SYM				\
	uk_plat_native_except_stack_base
#endif /* CONFIG_LIBUKPLAT_NATIVE_EXCEPT */

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

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
#define	UK_PLAT_NATIVE_EXCEPT_EVENT_UNHANDLED				\
	native_except_unhandled_except

enum uk_plat_native_arm64_except_id {
	UK_PLAT_NATIVE_ARM64_EXCEPT_ID_INVALID_OP,
	UK_PLAT_NATIVE_ARM64_EXCEPT_ID_DEBUG,
	UK_PLAT_NATIVE_ARM64_EXCEPT_ID_PAGE_FAULT,
	UK_PLAT_NATIVE_ARM64_EXCEPT_ID_BUS_ERROR,
	UK_PLAT_NATIVE_ARM64_EXCEPT_ID_MATH,
	UK_PLAT_NATIVE_ARM64_EXCEPT_ID_SECURITY,
	UK_PLAT_NATIVE_ARM64_EXCEPT_ID_SYSCALL,
	UK_PLAT_NATIVE_ARM64_EXCEPT_ID_MAX
};

/**
 * Exception context, passed to event handlers.
 */
struct uk_plat_native_except_err_ctx {
	enum uk_plat_native_arm64_except_id eid;
	const char *str;
	__u64 esr;
	__u64 far;
	int handler_err; /* set by handler if unable to process exception */
	struct uk_plat_native_regs *regs;
};

__isr static inline enum uk_plat_native_arm64_except_id
uk_plat_native_arm64_except_err_ctx_get_eid(
	const struct uk_plat_native_except_err_ctx *ctx)
{
	return ctx->eid;
}

__isr static inline void
uk_plat_native_arm64_except_err_ctx_set_eid(
	struct uk_plat_native_except_err_ctx *ctx,
	enum uk_plat_native_arm64_except_id eid)
{
	ctx->eid = eid;
}

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

__isr static inline __u64
uk_plat_native_arm64_except_err_ctx_get_esr(
	const struct uk_plat_native_except_err_ctx *ctx)
{
	return ctx->esr;
}

__isr static inline void
uk_plat_native_arm64_except_err_ctx_set_esr(
	struct uk_plat_native_except_err_ctx *ctx,
	__u64 esr)
{
	ctx->esr = esr;
}

__isr static inline __u64
uk_plat_native_except_err_ctx_get_fault_addr(
	const struct uk_plat_native_except_err_ctx *ctx)
{
	return ctx->far;
}

__isr static inline void
uk_plat_native_except_err_ctx_set_fault_addr(
	struct uk_plat_native_except_err_ctx *ctx,
	__u64 fault_addr)
{
	ctx->far = fault_addr;
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

/**
 * Execution context of interrupted code.
 */
struct uk_plat_native_except_irq_ctx {
	/** The registers of the interrupted code */
	struct uk_plat_native_regs *regs;
	/** The platform specific interrupt number.
	 *  In the native platform this is filled
	 *  in by the GIC.
	 */
	__u32 irq;
};

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

__isr static inline __u32
uk_plat_native_except_irq_ctx_get_irq(
	const struct uk_plat_native_except_irq_ctx *ctx)
{
	return ctx->irq;
}

__isr static inline void
uk_plat_native_except_irq_ctx_set_irq(
	struct uk_plat_native_except_irq_ctx *ctx,
	__u32 irq)
{
	ctx->irq = irq;
}

#if CONFIG_LIBUKPLAT_NATIVE_EXCEPT
/**
 * Unmask interrupts
 */
__isr static inline
void uk_plat_native_enable_irq(void)
{
	__asm __volatile__("msr daifclr, #2" : : : "memory");
}

/**
 * Mask interrupts
 */
__isr static inline
void uk_plat_native_disable_irq(void)
{
	__asm __volatile__("msr daifset, #2" : : : "memory");
}

__isr static inline
unsigned long uk_plat_native_daif_get(void)
{
	unsigned long flags;

	__asm __volatile__("mrs	%x0, daif\n" : "=&r" (flags) ::);

	return flags;
}

/**
 * Save current IRQ state and disable interrupts.
 * Use with uk_plat_native_restore_irqf() for critical sections.
 *
 * @return flags
 */
__isr static inline
unsigned long uk_plat_native_save_irqf(void)
{
	unsigned long flags;

	flags = uk_plat_native_daif_get();
	uk_plat_native_disable_irq();

	return !(flags & UK_ARCH_ARM64_DAIF_I_BIT);
}

/**
 * Restore IRQ state from uk_plat_native_save_irqf().
 *
 * @param flags  Return value from uk_plat_native_save_irqf()
 */
__isr static inline
void uk_plat_native_restore_irqf(unsigned long flags)
{
	if (flags)
		uk_plat_native_enable_irq();
	else
		uk_plat_native_disable_irq();
}

/**
 * Check if interrupts are currently disabled.
 *
 * @return Non-zero if DAIF.I is set (IRQs disabled), 0 otherwise
 */
__isr static inline
int uk_plat_native_irqs_disabled(void)
{
	return (uk_plat_native_daif_get() & UK_ARCH_ARM64_DAIF_I_BIT);
}

#if CONFIG_HAVE_SMP
int uk_plat_native_except_send_ipi(__u64 id, __u32 irq);
#endif /* CONFIG_HAVE_SMP */

/**
 * Handle pending interrupts.
 */
__isr static inline
void uk_plat_native_irqs_handle_pending(void) { }

/* Nested exception context management */
extern __uk_pcpuvar __u8 uk_plat_native_except_switch_stack;
extern __uk_pcpuvar __uptr uk_plat_native_except_stack_base;

__isr __uptr uk_plat_native_except_get_except_stack_base(void);

static inline
void uk_plat_native_except_push_nested(void)
{
	uk_pcpuvar_current_set(
		uk_plat_native_except_switch_stack,
		uk_pcpuvar_current_get(uk_plat_native_except_switch_stack) + 1);
}

static inline
void uk_plat_native_except_pop_nested(void)
{
	uk_pcpuvar_current_set(
		uk_plat_native_except_switch_stack,
		uk_pcpuvar_current_get(uk_plat_native_except_switch_stack) - 1);
}

/**
 * Initialize exception handling for the current CPU.
 */
__isr int uk_plat_native_except_init(void);
#endif /* CONFIG_LIBUKPLAT_NATIVE_EXCEPT */

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PLAT_NATIVE_ARCH_EXCEPT_H__ */
