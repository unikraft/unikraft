/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2018 Arm Ltd.
 * Copyright (c) 2022 Karlsruhe Institute of Technology (KIT)
 * Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/arch/ctx.h>
#include <uk/arch/types.h>
#include <uk/arch/util.h>
#include <uk/assert.h>
#include <uk/event.h>
#include <uk/intctlr/gic.h>
#include <uk/pcpuvar.h>
#include <uk/plat/config.h>
#include <uk/print.h>

#if CONFIG_ARM64_FEAT_MTE
#include <arm/arm64/mte.h>
#endif /* CONFIG_ARM64_FEAT_MTE */

extern struct _gic_dev *gic;

__uk_pcpuvar __u8 uk_plat_native_except_switch_stack;
__uk_pcpuvar __uptr uk_plat_native_except_stack_base;

/* Raised on exception */
UK_EVENT(UK_PLAT_NATIVE_EXCEPT_EVENT_DEBUG);
UK_EVENT(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_INVALID_OP);
UK_EVENT(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_PAGE_FAULT);
UK_EVENT(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_BUS_ERROR);
UK_EVENT(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_MATH);
UK_EVENT(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_SECURITY);
UK_EVENT(UK_PLAT_NATIVE_EXCEPT_EVENT_SYSCALL);
UK_EVENT(UK_PLAT_NATIVE_EXCEPT_EVENT_IRQ);

/*
 *  CPU_EXCEPT_STACK_SIZE  CPU_EXCEPT_STACK_SIZE  CPU_EXCEPT_STACK_SIZE
 *<--------------------><---------------------><-------------------->
 *|=================================================================|
 *|                     |                     |                     |
 *|     crit stack      |       trap stack    |        IRQ stack    |
 *|                     |                     |                     |
 *|=================================================================|
 * ^
 * except_stack
 *
 * Why do we need separate stacks for IRQ and traps?
 * We could use a single stack for exceptions, but in that case when we have a
 * nested case (e.g. an IRQ triggers a synchronous exception) we would need to
 * check at the interrupt vector entry whether we are coming from an exception
 * (i.e. keep using the exception stack) or not (switch to the exception stack).
 * This may be correct but it is a costlier operation than simply adding
 * CPU_EXCEPT_STACK_SIZE, especially on an IRQ handler path.
 *
 * Why can one stack not corrupt the other/Why is trap stack before IRQ stack?
 * During a trap IRQs are disabled. So only the trap stack could potentially
 * corrupt the IRQ stack if they were the other way around. The ordering of the
 * stacks is made so that this cannot happen. Same thing goes for the crit
 * stack as a crit exception may happen during a trap.
 *
 */
static __align(UKARCH_SP_ALIGN)
__u8 except_stack[CONFIG_UKPLAT_CPU_MAXCOUNT][3 * CPU_EXCEPT_STACK_SIZE];

/* Raised on unhandled exception */
UK_EVENT(UK_PLAT_NATIVE_EXCEPT_EVENT_UNHANDLED);

static const char * const exception_modes[] = {
	"Synchronous Abort",
	"IRQ",
	"FIQ",
	"Error"
};

static const
char *uk_plat_native_arm64_exception_table[UK_PLAT_NATIVE_ARM64_EXCEPT_ID_MAX] = {
	"invalid op",     /* UK_PLAT_NATIVE_ARM64_EXCEPT_ID_INVALID_OP */
	"debug",          /* UK_PLAT_NATIVE_ARM64_EXCEPT_ID_DEBUG */
	"page fault",     /* UK_PLAT_NATIVE_ARM64_EXCEPT_ID_PAGE_FAULT */
	"bus error",      /* UK_PLAT_NATIVE_ARM64_EXCEPT_ID_BUS_ERROR */
	"floating point", /* UK_PLAT_NATIVE_ARM64_EXCEPT_ID_MATH */
	"security",       /* UK_PLAT_NATIVE_ARM64_EXCEPT_ID_SECURITY */
	"system call",    /* UK_PLAT_NATIVE_ARM64_EXCEPT_ID_SYSCALL */
};

