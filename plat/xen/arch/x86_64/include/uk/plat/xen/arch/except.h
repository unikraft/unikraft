/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_XEN_ARCH_EXCEPT_H__
#define __UK_PLAT_XEN_ARCH_EXCEPT_H__

#include <uk/arch/types.h>
#include <uk/arch/util.h>
#include <uk/plat/native/arch/regs.h>

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

/* Exception event identifiers for uk_event handlers */
#define UK_PLAT_XEN_X86_64_EXCEPT_EVENT_ERR_GP_FAULT			\
	xen_x86_64_except_event_err_gp_fault
#define UK_PLAT_XEN_X86_64_EXCEPT_EVENT_NMI				\
	xen_x86_64_except_event_err_nmi
#define UK_PLAT_XEN_EXCEPT_EVENT_DEBUG					\
	xen_except_event_debug
#define UK_PLAT_XEN_EXCEPT_EVENT_ERR_INVALID_OP				\
	xen_except_event_err_invalid_op
#define UK_PLAT_XEN_EXCEPT_EVENT_ERR_PAGE_FAULT				\
	xen_except_event_err_page_fault
#define UK_PLAT_XEN_EXCEPT_EVENT_ERR_BUS_ERROR				\
	xen_except_event_err_bus_error
#define UK_PLAT_XEN_EXCEPT_EVENT_ERR_MATH				\
	xen_except_event_err_math
#define UK_PLAT_XEN_EXCEPT_EVENT_ERR_SECURITY				\
	xen_except_event_err_security
#define UK_PLAT_XEN_EXCEPT_EVENT_SYSCALL				\
	xen_except_event_syscall
#define UK_PLAT_XEN_EXCEPT_EVENT_IRQ					\
	xen_except_event_irq
#define	UK_PLAT_XEN_EXCEPT_EVENT_UNHANDLED				\
	xen_except_event_unhandled

/**
 * x86_64 exception context passed to error event handlers.
 * Contains CPU state at exception time plus exception-specific metadata.
 */
struct uk_plat_xen_except_err_ctx {
	/* Exception description string */
	const char *str;
	/* x86 exception vector number (0-31) */
	__u32 trapnr;
	/* Hardware error code (only valid for #PF, #GP, #TS, etc.) */
	__u64 error_code;
	/* Set by handler if exception unrecoverable */
	int handler_err;
	/* CPU registers at exception time */
	struct uk_plat_native_regs *regs;
	/* Faulting virtual address (page faults only, from CR2) */
	__u64 cr2;
};

/* Exception context accessors */
__isr static inline
const char *uk_plat_xen_except_err_ctx_get_str(
	const struct uk_plat_xen_except_err_ctx *ctx)
{
	return ctx->str;
}

__isr static inline
void uk_plat_xen_except_err_ctx_set_str(
	struct uk_plat_xen_except_err_ctx *ctx, const char *str)
{
	ctx->str = str;
}

__isr static inline
__u32 uk_plat_xen_x86_64_except_err_ctx_get_trapnr(
	const struct uk_plat_xen_except_err_ctx *ctx)
{
	return ctx->trapnr;
}

__isr static inline
void uk_plat_xen_x86_64_except_err_ctx_set_trapnr(
	struct uk_plat_xen_except_err_ctx *ctx, __u32 trapnr)
{
	ctx->trapnr = trapnr;
}

__isr static inline
__u64 uk_plat_xen_x86_64_except_err_ctx_get_error_code(
	const struct uk_plat_xen_except_err_ctx *ctx)
{
	return ctx->error_code;
}

__isr static inline
void uk_plat_xen_x86_64_except_err_ctx_set_error_code(
	struct uk_plat_xen_except_err_ctx *ctx, __u64 error_code)
{
	ctx->error_code = error_code;
}

__isr static inline
int uk_plat_xen_except_err_ctx_get_handler_err(
	const struct uk_plat_xen_except_err_ctx *ctx)
{
	return ctx->handler_err;
}

__isr static inline
void uk_plat_xen_except_err_ctx_set_handler_err(
	struct uk_plat_xen_except_err_ctx *ctx, int handler_err)
{
	ctx->handler_err = handler_err;
}

__isr static inline
struct uk_plat_native_regs *uk_plat_xen_except_err_ctx_get_regs(
	const struct uk_plat_xen_except_err_ctx *ctx)
{
	return ctx->regs;
}

