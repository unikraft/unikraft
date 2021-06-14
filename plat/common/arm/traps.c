/* SPDX-License-Identifier: ISC */
/*
 * Authors: Wei Chen <Wei.Chen@arm.com>
 *
 * Copyright (c) 2018 Arm Ltd.
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose with or without fee is hereby granted, provided
 * that the above copyright notice and this permission notice appear
 * in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <uk/arch/lcpu.h>
#include <uk/arch/types.h>
#include <arm/traps.h>
#include <arm/cpu_defs.h>
#include <stdint.h>
#include <string.h>
#include <uk/print.h>
#include <uk/assert.h>
#include <gic/gic-v2.h>

static const char *exception_modes[]= {
	"Synchronous Abort",
	"IRQ",
	"FIQ",
	"Error"
};

enum aarch64_trap {
	AARCH64_TRAP_INVALID_INSN,
	AARCH64_TRAP_DEBUG,
	AARCH64_TRAP_PAGE_FAULT,
	AARCH64_TRAP_BUS_ERROR,
	AARCH64_TRAP_MATH,
	AARCH64_TRAP_SECURITY,

	AARCH64_TRAP_MAX
};

/* Mapping of the fault status code (FSC) for instruction and data aborts to
 * trap type (either page fault or bus error). Zero means invalid. The map
 * takes 64 bytes but saves a ton of comparisons.
 */
#define NUM_ABORT_MAP_ENTRIES 64
static const __u8 abort_map[NUM_ABORT_MAP_ENTRIES] = {
	[ESR_ISS_ABRT_FSC_ADDR_L0]		= AARCH64_TRAP_PAGE_FAULT,
	[ESR_ISS_ABRT_FSC_ADDR_L1]		= AARCH64_TRAP_PAGE_FAULT,
	[ESR_ISS_ABRT_FSC_ADDR_L2]		= AARCH64_TRAP_PAGE_FAULT,
	[ESR_ISS_ABRT_FSC_ADDR_L3]		= AARCH64_TRAP_PAGE_FAULT,
	[ESR_ISS_ABRT_FSC_TRANS_L0]		= AARCH64_TRAP_PAGE_FAULT,
	[ESR_ISS_ABRT_FSC_TRANS_L1]		= AARCH64_TRAP_PAGE_FAULT,
	[ESR_ISS_ABRT_FSC_TRANS_L2]		= AARCH64_TRAP_PAGE_FAULT,
	[ESR_ISS_ABRT_FSC_TRANS_L3]		= AARCH64_TRAP_PAGE_FAULT,
	[ESR_ISS_ABRT_FSC_ACCF_L0]		= AARCH64_TRAP_PAGE_FAULT,
	[ESR_ISS_ABRT_FSC_ACCF_L1]		= AARCH64_TRAP_PAGE_FAULT,
	[ESR_ISS_ABRT_FSC_ACCF_L2]		= AARCH64_TRAP_PAGE_FAULT,
	[ESR_ISS_ABRT_FSC_ACCF_L3]		= AARCH64_TRAP_PAGE_FAULT,
	[ESR_ISS_ABRT_FSC_PERM_L0]		= AARCH64_TRAP_PAGE_FAULT,
	[ESR_ISS_ABRT_FSC_PERM_L1]		= AARCH64_TRAP_PAGE_FAULT,
	[ESR_ISS_ABRT_FSC_PERM_L2]		= AARCH64_TRAP_PAGE_FAULT,
	[ESR_ISS_ABRT_FSC_PERM_L3]		= AARCH64_TRAP_PAGE_FAULT,
	[ESR_ISS_ABRT_FSC_SYNC]			= AARCH64_TRAP_BUS_ERROR,
	[ESR_ISS_ABRT_FSC_TAG]			= AARCH64_TRAP_PAGE_FAULT,
	[ESR_ISS_ABRT_FSC_SYNC_PT_LM1]		= AARCH64_TRAP_BUS_ERROR,
	[ESR_ISS_ABRT_FSC_SYNC_PT_L0]		= AARCH64_TRAP_BUS_ERROR,
	[ESR_ISS_ABRT_FSC_SYNC_PT_L1]		= AARCH64_TRAP_BUS_ERROR,
	[ESR_ISS_ABRT_FSC_SYNC_PT_L2]		= AARCH64_TRAP_BUS_ERROR,
	[ESR_ISS_ABRT_FSC_SYNC_PT_L3]		= AARCH64_TRAP_BUS_ERROR,
	[ESR_ISS_ABRT_FSC_SYNC_ECC]		= AARCH64_TRAP_BUS_ERROR,
	[ESR_ISS_ABRT_FSC_SYNC_ECC_PT_LM1]	= AARCH64_TRAP_BUS_ERROR,
	[ESR_ISS_ABRT_FSC_SYNC_ECC_PT_L0]	= AARCH64_TRAP_BUS_ERROR,
	[ESR_ISS_ABRT_FSC_SYNC_ECC_PT_L1]	= AARCH64_TRAP_BUS_ERROR,
	[ESR_ISS_ABRT_FSC_SYNC_ECC_PT_L2]	= AARCH64_TRAP_BUS_ERROR,
	[ESR_ISS_ABRT_FSC_SYNC_ECC_PT_L3]	= AARCH64_TRAP_BUS_ERROR,
	[ESR_ISS_ABRT_FSC_ALIGN]		= AARCH64_TRAP_BUS_ERROR,
	[ESR_ISS_ABRT_FSC_ADDR_LM1]		= AARCH64_TRAP_PAGE_FAULT,
	[ESR_ISS_ABRT_FSC_TRANS_LM1]		= AARCH64_TRAP_PAGE_FAULT
};