static struct uk_event *trap_event_table[UK_PLAT_NATIVE_ARM64_EXCEPT_ID_MAX] = {
	[UK_PLAT_NATIVE_ARM64_EXCEPT_ID_INVALID_OP] = UK_EVENT_PTR(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_INVALID_OP),
	[UK_PLAT_NATIVE_ARM64_EXCEPT_ID_DEBUG]      = UK_EVENT_PTR(UK_PLAT_NATIVE_EXCEPT_EVENT_DEBUG),
	[UK_PLAT_NATIVE_ARM64_EXCEPT_ID_PAGE_FAULT] = UK_EVENT_PTR(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_PAGE_FAULT),
	[UK_PLAT_NATIVE_ARM64_EXCEPT_ID_BUS_ERROR]  = UK_EVENT_PTR(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_BUS_ERROR),
	[UK_PLAT_NATIVE_ARM64_EXCEPT_ID_MATH]       = UK_EVENT_PTR(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_MATH),
	[UK_PLAT_NATIVE_ARM64_EXCEPT_ID_SECURITY]   = UK_EVENT_PTR(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_SECURITY),
	[UK_PLAT_NATIVE_ARM64_EXCEPT_ID_SYSCALL]    = UK_EVENT_PTR(UK_PLAT_NATIVE_EXCEPT_EVENT_SYSCALL),
};

/* Mapping of the fault status code (FSC) for instruction and data aborts to
 * trap type (either page fault or bus error). Zero means invalid. The map
 * takes 64 bytes but saves a ton of comparisons.
 */
static const __u8 arm64_exception_map[] = {
	[UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_ADDR_L0]	 = UK_PLAT_NATIVE_ARM64_EXCEPT_ID_PAGE_FAULT,
	[UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_ADDR_L1]	 = UK_PLAT_NATIVE_ARM64_EXCEPT_ID_PAGE_FAULT,
	[UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_ADDR_L2]	 = UK_PLAT_NATIVE_ARM64_EXCEPT_ID_PAGE_FAULT,
	[UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_ADDR_L3]	 = UK_PLAT_NATIVE_ARM64_EXCEPT_ID_PAGE_FAULT,
	[UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_TRANS_L0]	 = UK_PLAT_NATIVE_ARM64_EXCEPT_ID_PAGE_FAULT,
	[UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_TRANS_L1]	 = UK_PLAT_NATIVE_ARM64_EXCEPT_ID_PAGE_FAULT,
	[UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_TRANS_L2]	 = UK_PLAT_NATIVE_ARM64_EXCEPT_ID_PAGE_FAULT,
	[UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_TRANS_L3]	 = UK_PLAT_NATIVE_ARM64_EXCEPT_ID_PAGE_FAULT,
	[UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_ACCF_L0]	 = UK_PLAT_NATIVE_ARM64_EXCEPT_ID_PAGE_FAULT,
	[UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_ACCF_L1]	 = UK_PLAT_NATIVE_ARM64_EXCEPT_ID_PAGE_FAULT,
	[UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_ACCF_L2]	 = UK_PLAT_NATIVE_ARM64_EXCEPT_ID_PAGE_FAULT,
	[UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_ACCF_L3]	 = UK_PLAT_NATIVE_ARM64_EXCEPT_ID_PAGE_FAULT,
	[UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_PERM_L0]	 = UK_PLAT_NATIVE_ARM64_EXCEPT_ID_PAGE_FAULT,
	[UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_PERM_L1]	 = UK_PLAT_NATIVE_ARM64_EXCEPT_ID_PAGE_FAULT,
	[UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_PERM_L2]	 = UK_PLAT_NATIVE_ARM64_EXCEPT_ID_PAGE_FAULT,
	[UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_PERM_L3]	 = UK_PLAT_NATIVE_ARM64_EXCEPT_ID_PAGE_FAULT,
	[UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_SYNC]		 = UK_PLAT_NATIVE_ARM64_EXCEPT_ID_BUS_ERROR,
	[UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_TAG]		 = UK_PLAT_NATIVE_ARM64_EXCEPT_ID_PAGE_FAULT,
	[UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_SYNC_PT_LM1]	 = UK_PLAT_NATIVE_ARM64_EXCEPT_ID_BUS_ERROR,
	[UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_SYNC_PT_L0]	 = UK_PLAT_NATIVE_ARM64_EXCEPT_ID_BUS_ERROR,
	[UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_SYNC_PT_L1]	 = UK_PLAT_NATIVE_ARM64_EXCEPT_ID_BUS_ERROR,
	[UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_SYNC_PT_L2]	 = UK_PLAT_NATIVE_ARM64_EXCEPT_ID_BUS_ERROR,
	[UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_SYNC_PT_L3]	 = UK_PLAT_NATIVE_ARM64_EXCEPT_ID_BUS_ERROR,
	[UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_SYNC_ECC]	 = UK_PLAT_NATIVE_ARM64_EXCEPT_ID_BUS_ERROR,
	[UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_SYNC_ECC_PT_LM1] = UK_PLAT_NATIVE_ARM64_EXCEPT_ID_BUS_ERROR,
	[UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_SYNC_ECC_PT_L0]  = UK_PLAT_NATIVE_ARM64_EXCEPT_ID_BUS_ERROR,
	[UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_SYNC_ECC_PT_L1]  = UK_PLAT_NATIVE_ARM64_EXCEPT_ID_BUS_ERROR,
	[UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_SYNC_ECC_PT_L2]  = UK_PLAT_NATIVE_ARM64_EXCEPT_ID_BUS_ERROR,
	[UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_SYNC_ECC_PT_L3]  = UK_PLAT_NATIVE_ARM64_EXCEPT_ID_BUS_ERROR,
	[UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_ALIGN]		 = UK_PLAT_NATIVE_ARM64_EXCEPT_ID_BUS_ERROR,
	[UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_ADDR_LM1]	 = UK_PLAT_NATIVE_ARM64_EXCEPT_ID_PAGE_FAULT,
	[UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_TRANS_LM1]	 = UK_PLAT_NATIVE_ARM64_EXCEPT_ID_PAGE_FAULT
};

