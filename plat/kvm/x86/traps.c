/* SPDX-License-Identifier: ISC */
/*
 * Authors: Dan Williams
 *          Martin Lucina
 *          Felipe Huici <felipe.huici@neclab.eu>
 *          Florian Schmidt <florian.schmidt@neclab.eu>
 *
 * Copyright (c) 2015-2017 IBM
 * Copyright (c) 2016-2017 Docker, Inc.
 * Copyright (c) 2017 NEC Europe Ltd., NEC Corporation
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

#include <string.h>
#include <uk/essentials.h>
#include <uk/arch/lcpu.h>
#include <uk/arch/paging.h>
#include <uk/plat/lcpu.h>
#include <uk/plat/config.h>
#include <x86/desc.h>
#include <kvm-x86/traps.h>

/*
 * CPUs should get dedicated IRQ and exception stacks. We use the interrupt
 * stack table (IST) feature for switching to the stack. This means when an
 * IRQ or a critical exception (e.g., a double fault) occurs, the CPU
 * automatically switches to the stack pointer configured in the IST entry.
 * Since the IST is part of the 64-bit task state segment (TSS), we need one
 * TSS per CPU. The TSS in turn is referenced by the global descriptor
 * table (GDT). Consequently, we also need separate GDTs per CPU.
 *
 *  CPU ─────┐ per CPU
 *           ▼
 *  ┌──────────────────┐
 *  │       GDT        │     ┌──────────────────┐
 *  ├──────────────────┤  ┌─►│       TSS        │
 *  │   null segment   │  │  ├──────────────────┤    ┌────────────────────┐
 *  ├──────────────────┤  │  │   IST stack[0] ──┼───►│     IRQ Stack      ├─┐
 *  │    cs  segment   │  │  │   IST stack[1] ──┼────│                  │ │ │
 *  ├──────────────────┤  │  │   IST stack[2]   │    │                  │ │ │
 *  │    ds  segment   │  │  │        .         │    │                  │ │ │
 *  ├──────────────────┤  │  │        .         │    │                  ▼ │ │
 *  │   tss  segment   ├──┘  │        .         │    └─┬──────────────────┘ │
 *  └──────────────────┘     └──────────────────┘      └────────────────────┘
 */
__align(STACK_SIZE) /* IST1 */
char cpu_intr_stack[CONFIG_UKPLAT_LCPU_MAXCOUNT][STACK_SIZE];
__align(STACK_SIZE) /* IST2 */
char cpu_trap_stack[CONFIG_UKPLAT_LCPU_MAXCOUNT][STACK_SIZE];
__align(STACK_SIZE) /* IST3 */
char cpu_crit_stack[CONFIG_UKPLAT_LCPU_MAXCOUNT][STACK_SIZE];

static __align(8)
struct tss64 cpu_tss[CONFIG_UKPLAT_LCPU_MAXCOUNT];

static __align(8)
struct seg_desc32 cpu_gdt64[CONFIG_UKPLAT_LCPU_MAXCOUNT][GDT_NUM_ENTRIES];

static void gdt_init(__lcpuidx idx)
{
	volatile struct desc_table_ptr64 gdtptr; /* needs to be volatile so
						  * setting its values is not
						  * optimized out
						  */

	cpu_gdt64[idx][GDT_DESC_CODE].raw = GDT_DESC_CODE64_VAL;
	cpu_gdt64[idx][GDT_DESC_DATA].raw = GDT_DESC_DATA64_VAL;

	gdtptr.limit = sizeof(cpu_gdt64[idx]) - 1;
	gdtptr.base = (__u64) &cpu_gdt64[idx];

	__asm__ goto(
		/* Load the global descriptor table */
		"lgdt	%0\n"

		/* Perform a far return to enable the new CS */
		"leaq	%l[jump_to_new_cs](%%rip), %%rax\n"

		"pushq	%1\n"
		"pushq	%%rax\n"
		"lretq\n"
		:
		: "m"(gdtptr),
		  "i"(GDT_DESC_OFFSET(GDT_DESC_CODE))
		: "rax", "memory" : jump_to_new_cs);
jump_to_new_cs:

	__asm__ __volatile__(
		/* Update remaining segment registers */
		"movl	%0, %%es\n"
		"movl	%0, %%ss\n"
		"movl	%0, %%ds\n"

		/* Initialize fs and gs to 0 */
		"movl	%1, %%fs\n"
		"movl	%1, %%gs\n"
		:
		: "r"(GDT_DESC_OFFSET(GDT_DESC_DATA)),
		  "r"(0));
}

static void tss_init(__lcpuidx idx)
{
	struct seg_desc64 *tss_desc;

	cpu_tss[idx].ist[0] =
		(__u64) &cpu_intr_stack[idx][sizeof(cpu_intr_stack[idx])];
	cpu_tss[idx].ist[1] =
		(__u64) &cpu_trap_stack[idx][sizeof(cpu_trap_stack[idx])];
	cpu_tss[idx].ist[2] =
		(__u64) &cpu_crit_stack[idx][sizeof(cpu_crit_stack[idx])];

	tss_desc = (void *) &cpu_gdt64[idx][GDT_DESC_TSS_LO];
	tss_desc->limit_lo	= sizeof(cpu_tss[idx]);
	tss_desc->base_lo	= (__u64) &(cpu_tss[idx]);
	tss_desc->base_hi	= (__u64) &(cpu_tss[idx]) >> 24;
	tss_desc->type		= GDT_DESC_TYPE_TSS_AVAIL;
	tss_desc->p		= 1;

	__asm__ __volatile__(
		"ltr	%0\n"
		:
		: "r"((__u16) (GDT_DESC_OFFSET(GDT_DESC_TSS_LO))));
}

