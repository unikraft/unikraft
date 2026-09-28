/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/arch/x86_64.h>
#include <uk/arch/ctx.h>
#include <uk/essentials.h>

#include <string.h>

void execve_arch_execenv_init(struct ukarch_execenv *execenv_new,
			      struct ukarch_execenv *execenv,
			      __uptr ip, __uptr sp)
{
	UK_ASSERT(execenv_new);
	UK_ASSERT(execenv);
	UK_ASSERT(ip);
	UK_ASSERT(sp);
	UK_ASSERT(IS_ALIGNED(sp, UKARCH_SP_ALIGN));

	/* The new program starts with its registers zeroed but for those set
	 * below, as on Linux: the execenv sits on a stack fresh from the
	 * allocator, whose contents are stale, and libc's _start takes %rdx as
	 * an exit handler to register when it is not 0.
	 */
	memset(execenv_new->regs, 0, sizeof(execenv_new->regs));

	uk_lcpu_regs_set(execenv_new->regs, RIP, ip);
	uk_lcpu_regs_set(execenv_new->regs, RSP, sp);

	/* Prepare for iretq
	 * FIXME re-arch: use GDT macros once moved out of plat/common
	 */
	uk_lcpu_regs_set(execenv_new->regs, RFLAGS,
			 uk_lcpu_regs_get(execenv->regs, RFLAGS));

	uk_lcpu_regs_set(execenv_new->regs, CS,
			 UK_ARCH_X86_64_GDT_DESC_OFFSET(UK_ARCH_X86_64_GDT_DESC_CODE));
	uk_lcpu_regs_set(execenv_new->regs, SS,
			 UK_ARCH_X86_64_GDT_DESC_OFFSET(UK_ARCH_X86_64_GDT_DESC_DATA));

	/* The FPU and SSE state of a fresh process, as on Linux, not the
	 * exec'ing program's rounding and flush-to-zero modes.  With XSAVE, a
	 * zero header puts every component in its init state (x87: FCW
	 * 0x37f) and MXCSR is still read from offset 24; FXSAVE reads both
	 * control words from the legacy area.
	 */
	memset(execenv_new->ectx, 0, sizeof(execenv_new->ectx));
	*(__u16 *)&execenv_new->ectx[0] = 0x037f;
	*(__u32 *)&execenv_new->ectx[24] = 0x1f80;

	/* Also copy the current sysregs to avoid ending up with undefined
	 * values that trigger alignment errors.
	 */
	uk_lcpu_sysctx_store((struct uk_lcpu_sysctx *)execenv_new->sysctx);
}
