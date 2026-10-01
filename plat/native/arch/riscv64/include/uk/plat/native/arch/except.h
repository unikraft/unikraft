#ifndef __UK_PLAT_NATIVE_ARCH_EXCEPT_H__
#define __UK_PLAT_NATIVE_ARCH_EXCEPT_H__

#include <uk/arch/types.h>
#include <uk/compiler.h>
#include <uk/config.h>
#include <uk/pcpuvar.h>
#include <uk/plat/native/arch/regs.h>

#if CONFIG_LIBUKPLAT_NATIVE_EXCEPT
#define UK_PLAT_NATIVE_EXCEPT_SWITCH_STACK_SYM				\
	uk_plat_native_except_switch_stack
#define UK_PLAT_NATIVE_EXCEPT_STACK_BASE_SYM				\
	uk_plat_native_except_stack_base
#endif /* CONFIG_LIBUKPLAT_NATIVE_EXCEPT */

#if !__ASSEMBLY__
#include <errno.h>
#include <uk/asm/lcpu.h>

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
#define UK_PLAT_NATIVE_EXCEPT_EVENT_UNHANDLED				\
	native_except_unhandled_except

enum uk_plat_native_riscv64_except_id {
	UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_INVALID_OP,
	UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_DEBUG,
	UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_PAGE_FAULT,
	UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_BUS_ERROR,
	UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_MATH,
	UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_SECURITY,
	UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_SYSCALL,
	UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_MAX
};

struct uk_plat_native_except_err_ctx {
	enum uk_plat_native_riscv64_except_id eid;
	const char *str;
	__u64 scause;
	__u64 stval;
	int handler_err;
	struct uk_plat_native_regs *regs;
};

__isr static inline enum uk_plat_native_riscv64_except_id
uk_plat_native_riscv64_except_err_ctx_get_eid(
	const struct uk_plat_native_except_err_ctx *ctx)
{
	return ctx->eid;
}

__isr static inline void
uk_plat_native_riscv64_except_err_ctx_set_eid(
	struct uk_plat_native_except_err_ctx *ctx,
	enum uk_plat_native_riscv64_except_id eid)
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
uk_plat_native_riscv64_except_err_ctx_get_scause(
	const struct uk_plat_native_except_err_ctx *ctx)
{
	return ctx->scause;
}

__isr static inline void
uk_plat_native_riscv64_except_err_ctx_set_scause(
	struct uk_plat_native_except_err_ctx *ctx,
	__u64 scause)
{
	ctx->scause = scause;
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
	return ctx->stval;
}

__isr static inline void
uk_plat_native_except_err_ctx_set_fault_addr(
	struct uk_plat_native_except_err_ctx *ctx,
	__u64 fault_addr)
{
	ctx->stval = fault_addr;
}

struct uk_plat_native_except_irq_ctx {
	struct uk_plat_native_regs *regs;
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
__isr static inline void uk_plat_native_enable_irq(void)
{
	_csr_set(UK_ARCH_RISCV64_CSR_SSTATUS, UK_ARCH_RISCV64_SSTATUS_SIE);
}

__isr static inline void uk_plat_native_disable_irq(void)
{
	_csr_clear(UK_ARCH_RISCV64_CSR_SSTATUS, UK_ARCH_RISCV64_SSTATUS_SIE);
}

__isr static inline unsigned long uk_plat_native_save_irqf(void)
{
	unsigned long flags =
		!!(_csr_read(UK_ARCH_RISCV64_CSR_SSTATUS) &
		   UK_ARCH_RISCV64_SSTATUS_SIE);
	_csr_clear(UK_ARCH_RISCV64_CSR_SSTATUS, UK_ARCH_RISCV64_SSTATUS_SIE);
	return flags;
}

__isr static inline void uk_plat_native_restore_irqf(unsigned long flags)
{
	if (flags)
		_csr_set(UK_ARCH_RISCV64_CSR_SSTATUS,
			 UK_ARCH_RISCV64_SSTATUS_SIE);
	else
		_csr_clear(UK_ARCH_RISCV64_CSR_SSTATUS,
			   UK_ARCH_RISCV64_SSTATUS_SIE);
}

__isr static inline int uk_plat_native_irqs_disabled(void)
{
	return !(_csr_read(UK_ARCH_RISCV64_CSR_SSTATUS) &
		 UK_ARCH_RISCV64_SSTATUS_SIE);
}

__isr static inline void uk_plat_native_halt(void)
{
	while (!_csr_read(UK_ARCH_RISCV64_CSR_SIP))
		__asm__ __volatile__("wfi");
}

__isr static inline void uk_plat_native_halt_irq(void)
{
	uk_plat_native_halt();
	uk_plat_native_enable_irq();
	uk_plat_native_disable_irq();
}

__isr static inline void uk_plat_native_irqs_handle_pending(void)
{
	unsigned long flags;

	flags = uk_plat_native_save_irqf();

	if (_csr_read(UK_ARCH_RISCV64_CSR_SIP) &
	    (UK_ARCH_RISCV64_SIP_SSIP |
	     UK_ARCH_RISCV64_SIP_STIP |
	     UK_ARCH_RISCV64_SIP_SEIP))
		uk_plat_native_enable_irq();

	uk_plat_native_restore_irqf(flags);
}

extern __uk_pcpuvar __u8 uk_plat_native_except_switch_stack;
extern __uk_pcpuvar __uptr uk_plat_native_except_stack_base;

__isr __uptr uk_plat_native_except_get_except_stack_base(void);

__isr static inline void uk_plat_native_except_push_nested(void)
{
	uk_pcpuvar_current_set(
		uk_plat_native_except_switch_stack,
		uk_pcpuvar_current_get(uk_plat_native_except_switch_stack) + 1);
}

__isr static inline void uk_plat_native_except_pop_nested(void)
{
	uk_pcpuvar_current_set(
		uk_plat_native_except_switch_stack,
		uk_pcpuvar_current_get(uk_plat_native_except_switch_stack) - 1);
}

#if CONFIG_HAVE_SMP
__isr static inline int
uk_plat_native_except_send_ipi(__u64 id __unused, __u32 irq __unused)
{
	return -ENOTSUP;
}
#endif /* CONFIG_HAVE_SMP */

__isr __uptr uk_plat_native_riscv64_traps_get_except_stack_base(void);
__isr void uk_plat_native_riscv64_traps_init(void);

__isr int uk_plat_native_except_init(void);
#endif /* CONFIG_LIBUKPLAT_NATIVE_EXCEPT */

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PLAT_NATIVE_ARCH_EXCEPT_H__ */
