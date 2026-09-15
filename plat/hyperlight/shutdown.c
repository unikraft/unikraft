/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/*
 * Hyperlight shutdown / power-management ops.
 *
 * Halt: writes to port 108 with the dispatch function address in EAX.
 * The host intercepts this VM exit to know the guest has finished
 * initialisation and is ready to receive function calls.
 *
 * RAX holds the address of hyperlight_dispatch_function — the host
 * captures this during evolve() and uses it as the RIP for subsequent
 * guest function calls via MultiUseSandbox::call().
 */

#include <uk/boot/earlytab.h>
#include <uk/lcpu.h>
#include <uk/lcpu/pm.h>
#include <uk/pm.h>
#include <uk/prio.h>
#include <uk/plat/common/bootinfo.h>

#include <hyperlight-x86/outb.h>
#include <hyperlight-x86/step.h>

#ifdef CONFIG_LIBHOSTSOCK
extern int hostsock_rescan_events(void);
#endif

/* Provided by dispatch.c */
extern void hyperlight_dispatch_function(void)
	__attribute__((noreturn));
extern void hyperlight_dispatch_push_void_result(void);

/*
 * Halt via port 108.  The host intercepts this VM exit.
 * RAX = dispatch function address — the host stores this
 * and sets RIP here for each call() after evolve().
 * cli + hlt after outl is a backstop — the VM exit from
 * outl already stops the vCPU.
 *
 * This is the bare halt: no result is pushed and no shutdown work is
 * done.  The step model's yield thread uses it to signal boot complete.
 */
void __noreturn hyperlight_halt_to_host(void)
{
	__asm__ volatile(
		/* Hyperlight checks RSP alignment after halt */
		"andq $~0xf, %%rsp\n\t"
		"movq %0, %%rax\n\t"
		"movw $108, %%dx\n\t"
		"outl %%eax, %%dx\n\t"
		"cli\n\t"
		"hlt\n\t"
		: : "r"((__u64)hyperlight_dispatch_function) : "rax", "rdx"
	);
	__builtin_unreachable();
}

static int hyperlight_shutdown(void)
{
	/*
	 * Tell the host the process is gone and with what status.  Under a
	 * step pump this halt lands in the middle of the host's `step` call
	 * (the workload's main thread exited and ukboot is shutting the
	 * system down): complete that call with a void result too, so the
	 * host reads a clean exit rather than a truncated step.  The report
	 * goes first; it is a host call and must not sit on top of the
	 * result.
	 */
	hyperlight_step_report_exit();
	if (hyperlight_step_active())
		hyperlight_dispatch_push_void_result();

	hyperlight_halt_to_host();
}

static int hyperlight_crash(void)
{
	hyperlight_out32(102, 0xFF); /* port 102 = Abort */
	__builtin_unreachable();
}

static const struct uk_pm_ops hyperlight_pm_ops = {
	.syshalt    = hyperlight_shutdown,
	.sysrestart = hyperlight_shutdown,
	.syscrash   = hyperlight_crash,
};

/*
 * LCPU halt_irq — called by the cooperative scheduler's idle thread
 * when no thread has a timed wakeup (wake_up_time == 0).
 * Poll hostsock to discover ready sockets and wake blocked threads.
 */
static void hyperlight_halt_irq(void)
{
	/* Under a step pump the idle thread yields the vCPU back to the
	 * host here (and at boot, the first idle hands it over).  Only a
	 * kernel without the step model falls through to the busy-wait.
	 */
	if (hyperlight_step_halt(0))
		return;

	uk_pal_enable_irq();
#ifdef CONFIG_LIBHOSTSOCK
	hostsock_rescan_events();
#endif
	uk_arch_spinwait();
}

static const struct uk_lcpu_pm_ops hyperlight_lcpu_pm_ops = {
	.halt_irq = hyperlight_halt_irq,
};

static int hyperlight_register_pm_ops(struct ukplat_bootinfo __unused *bi)
{
	int rc;

	rc = uk_pm_ops_register(&hyperlight_pm_ops);
	if (rc)
		return rc;
	return uk_lcpu_pm_ops_register(&hyperlight_lcpu_pm_ops);
}

UK_BOOT_EARLYTAB_ENTRY(hyperlight_register_pm_ops, UK_PRIO_EARLIEST);
