/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#define _GNU_SOURCE
#include <signal.h>

#include <uk/syscall.h>

void pprocess_signal_arch_set_ucontext(struct ukarch_execenv *execenv,
				       ucontext_t *ucontext)
{
	UK_ASSERT(execenv);
	UK_ASSERT(ucontext);

	ucontext->uc_mcontext.gregs[REG_R8] = execenv->regs.r8;
	ucontext->uc_mcontext.gregs[REG_R9] = execenv->regs.r9;
	ucontext->uc_mcontext.gregs[REG_R10] = execenv->regs.r10;
	ucontext->uc_mcontext.gregs[REG_R11] = execenv->regs.r11;
	ucontext->uc_mcontext.gregs[REG_R12] = execenv->regs.r12;
	ucontext->uc_mcontext.gregs[REG_R13] = execenv->regs.r13;
	ucontext->uc_mcontext.gregs[REG_R14] = execenv->regs.r14;
	ucontext->uc_mcontext.gregs[REG_R15] = execenv->regs.r15;
	ucontext->uc_mcontext.gregs[REG_RDI] = execenv->regs.rdi;
	ucontext->uc_mcontext.gregs[REG_RSI] = execenv->regs.rsi;
	ucontext->uc_mcontext.gregs[REG_RBP] = execenv->regs.rbp;
	ucontext->uc_mcontext.gregs[REG_RBX] = execenv->regs.rbx;
	ucontext->uc_mcontext.gregs[REG_RDX] = execenv->regs.rdx;
	ucontext->uc_mcontext.gregs[REG_RAX] = execenv->regs.rax;
	ucontext->uc_mcontext.gregs[REG_RCX] = execenv->regs.rcx;
	ucontext->uc_mcontext.gregs[REG_RSP] = execenv->regs.rsp;
	ucontext->uc_mcontext.gregs[REG_RIP] = execenv->regs.rip;

	/* TODO Populate the rest of the context */
}

void pprocess_signal_arch_get_ucontext(ucontext_t *ucontext,
				       struct ukarch_execenv *execenv)
{
	UK_ASSERT(ucontext);
	UK_ASSERT(execenv);

	execenv->regs.r8 = ucontext->uc_mcontext.gregs[REG_R8];
	execenv->regs.r9 = ucontext->uc_mcontext.gregs[REG_R9];
	execenv->regs.r10 = ucontext->uc_mcontext.gregs[REG_R10];
	execenv->regs.r11 = ucontext->uc_mcontext.gregs[REG_R11];
	execenv->regs.r12 = ucontext->uc_mcontext.gregs[REG_R12];
	execenv->regs.r13 = ucontext->uc_mcontext.gregs[REG_R13];
	execenv->regs.r14 = ucontext->uc_mcontext.gregs[REG_R14];
	execenv->regs.r15 = ucontext->uc_mcontext.gregs[REG_R15];
	execenv->regs.rdi = ucontext->uc_mcontext.gregs[REG_RDI];
	execenv->regs.rsi = ucontext->uc_mcontext.gregs[REG_RSI];
	execenv->regs.rbp = ucontext->uc_mcontext.gregs[REG_RBP];
	execenv->regs.rbx = ucontext->uc_mcontext.gregs[REG_RBX];
	execenv->regs.rdx = ucontext->uc_mcontext.gregs[REG_RDX];
	execenv->regs.rax = ucontext->uc_mcontext.gregs[REG_RAX];
	execenv->regs.rcx = ucontext->uc_mcontext.gregs[REG_RCX];
	execenv->regs.rsp = ucontext->uc_mcontext.gregs[REG_RSP];
	execenv->regs.rip = ucontext->uc_mcontext.gregs[REG_RIP];

	/* TODO Populate the rest of the context */
}

