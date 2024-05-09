/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#include <uk/assert.h>
#include <uk/thread.h>
#include <uk/syscall.h>

void ukarch_sysregs_switch_uk(struct ukarch_sysregs *sysregs)
{
	UK_ASSERT(sysregs);

#if CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS
	ukarch_sysregs_switch_uk_tls(sysregs);
#endif /* CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS */
}

void ukarch_sysregs_switch_ul(struct ukarch_sysregs *sysregs)
{
	UK_ASSERT(sysregs);

#if CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS
	ukarch_sysregs_switch_ul_tls(sysregs);
#endif /* CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS */
}

#if CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS
__uptr ukarch_sysregs_get_tlsp(struct ukarch_sysregs *sysregs)
{
	UK_ASSERT(sysregs);

	return sysregs->tpidr_el0;
}

void ukarch_sysregs_set_tlsp(struct ukarch_sysregs *sysregs, __uptr tlsp)
{
	UK_ASSERT(sysregs);

	uk_pr_debug("System call updated userland TLS pointer register to %p (before: %p)\n",
		    (void *)sysregs->tpidr_el0, (void *)tlsp);

	sysregs->tpidr_el0 = tlsp;
}

void ukarch_sysregs_switch_uk_tls(struct ukarch_sysregs *sysregs)
{
	struct uk_thread *t = uk_thread_current();

	UK_ASSERT(sysregs);
	UK_ASSERT(t);

	sysregs->tpidr_el0 = ukplat_tlsp_get();
	ukplat_tlsp_set(t->uktlsp);
	t->tlsp = t->uktlsp;
}

void ukarch_sysregs_switch_ul_tls(struct ukarch_sysregs *sysregs)
{
	struct uk_thread *t = uk_thread_current();

	UK_ASSERT(sysregs);
	UK_ASSERT(t);

	ukplat_tlsp_set(sysregs->tpidr_el0);
	t->tlsp = sysregs->tpidr_el0;
}
#endif /* CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS */
