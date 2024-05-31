/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#define _GNU_SOURCE
#include <signal.h>

#include <uk/syscall.h>

void pprocess_signal_arch_set_ucontext(struct uk_syscall_ctx *usc,
				       ucontext_t *ucontext)
{
	UK_ASSERT(usc);
	UK_ASSERT(ucontext);

	ucontext->uc_mcontext.gregs[REG_R8] = usc->regs.r8;
	ucontext->uc_mcontext.gregs[REG_R9] = usc->regs.r9;
	ucontext->uc_mcontext.gregs[REG_R10] = usc->regs.r10;
	ucontext->uc_mcontext.gregs[REG_R11] = usc->regs.r11;
	ucontext->uc_mcontext.gregs[REG_R12] = usc->regs.r12;
	ucontext->uc_mcontext.gregs[REG_R13] = usc->regs.r13;
	ucontext->uc_mcontext.gregs[REG_R14] = usc->regs.r14;
	ucontext->uc_mcontext.gregs[REG_R15] = usc->regs.r15;
	ucontext->uc_mcontext.gregs[REG_RDI] = usc->regs.rdi;
	ucontext->uc_mcontext.gregs[REG_RSI] = usc->regs.rsi;
	ucontext->uc_mcontext.gregs[REG_RBP] = usc->regs.rbp;
	ucontext->uc_mcontext.gregs[REG_RBX] = usc->regs.rbx;
	ucontext->uc_mcontext.gregs[REG_RDX] = usc->regs.rdx;
	ucontext->uc_mcontext.gregs[REG_RAX] = usc->regs.rax;
	ucontext->uc_mcontext.gregs[REG_RCX] = usc->regs.rcx;
	ucontext->uc_mcontext.gregs[REG_RSP] = usc->regs.rsp;
	ucontext->uc_mcontext.gregs[REG_RIP] = usc->regs.rip;

	/* TODO Populate the rest of the context */
}

void pprocess_signal_arch_get_ucontext(ucontext_t *ucontext,
				       struct uk_syscall_ctx *usc)
{
	UK_ASSERT(ucontext);
	UK_ASSERT(usc);

	usc->regs.r8 = ucontext->uc_mcontext.gregs[REG_R8];
	usc->regs.r9 = ucontext->uc_mcontext.gregs[REG_R9];
	usc->regs.r10 = ucontext->uc_mcontext.gregs[REG_R10];
	usc->regs.r11 = ucontext->uc_mcontext.gregs[REG_R11];
	usc->regs.r12 = ucontext->uc_mcontext.gregs[REG_R12];
	usc->regs.r13 = ucontext->uc_mcontext.gregs[REG_R13];
	usc->regs.r14 = ucontext->uc_mcontext.gregs[REG_R14];
	usc->regs.r15 = ucontext->uc_mcontext.gregs[REG_R15];
	usc->regs.rdi = ucontext->uc_mcontext.gregs[REG_RDI];
	usc->regs.rsi = ucontext->uc_mcontext.gregs[REG_RSI];
	usc->regs.rbp = ucontext->uc_mcontext.gregs[REG_RBP];
	usc->regs.rbx = ucontext->uc_mcontext.gregs[REG_RBX];
	usc->regs.rdx = ucontext->uc_mcontext.gregs[REG_RDX];
	usc->regs.rax = ucontext->uc_mcontext.gregs[REG_RAX];
	usc->regs.rcx = ucontext->uc_mcontext.gregs[REG_RCX];
	usc->regs.rsp = ucontext->uc_mcontext.gregs[REG_RSP];
	usc->regs.rip = ucontext->uc_mcontext.gregs[REG_RIP];

	/* TODO Populate the rest of the context */
}

