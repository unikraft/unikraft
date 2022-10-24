/* SPDX-License-Identifier: MIT */
/******************************************************************************
 * panic.c
 *
 * Displays a register dump and stack trace for debugging.
 *
 * Copyright (c) 2014, Thomas Leonard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <xen-arm/os.h>
#include <uk/print.h>
#include <uk/essentials.h>

extern int irqstack[];
extern int irqstack_end[];

typedef void handler(void);

extern handler fault_reset;
extern handler fault_undefined_instruction;
extern handler fault_svc;
extern handler fault_prefetch_call;
extern handler fault_prefetch_abort;
extern handler fault_data_abort;

#if defined(__aarch64__)
void dump_register(int *saved_registers __unused)
{
	/* TODO: Point to ukarch_dump_registers */
}

extern void (*IRQ_handler)(void);

/*
 * TODO
 */
void stack_walk(void)
{
	uk_pr_info("stack walk\n");
	while (1)
		;
}

/*
 * TODO
 */
void do_bad_mode(struct pt_regs *regs, int reason)
{
	BUG();
}

void do_irq(struct pt_regs *regs)
{
	if (IRQ_handler)
		IRQ_handler();
}

/*
 * TODO
 */
void do_sync(struct pt_regs *regs)
{
	uint64_t esr, far;

	uk_pr_info("*** Sync exception at SP_EL0 = %lx ***\n", regs->sp_el0);
	uk_pr_info("Thread state:\n");
	uk_pr_info("\tX0  = 0x%016lx X1  = 0x%016lx\n", regs->x[0], regs->x[1]);
	uk_pr_info("\tX2  = 0x%016lx X3  = 0x%016lx\n", regs->x[2], regs->x[3]);
	uk_pr_info("\tX4  = 0x%016lx X5  = 0x%016lx\n", regs->x[4], regs->x[5]);
	uk_pr_info("\tX6  = 0x%016lx X7  = 0x%016lx\n", regs->x[6], regs->x[7]);
	uk_pr_info("\tX8  = 0x%016lx X9  = 0x%016lx\n", regs->x[8], regs->x[9]);
	uk_pr_info("\tX10 = 0x%016lx X11 = 0x%016lx\n",
			   regs->x[10], regs->x[11]);
	uk_pr_info("\tX12 = 0x%016lx X13 = 0x%016lx\n",
			   regs->x[12], regs->x[13]);
	uk_pr_info("\tX14 = 0x%016lx X15 = 0x%016lx\n",
			   regs->x[14], regs->x[15]);
	uk_pr_info("\tX16 = 0x%016lx X17 = 0x%016lx\n",
			   regs->x[16], regs->x[17]);
	uk_pr_info("\tX18 = 0x%016lx X19 = 0x%016lx\n",
			   regs->x[18], regs->x[19]);
	uk_pr_info("\tX20 = 0x%016lx X21 = 0x%016lx\n",
			   regs->x[20], regs->x[21]);
	uk_pr_info("\tX22 = 0x%016lx X23 = 0x%016lx\n",
			   regs->x[22], regs->x[23]);
	uk_pr_info("\tX24 = 0x%016lx X25 = 0x%016lx\n",
			   regs->x[24], regs->x[25]);
	uk_pr_info("\tX26 = 0x%016lx X27 = 0x%016lx\n",
			   regs->x[26], regs->x[27]);
	uk_pr_info("\tX28 = 0x%016lx X29 = 0x%016lx\n",
			   regs->x[28], regs->x[19]);
	uk_pr_info("\tX30 (lr) = 0x%016lx\n", regs->lr);
	uk_pr_info("\tsp  = 0x%016lx\n", regs->sp);
	uk_pr_info("\tpstate  = 0x%016lx\n", regs->spsr_el1);

	__asm__ __volatile__("mrs %0, esr_el1":"=r"(esr));
	__asm__ __volatile__("mrs %0, far_el1":"=r"(far));
	uk_pr_info("\tesr_el1 = %08lx\n", esr);
	uk_pr_info("\tfar_el1 = %08lx\n", far);
	while (1)
		;
}

/*
 * TODO: need implementation.
 */
void trap_init(void)
{
}

/*
 * TODO
 */
void trap_fini(void)
{
}
#endif /* __aarch64__ */
