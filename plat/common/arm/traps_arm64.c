/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2018 Arm Ltd.
 * Copyright (c) 2022 Karlsruhe Institute of Technology (KIT)
 * Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <arm/cpu.h>
#include <arm/traps.h>

#include <uk/arch/types.h>
#include <uk/arch/ctx.h>
#include <uk/print.h>
#include <uk/assert.h>
#include <uk/intctlr/gic.h>
#include <uk/asm/lcpu.h>
#include <uk/lcpu.h>
#include <uk/plat/syscall.h>

#ifdef CONFIG_ARM64_FEAT_MTE
#include <arm/arm64/mte.h>
#endif /* CONFIG_ARM64_FEAT_MTE */

/** GIC driver to call interrupt handler */
extern struct _gic_dev *gic;

DECLARE_TRAP_EVENT(UKARCH_TRAP_INVALID_OP);
DECLARE_TRAP_EVENT(UKARCH_TRAP_DEBUG);
DECLARE_TRAP_EVENT(UKARCH_TRAP_PAGE_FAULT);
DECLARE_TRAP_EVENT(UKARCH_TRAP_BUS_ERROR);
DECLARE_TRAP_EVENT(UKARCH_TRAP_MATH);
DECLARE_TRAP_EVENT(UKARCH_TRAP_SECURITY);
DECLARE_TRAP_EVENT(UKARCH_TRAP_SYSCALL);

static const char * const exception_modes[] = {
	"Synchronous Abort",
	"IRQ",
	"FIQ",
	"Error"
};

static struct uk_event *trap_event_table[ARM64_EXCEPTION_MAX] = {
	[ARM64_EXCEPTION_INVALID_OP] = UK_EVENT_PTR(UKARCH_TRAP_INVALID_OP),
	[ARM64_EXCEPTION_DEBUG]      = UK_EVENT_PTR(UKARCH_TRAP_DEBUG),
	[ARM64_EXCEPTION_PAGE_FAULT] = UK_EVENT_PTR(UKARCH_TRAP_PAGE_FAULT),
	[ARM64_EXCEPTION_BUS_ERROR]  = UK_EVENT_PTR(UKARCH_TRAP_BUS_ERROR),
	[ARM64_EXCEPTION_MATH]       = UK_EVENT_PTR(UKARCH_TRAP_MATH),
	[ARM64_EXCEPTION_SECURITY]   = UK_EVENT_PTR(UKARCH_TRAP_SECURITY),
	[ARM64_EXCEPTION_SYSCALL]    = UK_EVENT_PTR(UKARCH_TRAP_SYSCALL),
};

static enum arm64_exception esr_to_exception(__u64 esr)
{
	__u8 fsc, trap;

	/* We expect Unikraft to run in EL1. So do not catch traps from EL0. */
	switch (ESR_EC_FROM(esr)) {
	case ESR_EL1_EC_SVC64:
		return ARM64_EXCEPTION_SYSCALL;

	case ESR_EL1_EC_MMU_IABRT_EL1:
	case ESR_EL1_EC_MMU_DABRT_EL1:
		fsc = ESR_ISS_ABRT_FSC_FROM(ESR_ISS_FROM(esr));
		UK_ASSERT(fsc < ARRAY_SIZE(arm64_exception_map));

		trap = arm64_exception_map[fsc];
		if (trap == 0)
			break; /* Neither page fault, nor bus error */

		return trap;

	case ESR_EL1_EC_PC_ALIGN:
	case ESR_EL1_EC_SP_ALIGN:
		return ARM64_EXCEPTION_BUS_ERROR;

	case ESR_EL1_EC_SVE_ASIMD_FP_ACC:
	case ESR_EL1_EC_SVE_ACC:
	case ESR_EL1_EC_FP64:
		return ARM64_EXCEPTION_MATH;

	case ESR_EL1_EC_UNKNOWN:
	case ESR_EL1_EC_ILL:
		return ARM64_EXCEPTION_INVALID_OP;

	case ESR_EL1_EC_BTI:
	case ESR_EL1_EC_FPAC:
		return ARM64_EXCEPTION_SECURITY;

	case ESR_EL1_EC_BRK_EL1:
	case ESR_EL1_EC_STEP_EL1:
	case ESR_EL1_EC_WATCHP_EL1:
	case ESR_EL1_EC_BRK64:
		return ARM64_EXCEPTION_DEBUG;
	}

	return ARM64_EXCEPTION_MAX;
}

/* TODO Move into a per-cpu once we are able to access
 *      per-cpu vars from asm.
 */
__u8 ukarch_except_switch_stack;

void ukarch_push_nested_exceptions(void)
{
	ukarch_except_switch_stack++;
}

void ukarch_pop_nested_exceptions(void)
{
	--ukarch_except_switch_stack;
}

void invalid_trap_handler(struct __regs *regs __unused, __u32 el,
			  __u32 reason, __u64 __unused far)
{
	UK_CRASH("Invalid %s exception taken from EL%d\n",
		 exception_modes[reason], el);
}

/* Generic event for unhandled exceptions taken from EL1 */
UK_EVENT(UK_EVENT_UNHANDLED_EXCEPTION);

void trap_el1_sync(struct __regs *regs, __u64 far)
{
	enum arm64_exception exception = esr_to_exception(regs->esr_el1);
	struct ukarch_trap_ctx ctx;
	int rc;

	UK_ASSERT(exception < ARM64_EXCEPTION_MAX);

	ctx = (struct ukarch_trap_ctx) {
		.eid = exception,
		.str = arm64_exception_table[exception],
		.esr = regs->esr_el1,
		.far = far,
		.handler_err = 0,
		.regs = regs,
	};

	rc = uk_raise_event_ptr(trap_event_table[exception], &ctx);
	if (unlikely(rc < 0))
		uk_pr_crit("event handler returned error: %d\n", rc);
	else if (rc != UK_EVENT_NOT_HANDLED)
		return;

	rc = uk_raise_event(UK_EVENT_UNHANDLED_EXCEPTION, &ctx);
	if (unlikely(rc == UK_EVENT_NOT_HANDLED || rc < 0))
		uk_pr_crit("Unhandled Trap %d (%s), error code=0x%lx\n",
			   exception, arm64_exception_table[exception],
			   regs->esr_el1);
	/* We expect that if there is a handler it won't return,
	 * but as we can't control that, we place halt outside
	 * the above conditional.
	 */
	ukplat_lcpu_halt();
}

void trap_el1_irq(struct __regs *regs)
{
	UK_ASSERT(gic);

#ifdef CONFIG_ARM64_FEAT_MTE
	if (unlikely(mte_async_fault()))
		UK_CRASH("EL1 async tag check fault\n");
#endif /* CONFIG_ARM64_FEAT_MTE */

	gic->ops.handle_irq(regs);
}

#ifdef CONFIG_LIBSYSCALL_SHIM_HANDLER

extern void ukplat_syscall_handler(struct uk_syscall_ctx *usc);

static int arm64_syscall_adapter(void *data)
{
	struct uk_lcpu_except_err_ctx *ctx = data;
	struct ukarch_execenv *execenv = (struct ukarch_execenv *)ctx->regs;

	/* Save extended register state */
	uk_lcpu_ectx_sanitize((struct uk_lcpu_ectx *)&execenv->ectx);
	uk_lcpu_ectx_store((struct uk_lcpu_ectx *)&execenv->ectx);

	/* Save system context state */
	uk_lcpu_sysctx_store(&execenv->sysctx);

	ukplat_syscall_handler((struct uk_syscall_ctx *)execenv);

	/* Restore system context state */
	uk_lcpu_sysctx_load(&execenv->sysctx);

	/* Restore extended register state */
	uk_lcpu_ectx_load((struct uk_lcpu_ectx *)&execenv->ectx);

	return 1; /* Success */
}

UK_EVENT_HANDLER(UKARCH_TRAP_SYSCALL, arm64_syscall_adapter);

#endif /* CONFIG_LIBSYSCALL_SHIM_HANDLER */
