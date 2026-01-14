/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/print.h>
#include <uk/syscall.h>

#define ARCH_SET_GS				0x1001
#define ARCH_SET_FS				0x1002
#define ARCH_GET_FS				0x1003
#define ARCH_GET_GS				0x1004

UK_LLSYSCALL_R_E_DEFINE(long, arch_prctl, long, code, long, addr, long, arg2)
{
	switch (code) {
	case ARCH_SET_GS:
		uk_pr_debug("arch_prctl option SET_GS(%p)\n", (void *)addr);
		uk_lcpu_sysctx_set(execenv->sysctx, GSBASE, (__uptr)addr);
		break;
	case ARCH_SET_FS:
		uk_pr_debug("arch_prctl option SET_FS(%p)\n", (void *)addr);
		uk_lcpu_sysctx_set(execenv->sysctx, FSBASE, (__uptr)addr);
		break;
	case ARCH_GET_GS:
		uk_pr_debug("arch_prctl option GET_GS(%p)\n", (void *)addr);
		if (unlikely(!addr))
			return -EFAULT;
		*((long *)addr) = uk_lcpu_sysctx_get(execenv->sysctx, GSBASE);
		break;
	case ARCH_GET_FS:
		uk_pr_debug("arch_prctl option GET_FS(%p)\n", (void *)addr);
		if (unlikely(!addr))
			return -EFAULT;
		*((long *)addr) = uk_lcpu_sysctx_get(execenv->sysctx, FSBASE);
		break;
	default:
		uk_pr_debug("arch_prctl option code 0x%lx ignored\n", code);
		return -EINVAL;
	}

	return 0;
}

#if UK_LIBC_SYSCALLS
int arch_prctl(int code, void *addr)
{
	return uk_syscall_e_arch_prctl((long)code, (long)addr, 0x0);
}
#endif /* UK_LIBC_SYSCALLS */
