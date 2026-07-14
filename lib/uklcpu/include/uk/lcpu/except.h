/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_LCPU_EXCEPT_H__
#define __UK_LCPU_EXCEPT_H__

#include <uk/arch/types.h>
#include <uk/plat/pal/except.h>
#include <uk/pal/except.h>
#include <uk/lcpu/arch/except.h>

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

#define UK_LCPU_EXCEPT_EVENT_DEBUG					\
	UK_PAL_EXCEPT_EVENT_DEBUG
#define UK_LCPU_EXCEPT_EVENT_ERR_INVALID_OP				\
	UK_PAL_EXCEPT_EVENT_ERR_INVALID_OP
#define UK_LCPU_EXCEPT_EVENT_ERR_PAGE_FAULT				\
	UK_PAL_EXCEPT_EVENT_ERR_PAGE_FAULT
#define UK_LCPU_EXCEPT_EVENT_ERR_BUS_ERROR				\
	UK_PAL_EXCEPT_EVENT_ERR_BUS_ERROR
#define UK_LCPU_EXCEPT_EVENT_ERR_MATH					\
	UK_PAL_EXCEPT_EVENT_ERR_MATH
#define UK_LCPU_EXCEPT_EVENT_ERR_SECURITY				\
	UK_PAL_EXCEPT_EVENT_ERR_SECURITY
#define UK_LCPU_EXCEPT_EVENT_SYSCALL					\
	UK_PAL_EXCEPT_EVENT_SYSCALL
#define UK_LCPU_EXCEPT_EVENT_IRQ					\
	UK_PAL_EXCEPT_EVENT_IRQ
#define UK_LCPU_EXCEPT_EVENT_UNHANDLED				\
	UK_PAL_EXCEPT_EVENT_UNHANDLED

struct uk_lcpu_except_err_ctx;

__isr static inline const char *
uk_lcpu_except_err_ctx_get_str(const struct uk_lcpu_except_err_ctx *ctx)
{
	return uk_pal_except_err_ctx_get_str(
		(const struct uk_pal_except_err_ctx *)ctx);
}

__isr static inline void
uk_lcpu_except_err_ctx_set_str(struct uk_lcpu_except_err_ctx *ctx,
			       const char *str)
{
	uk_pal_except_err_ctx_set_str(
		(struct uk_pal_except_err_ctx *)ctx, str);
}

__isr static inline int
uk_lcpu_except_err_ctx_get_handler_err(const struct uk_lcpu_except_err_ctx *ctx)
{
	return uk_pal_except_err_ctx_get_handler_err(
		(const struct uk_pal_except_err_ctx *)ctx);
}

__isr static inline void
uk_lcpu_except_err_ctx_set_handler_err(struct uk_lcpu_except_err_ctx *ctx,
				       int handler_err)
{
	uk_pal_except_err_ctx_set_handler_err(
		(struct uk_pal_except_err_ctx *)ctx, handler_err);
}

__isr static inline __u64
uk_lcpu_except_err_ctx_get_fault_addr(const struct uk_lcpu_except_err_ctx *ctx)
{
	return uk_pal_except_err_ctx_get_fault_addr(
		(const struct uk_pal_except_err_ctx *)ctx);
}

__isr static inline void
uk_lcpu_except_err_ctx_set_fault_addr(struct uk_lcpu_except_err_ctx *ctx,
				      __u64 fault_addr)
{
	uk_pal_except_err_ctx_set_fault_addr(
		(struct uk_pal_except_err_ctx *)ctx, fault_addr);
}

__isr static inline struct uk_lcpu_regs *
uk_lcpu_except_err_ctx_get_regs(const struct uk_lcpu_except_err_ctx *ctx)
{
	return (struct uk_lcpu_regs *)uk_pal_except_err_ctx_get_regs(
		(const struct uk_pal_except_err_ctx *)ctx);
}

__isr static inline void
uk_lcpu_except_err_ctx_set_regs(struct uk_lcpu_except_err_ctx *ctx,
				struct uk_lcpu_regs *regs)
{
	uk_pal_except_err_ctx_set_regs(
		(struct uk_pal_except_err_ctx *)ctx,
		(struct uk_pal_regs *)regs);
}

struct uk_lcpu_except_irq_ctx;

__isr static inline struct uk_lcpu_regs *
uk_lcpu_except_irq_ctx_get_regs(const struct uk_lcpu_except_irq_ctx *ctx)
{
	return (struct uk_lcpu_regs *)uk_pal_except_irq_ctx_get_regs(
		(const struct uk_pal_except_irq_ctx *)ctx);
}

__isr static inline void
uk_lcpu_except_irq_ctx_set_regs(struct uk_lcpu_except_irq_ctx *ctx,
				struct uk_lcpu_regs *regs)
{
	uk_pal_except_irq_ctx_set_regs(
		(struct uk_pal_except_irq_ctx *)ctx,
		(struct uk_pal_regs *)regs);
}

__isr static inline __u64
uk_lcpu_except_irq_ctx_get_irq(const struct uk_lcpu_except_irq_ctx *ctx)
{
	return uk_pal_except_irq_ctx_get_irq(
		(const struct uk_pal_except_irq_ctx *)ctx);
}

__isr static inline void
uk_lcpu_except_irq_ctx_set_irq(struct uk_lcpu_except_irq_ctx *ctx,
			       __u32 irq)
{
	uk_pal_except_irq_ctx_set_irq((struct uk_pal_except_irq_ctx *)ctx, irq);
}

__isr static inline void uk_lcpu_enable_irq(void)
{
	uk_pal_enable_irq();
}

__isr static inline void uk_lcpu_disable_irq(void)
{
	uk_pal_disable_irq();
}

__isr static inline unsigned long uk_lcpu_save_irqf(void)
{
	return uk_pal_save_irqf();
}

__isr static inline void uk_lcpu_restore_irqf(unsigned long flags)
{
	uk_pal_restore_irqf(flags);
}

__isr static inline int uk_lcpu_irqs_disabled(void)
{
	return uk_pal_irqs_disabled();
}

static inline void uk_lcpu_irqs_handle_pending(void)
{
	uk_pal_irqs_handle_pending();
}

__isr static inline void uk_lcpu_except_push_nested(void)
{
	uk_pal_except_push_nested();
}

__isr static inline void uk_lcpu_except_pop_nested(void)
{
	uk_pal_except_pop_nested();
}

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_LCPU_EXCEPT_H__ */
