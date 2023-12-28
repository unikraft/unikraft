/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <signal.h>
#include <uk/arch/ctx.h>

void pprocess_signal_arch_jmp_handler(struct ukarch_execenv *execenv,
				      int signum, siginfo_t *si,
				      ucontext_t *ctx,
				      void *handler, void *sp)
{
	struct ukarch_sysctx uk_sysctx;

	ukarch_sysctx_store(&uk_sysctx);
	ukarch_sysctx_load(&execenv->sysctx);

	asm volatile ("movq    %%rsp, %%r12\t\n" /* save uk sp */
		      "movq    %4, %%rsp\t\n"    /* switch to handler sp */
		      "movq    %0, %%rdi\t\n"    /* arg0 signum */
		      "movq    %1, %%rsi\t\n"    /* arg1 siginfo */
		      "movq    %2, %%rdx\t\n"    /* arg2 ucontext */
		      "call    *%3\t\n"          /* call handler */
		      "movq    %%r12, %%rsp\t\n" /* restore uk sp */
		      :
		      : "r" ((unsigned long)signum), "r" (si),
			"r" (ctx), "r" (handler), "r" (sp)
		      /* clobber: modified + caller-saved */
		      : "rax", "rdi", "rsi", "rdx", "rcx", "r8", "r9", "r10",
			"r11", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5",
			"xmm6", "xmm7", "xmm8", "xmm9", "xmm10", "xmm11",
			"xmm12", "xmm13", "xmm14", "xmm15", "mm0", "mm1", "mm2",
			"mm3", "mm4", "mm5", "mm6", "mm6", "st", "st(1)",
			"st(2)", "st(3)", "st(4)", "st(5)", "st(6)", "st(7)",
			"memory"
			);

	ukarch_sysctx_load(&uk_sysctx);
}
