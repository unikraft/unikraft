/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, Karlsruhe Institute of Technology. All rights reserved.
 * Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <stdarg.h>
#include <stddef.h>

#include <uk/config.h>
#include <uk/crash.h>
#include <uk/essentials.h>
#include <uk/event.h>
#include <uk/lcpu.h>
#include <uk/pm.h>
#include <uk/plat/time.h>
#include <uk/preempt.h>

#if CONFIG_LIBUKSCHED && CONFIG_LIBUKNOFAULT
#include <uk/nofault.h>
#include <uk/thread.h>
#endif /* CONFIG_LIBUKSCHED && CONFIG_LIBUKNOFAULT */

#include "crashdump.h"

#define __CRASH_REBOOT_HINT(delay)					\
	"System rebooting in " #delay " seconds...\n"
#define _CRASH_REBOOT_HINT(delay)					\
	__CRASH_REBOOT_HINT(delay)
#define CRASH_REBOOT_HINT						\
	_CRASH_REBOOT_HINT(CONFIG_LIBUKDEBUG_CRASH_REBOOT_DELAY)

__bool _uk_crash_explicit;

#if CONFIG_LIBUKDEBUG_CRASH_SCREEN
static void crash_print_thread_info(void)
{
#if CONFIG_LIBUKSCHED && CONFIG_LIBUKNOFAULT
	struct uk_thread *current = uk_thread_current();
	const char *name;

	if (!current)
		return;

	if (uk_nofault_probe_r((__vaddr_t)current, sizeof(struct uk_thread),
			       0) != sizeof(struct uk_thread)) {
		crash_printk("Current thread information corrupted\n");
		return;
	}
	name = uk_nofault_probe_r((__vaddr_t)current->name, 1, 0) == 1
	       ? current->name
	       : "(corrupted)";

	crash_printk("Thread \"%s\"@%p\n", name, current);

#endif /* CONFIG_LIBUKSCHED && CONFIG_LIBUKNOFAULT */
}
#endif /* CONFIG_LIBUKDEBUG_CRASH_SCREEN */

__noreturn static void crash_shutdown(void)
{
#if CONFIG_LIBUKDEBUG_CRASH_ACTION_REBOOT
	__nsec until;
	__nsec now;

	now = ukplat_monotonic_clock();
	until = now + ukarch_time_sec_to_nsec(CONFIG_LIBUKDEBUG_CRASH_REBOOT_DELAY);

	if (until > 0) {
		crash_printk(CRASH_REBOOT_HINT);
		/* Interrupts are disabled. Just busy wait... */
		while (until > ukplat_monotonic_clock())
			; /* do nothing */
	}

	uk_pm_sysrestart();
#else /* !CONFIG_LIBUKDEBUG_CRASH_ACTION_REBOOT */
	uk_pm_syscrash();
#endif /* !CONFIG_LIBUKDEBUG_CRASH_ACTION_REBOOT */
}

UK_EVENT(UK_CRASH_EVENT);

__noreturn void _uk_crash(struct uk_lcpu_regs *regs,
			  struct uk_crash_descr *descr)
{
	struct uk_crash_event_param param;
#if HAVE_SMP
	static __s32 crash_cpu = -1;
	int cpu_id = uk_pcpuvar_current_get(uk_pcpuvar_cpu_id);
#endif /* HAVE_SMP */

	UK_ASSERT(regs);
	UK_ASSERT(descr);

	uk_lcpu_disable_irq();
	uk_preempt_disable();
	uk_lcpu_except_push_nested();

#if HAVE_SMP
	#warning The crash code does not support multicore systems

	/* Only let one CPU perform the crash */
	if (ukarch_compare_exchange_sync(&crash_cpu, -1, cpu_id) != cpu_id) {
		/* TODO: Finish SMP Support
		 * Freeze CPU or wait until the crash_cpu initiates a freeze
		 * (e.g., through IPI). For now, just busy wait.
		 */
		uk_lcpu_halt();
	}
#endif /* HAVE_SMP */

#if CONFIG_LIBUKDEBUG_CRASH_SCREEN
	crash_printk("Unikraft Crash - " STRINGIFY(UK_CODENAME)
		     " (" STRINGIFY(UK_FULLVERSION) ")\n");
	crash_print_thread_info();
	crash_printk("    _      \n");
	crash_printk("  cx xo    \n");
	crash_printk("  (|O|)/V  \n");
	crash_printk("           \n");
	crash_print_crashdump(regs);
#endif /* CONFIG_LIBUKDEBUG_CRASH_SCREEN */

	param.regs = regs;
	param.descr = descr;

	/* Ignore return value, we can't handle it anyway */
	uk_raise_event(UK_CRASH_EVENT, &param);

	/* Halt or reboot the system */
	crash_shutdown();
}

static int uk_crash_handler(void *data)
{
	struct uk_crash_descr descr;
	struct uk_lcpu_except_err_ctx *ctx;

	ctx = data;
	UK_ASSERT(ctx);

	uk_lcpu_disable_irq();
	uk_preempt_disable();

	/* Fill-in arch-specific event param from ctx */
	uk_crash_populate_descr(ctx, &descr);

	_uk_crash(uk_lcpu_except_err_ctx_get_regs(ctx),
		  &descr);
	UK_BUG(); /* noreturn */
}

UK_EVENT_HANDLER_PRIO(UK_LCPU_EXCEPT_EVENT_UNHANDLED, uk_crash_handler,
		      UK_PRIO_LATEST);
