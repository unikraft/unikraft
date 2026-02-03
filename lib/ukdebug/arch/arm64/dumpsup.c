/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, Karlsruhe Institute of Technology. All rights reserved.
 * Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/essentials.h>
#include <uk/lcpu.h>

#if CONFIG_LIBUKNOFAULT
#include <uk/nofault.h>
#endif /* CONFIG_LIBUKNOFAULT */

#include "../../crashdump.h"

void cdmp_arch_print_registers(struct uk_lcpu_regs *regs)
{
	crash_printk("Registers:\n");

	crash_printk("X00: %016lx X01: %016lx\n",
		     uk_lcpu_regs_get(regs, X0),
		     uk_lcpu_regs_get(regs, X1));
	crash_printk("X02: %016lx X03: %016lx\n",
		     uk_lcpu_regs_get(regs, X2),
		     uk_lcpu_regs_get(regs, X3));
	crash_printk("X04: %016lx X05: %016lx\n",
		     uk_lcpu_regs_get(regs, X4),
		     uk_lcpu_regs_get(regs, X5));
	crash_printk("X06: %016lx X07: %016lx\n",
		     uk_lcpu_regs_get(regs, X6),
		     uk_lcpu_regs_get(regs, X7));
	crash_printk("X08: %016lx X09: %016lx\n",
		     uk_lcpu_regs_get(regs, X8),
		     uk_lcpu_regs_get(regs, X9));
	crash_printk("X10: %016lx X11: %016lx\n",
		     uk_lcpu_regs_get(regs, X10),
		     uk_lcpu_regs_get(regs, X11));
	crash_printk("X12: %016lx X13: %016lx\n",
		     uk_lcpu_regs_get(regs, X12),
		     uk_lcpu_regs_get(regs, X13));
	crash_printk("X14: %016lx X15: %016lx\n",
		     uk_lcpu_regs_get(regs, X14),
		     uk_lcpu_regs_get(regs, X15));
	crash_printk("X16: %016lx X17: %016lx\n",
		     uk_lcpu_regs_get(regs, X16),
		     uk_lcpu_regs_get(regs, X17));
	crash_printk("X18: %016lx X19: %016lx\n",
		     uk_lcpu_regs_get(regs, X18),
		     uk_lcpu_regs_get(regs, X19));
	crash_printk("X20: %016lx X21: %016lx\n",
		     uk_lcpu_regs_get(regs, X20),
		     uk_lcpu_regs_get(regs, X21));
	crash_printk("X22: %016lx X23: %016lx\n",
		     uk_lcpu_regs_get(regs, X22),
		     uk_lcpu_regs_get(regs, X23));
	crash_printk("X24: %016lx X25: %016lx\n",
		     uk_lcpu_regs_get(regs, X24),
		     uk_lcpu_regs_get(regs, X25));
	crash_printk("X26: %016lx X27: %016lx\n",
		     uk_lcpu_regs_get(regs, X26),
		     uk_lcpu_regs_get(regs, X27));
	crash_printk("X28: %016lx X29: %016lx\n",
		     uk_lcpu_regs_get(regs, X28),
		     uk_lcpu_regs_get(regs, X29));

	crash_printk("SP:      0x%016lx\n", uk_lcpu_regs_get(regs, SP));
	crash_printk("ESR_EL1: 0x%016lx\n", uk_lcpu_regs_get(regs, ESR_EL1));
	crash_printk("ELR_EL1: 0x%016lx\n", uk_lcpu_regs_get(regs, ELR_EL1));
	crash_printk("LR:      0x%016lx\n", uk_lcpu_regs_get(regs, LR));
	crash_printk("PSTATE:  0x%016lx\n", uk_lcpu_regs_get(regs, SPSR_EL1));
}

#if CONFIG_LIBUKDEBUG_CRASH_PRINT_STACK
void cdmp_arch_print_stack(struct uk_lcpu_regs *regs)
{
	/* Nothing special to be done. Just call the generic version */
	cdmp_gen_print_stack(uk_lcpu_regs_get(regs, SP));
}
#endif /* CONFIG_LIBUKDEBUG_CRASH_PRINT_STACK */

#if CONFIG_LIBUKDEBUG_CRASH_PRINT_CALL_TRACE && !__OMIT_FRAMEPOINTER__
void cdmp_arch_print_call_trace(struct uk_lcpu_regs *regs)
{
	__u64 fp = uk_lcpu_regs_get(regs, X29);
	__u64 *frame;
	int depth_left = 32;
	__sz probe_len = sizeof(unsigned long) * 2;

	crash_printk("Call Trace:\n");

	cdmp_gen_print_call_trace_entry(uk_lcpu_regs_get(regs, ELR_EL1));

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
