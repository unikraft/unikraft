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

#include <xen-x86/traps.h>
#include <xen-x86/os.h>
#include <uk/print.h>

/*
 * These are assembler stubs in entry.S.
 * They are the actual entry points for virtual exceptions.
 */
void divide_error(void);
void debug(void);
void int3(void);
void overflow(void);
void bounds(void);
void invalid_op(void);
void device_not_available(void);
void coprocessor_segment_overrun(void);
void invalid_TSS(void);
void segment_not_present(void);
void stack_segment(void);
void general_protection(void);
void page_fault(void);
void coprocessor_error(void);
void simd_coprocessor_error(void);
void alignment_check(void);
void spurious_interrupt_bug(void);
void machine_check(void);

#define do_exit()                                                              \
	for (;;) {                                                             \
	}

static void do_trap(int trapnr, char *str, struct __regs *regs,
		    unsigned long error_code)
{
	uk_printk("FATAL:  Unhandled Trap %d (%s), error code=0x%lx\n", trapnr,
		  str, error_code);
	uk_printk("Regs address %p\n", regs);
	dump_regs(regs);
}

#define DO_ERROR(trapnr, str, name)                                            \
	void do_##name(struct __regs *regs, unsigned long error_code)          \
	{                                                                      \
		do_trap(trapnr, str, regs, error_code);                        \
	}

#define DO_ERROR_INFO(trapnr, str, name, sicode, siaddr)                       \
	void do_##name(struct __regs *regs, unsigned long error_code)          \
	{                                                                      \
		do_trap(trapnr, str, regs, error_code);                        \
	}

DO_ERROR_INFO(0, "divide error", divide_error, FPE_INTDIV, regs->eip)
DO_ERROR(3, "int3", int3)
DO_ERROR(4, "overflow", overflow)
DO_ERROR(5, "bounds", bounds)
DO_ERROR_INFO(6, "invalid operand", invalid_op, ILL_ILLOPN, regs->eip)
DO_ERROR(7, "device not available", device_not_available)
DO_ERROR(9, "coprocessor segment overrun", coprocessor_segment_overrun)
DO_ERROR(10, "invalid TSS", invalid_TSS)
DO_ERROR(11, "segment not present", segment_not_present)
DO_ERROR(12, "stack segment", stack_segment)
DO_ERROR_INFO(17, "alignment check", alignment_check, BUS_ADRALN, 0)
DO_ERROR(18, "machine check", machine_check)

static int handling_pg_fault;

void do_page_fault(struct __regs *regs, unsigned long error_code)
{
	unsigned long addr = read_cr2();
	struct sched_shutdown sched_shutdown = {.reason = SHUTDOWN_crash};

	/* If we are already handling a page fault, and got another one
	 * that means we faulted in pagetable walk. Continuing here would cause
	 * a recursive fault
	 */
	if (handling_pg_fault == 1) {
		uk_printk("Page fault in pagetable walk (access to invalid memory?).\n");
		HYPERVISOR_sched_op(SCHEDOP_shutdown, &sched_shutdown);
	}
	handling_pg_fault++;
	barrier();

#ifdef __X86_64__
	uk_printk("Page fault at linear address %lx, rip %lx, regs %p, sp %lx, our_sp %p, code %lx\n",
		  addr, regs->rip, regs, regs->rsp, &addr, error_code);
#else
	uk_printk("Page fault at linear address %lx, eip %lx, regs %p, sp %lx, our_sp %p, code %lx\n",
		  addr, regs->eip, regs, regs->esp, &addr, error_code);
#endif

	dump_regs(regs);
#ifdef __X86_64__
	stack_walk_for_frame(regs->rbp);
	dump_mem(regs->rsp);
	dump_mem(regs->rbp);
	dump_mem(regs->rip);
#else
	do_stack_walk(regs->ebp);
	dump_mem(regs->esp);
	dump_mem(regs->ebp);
	dump_mem(regs->eip);
#endif
	HYPERVISOR_sched_op(SCHEDOP_shutdown, &sched_shutdown);
	/* We should never get here ... but still */
	handling_pg_fault--;
}

void do_general_protection(struct __regs *regs, long error_code)
{
	struct sched_shutdown sched_shutdown = {.reason = SHUTDOWN_crash};
#ifdef __X86_64__
	uk_printk("GPF rip: %lx, error_code=%lx\n", regs->rip, error_code);
#else
	uk_printk("GPF eip: %lx, error_code=%lx\n", regs->eip, error_code);
#endif
	dump_regs(regs);
#ifdef __X86_64__
	stack_walk_for_frame(regs->rbp);
	dump_mem(regs->rsp);
	dump_mem(regs->rbp);
	dump_mem(regs->rip);
#else
	do_stack_walk(regs->ebp);
	dump_mem(regs->esp);
	dump_mem(regs->ebp);
	dump_mem(regs->eip);
#endif
	HYPERVISOR_sched_op(SCHEDOP_shutdown, &sched_shutdown);
}

void do_debug(struct __regs *regs)
{
	uk_printk("Debug exception\n");
#define TF_MASK 0x100
	regs->eflags &= ~TF_MASK;
	dump_regs(regs);
	do_exit();
}

void do_coprocessor_error(struct __regs *regs)
{
	uk_printk("Copro error\n");
	dump_regs(regs);
	do_exit();
}

