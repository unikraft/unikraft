/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, Karlsruhe Institute of Technology. All rights reserved.
 * Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/lcpu.h>
#include <uk/essentials.h>

#if CONFIG_LIBUKNOFAULT
#include <uk/nofault.h>
#endif /* CONFIG_LIBUKNOFAULT */

#include "../../crashdump.h"

void cdmp_arch_print_registers(struct uk_lcpu_regs *regs)
{
	crash_printk("Registers:\n");

	for (int i = 0; i < 30; i += 2) {
		crash_printk("X%-2d: %016lx X%-2d: %016lx\n",
			     i, regs->x[i], i + 1, regs->x[i + 1]);
	}

	crash_printk("SP:      0x%016lx\n", regs->sp);
	crash_printk("ESR_EL1: 0x%016lx\n", regs->esr_el1);
	crash_printk("ELR_EL1: 0x%016lx\n", regs->elr_el1);
	crash_printk("LR:      0x%016lx\n", regs->lr);
	crash_printk("PSTATE:  0x%016lx\n", regs->spsr_el1);
}

#if CONFIG_LIBUKDEBUG_CRASH_PRINT_STACK
void cdmp_arch_print_stack(struct uk_lcpu_regs *regs)
{
	/* Nothing special to be done. Just call the generic version */
	cdmp_gen_print_stack(regs->sp);
}
#endif /* CONFIG_LIBUKDEBUG_CRASH_PRINT_STACK */

#if CONFIG_LIBUKDEBUG_CRASH_PRINT_CALL_TRACE && !__OMIT_FRAMEPOINTER__
void cdmp_arch_print_call_trace(struct uk_lcpu_regs *regs)
{
	__u64 fp = regs->x[29];
	__u64 *frame;
	int depth_left = 32;
	__sz probe_len = sizeof(unsigned long) * 2;

	crash_printk("Call Trace:\n");

	cdmp_gen_print_call_trace_entry(regs->elr_el1);

	while (((frame = (void *)fp)) && (depth_left-- > 0)) {
#if CONFIG_LIBUKNOFAULT
		if (uk_nofault_probe_r(fp, probe_len, 0) != probe_len) {
			crash_printk(" Bad frame pointer\n");
			break;
		}
#endif /* CONFIG_LIBUKNOFAULT */

		cdmp_gen_print_call_trace_entry(frame[1]);

		/* Goto next frame */
		fp = frame[0];
	}
}
#endif /* CONFIG_LIBUKDEBUG_CRASH_PRINT_CALL_TRACE && !__OMIT_FRAMEPOINTER__ */