/* Declare the traps used only by this platform: */
DECLARE_TRAP_EC(nmi,           "NMI",                  NULL)
DECLARE_TRAP_EC(double_fault,  "double fault",         NULL)
DECLARE_TRAP_EC(virt_error,    "virtualization error", NULL)

static struct seg_gate_desc64 cpu_idt[IDT_NUM_ENTRIES] __align(8);
static struct desc_table_ptr64 idtptr;

static inline void idt_fillgate(unsigned int num, void *fun, unsigned int ist)
{
	struct seg_gate_desc64 *desc = &cpu_idt[num];

	/*
	 * All gates are interrupt gates, all handlers run with interrupts off.
	 */
	desc->offset_hi	= (__u64) fun >> 16;
	desc->offset_lo	= (__u64) fun & 0xffff;
	desc->selector	= IDT_DESC_OFFSET(IDT_DESC_CODE);
	desc->ist	= ist;
	desc->type	= IDT_DESC_TYPE_INTR;
	desc->dpl	= IDT_DESC_DPL_KERNEL;
	desc->p		= 1;
}

static void idt_init(void)
{
	/* Ensure that traps_table_init has been called */
	UK_ASSERT(idtptr.limit != 0);

	__asm__ __volatile__("lidt %0" :: "m" (idtptr));
}

void traps_table_init(void)
{
	/*
	 * Load trap vectors. All traps run on a dedicated trap stack, except
	 * critical and debug exceptions, which have a separate stack.
	 *
	 * NOTE: Nested IRQs/exceptions must not use IST as the CPU switches
	 * to the configured stack pointer irrespective of the fact that it may
	 * already run on the same stack. For example, if we would cause a
	 * page fault in the debug trap and both would use the same IST entry,
	 * the page fault handler corrupts the stack (of the debug trap) and
	 * the system eventually crashes when returning from the page fault
	 * handler.
	 */
#define FILL_TRAP_GATE(name, ist)					\
	extern void cpu_trap_##name(void);				\
	idt_fillgate(TRAP_##name, ASM_TRAP_SYM(name), ist)

	FILL_TRAP_GATE(divide_error,	2);
	FILL_TRAP_GATE(debug,		3); /* runs on IST3 (cpu_crit_stack) */
	FILL_TRAP_GATE(nmi,		3); /* runs on IST3 (cpu_crit_stack) */
	FILL_TRAP_GATE(int3,		3); /* runs on IST3 (cpu_crit_stack) */
	FILL_TRAP_GATE(overflow,	2);
	FILL_TRAP_GATE(bounds,		2);
	FILL_TRAP_GATE(invalid_op,	2);
	FILL_TRAP_GATE(no_device,	2);
	FILL_TRAP_GATE(double_fault,	3); /* runs on IST3 (cpu_crit_stack) */

	FILL_TRAP_GATE(invalid_tss,	2);
	FILL_TRAP_GATE(no_segment,	2);
	FILL_TRAP_GATE(stack_error,	2);
	FILL_TRAP_GATE(gp_fault,	2);
	FILL_TRAP_GATE(page_fault,	2);

	FILL_TRAP_GATE(coproc_error,	2);
	FILL_TRAP_GATE(alignment_check,	2);
	FILL_TRAP_GATE(machine_check,	3); /* runs on IST3 (cpu_crit_stack) */
	FILL_TRAP_GATE(simd_error,	2);
	FILL_TRAP_GATE(virt_error,	2);

	/*
	 * Load IRQ vectors. All IRQs run on IST1 (cpu_intr_stack).
	 */
#define FILL_IRQ_GATE(num, ist)						\
	extern void cpu_irq_##num(void);				\
	idt_fillgate(32 + num, cpu_irq_##num, ist)

	FILL_IRQ_GATE(0, 1);
	FILL_IRQ_GATE(1, 1);
	FILL_IRQ_GATE(2, 1);
	FILL_IRQ_GATE(3, 1);
	FILL_IRQ_GATE(4, 1);
	FILL_IRQ_GATE(5, 1);
	FILL_IRQ_GATE(6, 1);
	FILL_IRQ_GATE(7, 1);
	FILL_IRQ_GATE(8, 1);
	FILL_IRQ_GATE(9, 1);
	FILL_IRQ_GATE(10, 1);
	FILL_IRQ_GATE(11, 1);
	FILL_IRQ_GATE(12, 1);
	FILL_IRQ_GATE(13, 1);
	FILL_IRQ_GATE(14, 1);
	FILL_IRQ_GATE(15, 1);

	idtptr.limit = sizeof(cpu_idt) - 1;
	idtptr.base = (__u64) &cpu_idt;
}

void traps_lcpu_init(struct lcpu *this_lcpu)
{
	gdt_init(this_lcpu->idx);
	tss_init(this_lcpu->idx);
	idt_init();
}