void simd_math_error(void *eip __unused)
{
	uk_printk("SIMD error\n");
}

void do_simd_coprocessor_error(struct __regs *regs __unused)
{
	uk_printk("SIMD copro error\n");
}

void do_spurious_interrupt_bug(struct __regs *regs __unused)
{
}

/* Assembler interface fns in entry.S. */
void hypervisor_callback(void);
void failsafe_callback(void);

#ifdef CONFIG_PARAVIRT
/*
 * Submit a virtual IDT to teh hypervisor. This consists of tuples
 * (interrupt vector, privilege ring, CS:EIP of handler).
 * The 'privilege ring' field specifies the least-privileged ring that
 * can trap to that vector using a software-interrupt instruction (INT).
 */
static trap_info_t trap_table[] = {
	{  0, 0, __KERNEL_CS, (unsigned long)divide_error                },
	{  1, 0, __KERNEL_CS, (unsigned long)debug                       },
	{  3, 3, __KERNEL_CS, (unsigned long)int3                        },
	{  4, 3, __KERNEL_CS, (unsigned long)overflow                    },
	{  5, 3, __KERNEL_CS, (unsigned long)bounds                      },
	{  6, 0, __KERNEL_CS, (unsigned long)invalid_op                  },
	{  7, 0, __KERNEL_CS, (unsigned long)device_not_available        },
	{  9, 0, __KERNEL_CS, (unsigned long)coprocessor_segment_overrun },
	{ 10, 0, __KERNEL_CS, (unsigned long)invalid_TSS                 },
	{ 11, 0, __KERNEL_CS, (unsigned long)segment_not_present         },
	{ 12, 0, __KERNEL_CS, (unsigned long)stack_segment               },
	{ 13, 0, __KERNEL_CS, (unsigned long)general_protection          },
	{ 14, 0, __KERNEL_CS, (unsigned long)page_fault                  },
	{ 15, 0, __KERNEL_CS, (unsigned long)spurious_interrupt_bug      },
	{ 16, 0, __KERNEL_CS, (unsigned long)coprocessor_error           },
	{ 17, 0, __KERNEL_CS, (unsigned long)alignment_check             },
	{ 19, 0, __KERNEL_CS, (unsigned long)simd_coprocessor_error      },
	{  0, 0,           0, 0                                          }
};

void trap_init(void)
{
	HYPERVISOR_set_trap_table(trap_table);

#ifdef __i386__
	HYPERVISOR_set_callbacks(__KERNEL_CS,
				 (unsigned long)hypervisor_callback,
				 __KERNEL_CS, (unsigned long)failsafe_callback);
#else
	HYPERVISOR_set_callbacks((unsigned long)hypervisor_callback,
				 (unsigned long)failsafe_callback, 0);
#endif
}

void trap_fini(void)
{
	HYPERVISOR_set_trap_table(NULL);
}
#else

#define INTR_STACK_SIZE PAGE_SIZE
static uint8_t intr_stack[INTR_STACK_SIZE] __attribute__((aligned(16)));

hw_tss tss __attribute__((aligned(16))) = {
#ifdef __X86_64__
	.rsp0 = (unsigned long)&intr_stack[INTR_STACK_SIZE],
#else
	.esp0 = (unsigned long)&intr_stack[INTR_STACK_SIZE],
	.ss0 = __KERN_DS,
#endif
	.iopb = X86_TSS_INVALID_IO_BITMAP,
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

void trap_init(void)
{
	setup_gate(TRAP_divide_error, &divide_error, 0);
	setup_gate(TRAP_debug, &debug, 0);
	setup_gate(TRAP_int3, &int3, 3);
	setup_gate(TRAP_overflow, &overflow, 3);
	setup_gate(TRAP_bounds, &bounds, 0);
	setup_gate(TRAP_invalid_op, &invalid_op, 0);
	setup_gate(TRAP_no_device, &device_not_available, 0);
	setup_gate(TRAP_copro_seg, &coprocessor_segment_overrun, 0);
	setup_gate(TRAP_invalid_tss, &invalid_TSS, 0);
	setup_gate(TRAP_no_segment, &segment_not_present, 0);
	setup_gate(TRAP_stack_error, &stack_segment, 0);
	setup_gate(TRAP_gp_fault, &general_protection, 0);
	setup_gate(TRAP_page_fault, &page_fault, 0);
	setup_gate(TRAP_spurious_int, &spurious_interrupt_bug, 0);
	setup_gate(TRAP_copro_error, &coprocessor_error, 0);
	setup_gate(TRAP_alignment_check, &alignment_check, 0);
	setup_gate(TRAP_simd_error, &simd_coprocessor_error, 0);
	setup_gate(TRAP_xen_callback, hypervisor_callback, 0);

	asm volatile("lidt idt_ptr");

	gdt[GDTE_TSS] =
	    (typeof(*gdt))INIT_GDTE((unsigned long)&tss, 0x67, 0x89);
	asm volatile("ltr %w0" ::"rm"(GDTE_TSS * 8));

	if (hvm_set_parameter(HVM_PARAM_CALLBACK_IRQ,
			      (2ULL << 56) | TRAP_xen_callback)) {
		uk_printk("Request for Xen HVM callback vector failed\n");
		do_exit();
	}
}

void trap_fini(void)
{
}
#endif
