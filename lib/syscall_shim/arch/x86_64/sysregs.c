/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#include <uk/arch/lcpu.h>
#include <uk/assert.h>
#include <uk/plat/common/cpu.h>
#include <uk/thread.h>
#include <x86/gsbase.h>
#include <uk/syscall.h>

void ukarch_sysregs_switch_uk(struct ukarch_sysregs *sysregs)
{
	UK_ASSERT(sysregs);
	UK_ASSERT(lcpu_get_current());

	/* This can only be called from Unikraft ctx in bincompat mode.
	 * Therefore, X86_MSR_GS_BASE holds the current `struct lcpu` and
	 * X86_MSR_KERNEL_GS_BASE contains the app-saved gs_base.
	 */
	sysregs->gs_base = rdkgsbase();

#if CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS
	ukarch_sysregs_switch_uk_tls(sysregs);
#endif /* CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS */
}

void ukarch_sysregs_switch_ul(struct ukarch_sysregs *sysregs)
{
	UK_ASSERT(sysregs);
	UK_ASSERT(lcpu_get_current());

#if CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS
	ukarch_sysregs_switch_ul_tls(sysregs);
#endif /* CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS */

	/* This can only be called from Unikraft ctx in bincompat mode.
	 * Therefore, X86_MSR_GS_BASE holds the current `struct lcpu` and
	 * X86_MSR_KERNEL_GS_BASE contains the app-saved gs_base.
	 */
	wrgsbase((__uptr)lcpu_get_current());
	wrkgsbase(sysregs->gs_base);
}

#if CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS
__uptr ukarch_sysregs_get_tlsp(struct ukarch_sysregs *sysregs)
{
	UK_ASSERT(sysregs);

	return sysregs->fs_base;
}

void ukarch_sysregs_set_tlsp(struct ukarch_sysregs *sysregs, __uptr tlsp)
{
	UK_ASSERT(sysregs);

	uk_pr_debug("System call updated userland TLS pointer register to %p (before: %p)\n",
		    (void *)sysregs->fs_base, (void *)tlsp);

	sysregs->fs_base = tlsp;
}

void ukarch_sysregs_switch_uk_tls(struct ukarch_sysregs *sysregs)
{
	struct uk_thread *t = uk_thread_current();

	UK_ASSERT(sysregs);
	UK_ASSERT(t);

	sysregs->fs_base = ukplat_tlsp_get();
	ukplat_tlsp_set(t->uktlsp);
	t->tlsp = t->uktlsp;
}

void ukarch_sysregs_switch_ul_tls(struct ukarch_sysregs *sysregs)
{
	struct uk_thread *t = uk_thread_current();

	UK_ASSERT(sysregs);
	UK_ASSERT(t);

	ukplat_tlsp_set(sysregs->fs_base);
	t->tlsp = sysregs->fs_base;
}
#endif /* CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS */
