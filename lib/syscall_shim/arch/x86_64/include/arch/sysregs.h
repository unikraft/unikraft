/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_SYSCALL_H__
#error Do not include this header directly
#endif

#define UKARCH_SYSREGS_SIZE			16

#if !__ASSEMBLY__

#include <uk/essentials.h>

/* Architecture specific userland context */
struct ukarch_sysregs {
	/* The current value of %gs's gs_base register of the application.
	 * On syscall entry, this will be updated to hold the value of
	 * MSR_KERNEL_GS_BASE following a swapgs instruction.
	 */
	__uptr gs_base;

	__uptr fs_base;
};

UK_CTASSERT(sizeof(struct ukarch_sysregs) == UKARCH_SYSREGS_SIZE);

#if CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS
__uptr ukarch_sysregs_get_tlsp(struct ukarch_sysregs *sysregs);

void ukarch_sysregs_set_tlsp(struct ukarch_sysregs *sysregs, __uptr tlsp);
#endif /* CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS */

#endif /* !__ASSEMBLY__ */
