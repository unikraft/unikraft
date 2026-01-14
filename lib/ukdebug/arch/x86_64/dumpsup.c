/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, Karlsruhe Institute of Technology. All rights reserved.
 * Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/essentials.h>
#include <uk/lcpu.h>
#include <uk/print.h>

#if CONFIG_LIBUKNOFAULT
#include <uk/nofault.h>
#endif /* CONFIG_LIBUKNOFAULT */

#include "../../crashdump.h"

void cdmp_arch_print_registers(struct uk_lcpu_regs *regs)
{
	crash_printk("Registers:\n");
	crash_printk(" rip: %04lx:%016lx\n",
		     uk_lcpu_regs_get(regs, CS) & 0xffff,
		     uk_lcpu_regs_get(regs, RIP));
	crash_printk(" rsp: %04lx:%016lx eflags: %08lx orig_rax: %016lx\n",
		     uk_lcpu_regs_get(regs, SS) & 0xffff,
		     uk_lcpu_regs_get(regs, RSP),
		     uk_lcpu_regs_get(regs, RFLAGS) & 0xffffffff,
		     uk_lcpu_regs_get(regs, ORIG_RAX));
	crash_printk(" rax: %016lx rbx: %016lx rcx:%016lx\n",
		     uk_lcpu_regs_get(regs, RAX),
		     uk_lcpu_regs_get(regs, RBX),
		     uk_lcpu_regs_get(regs, RCX));
	crash_printk(" rdx: %016lx rsi: %016lx rdi:%016lx\n",
		     uk_lcpu_regs_get(regs, RDX),
		     uk_lcpu_regs_get(regs, RSI),
		     uk_lcpu_regs_get(regs, RDI));
	crash_printk(" rbp: %016lx r08: %016lx r09:%016lx\n",
		     uk_lcpu_regs_get(regs, RBP),
		     uk_lcpu_regs_get(regs, R8),
		     uk_lcpu_regs_get(regs, R9));
	crash_printk(" r10: %016lx r11: %016lx r12:%016lx\n",
		     uk_lcpu_regs_get(regs, R10),
		     uk_lcpu_regs_get(regs, R11),
		     uk_lcpu_regs_get(regs, R12));
	crash_printk(" r13: %016lx r14: %016lx r15:%016lx\n",
		     uk_lcpu_regs_get(regs, R13),
		     uk_lcpu_regs_get(regs, R14),
		     uk_lcpu_regs_get(regs, R15));
}

#if CONFIG_LIBUKDEBUG_CRASH_PRINT_STACK
void cdmp_arch_print_stack(struct uk_lcpu_regs *regs)
{
	/* Nothing special to be done. Just call the generic version */
	cdmp_gen_print_stack(uk_lcpu_regs_get(regs, RSP));
}
#endif /* CONFIG_LIBUKDEBUG_CRASH_PRINT_STACK */

#if CONFIG_LIBUKDEBUG_CRASH_PRINT_CALL_TRACE && !__OMIT_FRAMEPOINTER__
void cdmp_arch_print_call_trace(struct uk_lcpu_regs *regs)
{
	__sz probe_len __maybe_unused;
	unsigned long fp = uk_lcpu_regs_get(regs, RBP);
	unsigned long *frame;
	int depth_left = 32;

	probe_len = sizeof(unsigned long) * 2;

	crash_printk("Call Trace:\n");

	cdmp_gen_print_call_trace_entry(uk_lcpu_regs_get(regs, RIP));

	while (((frame = (void *)fp)) && (depth_left-- > 0)) {
#if CONFIG_LIBUKNOFAULT
		if (uk_nofault_probe_r(fp, probe_len, 0) != probe_len) {
			crash_printk(" Bad frame pointer\n");
			break;
		}
#endif /* CONFIG_LIBUKNOFAULT */

		/*
		 * Subtract one from the address to get the symbol for the
		 * function that contains the subroutine call. Calls at the end
		 * of a function that does not return otherwise deliver the name
		 * of the symbol which follows the call.
		 */
		cdmp_gen_print_call_trace_entry(frame[1] > 0 ?
						frame[1] - 1 :
						frame[1]);

		/* Goto next frame */
		fp = frame[0];
	}
}
#endif /* CONFIG_LIBUKDEBUG_CRASH_PRINT_CALL_TRACE && !__OMIT_FRAMEPOINTER__ */
