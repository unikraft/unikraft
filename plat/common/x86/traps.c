/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Costin Lupu <costin.lupu@cs.pub.ro>
 *
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */
/* Ported from Mini-OS */

#include <uk/arch/lcpu.h>
#include <uk/plat/common/trace.h>
#include <x86/cpu.h>
#include <x86/traps.h>
#include <uk/print.h>
#include <uk/assert.h>
#include <uk/asmdump.h>

/* A general word of caution when writing trap handlers. The platform trap
 * entry code is set up to properly save general-purpose registers (e.g., rsi,
 * rdi, rax, r8, ...), but it does NOT save any floating-point or SSE/AVX
 * registers. (This would require figuring out in the trap handler code whether
 * these are available to not risk a #UD trap inside the trap handler itself.)
 * Hence, you need to be extra careful not to do anything that clobbers these
 * registers if you intend to return from the handler. This includes calling
 * other functions, which may clobber those registers.
 * Of course, if you end your trap handler with a UK_CRASH, knock yourself out,
 * it's not like the function you came from will ever have the chance to notice.
 */

/* Traps handled on both Xen and KVM */

DECLARE_TRAP_EC(divide_error,      "divide error")
DECLARE_TRAP   (debug,             "debug exception")
DECLARE_TRAP_EC(int3,              "int3")
DECLARE_TRAP_EC(overflow,          "overflow")
DECLARE_TRAP_EC(bounds,            "bounds")
DECLARE_TRAP_EC(invalid_op,        "invalid opcode")
DECLARE_TRAP_EC(no_device,         "device not available")
DECLARE_TRAP_EC(invalid_tss,       "invalid TSS")
DECLARE_TRAP_EC(no_segment,        "segment not present")
DECLARE_TRAP_EC(stack_error,       "stack segment")
DECLARE_TRAP   (coproc_error,      "coprocessor error")
DECLARE_TRAP_EC(alignment_check,   "alignment check")
DECLARE_TRAP_EC(machine_check,     "machine check")
DECLARE_TRAP   (simd_error,        "SIMD coprocessor error")


void do_unhandled_trap(int trapnr, char *str, struct __regs *regs,
		unsigned long error_code)
{
	uk_pr_crit("Unhandled Trap %d (%s), error code=0x%lx\n",
		   trapnr, str, error_code);
	uk_pr_info("Regs address %p\n", regs);
	/* TODO revisit when UK_CRASH will also dump the registers */
	dump_regs(regs);
	uk_asmdumpk(KLVL_CRIT, (void *) regs->rip, 8);
	UK_CRASH("Crashing\n");
}

static int handling_fault;

static void fault_prologue(void)
{
	/* If we are already handling a page fault, and got another one
	 * that means we faulted in pagetable walk. Continuing here would cause
	 * a recursive fault
	 */
	if (handling_fault == 1) {
		UK_CRASH("Page fault in pagetable walk "
			 "(access to invalid memory?).\n");
	}
	handling_fault++;
	barrier();
}

void do_gp_fault(struct __regs *regs, long error_code)
{
	fault_prologue();
	uk_pr_crit("GPF rip: %lx, error_code=%lx\n",
		   regs->rip, error_code);
	dump_regs(regs);
	stack_walk_for_frame(regs->rbp);
	uk_asmdumpk(KLVL_CRIT, (void *) regs->rip, 6);
	dump_mem(regs->rsp);
	dump_mem(regs->rbp);
	dump_mem(regs->rip);
	UK_CRASH("Crashing\n");
}

void do_page_fault(struct __regs *regs, unsigned long error_code)
{
	unsigned long addr = read_cr2();

	fault_prologue();
	uk_pr_crit("Page fault at linear address %lx, rip %lx, "
		   "regs %p, sp %lx, our_sp %p, code %lx\n",
		   addr, regs->rip, regs, regs->rsp, &addr, error_code);

	dump_regs(regs);
	stack_walk_for_frame(regs->rbp);
	uk_asmdumpk(KLVL_CRIT, (void *) regs->rip, 6);
	dump_mem(regs->rsp);
	dump_mem(regs->rbp);
	dump_mem(regs->rip);
	UK_CRASH("Crashing\n");
}
