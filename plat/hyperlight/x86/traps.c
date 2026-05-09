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
 * Copyright (c) 2024 Microsoft Corporation (Hyperlight adaptation)
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

#include <uk/arch/ctx.h>
#include <string.h>
#include <uk/essentials.h>
#include <uk/lcpu.h>
#include <uk/paging.h>
#include <uk/lcpu.h>
#include <uk/plat/config.h>


#include <hyperlight-x86/traps.h>

/*
 * Hyperlight is single-CPU, so we use simpler static structures.
 * We still need GDT, TSS, and IDT for proper exception handling.
 *
 * Stack layout for exception handling:
 *  CPU_EXCEPT_STACK_SIZE  CPU_EXCEPT_STACK_SIZE  CPU_EXCEPT_STACK_SIZE
 * <--------------------><---------------------><-------------------->
 * |=================================================================|
 * |                     |                     |                     |
 * |     crit stack      |       trap stack    |        IRQ stack    |
 * |                     |                     |                     |
 * |=================================================================|
 * ^
 * lcpu_except_stack
 */
static __align(UKARCH_SP_ALIGN)
__u8 lcpu_except_stack[3 * CPU_EXCEPT_STACK_SIZE];

static __align(8)
struct uk_arch_x86_64_tss64 cpu_tss;

static __align(8)
struct seg_desc32 cpu_gdt64[GDT_NUM_ENTRIES];

