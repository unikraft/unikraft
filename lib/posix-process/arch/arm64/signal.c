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

	asm volatile ("mov x19, sp\n"  /* save uk sp */
		      "mov sp, %4\n"  /* switch to handler sp */
		      "mov x0, %0\n"  /* arg0 signum */
		      "mov x1, %1\n"  /* arg1 siginfo */
		      "mov x2, %2\n"  /* arg2 ucontext */
		      "blr %3\n"      /* call handler */
		      "mov sp, x19\n" /* restore uk sp */
		      :
		      : "r" ((unsigned long)signum), "r" (si),
		      "r" (ctx), "r" (handler), "r" (sp)
		      /* clobber: modified + caller-saved */
		      : "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7",
			"x8", "x9", "x10", "x11", "x12", "x13", "x14",
			"x15", "x16", "x17", "x18", "v0", "v1", "v2",
			"v3", "v4", "v5", "v6", "v7", "memory"
	);

	ukarch_sysctx_load(&uk_sysctx);
}