static
enum uk_plat_native_arm64_except_id esr_to_exception(__u64 esr)
{
	__u8 fsc, trap;

	/* We expect Unikraft to run in EL1. So do not catch traps from EL0. */
	switch (UK_ARCH_ARM64_ESR_EC_FROM(esr)) {
	case UK_ARCH_ARM64_ESR_EL1_EC_SVC64:
		return UK_PLAT_NATIVE_ARM64_EXCEPT_ID_SYSCALL;

	case UK_ARCH_ARM64_ESR_EL1_EC_MMU_IABRT_EL1:
	case UK_ARCH_ARM64_ESR_EL1_EC_MMU_DABRT_EL1:
		fsc = UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_FROM(UK_ARCH_ARM64_ESR_ISS_FROM(esr));
		UK_ASSERT(fsc < ARRAY_SIZE(arm64_exception_map));

		trap = arm64_exception_map[fsc];
		if (trap == 0)
			break; /* Neither page fault, nor bus error */

		return trap;

	case UK_ARCH_ARM64_ESR_EL1_EC_PC_ALIGN:
	case UK_ARCH_ARM64_ESR_EL1_EC_SP_ALIGN:
		return UK_PLAT_NATIVE_ARM64_EXCEPT_ID_BUS_ERROR;

	case UK_ARCH_ARM64_ESR_EL1_EC_SVE_ASIMD_FP_ACC:
	case UK_ARCH_ARM64_ESR_EL1_EC_SVE_ACC:
	case UK_ARCH_ARM64_ESR_EL1_EC_FP64:
		return UK_PLAT_NATIVE_ARM64_EXCEPT_ID_MATH;

	case UK_ARCH_ARM64_ESR_EL1_EC_UNKNOWN:
	case UK_ARCH_ARM64_ESR_EL1_EC_ILL:
		return UK_PLAT_NATIVE_ARM64_EXCEPT_ID_INVALID_OP;

	case UK_ARCH_ARM64_ESR_EL1_EC_BTI:
	case UK_ARCH_ARM64_ESR_EL1_EC_FPAC:
		return UK_PLAT_NATIVE_ARM64_EXCEPT_ID_SECURITY;

	case UK_ARCH_ARM64_ESR_EL1_EC_BRK_EL1:
	case UK_ARCH_ARM64_ESR_EL1_EC_STEP_EL1:
	case UK_ARCH_ARM64_ESR_EL1_EC_WATCHP_EL1:
	case UK_ARCH_ARM64_ESR_EL1_EC_BRK64:
		return UK_PLAT_NATIVE_ARM64_EXCEPT_ID_DEBUG;
	}