__isr static inline
void uk_plat_xen_except_err_ctx_set_regs(
	struct uk_plat_xen_except_err_ctx *ctx,
	struct uk_plat_native_regs *regs)
{
	ctx->regs = regs;
}

__isr static inline
__u64 uk_plat_xen_except_err_ctx_get_fault_addr(
	const struct uk_plat_xen_except_err_ctx *ctx)
{
	return ctx->cr2;
}

__isr static inline
void uk_plat_xen_except_err_ctx_set_fault_addr(
	struct uk_plat_xen_except_err_ctx *ctx, __u64 fault_addr)
{
	ctx->cr2 = fault_addr;
}

/**
 * x86_64 interrupt context passed to IRQ event handlers.
 * Contains CPU state at interrupt time plus interrupt vector number.
 */
struct uk_plat_xen_except_irq_ctx {
	/* CPU registers at IRQ time */
	struct uk_plat_native_regs *regs;
	/* x86 interrupt vector number */
	__u32 irq;
};

/* IRQ context accessors */
__isr static inline
struct uk_plat_native_regs *uk_plat_xen_except_irq_ctx_get_regs(
	const struct uk_plat_xen_except_irq_ctx *ctx)
{
	return ctx->regs;
}

__isr static inline
void uk_plat_xen_except_irq_ctx_set_regs(
	struct uk_plat_xen_except_irq_ctx *ctx,
	struct uk_plat_native_regs *regs)
{
	ctx->regs = regs;
}

__isr static inline
__u64 uk_plat_xen_except_irq_ctx_get_irq(
	const struct uk_plat_xen_except_irq_ctx *ctx)
{
	return ctx->irq;
}

__isr static inline
void uk_plat_xen_except_irq_ctx_set_irq(
	struct uk_plat_xen_except_irq_ctx *ctx, __u32 irq)
{
	ctx->irq = irq;
}

#ifdef XEN_PARAVIRT

#include <common/hypervisor.h>
#include <xen-x86/smp.h>

/* IRQ control */

__isr void uk_plat_xen_irqs_handle_pending(void);

__isr static inline
void uk_plat_xen_enable_irq(void)
{
	vcpu_info_t *vcpu;

	vcpu = &HYPERVISOR_shared_info->vcpu_info[smp_processor_id()];
	__barrier(); /* Stop reordering before clearing the mask */
	vcpu->evtchn_upcall_mask = 0;
	__barrier(); /* Avoid races between unmasking and checking */
	if (unlikely(vcpu->evtchn_upcall_pending))
		uk_plat_xen_irqs_handle_pending();
}

__isr static inline
void uk_plat_xen_disable_irq(void)
{
	vcpu_info_t *vcpu;

	vcpu = &HYPERVISOR_shared_info->vcpu_info[smp_processor_id()];
	vcpu->evtchn_upcall_mask = 1;
	__barrier(); /* Ensure mask is set before continuing */
}

__isr static inline
unsigned long uk_plat_xen_save_irqf(void)
{
	unsigned long flags;
	vcpu_info_t *vcpu;

	vcpu = &HYPERVISOR_shared_info->vcpu_info[smp_processor_id()];
	flags = vcpu->evtchn_upcall_mask;
	vcpu->evtchn_upcall_mask = 1;
	__barrier(); /* Ensure mask is set before continuing */

	return flags;
}

__isr static inline
void uk_plat_xen_restore_irqf(unsigned long flags)
{
	vcpu_info_t *vcpu;

	vcpu = &HYPERVISOR_shared_info->vcpu_info[smp_processor_id()];
	__barrier(); /* Stop reordering before potentially clearing the mask */
	vcpu->evtchn_upcall_mask = flags;
	if (!flags) {
		__barrier(); /* Avoid races between unmasking and checking */
		if (unlikely(vcpu->evtchn_upcall_pending))
			uk_plat_xen_irqs_handle_pending();
	}
}

__isr static inline
int uk_plat_xen_irqs_disabled(void)
{
	vcpu_info_t *vcpu;

	vcpu = &HYPERVISOR_shared_info->vcpu_info[smp_processor_id()];
	return !!vcpu->evtchn_upcall_mask;
}

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

#endif /* XEN_PARAVIRT */

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PLAT_XEN_ARCH_EXCEPT_H__ */
