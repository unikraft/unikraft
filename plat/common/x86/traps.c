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

DECLARE_TRAP_EVENT(UKARCH_TRAP_INVALID_OP);
DECLARE_TRAP_EVENT(UKARCH_TRAP_DEBUG);
DECLARE_TRAP_EVENT(UKARCH_TRAP_PAGE_FAULT);
DECLARE_TRAP_EVENT(UKARCH_TRAP_BUS_ERROR);
DECLARE_TRAP_EVENT(UKARCH_TRAP_MATH);
DECLARE_TRAP_EVENT(UKARCH_TRAP_SECURITY);
DECLARE_TRAP_EVENT(UKARCH_TRAP_X86_GP);

DECLARE_TRAP_EC(divide_error,    "divide error",         UKARCH_TRAP_MATH)
DECLARE_TRAP   (debug,           "debug",                UKARCH_TRAP_DEBUG)
DECLARE_TRAP_EC(int3,            "int3",                 UKARCH_TRAP_DEBUG)
DECLARE_TRAP_EC(overflow,        "overflow",             NULL)
DECLARE_TRAP_EC(bounds,          "bounds",               NULL)
DECLARE_TRAP_EC(invalid_op,      "invalid opcode",       UKARCH_TRAP_INVALID_OP)
DECLARE_TRAP_EC(no_device,       "device not available", UKARCH_TRAP_MATH)
DECLARE_TRAP_EC(invalid_tss,     "invalid TSS",          NULL)
DECLARE_TRAP_EC(no_segment,      "segment not present",  UKARCH_TRAP_BUS_ERROR)
DECLARE_TRAP_EC(stack_error,     "stack segment",        UKARCH_TRAP_BUS_ERROR)
DECLARE_TRAP_EC(gp_fault,        "general protection",   UKARCH_TRAP_X86_GP)
DECLARE_TRAP   (coproc_error,    "coprocessor",          UKARCH_TRAP_MATH)
DECLARE_TRAP_EC(alignment_check, "alignment check",      UKARCH_TRAP_BUS_ERROR)
DECLARE_TRAP_EC(machine_check,   "machine check",        NULL)
DECLARE_TRAP   (simd_error,      "SIMD coprocessor",     UKARCH_TRAP_MATH)
DECLARE_TRAP_EC(security_error,  "control protection",   UKARCH_TRAP_SECURITY)

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

void do_page_fault(struct __regs *regs, unsigned long error_code)
{
	int rc;
	unsigned long vaddr = read_cr2();
	struct ukarch_trap_ctx ctx = {
		.regs = regs,
		.trapnr = TRAP_page_fault,
		.error_code = error_code,
		.lcpu = lcpu_get_current_in_except(),
		.fault_address = vaddr,
	};

	rc = uk_raise_event(UKARCH_TRAP_PAGE_FAULT, &ctx);
	if (unlikely(rc < 0))
		uk_pr_crit("page fault handler returned error: %d\n", rc);
	else if (rc)
		return;

	dump_regs(regs);
#if !__OMIT_FRAMEPOINTER__
	stack_walk_for_frame(regs->rbp);
#endif /* !__OMIT_FRAMEPOINTER__ */
	uk_asmdumpk(KLVL_CRIT, (void *) regs->rip, 6);
	dump_mem(regs->rsp);
	dump_mem(regs->rbp);
	dump_mem(regs->rip);
	UK_CRASH("Crashing\n");
}
