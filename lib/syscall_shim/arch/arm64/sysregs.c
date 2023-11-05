/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#include <uk/assert.h>
#include <uk/thread.h>
#include <uk/syscall.h>

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
#endif /* CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS */
