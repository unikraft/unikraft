/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#define _GNU_SOURCE
#include <signal.h>

#include <uk/arch/ctx.h>
#include <uk/lcpu.h>
#include <uk/syscall.h>

void pprocess_signal_arch_set_ucontext(struct ukarch_execenv *execenv,
				       ucontext_t *ucontext)
{
	UK_ASSERT(execenv);
	UK_ASSERT(ucontext);

	ucontext->uc_mcontext.gregs[REG_R8] = uk_lcpu_regs_get(execenv->regs,
							       R8);
	ucontext->uc_mcontext.gregs[REG_R9] = uk_lcpu_regs_get(execenv->regs,
							       R9);
	ucontext->uc_mcontext.gregs[REG_R10] = uk_lcpu_regs_get(execenv->regs,
								R10);
	ucontext->uc_mcontext.gregs[REG_R11] = uk_lcpu_regs_get(execenv->regs,
								R11);
	ucontext->uc_mcontext.gregs[REG_R12] = uk_lcpu_regs_get(execenv->regs,
								R12);
	ucontext->uc_mcontext.gregs[REG_R13] = uk_lcpu_regs_get(execenv->regs,
								R13);
	ucontext->uc_mcontext.gregs[REG_R14] = uk_lcpu_regs_get(execenv->regs,
								R14);
	ucontext->uc_mcontext.gregs[REG_R15] = uk_lcpu_regs_get(execenv->regs,
								R15);
	ucontext->uc_mcontext.gregs[REG_RDI] = uk_lcpu_regs_get(execenv->regs,
								RDI);
	ucontext->uc_mcontext.gregs[REG_RSI] = uk_lcpu_regs_get(execenv->regs,
								RSI);
	ucontext->uc_mcontext.gregs[REG_RBP] = uk_lcpu_regs_get(execenv->regs,
								RBP);
	ucontext->uc_mcontext.gregs[REG_RBX] = uk_lcpu_regs_get(execenv->regs,
								RBX);
	ucontext->uc_mcontext.gregs[REG_RDX] = uk_lcpu_regs_get(execenv->regs,
								RDX);
	ucontext->uc_mcontext.gregs[REG_RAX] = uk_lcpu_regs_get(execenv->regs,
								RAX);
	ucontext->uc_mcontext.gregs[REG_RCX] = uk_lcpu_regs_get(execenv->regs,
								RCX);
	ucontext->uc_mcontext.gregs[REG_RSP] = uk_lcpu_regs_get(execenv->regs,
								RSP);
	ucontext->uc_mcontext.gregs[REG_RIP] = uk_lcpu_regs_get(execenv->regs,
								RIP);

	/* TODO Populate the rest of the context */
}

void pprocess_signal_arch_get_ucontext(ucontext_t *ucontext,
				       struct ukarch_execenv *execenv)
{
	UK_ASSERT(ucontext);
	UK_ASSERT(execenv);

	uk_lcpu_regs_set(execenv->regs, R8,
			 ucontext->uc_mcontext.gregs[REG_R8]);
	uk_lcpu_regs_set(execenv->regs, R9,
			 ucontext->uc_mcontext.gregs[REG_R9]);
	uk_lcpu_regs_set(execenv->regs, R10,
			 ucontext->uc_mcontext.gregs[REG_R10]);
	uk_lcpu_regs_set(execenv->regs, R11,
			 ucontext->uc_mcontext.gregs[REG_R11]);
	uk_lcpu_regs_set(execenv->regs, R12,
			 ucontext->uc_mcontext.gregs[REG_R12]);
	uk_lcpu_regs_set(execenv->regs, R13,
			 ucontext->uc_mcontext.gregs[REG_R13]);
	uk_lcpu_regs_set(execenv->regs, R14,
			 ucontext->uc_mcontext.gregs[REG_R14]);
	uk_lcpu_regs_set(execenv->regs, R15,
			 ucontext->uc_mcontext.gregs[REG_R15]);
	uk_lcpu_regs_set(execenv->regs, RDI,
			 ucontext->uc_mcontext.gregs[REG_RDI]);
	uk_lcpu_regs_set(execenv->regs, RSI,
			 ucontext->uc_mcontext.gregs[REG_RSI]);
	uk_lcpu_regs_set(execenv->regs, RBP,
			 ucontext->uc_mcontext.gregs[REG_RBP]);
	uk_lcpu_regs_set(execenv->regs, RBX,
			 ucontext->uc_mcontext.gregs[REG_RBX]);
	uk_lcpu_regs_set(execenv->regs, RDX,
			 ucontext->uc_mcontext.gregs[REG_RDX]);
	uk_lcpu_regs_set(execenv->regs, RAX,
			 ucontext->uc_mcontext.gregs[REG_RAX]);
	uk_lcpu_regs_set(execenv->regs, RCX,
			 ucontext->uc_mcontext.gregs[REG_RCX]);
	uk_lcpu_regs_set(execenv->regs, RSP,
			 ucontext->uc_mcontext.gregs[REG_RSP]);
	uk_lcpu_regs_set(execenv->regs, RIP,
			 ucontext->uc_mcontext.gregs[REG_RIP]);

	/* TODO Populate the rest of the context */
}

