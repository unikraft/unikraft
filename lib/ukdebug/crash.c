/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Author(s): Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *
 * Copyright (c) 2021, Karlsruhe Institute of Technology. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <uk/crash.h>
#include <uk/config.h>
#include <uk/crashdump.h>
#include <uk/plat/lcpu.h>
#include <uk/plat/bootstrap.h>
#include <uk/plat/time.h>
#include <uk/arch/traps.h>
#include <uk/preempt.h>
#include <uk/event.h>
#include <uk/essentials.h>
#include <uk/arch/atomic.h>
#if defined(CONFIG_LIBUKSCHED) && defined(CONFIG_LIBUKNOFAULT)
#include <uk/nofault.h>
#include <uk/thread.h>
#endif /* CONFIG_LIBUKSCHED && CONFIG_LIBUKNOFAULT */

#include <stdarg.h>
#include <stddef.h>

#ifdef CONFIG_LIBUKDEBUG_CRASH_SCREEN
#define crsh_printk(fmt, ...)						\
	_uk_printk(KLVL_CRIT, UKLIBID_NONE, NULL, 0, fmt, ##__VA_ARGS__)
#define crsh_vprintk(fmt, ap)						\
	_uk_vprintk(KLVL_CRIT, UKLIBID_NONE, NULL, 0, fmt, ap)
#define crsh_crashdumpk(regs)						\
	_uk_crashdumpk(KLVL_CRIT, UKLIBID_NONE, NULL, 0, regs)

static void crsh_print_thread_info(void)
{
#if defined(CONFIG_LIBUKSCHED) && defined(CONFIG_LIBUKNOFAULT)
	struct uk_thread *current = uk_thread_current();
	const char *name;

	if (!current)
		return;

	if (uk_nofault_probe_r((__vaddr_t)current, sizeof(struct uk_thread),
			       0) != sizeof(struct uk_thread)) {
		crsh_printk("Current thread information corrupted\n");
		return;
	}
	name = uk_nofault_probe_r((__vaddr_t)current, 1, 0) == 1
	       ? current->name
	       : "(corrupted)";

	crsh_printk("Thread \"%s\"@%p\n", name, current);

#endif /* CONFIG_LIBUKSCHED && CONFIG_LIBUKNOFAULT */
}
#else /* CONFIG_LIBUKDEBUG_CRASH_SCREEN */
#define crsh_printk(fmt, ...)
#define crsh_vprintk(fmt, ap)
#define crsh_crashdumpk(regs)
#endif /* !CONFIG_LIBUKDEBUG_CRASH_SCREEN */

#define __CRSH_REBOOT_HINT(delay)					\
	"System rebooting in " #delay " seconds...\n"
#define _CRSH_REBOOT_HINT(delay)					\
	__CRSH_REBOOT_HINT(delay)
#define CRSH_REBOOT_HINT						\
	_CRSH_REBOOT_HINT(CONFIG_LIBUKDEBUG_CRASH_REBOOT_DELAY)

#ifdef CONFIG_LIBUKDEBUG_CRASH_ACTION_REBOOT
#define CRSH_SHUTDONW_ACTION "reboot"
#else /* CONFIG_LIBUKDEBUG_CRASH_ACTION_REBOOT */
#define CRSH_SHUTDONW_ACTION "halt"
#endif /* !CONFIG_LIBUKDEBUG_CRASH_ACTION_REBOOT */

#define CRSH_SHUTDOWN_WARNING						\
	"==========================================\n"			\
	"The system crashed. Continuing or stepping\n"			\
	"will " CRSH_SHUTDONW_ACTION " the system!\n"			\
	"==========================================\n"

__noreturn static void crsh_shutdown(void)
{
#ifdef CONFIG_LIBUKDEBUG_CRASH_ACTION_REBOOT
	__nsec until = CONFIG_LIBUKDEBUG_CRASH_REBOOT_DELAY;

	if (until > 0) {
		until += ukarch_time_sec_to_nsec(secs);

		crsh_printk(CRSH_REBOOT_HINT);

		/* Interrupts are disabled. Just busy spin... */
		while (until > ukplat_monotonic_clock()) {
			/* do nothing
			 */
		}
	}

	ukplat_restart();
#else /* CONFIG_LIBUKDEBUG_CRASH_ACTION_REBOOT */
	ukplat_crash();
#endif /* CONFIG_LIBUKDEBUG_CRASH_ACTION_REBOOT */
}

UK_EVENT(UK_EVENT_CRASH);

void _uk_crash(struct __regs *regs, struct uk_crash_description *description)
{
	struct uk_event_crash_parameter param;
#ifdef UKPLAT_LCPU_MULTICORE
	static __s32 crash_cpu = -1;
	int cpu_id = ukplat_lcpu_id();
#endif /* UKPLAT_LCPU_MULTICORE */
	va_list ap __maybe_unused;

	ukarch_push_nested_exceptions();
	ukplat_lcpu_disable_irq();
	uk_preempt_disable();

#ifdef UKPLAT_LCPU_MULTICORE
	#warning The crash code does not support multicore systems

	/* Only let one CPU perform the crash */
	if (ukarch_compare_exchange_sync(&crash_cpu, -1, cpu_id) != cpu_id) {
		/* TODO: Finish SMP Support
		 * Freeze CPU or wait until the crash_cpu initiates a freeze
		 * (e.g., through IPI). For now, just busy wait.
		 */
		ukplat_lcpu_halt();
	}
#endif /* UKPLAT_LCPU_MULTICORE */

#ifdef CONFIG_LIBUKDEBUG_CRASH_SCREEN
	crsh_printk("Unikraft crash - " STRINGIFY(UK_CODENAME)
		    " (" STRINGIFY(UK_FULLVERSION) ")\n");

	crsh_print_thread_info();
	crsh_crashdumpk(regs);

#endif /* CONFIG_LIBUKDEBUG_CRASH_SCREEN */

	param.regs = regs;
	param.descr = description;
	/* We ignore the return value, because we cannot handle it anyway. */
	uk_raise_event(UK_EVENT_CRASH, &param);

	/* Halt or reboot the system */
	crsh_shutdown();
}
