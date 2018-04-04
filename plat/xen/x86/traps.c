/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/* Taken from Mini-OS */

#include <stddef.h>
#include <xen-x86/traps.h>
#include <xen-x86/hypercall.h>
#include <uk/print.h>

/* Traps used only on Xen */

DECLARE_TRAP_EC(coproc_seg_overrun, "coprocessor segment overrun")
DECLARE_TRAP   (spurious_int,       "spurious interrupt bug")


#ifdef CONFIG_PARAVIRT

#define TRAP_TABLE_ENTRY(trapname, pl) \
	{ TRAP_##trapname, pl, __KERNEL_CS, (unsigned long) ASM_TRAP_SYM(trapname) }

/*
 * Submit a virtual IDT to teh hypervisor. This consists of tuples
 * (interrupt vector, privilege ring, CS:EIP of handler).
 * The 'privilege ring' field specifies the least-privileged ring that
 * can trap to that vector using a software-interrupt instruction (INT).
 */
static trap_info_t trap_table[] = {
	TRAP_TABLE_ENTRY(divide_error,        0),
	TRAP_TABLE_ENTRY(debug,               0),
	TRAP_TABLE_ENTRY(int3,                3),
	TRAP_TABLE_ENTRY(overflow,            3),
	TRAP_TABLE_ENTRY(bounds,              3),
	TRAP_TABLE_ENTRY(invalid_op,          0),
	TRAP_TABLE_ENTRY(no_device,           0),
	TRAP_TABLE_ENTRY(coproc_seg_overrun,  0),
	TRAP_TABLE_ENTRY(invalid_tss,         0),
	TRAP_TABLE_ENTRY(no_segment,          0),
	TRAP_TABLE_ENTRY(stack_error,         0),
	TRAP_TABLE_ENTRY(gp_fault,            0),
	TRAP_TABLE_ENTRY(page_fault,          0),
	TRAP_TABLE_ENTRY(spurious_int,        0),
	TRAP_TABLE_ENTRY(coproc_error,        0),
	TRAP_TABLE_ENTRY(alignment_check,     0),
	TRAP_TABLE_ENTRY(simd_error,          0),
	{ 0, 0, 0, 0 }
};

void traps_init(void)
{
	HYPERVISOR_set_trap_table(trap_table);

#ifdef __i386__
	HYPERVISOR_set_callbacks(__KERNEL_CS,
				 (unsigned long) asm_trap_hypervisor_callback,
				 __KERNEL_CS, (unsigned long) asm_failsafe_callback);
#else
	HYPERVISOR_set_callbacks((unsigned long) asm_trap_hypervisor_callback,
				 (unsigned long) asm_failsafe_callback, 0);
#endif
}

void traps_fini(void)
{
	HYPERVISOR_set_trap_table(NULL);
}
#else

#define INTR_STACK_SIZE PAGE_SIZE
static uint8_t intr_stack[INTR_STACK_SIZE] __attribute__((aligned(16)));

hw_tss tss __attribute__((aligned(16))) = {
#ifdef __X86_64__
	.rsp[0] = (unsigned long)&intr_stack[INTR_STACK_SIZE],
#else
	.esp0 = (unsigned long)&intr_stack[INTR_STACK_SIZE],
	.ss0 = __KERN_DS,
#endif
	.iomap_base = X86_TSS_INVALID_IO_BITMAP,
};

static void setup_gate(unsigned int entry, void *addr, unsigned int dpl)
{
	idt[entry].offset_lo = (unsigned long) addr & 0xffff;
	idt[entry].selector = __KERN_CS;
	idt[entry].reserved = 0;
	idt[entry].type = 14; /* == 0b1110 */
	idt[entry].s = 0;
	idt[entry].dpl = dpl;
	idt[entry].p = 1;
	idt[entry].offset_hi = (unsigned long) addr >> 16;
#ifdef __X86_64__
	idt[entry].ist = 0;
	idt[entry].reserved1 = 0;
#endif
}

void traps_init(void)
{
#define SETUP_TRAP_GATE(trapname, dpl) \
	setup_gate(TRAP_##trapname, &ASM_TRAP_SYM(trapname), dpl)
	SETUP_TRAP_GATE(divide_error, 0);
	SETUP_TRAP_GATE(debug, 0);
	SETUP_TRAP_GATE(int3, 3);
	SETUP_TRAP_GATE(overflow, 3);
	SETUP_TRAP_GATE(bounds, 0);
	SETUP_TRAP_GATE(invalid_op, 0);
	SETUP_TRAP_GATE(no_device, 0);
	SETUP_TRAP_GATE(coproc_seg_overrun, 0);
	SETUP_TRAP_GATE(invalid_tss, 0);
	SETUP_TRAP_GATE(no_segment, 0);
	SETUP_TRAP_GATE(stack_error, 0);
	SETUP_TRAP_GATE(gp_fault, 0);
	SETUP_TRAP_GATE(page_fault, 0);
	SETUP_TRAP_GATE(spurious_int, 0);
	SETUP_TRAP_GATE(coproc_error, 0);
	SETUP_TRAP_GATE(alignment_check, 0);
	SETUP_TRAP_GATE(simd_error, 0);
	setup_gate(TRAP_xen_callback, ASM_TRAP_SYM(hypervisor_callback), 0);

	asm volatile("lidt idt_ptr");

	gdt[GDTE_TSS] =
	    (typeof(*gdt))INIT_GDTE((unsigned long)&tss, 0x67, 0x89);
	asm volatile("ltr %w0" ::"rm"(GDTE_TSS * 8));

	if (hvm_set_parameter(HVM_PARAM_CALLBACK_IRQ,
			      (2ULL << 56) | TRAP_xen_callback)) {
		UK_CRASH("Request for Xen HVM callback vector failed\n");
	}
}

void traps_fini(void)
{
}
#endif