DECLARE_TRAP_EVENT(TRAP_INVALID_INSN);
DECLARE_TRAP_EVENT(TRAP_DEBUG);
DECLARE_TRAP_EVENT(TRAP_PAGE_FAULT);
DECLARE_TRAP_EVENT(TRAP_BUS_ERROR);
DECLARE_TRAP_EVENT(TRAP_MATH);
DECLARE_TRAP_EVENT(TRAP_SECURITY);

static const struct {
	const struct uk_event *event;
	const char *str;
	int signr;
} _trap_table[AARCH64_TRAP_MAX] = {
	{ UK_EVENT_PTR(TRAP_INVALID_INSN), "invalid insn",    4 /* SIGILL  */ },
	{ UK_EVENT_PTR(TRAP_DEBUG),        "debug",           5 /* SIGTRAP */ },
	{ UK_EVENT_PTR(TRAP_PAGE_FAULT),   "page fault",     11 /* SIGSEGV */ },
	{ UK_EVENT_PTR(TRAP_BUS_ERROR),    "bus error",       7 /* SIGBUS  */ },
	{ UK_EVENT_PTR(TRAP_MATH),         "floating point",  8 /* SIGFPE  */ },
	{ UK_EVENT_PTR(TRAP_SECURITY),     "security",        4 /* SIGILL  */ }
};

static enum aarch64_trap esr_to_trap(unsigned long esr)
{
	__u8 fsc, trap;

	/* We expect Unikraft to run in EL1. So do not catch traps from EL0. */
	switch (ESR_EC_FROM(esr)) {
	case ESR_EL1_EC_MMU_IABRT_EL1:
	case ESR_EL1_EC_MMU_DABRT_EL1:
		fsc = ESR_ISS_ABRT_FSC_FROM(ESR_ISS_FROM(esr));
		UK_ASSERT(fsc < NUM_ABORT_MAP_ENTRIES);

		trap = abort_map[fsc];
		if (trap == 0)
			break; /* Neither page fault, nor bus error */

		return trap;

	case ESR_EL1_EC_PC_ALIGN:
	case ESR_EL1_EC_SP_ALIGN:
		return AARCH64_TRAP_BUS_ERROR;

	case ESR_EL1_EC_SVE_ASIMD_FP_ACC:
	case ESR_EL1_EC_SVE_ACC:
	case ESR_EL1_EC_FP64:
		return AARCH64_TRAP_MATH;

	case ESR_EL1_EC_UNKNOWN:
	case ESR_EL1_EC_ILL:
		return AARCH64_TRAP_INVALID_INSN;

	case ESR_EL1_EC_BTI:
	case ESR_EL1_EC_FPAC:
		return AARCH64_TRAP_SECURITY;

	case ESR_EL1_EC_BRK_EL1:
	case ESR_EL1_EC_STEP_EL1:
	case ESR_EL1_EC_WATCHP_EL1:
	case ESR_EL1_EC_BRK64:
		return AARCH64_TRAP_DEBUG;
	}

	return AARCH64_TRAP_MAX;
}

static void dump_registers(struct __regs *regs, uint64_t far)
{
	unsigned char idx;

	uk_pr_crit("Unikraft: Dump registers:\n");
	uk_pr_crit("\t SP       : 0x%016lx\n", regs->sp);
	uk_pr_crit("\t ESR_EL1  : 0x%016lx\n", regs->esr_el1);
	uk_pr_crit("\t ELR_EL1  : 0x%016lx\n", regs->elr_el1);
	uk_pr_crit("\t LR (x30) : 0x%016lx\n", regs->lr);
	uk_pr_crit("\t PSTATE   : 0x%016lx\n", regs->spsr_el1);
	uk_pr_crit("\t FAR_EL1  : 0x%016lx\n", far);

	for (idx = 0; idx < 28; idx += 4)
		uk_pr_crit("\t x%02d ~ x%02d: 0x%016lx 0x%016lx 0x%016lx 0x%016lx\n",
			   idx, idx + 3, regs->x[idx], regs->x[idx + 1],
			   regs->x[idx + 2], regs->x[idx + 3]);

	uk_pr_crit("\t x28 ~ x29: 0x%016lx 0x%016lx\n",
		   regs->x[28], regs->x[29]);
}

void do_unhandled_trap(struct __regs *regs, uint32_t el,
				uint32_t reason, uint64_t far)
{
	uk_pr_crit("Unikraft: EL%d invalid %s trap caught\n",
		   el, exception_modes[reason]);
	dump_registers(regs, far);
	ukplat_crash();
}

void do_el1_sync(struct __regs *regs, uint64_t far)
{
	struct ukarch_trap_ctx ctx = {regs, regs->esr_el1, 1, 0, far};
	enum aarch64_trap trap = esr_to_trap(regs->esr_el1);

	if (trap < AARCH64_TRAP_MAX) {
		if (uk_raise_event_ptr(_trap_table[trap].event, &ctx))
			return;
	}

	uk_pr_crit("Unikraft: EL1 sync trap caught\n");

	dump_registers(regs, far);
	ukplat_crash();
}

void do_el1_irq(void)
{
	gic_handle_irq();
}
