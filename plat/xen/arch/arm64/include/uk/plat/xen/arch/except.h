/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_XEN_ARCH_EXCEPT_H__
#define __UK_PLAT_XEN_ARCH_EXCEPT_H__

/* On ARM64 exception handling is delegated to native */

#include <uk/plat/native/except.h>

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

/* Exception event identifiers for uk_event handlers */
#define UK_PLAT_XEN_EXCEPT_EVENT_DEBUG					\
	UK_PLAT_NATIVE_EXCEPT_EVENT_DEBUG
#define UK_PLAT_XEN_EXCEPT_EVENT_ERR_INVALID_OP				\
	UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_INVALID_OP
#define UK_PLAT_XEN_EXCEPT_EVENT_ERR_PAGE_FAULT				\
	UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_PAGE_FAULT
#define UK_PLAT_XEN_EXCEPT_EVENT_ERR_BUS_ERROR				\
	UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_BUS_ERROR
#define UK_PLAT_XEN_EXCEPT_EVENT_ERR_MATH				\
	UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_MATH
#define UK_PLAT_XEN_EXCEPT_EVENT_ERR_SECURITY				\
	UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_SECURITY
#define UK_PLAT_XEN_EXCEPT_EVENT_SYSCALL				\
	UK_PLAT_NATIVE_EXCEPT_EVENT_SYSCALL
#define UK_PLAT_XEN_EXCEPT_EVENT_IRQ					\
	UK_PLAT_NATIVE_EXCEPT_EVENT_IRQ
#define	UK_PLAT_XEN_EXCEPT_EVENT_UNHANDLED				\
	UK_PLAT_NATIVE_EXCEPT_EVENT_UNHANDLED

/* Opaque type not concretely defined on ARM64; used as alias to native ctx */
struct uk_plat_xen_except_err_ctx;

/* Exception context accessors */

__isr static inline
int uk_plat_xen_arm64_except_err_ctx_get_eid(
	const struct uk_plat_xen_except_err_ctx *ctx)
{
	return uk_plat_native_arm64_except_err_ctx_get_eid(
		(const struct uk_plat_native_except_err_ctx *)ctx);
}

__isr static inline
void uk_plat_xen_arm64_except_err_ctx_set_eid(
	struct uk_plat_xen_except_err_ctx *ctx, int eid)
{
	uk_plat_native_arm64_except_err_ctx_set_eid(
		(struct uk_plat_native_except_err_ctx *)ctx, eid);
}

__isr static inline
__u64 uk_plat_xen_arm64_except_err_ctx_get_esr(
	const struct uk_plat_xen_except_err_ctx *ctx)
{
	return uk_plat_native_arm64_except_err_ctx_get_esr(
		(const struct uk_plat_native_except_err_ctx *)ctx);
}

__isr static inline
void uk_plat_xen_arm64_except_err_ctx_set_esr(
	struct uk_plat_xen_except_err_ctx *ctx, __u64 esr)
{
	uk_plat_native_arm64_except_err_ctx_set_esr(
		(struct uk_plat_native_except_err_ctx *)ctx, esr);
}

__isr static inline
const char *uk_plat_xen_except_err_ctx_get_str(
	const struct uk_plat_xen_except_err_ctx *ctx)
{
	return uk_plat_native_except_err_ctx_get_str(
		(const struct uk_plat_native_except_err_ctx *)ctx);
}

__isr static inline
void uk_plat_xen_except_err_ctx_set_str(struct uk_plat_xen_except_err_ctx *ctx,
					const char *str)
{
	uk_plat_native_except_err_ctx_set_str(
		(struct uk_plat_native_except_err_ctx *)ctx, str);
}

__isr static inline
int uk_plat_xen_except_err_ctx_get_handler_err(
	const struct uk_plat_xen_except_err_ctx *ctx)
{
	return uk_plat_native_except_err_ctx_get_handler_err(
		(const struct uk_plat_native_except_err_ctx *)ctx);
}

__isr static inline
void uk_plat_xen_except_err_ctx_set_handler_err(
	struct uk_plat_xen_except_err_ctx *ctx, int handler_err)
{
	uk_plat_native_except_err_ctx_set_handler_err(
		(struct uk_plat_native_except_err_ctx *)ctx, handler_err);
}

