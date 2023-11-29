/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Author(s): Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *
 * Copyright (c) 2021, Karlsruhe Institute of Technology. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "crashdump.h"
#include "outf.h"

#include <uk/nofault.h>

void cdmp_arch_print_registers(struct out_dev *o, struct __regs *regs)
{
	int i;

	outf(o, "SP:      0x%016lx\n", regs->sp);
	outf(o, "ESR_EL1: 0x%016lx\n", regs->esr_el1);
	outf(o, "ELR_EL1: 0x%016lx\n", regs->elr_el1);
	outf(o, "LR:      0x%016lx\n", regs->lr);
	outf(o, "PSTATE:  0x%016lx\n", regs->spsr_el1);

	for (i = 0; i < 30; i += 2) {
		outf(o, "X%-2d: %016lx X%-2d: %016lx\n",
		     i, regs->x[i], i + 1, regs->x[i + 1]);
	}
}

void cdmp_arch_print_stack(struct out_dev *o, struct __regs *regs)
{
	/* Nothing special to be done. Just call the generic version */
	cdmp_gen_print_stack(o, regs->sp);
}

#if !__OMIT_FRAMEPOINTER__
void cdmp_arch_print_call_trace(struct out_dev *o, struct __regs *regs)
{
	unsigned long fp = regs->x[29];
	unsigned long *frame;
	int depth_left = 32;
	size_t probe_len = sizeof(unsigned long) * 2;

	outf(o, "Call Trace:\n");

	cdmp_gen_print_call_trace_entry(o, regs->pc);

	while (((frame = (void*)fp)) && (depth_left-- > 0)) {
		if (uk_nofault_probe_r(fp, probe_len, 0) != probe_len) {
			outf(o, " Bad frame pointer\n");
			break;
		}

		cdmp_gen_print_call_trace_entry(o, frame[1]);

		/* Goto next frame */
		fp = frame[0];
	}
}
#endif /* !__OMIT_FRAMEPOINTER__ */