static void gdt_init(void)
{
	volatile struct desc_table_ptr64 gdtptr;

	cpu_gdt64[GDT_DESC_CODE].raw = GDT_DESC_CODE64_VAL;
	cpu_gdt64[GDT_DESC_DATA].raw = GDT_DESC_DATA64_VAL;

	gdtptr.limit = sizeof(cpu_gdt64) - 1;
	gdtptr.base = (__u64) &cpu_gdt64;

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

static void tss_init(void)
{
	struct uk_arch_x86_64_seg_desc64 *tss_desc;

	/* Configure interrupt stack table (IST) entries */
	cpu_tss.ist[0] = (__u64)&lcpu_except_stack[CPU_EXCEPT_STACK_SIZE * 3];
	cpu_tss.ist[1] = (__u64)&lcpu_except_stack[CPU_EXCEPT_STACK_SIZE * 2];
	cpu_tss.ist[2] = (__u64)&lcpu_except_stack[CPU_EXCEPT_STACK_SIZE];

	/* Set up TSS descriptor in GDT */
	tss_desc = (void *) &cpu_gdt64[GDT_DESC_TSS_LO];
	tss_desc->limit_lo	= sizeof(cpu_tss);
	tss_desc->base_lo	= (__u64) &cpu_tss;
	tss_desc->base_hi	= (__u64) &cpu_tss >> 24;
	tss_desc->type		= GDT_DESC_TYPE_TSS_AVAIL;
	tss_desc->p		= 1;

	__asm__ __volatile__(
		"ltr	%0\n"
		:
		: "r"((__u16) (GDT_DESC_OFFSET(GDT_DESC_TSS_LO))));
}

static __align(8)
struct uk_arch_x86_64_seg_gate_desc64 cpu_idt[IDT_NUM_ENTRIES];

static __u8 idt_ist_disable_nesting;

#define IDT_IST_SAVE_LEN 32
static __u8 idt_ist_saved[IDT_IST_SAVE_LEN];

void ukarch_push_nested_exceptions(void)
{
	struct uk_arch_x86_64_seg_gate_desc64 *desc;
	unsigned int i;

	if (idt_ist_disable_nesting >= __U8_MAX) {
		__asm__ volatile("cli; hlt");
		__builtin_unreachable();
	}

	if (idt_ist_disable_nesting++)
		return;

	/* Save the value of the IST field and disable IST for the exception */
	for (i = 0; i < IDT_IST_SAVE_LEN; i++) {
		desc = &cpu_idt[i];
		idt_ist_saved[i] = desc->ist;
		desc->ist = 0;
	}
}

void ukarch_pop_nested_exceptions(void)
{
	struct uk_arch_x86_64_seg_gate_desc64 *desc;
	unsigned int i;

	UK_ASSERT(idt_ist_disable_nesting > 0);

	if (--idt_ist_disable_nesting != 0)
		return;

	/* Restore the IST field values */
	for (i = 0; i < IDT_IST_SAVE_LEN; i++) {
		desc = &cpu_idt[i];
		desc->ist = idt_ist_saved[i];
	}
}

void ukarch_reset_nested_exceptions(void)
{
	struct uk_arch_x86_64_seg_gate_desc64 *desc;
	unsigned int i;

	/*
	 * After snapshot/restore the nesting counter may be non-zero
	 * (the crash handler called push but never got to pop before
	 * the snapshot was taken).  Force it back to 0 and restore
	 * the saved IST values so the next crash can push/pop cleanly.
	 */
	if (idt_ist_disable_nesting == 0)
		return;

	idt_ist_disable_nesting = 0;

	for (i = 0; i < IDT_IST_SAVE_LEN; i++) {
		desc = &cpu_idt[i];
		desc->ist = idt_ist_saved[i];
	}
}

/* Declare the traps used only by this platform: */
DECLARE_TRAP_EVENT(UKARCH_TRAP_NMI);

DECLARE_TRAP_EC(nmi,           "NMI",                  UKARCH_TRAP_NMI)
DECLARE_TRAP_EC(double_fault,  "double fault",         NULL)
DECLARE_TRAP_EC(virt_error,    "virtualization error", NULL)

static struct desc_table_ptr64 idtptr;

static inline void idt_fillgate(unsigned int num, void *fun, unsigned int ist)
{
	struct uk_arch_x86_64_seg_gate_desc64 *desc = &cpu_idt[num];

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
	 * IST entry usage:
	 * IST1 = IRQ stack
	 * IST2 = Trap stack
	 * IST3 = Critical exception stack (double fault, NMI, debug)
	 */
#define FILL_TRAP_GATE(name, ist)					\
	extern void cpu_trap_##name(void);				\
	idt_fillgate(TRAP_##name, ASM_TRAP_SYM(name), ist)

	FILL_TRAP_GATE(divide_error,	2);
	FILL_TRAP_GATE(debug,		3); /* on IST3 (crit stack) */
	FILL_TRAP_GATE(nmi,		3); /* on IST3 (crit stack) */
	FILL_TRAP_GATE(int3,		3); /* on IST3 (crit stack) */
	FILL_TRAP_GATE(overflow,	2);
	FILL_TRAP_GATE(bounds,		2);
	FILL_TRAP_GATE(invalid_op,	2);
	FILL_TRAP_GATE(no_device,	2);
	FILL_TRAP_GATE(double_fault,	3); /* on IST3 (crit stack) */

	FILL_TRAP_GATE(invalid_tss,	2);
	FILL_TRAP_GATE(no_segment,	2);
	FILL_TRAP_GATE(stack_error,	2);
	FILL_TRAP_GATE(gp_fault,	2);
	FILL_TRAP_GATE(page_fault,	2); /* IST=2: trap stack, avoids triple-fault on CoW thread stacks */

	FILL_TRAP_GATE(coproc_error,	2);
	FILL_TRAP_GATE(alignment_check,	2);
	FILL_TRAP_GATE(machine_check,	3); /* on IST3 (crit stack) */
	FILL_TRAP_GATE(simd_error,	2);
	FILL_TRAP_GATE(virt_error,	2);

	/*
	 * Load IRQ vectors. All IRQs run on IST1 (IRQ stack).
	 * For Hyperlight, we may not have external interrupts,
	 * but we set them up for completeness.
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
	FILL_IRQ_GATE(16, 1);
	FILL_IRQ_GATE(17, 1);
	FILL_IRQ_GATE(18, 1);
	FILL_IRQ_GATE(19, 1);
	FILL_IRQ_GATE(20, 1);
	FILL_IRQ_GATE(21, 1);
	FILL_IRQ_GATE(22, 1);
	FILL_IRQ_GATE(23, 1);
	FILL_IRQ_GATE(24, 1);
	FILL_IRQ_GATE(25, 1);
	FILL_IRQ_GATE(26, 1);
	FILL_IRQ_GATE(27, 1);
	FILL_IRQ_GATE(28, 1);
	FILL_IRQ_GATE(29, 1);
	FILL_IRQ_GATE(30, 1);
	FILL_IRQ_GATE(31, 1);
	FILL_IRQ_GATE(32, 1);
	FILL_IRQ_GATE(33, 1);
	FILL_IRQ_GATE(34, 1);
	FILL_IRQ_GATE(35, 1);
	FILL_IRQ_GATE(36, 1);
	FILL_IRQ_GATE(37, 1);
	FILL_IRQ_GATE(38, 1);
	FILL_IRQ_GATE(39, 1);
	FILL_IRQ_GATE(40, 1);
	FILL_IRQ_GATE(41, 1);
	FILL_IRQ_GATE(42, 1);
	FILL_IRQ_GATE(43, 1);
	FILL_IRQ_GATE(44, 1);
	FILL_IRQ_GATE(45, 1);
	FILL_IRQ_GATE(46, 1);
	FILL_IRQ_GATE(47, 1);
	FILL_IRQ_GATE(48, 1);
	FILL_IRQ_GATE(49, 1);
	FILL_IRQ_GATE(50, 1);
	FILL_IRQ_GATE(51, 1);
	FILL_IRQ_GATE(52, 1);
	FILL_IRQ_GATE(53, 1);
	FILL_IRQ_GATE(54, 1);
	FILL_IRQ_GATE(55, 1);
	FILL_IRQ_GATE(56, 1);
	FILL_IRQ_GATE(57, 1);
	FILL_IRQ_GATE(58, 1);
	FILL_IRQ_GATE(59, 1);
	FILL_IRQ_GATE(60, 1);
	FILL_IRQ_GATE(61, 1);
	FILL_IRQ_GATE(62, 1);
	FILL_IRQ_GATE(63, 1);
	/* For brevity, we handle fewer IRQs in Hyperlight */
	/* Hyperlight typically doesn't use hardware interrupts */

	idtptr.limit = sizeof(cpu_idt) - 1;
	idtptr.base = (__u64) &cpu_idt;
}

void traps_lcpu_init(struct uk_lcpu *this_lcpu __unused)
{
	gdt_init();
	tss_init();

	/*
	 * Pre-fault the TOP page of each IST stack before loading the IDT.
	 *
	 * With Hyperlight CoW, .bss pages are read-only until written.
	 * The early-boot asm IDT (IST=0) is still active here, so these
	 * writes are handled by the asm #PF handler on the current stack.
	 *
	 * We only need the top page of each IST stack to be writable so
	 * the CPU can push the initial exception frame.  The #PF handler
	 * itself uses IST=0 (current stack), so deeper stack pages are
	 * resolved on demand without recursion.
	 *
	 * This costs only 3 scratch pages instead of 48 (the full stacks).
	 */
	{
		volatile __u8 *ist_top;

		/* IST3 (critical): double fault, NMI, debug */
		ist_top = lcpu_except_stack + CPU_EXCEPT_STACK_SIZE;
		*(volatile __u64 *)(ist_top - 8) = 0;
		*(volatile __u64 *)(ist_top - 4096) = 0; /* cover non-page-aligned top */

		/* IST2 (trap): page fault + most exceptions.
		 * page_fault uses IST=2 so the CoW handler always runs on this
		 * stack.  Two pages are pre-faulted: the top page for the
		 * exception frame push, and one page below for the handler
		 * frame, regardless of lcpu_except_stack alignment.
		 */
		ist_top = lcpu_except_stack + CPU_EXCEPT_STACK_SIZE * 2;
		*(volatile __u64 *)(ist_top - 8) = 0;
		*(volatile __u64 *)(ist_top - 4096) = 0; /* cover non-page-aligned top */

		/* IST1 (IRQ): interrupt handlers */
		ist_top = lcpu_except_stack + CPU_EXCEPT_STACK_SIZE * 3;
		*(volatile __u64 *)(ist_top - 8) = 0;
		*(volatile __u64 *)(ist_top - 4096) = 0; /* cover non-page-aligned top */
	}

	idt_init();
}

__isr __uptr traps_lcpu_get_except_stack_base(void)
{
	return (__uptr)&lcpu_except_stack;
}