__isr static inline
struct uk_plat_native_regs *uk_plat_xen_except_err_ctx_get_regs(
	const struct uk_plat_xen_except_err_ctx *ctx)
{
	return uk_plat_native_except_err_ctx_get_regs(
		(const struct uk_plat_native_except_err_ctx *)ctx);
}

__isr static inline
void uk_plat_xen_except_err_ctx_set_regs(struct uk_plat_xen_except_err_ctx *ctx,
					 struct uk_plat_native_regs *regs)
{
	uk_plat_native_except_err_ctx_set_regs(
		(struct uk_plat_native_except_err_ctx *)ctx, regs);
}

__isr static inline
__u64 uk_plat_xen_except_err_ctx_get_fault_addr(
	const struct uk_plat_xen_except_err_ctx *ctx)
{
	return uk_plat_native_except_err_ctx_get_fault_addr(
		(const struct uk_plat_native_except_err_ctx *)ctx);
}

__isr static inline
void uk_plat_xen_except_err_ctx_set_fault_addr(
	struct uk_plat_xen_except_err_ctx *ctx, __u64 fault_addr)
{
	uk_plat_native_except_err_ctx_set_fault_addr(
		(struct uk_plat_native_except_err_ctx *)ctx, fault_addr);
}

/* IRQ context accessors */

/* Opaque type not concretely defined on ARM64; used as alias to native ctx */
struct uk_plat_xen_except_irq_ctx;

__isr static inline
struct uk_plat_native_regs *uk_plat_xen_except_irq_ctx_get_regs(
	const struct uk_plat_xen_except_irq_ctx *ctx)
{
	return uk_plat_native_except_irq_ctx_get_regs(
		(const struct uk_plat_native_except_irq_ctx *)ctx);
}

__isr static inline
void uk_plat_xen_except_irq_ctx_set_regs(struct uk_plat_xen_except_irq_ctx *ctx,
					 struct uk_plat_native_regs *regs)
{
	uk_plat_native_except_irq_ctx_set_regs(
		(struct uk_plat_native_except_irq_ctx *)ctx, regs);
}

__isr static inline
__u64 uk_plat_xen_except_irq_ctx_get_irq(
	const struct uk_plat_xen_except_irq_ctx *ctx)
{
	return uk_plat_native_except_irq_ctx_get_irq(
		(const struct uk_plat_native_except_irq_ctx *)ctx);
}

__isr static inline
void uk_plat_xen_except_irq_ctx_set_irq(struct uk_plat_xen_except_irq_ctx *ctx,
					__u32 irq)
{
	uk_plat_native_except_irq_ctx_set_irq(
		(struct uk_plat_native_except_irq_ctx *)ctx, irq);
}

/* IRQ control */

__isr static inline
void uk_plat_xen_enable_irq(void)
{
	uk_plat_native_enable_irq();
}

__isr static inline
void uk_plat_xen_disable_irq(void)
{
	uk_plat_native_disable_irq();
}

__isr static inline
unsigned long uk_plat_xen_save_irqf(void)
{
	return uk_plat_native_save_irqf();
}

__isr static inline
void uk_plat_xen_restore_irqf(unsigned long flags)
{
	uk_plat_native_restore_irqf(flags);
}

__isr static inline
int uk_plat_xen_irqs_disabled(void)
{
	return uk_plat_native_irqs_disabled();
}

/* CPU halt operations */

static inline
void uk_plat_xen_halt(void)
{
	uk_plat_native_halt();
}

static inline
void uk_plat_xen_halt_irq(void)
{
	uk_plat_native_halt_irq();
}

/* Platform-specific IRQ handling */

__isr static inline
__uptr uk_plat_xen_except_get_except_stack_base(void)
{
	return uk_plat_native_except_get_except_stack_base();
}

/* Nested exception support */

__isr static inline
void uk_plat_xen_except_push_nested(void)
{
	uk_plat_native_except_push_nested();
}

__isr static inline
void uk_plat_xen_except_pop_nested(void)
{
	uk_plat_native_except_pop_nested();
}

/* Exception init */
static inline
int uk_plat_xen_except_init(void)
{
	return uk_plat_native_except_init();
}

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */

#endif /* __UK_PLAT_XEN_ARCH_EXCEPT_H__ */
