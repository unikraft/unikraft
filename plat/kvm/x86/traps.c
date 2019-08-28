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
#include <uk/plat/config.h>
#include <x86/desc.h>
#include <kvm-x86/traps.h>

static struct seg_desc32 cpu_gdt64[GDT_NUM_ENTRIES] __align64b;

/*
 * The monitor (ukvm) or bootloader + bootstrap (virtio) starts us up with a
 * bootstrap GDT which is "invisible" to the guest, init and switch to our own
 * GDT.
 *
 * This is done primarily since we need to do LTR later in a predictable
 * fashion.
 */
static void gdt_init(void)
{
	volatile struct desc_table_ptr64 gdtptr;

	memset(cpu_gdt64, 0, sizeof(cpu_gdt64));
	cpu_gdt64[GDT_DESC_CODE].raw = GDT_DESC_CODE_VAL;
	cpu_gdt64[GDT_DESC_DATA].raw = GDT_DESC_DATA_VAL;

	gdtptr.limit = sizeof(cpu_gdt64) - 1;
	gdtptr.base = (__u64) &cpu_gdt64;
	__asm__ __volatile__("lgdt (%0)" ::"r"(&gdtptr));
	/*
	 * TODO: Technically we should reload all segment registers here, in
	 * practice this doesn't matter since the bootstrap GDT matches ours,
	 * for now.
	 */
}

static struct tss64 cpu_tss;

__section(".intrstack")  __align(STACK_SIZE)
char cpu_intr_stack[STACK_SIZE];  /* IST1 */
__section(".intrstack")  __align(STACK_SIZE)
char cpu_trap_stack[STACK_SIZE];  /* IST2 */
static char cpu_nmi_stack[4096];  /* IST3 */

static void tss_init(void)
{
	struct seg_desc64 *td = (void *) &cpu_gdt64[GDT_DESC_TSS_LO];

	cpu_tss.ist[0] = (__u64) &cpu_intr_stack[sizeof(cpu_intr_stack)];
	cpu_tss.ist[1] = (__u64) &cpu_trap_stack[sizeof(cpu_trap_stack)];
	cpu_tss.ist[2] = (__u64) &cpu_nmi_stack[sizeof(cpu_nmi_stack)];

	td->limit_lo = sizeof(cpu_tss);
	td->base_lo = (__u64) &cpu_tss;
	td->type = 0x9;
	td->zero = 0;
	td->dpl = 0;
	td->p = 1;
	td->limit_hi = 0;
	td->gran = 0;
	td->base_hi = (__u64) &cpu_tss >> 24;
	td->zero1 = 0;

	barrier();
	__asm__ __volatile__(
		"ltr %0"
		:
		: "r" ((unsigned short) (GDT_DESC_TSS_LO * 8))
	);
}


/* Declare the traps used only by this platform: */
DECLARE_TRAP_EC(nmi,           "NMI")
DECLARE_TRAP_EC(double_fault,  "double fault")
DECLARE_TRAP_EC(virt_error,    "virtualization error")


static struct seg_gate_desc64 cpu_idt[IDT_NUM_ENTRIES] __align64b;

static void idt_fillgate(unsigned int num, void *fun, unsigned int ist)
{
	struct seg_gate_desc64 *desc = &cpu_idt[num];

	/*
	 * All gates are interrupt gates, all handlers run with interrupts off.
	 */
	desc->offset_hi = (__u64) fun >> 16;
	desc->offset_lo = (__u64) fun & 0xffff;
	desc->selector = GDT_DESC_OFFSET(GDT_DESC_CODE);
	desc->ist = ist;
	desc->type = 14; /* == 0b1110 */
	desc->dpl = 0;
	desc->p = 1;
}

volatile struct desc_table_ptr64 idtptr;

static void idt_init(void)
{
	/*
	 * Load trap vectors. All traps run on IST2 (cpu_trap_stack), except for
	 * the exceptions.
	 */
#define FILL_TRAP_GATE(name, ist) extern void cpu_trap_##name(void); \
	idt_fillgate(TRAP_##name, ASM_TRAP_SYM(name), ist)
	FILL_TRAP_GATE(divide_error,    2);
	FILL_TRAP_GATE(debug,           2);
	FILL_TRAP_GATE(nmi,             3); /* #NMI runs on IST3 (cpu_nmi_stack) */
	FILL_TRAP_GATE(int3,            2);
	FILL_TRAP_GATE(overflow,        2);
	FILL_TRAP_GATE(bounds,          2);
	FILL_TRAP_GATE(invalid_op,      2);
	FILL_TRAP_GATE(no_device,       2);
	FILL_TRAP_GATE(double_fault,    3); /* #DF runs on IST3 (cpu_nmi_stack) */

	FILL_TRAP_GATE(invalid_tss,     2);
	FILL_TRAP_GATE(no_segment,      2);
	FILL_TRAP_GATE(stack_error,     2);
	FILL_TRAP_GATE(gp_fault,        2);
	FILL_TRAP_GATE(page_fault,      2);

	FILL_TRAP_GATE(coproc_error,    2);
	FILL_TRAP_GATE(alignment_check, 2);
	FILL_TRAP_GATE(machine_check,   2);
	FILL_TRAP_GATE(simd_error,      2);
	FILL_TRAP_GATE(virt_error,      2);

	/*
	 * Load irq vectors. All irqs run on IST1 (cpu_intr_stack).
	 */
#define FILL_IRQ_GATE(num, ist) extern void cpu_irq_##num(void); \
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
	__asm__ __volatile__("lidt (%0)" :: "r" (&idtptr));
}

void traps_init(void)
{
	gdt_init();
	tss_init();
	idt_init();
}

void traps_fini(void)
{
}
