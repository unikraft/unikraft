/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_PAL_EXCEPT_H__
#define __UK_PLAT_PAL_EXCEPT_H__

#include <uk/plat/xen/except.h>
#include <uk/plat/pal/arch/except.h>

#define UK_PAL_EXCEPT_EVENT_DEBUG					\
	UK_PLAT_XEN_EXCEPT_EVENT_DEBUG
#define UK_PAL_EXCEPT_EVENT_ERR_INVALID_OP				\
	UK_PLAT_XEN_EXCEPT_EVENT_ERR_INVALID_OP
#define UK_PAL_EXCEPT_EVENT_ERR_PAGE_FAULT				\
	UK_PLAT_XEN_EXCEPT_EVENT_ERR_PAGE_FAULT
#define UK_PAL_EXCEPT_EVENT_ERR_BUS_ERROR				\
	UK_PLAT_XEN_EXCEPT_EVENT_ERR_BUS_ERROR
#define UK_PAL_EXCEPT_EVENT_ERR_MATH					\
	UK_PLAT_XEN_EXCEPT_EVENT_ERR_MATH
#define UK_PAL_EXCEPT_EVENT_ERR_SECURITY				\
	UK_PLAT_XEN_EXCEPT_EVENT_ERR_SECURITY
#define UK_PAL_EXCEPT_EVENT_SYSCALL					\
	UK_PLAT_XEN_EXCEPT_EVENT_SYSCALL
#define UK_PAL_EXCEPT_EVENT_IRQ						\
	UK_PLAT_XEN_EXCEPT_EVENT_IRQ
#define UK_PAL_EXCEPT_EVENT_UNHANDLED					\
	UK_PLAT_XEN_EXCEPT_EVENT_UNHANDLED

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

/* Exception and IRQ context getters and setters are identical to native */

struct uk_pal_except_err_ctx;

static inline
const char *uk_pal_except_err_ctx_get_str(
	const struct uk_pal_except_err_ctx *ctx)
{
	return uk_plat_xen_except_err_ctx_get_str(
		(const struct uk_plat_xen_except_err_ctx *)ctx);
}

static inline
void uk_pal_except_err_ctx_set_str(struct uk_pal_except_err_ctx *ctx,
				   const char *str)
{
	uk_plat_xen_except_err_ctx_set_str(
		(struct uk_plat_xen_except_err_ctx *)ctx, str);
}

static inline
int uk_pal_except_err_ctx_get_handler_err(
	const struct uk_pal_except_err_ctx *ctx)
{
	return uk_plat_xen_except_err_ctx_get_handler_err(
		(const struct uk_plat_xen_except_err_ctx *)ctx);
}

static inline
void uk_pal_except_err_ctx_set_handler_err(struct uk_pal_except_err_ctx *ctx,
					   int handler_err)
{
	uk_plat_xen_except_err_ctx_set_handler_err(
		(struct uk_plat_xen_except_err_ctx *)ctx, handler_err);
}

static inline
struct uk_pal_regs *uk_pal_except_err_ctx_get_regs(
	const struct uk_pal_except_err_ctx *ctx)
{
	return (struct uk_pal_regs *)uk_plat_xen_except_err_ctx_get_regs(
		(const struct uk_plat_xen_except_err_ctx *)ctx);
}

static inline
void uk_pal_except_err_ctx_set_regs(struct uk_pal_except_err_ctx *ctx,
				    struct uk_pal_regs *regs)
{
	uk_plat_xen_except_err_ctx_set_regs(
		(struct uk_plat_xen_except_err_ctx *)ctx,
		(struct uk_plat_native_regs *)regs);
}

static inline
__u64 uk_pal_except_err_ctx_get_fault_addr(
	const struct uk_pal_except_err_ctx *ctx)
{
	return uk_plat_xen_except_err_ctx_get_fault_addr(
		(const struct uk_plat_xen_except_err_ctx *)ctx);
}

static inline
void uk_pal_except_err_ctx_set_fault_addr(struct uk_pal_except_err_ctx *ctx,
					  __u64 fault_addr)
{
	uk_plat_xen_except_err_ctx_set_fault_addr(
		(struct uk_plat_xen_except_err_ctx *)ctx, fault_addr);
}

struct uk_pal_except_irq_ctx;

static inline
struct uk_pal_regs *uk_pal_except_irq_ctx_get_regs(
	const struct uk_pal_except_irq_ctx *ctx)
{
	return (struct uk_pal_regs *)uk_plat_xen_except_irq_ctx_get_regs(
		(const struct uk_plat_xen_except_irq_ctx *)ctx);
}

static inline
void uk_pal_except_irq_ctx_set_regs(struct uk_pal_except_irq_ctx *ctx,
				    struct uk_pal_regs *regs)
{
	uk_plat_xen_except_irq_ctx_set_regs(
		(struct uk_plat_xen_except_irq_ctx *)ctx,
		(struct uk_plat_native_regs *)regs);
}

static inline
__u64 uk_pal_except_irq_ctx_get_irq(const struct uk_pal_except_irq_ctx *ctx)
{
	return uk_plat_xen_except_irq_ctx_get_irq(
		(const struct uk_plat_xen_except_irq_ctx *)ctx);
}

static inline
void uk_pal_except_irq_ctx_set_irq(struct uk_pal_except_irq_ctx *ctx,
				__u64 irq)
{
	uk_plat_xen_except_irq_ctx_set_irq(
		(struct uk_plat_xen_except_irq_ctx *)ctx, irq);
}

static inline
void uk_pal_enable_irq(void)
{
	uk_plat_xen_enable_irq();
}

static inline
void uk_pal_disable_irq(void)
{
	uk_plat_xen_disable_irq();
}

static inline
unsigned long uk_pal_save_irqf(void)
{
	return uk_plat_xen_save_irqf();
}

static inline
void uk_pal_restore_irqf(unsigned long flags)
{
	uk_plat_xen_restore_irqf(flags);
}

static inline
int uk_pal_irqs_disabled(void)
{
	return uk_plat_xen_irqs_disabled();
}

static inline
void uk_pal_irqs_handle_pending(void)
{
	uk_plat_xen_irqs_handle_pending();
}

static inline
__uptr uk_pal_except_get_except_stack_base(void)
{
	return uk_plat_xen_except_get_except_stack_base();
}

static inline
void uk_pal_except_push_nested(void)
{
	uk_plat_xen_except_push_nested();
}

static inline
void uk_pal_except_pop_nested(void)
{
	uk_plat_xen_except_pop_nested();
}

static inline
int uk_pal_except_init(void)
{
	return uk_plat_xen_except_init();
}

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PLAT_PAL_EXCEPT_H__ */
