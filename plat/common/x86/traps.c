/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
 * Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
/* Ported from Mini-OS */

#include <uk/arch/lcpu.h>
#include <uk/assert.h>
#include <x86/cpu.h>
#include <x86/traps.h>
#include <uk/print.h>

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
DECLARE_TRAP_EC(page_fault,      "page fault",           UKARCH_TRAP_PAGE_FAULT)
DECLARE_TRAP   (coproc_error,    "coprocessor",          UKARCH_TRAP_MATH)
DECLARE_TRAP_EC(alignment_check, "alignment check",      UKARCH_TRAP_BUS_ERROR)
DECLARE_TRAP_EC(machine_check,   "machine check",        NULL)
DECLARE_TRAP   (simd_error,      "SIMD coprocessor",     UKARCH_TRAP_MATH)
DECLARE_TRAP_EC(security_error,  "control protection",   UKARCH_TRAP_SECURITY)

UK_EVENT(UK_EVENT_UNHANDLED_EXCEPTION);

void do_unhandled_trap(int trapnr, char *str, struct __regs *regs,
		unsigned long error_code)
{
	struct ukarch_trap_ctx ctx;
	unsigned long vaddr;
	int rc;

	vaddr = read_cr2(); /* valid conditionally to trap type */
	ctx = (struct ukarch_trap_ctx){regs, trapnr, str, error_code, 0, vaddr};

	rc = uk_raise_event(UK_EVENT_UNHANDLED_EXCEPTION, &ctx);
	if (unlikely(rc == UK_EVENT_NOT_HANDLED || rc < 0))
		uk_pr_crit("Unhandled Trap %d (%s), error code=0x%lx\n",
			   trapnr, str, error_code);
	/* We expect that if there is a handler it won't return,
	 * but as we can't control that, we place halt outside
	 * the above conditional.
	 */
	ukplat_lcpu_halt();
}
