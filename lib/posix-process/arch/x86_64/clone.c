/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <string.h>
#include <uk/plat/common/cpu.h>
#include <uk/process.h>
#include <uk/arch/ctx.h>

void clone_setup_child_ctx(struct ukarch_execenv *pexecenv,
			   struct uk_thread *child, __uptr sp)
{
	struct ukarch_execenv *cexecenv;
	__uptr auxsp_pos;

	UK_ASSERT(pexecenv);
	UK_ASSERT(child);
	UK_ASSERT(sp);

	auxsp_pos = ukarch_auxsp_get_curr_frame(child->auxsp);

	/* Create a child context whose stack pointer is that of the auxiliary
	 * stack, minus the parent's `struct ukarch_execenv` saved on the
	 * auxiliary stack that we will have to first patch now and then pop off
	 */

	/* Make room for child's copy of `struct ukarch_execenv` */
	auxsp_pos = ALIGN_DOWN(auxsp_pos, UKARCH_EXECENV_END_ALIGN);
	auxsp_pos -= UKARCH_EXECENV_SIZE;

	/* Now patch the child's return registers */
	cexecenv = (struct ukarch_execenv *)auxsp_pos;
	*cexecenv = *pexecenv;

	/* Child must see %rax as 0 */
	cexecenv->regs.rax = 0x0;

	/* Make sure we have interrupts enabled, as this is supposedly a normal
	 * userspace thread - the other flags don't really matter since the
	 * first thing the child does is compare %rax to 0x0.
	 */
	cexecenv->regs.eflags |= X86_EFLAGS_IF;

	/* Finally, make sure we do return to what the child is expected to
	 * have as an instruction pointer as well as a stack pointer.
	 */
	cexecenv->regs.rip = pexecenv->regs.rip;
	cexecenv->regs.rsp = sp;

	/* Use parent's userland gs_base */
	cexecenv->sysctx.gs_base = pexecenv->sysctx.gs_base;

	/* Use parent's fs_base if clone did not have SETTLS */
	if (!child->tlsp)
		cexecenv->sysctx.fs_base = pexecenv->sysctx.fs_base;
	else
		cexecenv->sysctx.fs_base = child->tlsp;

	ukarch_ctx_init_entry1(&child->ctx,
			       auxsp_pos,
			       1,
			       (ukarch_ctx_entry1)&ukarch_execenv_load,
			       auxsp_pos);
}