	return UK_PLAT_NATIVE_ARM64_EXCEPT_ID_MAX;
}

void uk_plat_native_except_inv_handler(struct uk_plat_native_regs *regs __unused,
				       __u32 el, __u32 reason,
				       __u64 __unused far)
{
	UK_CRASH("Invalid %s exception taken from EL%d\n",
		 exception_modes[reason], el);
}

void uk_plat_native_except_err_handler(struct uk_plat_native_regs *regs,
				       __u64 far)
{
	struct uk_plat_native_except_err_ctx ctx;
	enum uk_plat_native_arm64_except_id exception;
	__u64 esr;
	int rc;

	esr = uk_plat_native_regs_get(regs,
				      UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_ESR_EL1);
	exception = esr_to_exception(esr);

	UK_ASSERT(exception < UK_PLAT_NATIVE_ARM64_EXCEPT_ID_MAX);

	ctx = (struct uk_plat_native_except_err_ctx) {
		.eid = exception,
		.str = uk_plat_native_arm64_exception_table[exception],
		.esr = esr,
		.far = far,
		.handler_err = 0,
		.regs = regs,
	};

	rc = uk_raise_event_ptr(trap_event_table[exception], &ctx);
	if (unlikely(rc < 0))
		uk_pr_crit("event handler returned error: %d\n", rc);
	else if (rc != UK_EVENT_NOT_HANDLED)
		return;

	rc = uk_raise_event(UK_PLAT_NATIVE_EXCEPT_EVENT_UNHANDLED, &ctx);
	if (unlikely(rc == UK_EVENT_NOT_HANDLED || rc < 0))
		uk_pr_crit("Unhandled Trap %d (%s), error code=0x%lx\n",
			   exception, uk_plat_native_arm64_exception_table[exception],
			   esr);
	/* We expect that if there is a handler it won't return,
	 * but as we can't control that, we place halt outside
	 * the above conditional.
	 */
	uk_arch_arm64_halt();
}

void uk_plat_native_except_irq_handler(struct uk_plat_native_regs *regs)
{
	struct uk_plat_native_except_irq_ctx ctx;
	int rc;

#if CONFIG_ARM64_FEAT_MTE
	if (unlikely(mte_async_fault()))
		UK_CRASH("EL1 async tag check fault\n");
#endif /* CONFIG_ARM64_FEAT_MTE */

	ctx.regs = regs;
	rc = uk_raise_event(UK_PLAT_NATIVE_EXCEPT_EVENT_IRQ, &ctx);
	if (unlikely(rc < 0))
		UK_CRASH("IRQ event handler returned error: %d\n", rc);
}

#if CONFIG_HAVE_SMP
int uk_plat_native_except_send_ipi(__u64 id, __u32 irq)
{
	gic->ops.gic_sgi_gen(irq, id);
	return 0;
}
#endif /* CONFIG_HAVE_SMP */

__uptr uk_plat_native_except_get_except_stack_base(void)
{
	return (__uptr)&except_stack;
}

int uk_plat_native_except_init(void)
{
	__uptr except_stack_base;
	__u32 this_cpu_idx;
	__u64 this_cpu_id;
	__u64 boot_cpu_id;
	int ret;

	this_cpu_idx = uk_pcpuvar_current_get(uk_pcpuvar_cpu_idx);
	this_cpu_id = uk_pcpuvar_current_get(uk_pcpuvar_cpu_id);

	boot_cpu_id = uk_pcpuvar_lval(0, uk_pcpuvar_cpu_id);

	if (this_cpu_id != boot_cpu_id) {
		/* Initialize the interrupt controller */
		ret = gic->ops.initialize();
		if (unlikely(ret))
			return ret;
	}

	/* Initialize this cpu's except stack base */

	except_stack_base = (__uptr)&except_stack[this_cpu_idx][0];

	uk_pcpuvar_current_set(uk_plat_native_except_stack_base, except_stack_base);

	return 0;
}
