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

void invalid_trap_handler(struct __regs *regs, uint32_t el,
				uint32_t reason, uint64_t far)
{
	uk_pr_crit("Unikraft: EL%d invalid %s trap caught\n",
		   el, exception_modes[reason]);
	dump_registers(regs, far);
	ukplat_crash();
}

void trap_el1_sync(struct __regs *regs, uint64_t far)
{
	uk_pr_crit("Unikraft: EL1 sync trap caught\n");

	dump_registers(regs, far);
	ukplat_crash();
}

void trap_el1_irq(void)
{
	gic_handle_irq();
}
