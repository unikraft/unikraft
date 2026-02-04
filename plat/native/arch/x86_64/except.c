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
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
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

#include <uk/asm.h>
#include <uk/arch.h>
#include <uk/arch/util.h>
#include <uk/arch/ctx.h>
#include <uk/arch/limits.h>
#include <uk/plat/config.h>
#include <uk/lcpu.h>
#include <uk/arch/types.h>
#include <uk/event.h>
#include <uk/print.h>
#include <uk/assert.h>
#include <x86/traps.h>
#include <uk/pm.h>

/*
 * CPUs should get dedicated IRQ and exception stacks. We use the interrupt
 * stack table (IST) feature for switching to the stack. This means when an
 * IRQ or a critical exception (e.g., a double fault) occurs, the CPU
 * automatically switches to the stack pointer configured in the IST entry.
 * Since the IST is part of the 64-bit task state segment (TSS), we need one
 * TSS per CPU. The TSS in turn is referenced by the global descriptor
 * table (UK_ARCH_GDT). Consequently, we also need separate UK_ARCH_GDTs
 * per CPU.
 *
 *  CPU ─────┐ per CPU
 *           ▼
 *  ┌──────────────────┐
 *  │   UK_ARCH_GDT    │     ┌──────────────────┐
 *  ├──────────────────┤  ┌─►│       TSS        │
 *  │   null segment   │  │  ├──────────────────┤    ┌────────────────────┐
 *  ├──────────────────┤  │  │   IST stack[0] ──┼───►│     IRQ Stack      ├─┐
 *  │    cs  segment   │  │  │   IST stack[1] ──┼────│                  │ │ │
 *  ├──────────────────┤  │  │   IST stack[2]   │    │                  │ │ │
 *  │    ds  segment   │  │  │        .         │    │                  │ │ │
 *  ├──────────────────┤  │  │        .         │    │                  ▼ │ │
 *  │   tss  segment   ├──┘  │        .         │    └─┬──────────────────┘ │
 *  └──────────────────┘     └──────────────────┘      └────────────────────┘
 *
 *  CPU_EXCEPT_STACK_SIZE  CPU_EXCEPT_STACK_SIZE  CPU_EXCEPT_STACK_SIZE
 *<--------------------><---------------------><-------------------->
 *|=================================================================|
 *|                     |                     |                     |
 *|     crit stack      |       trap stack    |        IRQ stack    |
 *|                     |                     |                     |
 *|=================================================================|
 * ^
 * lcpu_except_stack
 *
 * Why can one stack not corrupt the other/Why is trap stack before IRQ stack?
 * During a trap IRQs are disabled. So only the trap stack could potentially
 * corrupt the IRQ stack if they were the other way around. The ordering of the
 * stacks is made so that this cannot happen. Same thing goes for the crit
 * stack as a crit exception may happen during a trap.
 *
 */

static __align(8)
UK_PER_LCPU_ARRAY_DEFINE(struct uk_arch_seg_gate_desc64, cpu_idt,
			 UK_ARCH_IDT_NUM_ENTRIES);

static
UK_PER_LCPU_DEFINE(__u8, idt_ist_disable_nesting);

static __align(UKARCH_SP_ALIGN) /* IST{1, 2, 3} */
UK_PER_LCPU_ARRAY_DEFINE(__u8, lcpu_except_stack, 3 * CPU_EXCEPT_STACK_SIZE);

static __align(8)
UK_PER_LCPU_DEFINE(struct uk_arch_tss64, cpu_tss);

static __align(8)
UK_PER_LCPU_ARRAY_DEFINE(struct uk_arch_seg_desc32, cpu_gdt64,
			 UK_ARCH_GDT_NUM_ENTRIES);

static void gdt_init(__u32 idx)
{
	struct uk_arch_desc_table_ptr64 gdtptr;

	cpu_gdt64[idx][UK_ARCH_GDT_DESC_CODE].raw = UK_ARCH_GDT_DESC_CODE64_VAL;
	cpu_gdt64[idx][UK_ARCH_GDT_DESC_DATA].raw = UK_ARCH_GDT_DESC_DATA64_VAL;

	gdtptr.limit = sizeof(cpu_gdt64[idx]) - 1;
	gdtptr.base = (__u64)&cpu_gdt64[idx];

	uk_arch_lgdt(&gdtptr);
	uk_arch_set_segs(UK_ARCH_GDT_DESC_OFFSET(UK_ARCH_GDT_DESC_CODE),
			 UK_ARCH_GDT_DESC_OFFSET(UK_ARCH_GDT_DESC_DATA));
}

static void tss_init(__u32 idx)
{
	struct uk_arch_seg_desc64 *tss_desc;

	cpu_tss[idx].ist[0] =
		(__u64)&lcpu_except_stack[idx][CPU_EXCEPT_STACK_SIZE * 3];
	cpu_tss[idx].ist[1] =
		(__u64)&lcpu_except_stack[idx][CPU_EXCEPT_STACK_SIZE * 2];
	cpu_tss[idx].ist[2] =
		(__u64)&lcpu_except_stack[idx][CPU_EXCEPT_STACK_SIZE];

	tss_desc = (void *)&cpu_gdt64[idx][UK_ARCH_GDT_DESC_TSS_LO];
	tss_desc->limit_lo	= sizeof(cpu_tss[idx]);
	tss_desc->base_lo	= (__u64)&(cpu_tss[idx]);
	tss_desc->base_hi	= (__u64)&(cpu_tss[idx]) >> 24;
	tss_desc->type		= UK_ARCH_GDT_DESC_TYPE_TSS_AVAIL;
	tss_desc->p		= 1;

	uk_arch_ltr((__u16)(UK_ARCH_GDT_DESC_OFFSET(UK_ARCH_GDT_DESC_TSS_LO)));
}

#define IDT_IST_SAVE_LEN 32
/* Space for the IST values of all exception vectors */
static UK_PER_LCPU_ARRAY_DEFINE(__u8, idt_ist_saved, IDT_IST_SAVE_LEN);

void uk_plat_native_except_push_nested(void)
{
	struct uk_arch_seg_gate_desc64 *desc;
	struct uk_arch_seg_gate_desc64 *idt;
	__u8 *disable_nesting;
	__u8 *ist_saved;
	struct uk_lcpu *lcpu;
	unsigned int i;

	disable_nesting = &uk_per_lcpu_current(idt_ist_disable_nesting);
	UK_ASSERT(*disable_nesting < __U8_MAX);

	if (*disable_nesting++)
		return;

	lcpu = uk_lcpu_get_current_in_except();
	idt = uk_per_lcpu(cpu_idt, lcpu->idx);
	ist_saved = uk_per_lcpu(idt_ist_saved, lcpu->idx);

	/* Save the value of the IST field and disable IST for the exception */
	for (i = 0; i < IDT_IST_SAVE_LEN; i++) {
		desc = &idt[i];
		ist_saved[i] = desc->ist;
		desc->ist = 0;
	}
}

void uk_plat_native_except_pop_nested(void)
{
	struct uk_arch_seg_gate_desc64 *desc;
	struct uk_arch_seg_gate_desc64 *idt;
	__u8 *disable_nesting;
	__u8 *ist_saved;
	struct uk_lcpu *lcpu;
	unsigned int i;

	disable_nesting = &uk_per_lcpu_current(idt_ist_disable_nesting);
	UK_ASSERT(*disable_nesting > 1);

	if (--*disable_nesting != 0)
		return;

	lcpu = uk_lcpu_get_current_in_except();
	idt = uk_per_lcpu(cpu_idt, lcpu->idx);
	ist_saved = uk_per_lcpu(idt_ist_saved, lcpu->idx);

	/* Restore the IST field values */
	for (i = 0; i < IDT_IST_SAVE_LEN; i++) {
		desc = &idt[i];
		desc->ist = ist_saved[i];
	}
}

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
UK_EVENT(UK_PLAT_NATIVE_EXCEPT_EVENT_DEBUG);
UK_EVENT(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_INVALID_OP);
UK_EVENT(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_PAGE_FAULT);
UK_EVENT(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_BUS_ERROR);
UK_EVENT(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_MATH);
UK_EVENT(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_SECURITY);
UK_EVENT(UK_PLAT_NATIVE_X86_64_EXCEPT_EVENT_ERR_GP_FAULT);
UK_EVENT(UK_PLAT_NATIVE_EXCEPT_EVENT_SYSCALL);
UK_EVENT(UK_PLAT_NATIVE_X86_64_EXCEPT_EVENT_NMI);
UK_EVENT(UK_PLAT_NATIVE_EXCEPT_EVENT_UNHANDLED);

static const char *x86_exception_table[] = {
	"divide error",           /* TRAPNUM_DIVIDE_ERROR (0) */
	"debug",                  /* TRAPNUM_DEBUG (1) */
	"NMI",                    /* TRAPNUM_NMI (2) */
	"int3",                   /* TRAPNUM_INT3 (3) */
	"overflow",               /* TRAPNUM_OVERFLOW (4) */
	"bounds",                 /* TRAPNUM_BOUNDS (5) */
	"invalid opcode",         /* TRAPNUM_INVALID_OP (6) */
	"device not available",   /* TRAPNUM_NO_DEVICE (7) */
	"double fault",           /* TRAPNUM_DOUBLE_FAULT (8) */
	__NULL,		      /* TRAPNUM_COPROC_SEG_OVERRUN (9) */
	"invalid TSS",           /* TRAPNUM_INVALID_TSS (10) */
	"segment not present",   /* TRAPNUM_NO_SEGMENT (11) */
	"stack segment",         /* TRAPNUM_STACK_ERROR (12) */
	"general protection",    /* TRAPNUM_GP_FAULT (13) */
	"page fault",            /* TRAPNUM_PAGE_FAULT (14) */
	__NULL,                  /* reserved (15) */
	"coprocessor",           /* TRAPNUM_COPROC_ERROR (16) */
	"alignment check",       /* TRAPNUM_ALIGNMENT_CHECK (17) */
	"machine check",         /* TRAPNUM_MACHINE_CHECK (18) */
	"SIMD coprocessor",      /* TRAPNUM_SIMD_ERROR (19) */
	"virtualization error",  /* TRAPNUM_VIRT_ERROR (20) */
	"control protection",    /* TRAPNUM_SECURITY_ERROR (21) */
};

static struct uk_event *trap_event_table[] = {
	[UK_ARCH_TRAPNUM_DIVIDE_ERROR] =
		UK_EVENT_PTR(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_MATH),
	[UK_ARCH_TRAPNUM_DEBUG] =
		UK_EVENT_PTR(UK_PLAT_NATIVE_EXCEPT_EVENT_DEBUG),
	[UK_ARCH_TRAPNUM_NMI] =
		UK_EVENT_PTR(UK_PLAT_NATIVE_X86_64_EXCEPT_EVENT_NMI),
	[UK_ARCH_TRAPNUM_INT3] =
		UK_EVENT_PTR(UK_PLAT_NATIVE_EXCEPT_EVENT_DEBUG),
	[UK_ARCH_TRAPNUM_OVERFLOW] =
		UK_EVENT_PTR(UK_PLAT_NATIVE_EXCEPT_EVENT_UNHANDLED),
	[UK_ARCH_TRAPNUM_BOUNDS] =
		UK_EVENT_PTR(UK_PLAT_NATIVE_EXCEPT_EVENT_UNHANDLED),
	[UK_ARCH_TRAPNUM_INVALID_OP] =
		UK_EVENT_PTR(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_INVALID_OP),
	[UK_ARCH_TRAPNUM_NO_DEVICE] =
		UK_EVENT_PTR(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_MATH),
	[UK_ARCH_TRAPNUM_DOUBLE_FAULT] =
		UK_EVENT_PTR(UK_PLAT_NATIVE_EXCEPT_EVENT_UNHANDLED),
	[UK_ARCH_TRAPNUM_INVALID_TSS] =
		UK_EVENT_PTR(UK_PLAT_NATIVE_EXCEPT_EVENT_UNHANDLED),
	[UK_ARCH_TRAPNUM_NO_SEGMENT] =
		UK_EVENT_PTR(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_BUS_ERROR),
	[UK_ARCH_TRAPNUM_STACK_ERROR] =
		UK_EVENT_PTR(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_BUS_ERROR),
	[UK_ARCH_TRAPNUM_GP_FAULT] =
		UK_EVENT_PTR(UK_PLAT_NATIVE_X86_64_EXCEPT_EVENT_ERR_GP_FAULT),
	[UK_ARCH_TRAPNUM_PAGE_FAULT] =
		UK_EVENT_PTR(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_PAGE_FAULT),
	[UK_ARCH_TRAPNUM_COPROC_ERROR] =
		UK_EVENT_PTR(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_MATH),
	[UK_ARCH_TRAPNUM_ALIGNMENT_CHECK] =
		UK_EVENT_PTR(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_BUS_ERROR),
	[UK_ARCH_TRAPNUM_MACHINE_CHECK] =
		UK_EVENT_PTR(UK_PLAT_NATIVE_EXCEPT_EVENT_UNHANDLED),
	[UK_ARCH_TRAPNUM_SIMD_ERROR] =
		UK_EVENT_PTR(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_MATH),
	[UK_ARCH_TRAPNUM_VIRT_ERROR] =
		UK_EVENT_PTR(UK_PLAT_NATIVE_EXCEPT_EVENT_UNHANDLED),
	[UK_ARCH_TRAPNUM_SECURITY_ERROR] =
		UK_EVENT_PTR(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_SECURITY),
};

void uk_plat_native_except_err_handler(int trapnr,
				       struct uk_plat_native_regs *regs,
				       unsigned long error_code)
{
	struct uk_plat_native_except_err_ctx ctx;
	int rc;

	ctx = (struct uk_plat_native_except_err_ctx){
		.regs = regs,
		.trapnr = trapnr,
		.str = x86_exception_table[trapnr],
		.error_code = error_code,
		.handler_err = 0,
		.cr2 = uk_arch_rdcr2(),
	};

	rc = uk_raise_event_ptr(trap_event_table[trapnr], &ctx);
	if (unlikely(rc < 0))
		uk_pr_crit("event handler returned error: %d\n", rc);
	else if (rc != UK_EVENT_NOT_HANDLED)
		return;

	if (trap_event_table[trapnr] ==
		UK_EVENT_PTR(UK_PLAT_NATIVE_EXCEPT_EVENT_UNHANDLED))
		goto trap_halt;

	rc = uk_raise_event(UK_PLAT_NATIVE_EXCEPT_EVENT_UNHANDLED, &ctx);
	if (unlikely(rc == UK_EVENT_NOT_HANDLED || rc < 0))
		uk_pr_crit("Unhandled Trap %d (%s), error code=0x%lx\n",
			   trapnr, x86_exception_table[trapnr],
			   error_code);

trap_halt:
	/* We expect that if there is a handler it won't return,
	 * but as we can't control that, we place halt outside
	 * the above conditional.
	 */
	uk_pm_syscrash();
}

UK_EVENT(UK_PLAT_NATIVE_EXCEPT_EVENT_IRQ);

void uk_plat_native_except_irq_handler(struct uk_plat_native_regs *regs,
				       unsigned long irq)
{
	struct uk_plat_native_except_irq_ctx ctx;
#if CONFIG_LIBUKPLAT_NATIVE_ECTX_ISR_ASSERTIONS
	__sz ectx_align = UK_PLAT_NATIVE_ECTX_ALIGN;
	__u8 ectxbuf[UK_PLAT_NATIVE_ECTX_SIZE + ectx_align];
	struct uk_plat_native_ectx *ectx = (struct uk_plat_native_ectx *)
					ALIGN_UP((__uptr)ectxbuf, ectx_align);

	uk_plat_native_ectx_init(ectx);
#endif /* CONFIG_LIBUKPLAT_NATIVE_ECTX_ISR_ASSERTIONS */
	int rc;

	ctx.regs = regs;
	ctx.irq = irq;
	rc = uk_raise_event(UK_PLAT_NATIVE_EXCEPT_EVENT_IRQ, &ctx);
	if (unlikely(rc < 0))
		UK_CRASH("IRQ event handler returned error: %d\n", rc);

#if CONFIG_LIBUKPLAT_NATIVE_ECTX_ISR_ASSERTIONS
	uk_plat_native_ectx_assert_equal(ectx);
#endif /* CONFIG_LIBUKPLAT_NATIVE_ECTX_ISR_ASSERTIONS */
}

static struct uk_arch_desc_table_ptr64 idtptr;

static inline void idt_fillgate(unsigned int num, void *fun, unsigned int ist)
{
	struct uk_arch_seg_gate_desc64 *desc;
	struct uk_arch_seg_gate_desc64 *idt;

	idt = uk_per_lcpu_current(cpu_idt);
	desc = &idt[num];

	/*
	 * All gates are interrupt gates, all handlers run with interrupts off.
	 */
	desc->offset_hi	= (__u64)fun >> 16;
	desc->offset_lo	= (__u64)fun & 0xffff;
	desc->selector	= UK_ARCH_IDT_DESC_OFFSET(UK_ARCH_IDT_DESC_CODE);
	desc->ist	= ist;
	desc->type	= UK_ARCH_IDT_DESC_TYPE_INTR;
	desc->dpl	= UK_ARCH_IDT_DESC_DPL_KERNEL;
	desc->p		= 1;
}

static void idt_init(void)
{
	/* Ensure that traps_table_init has been called */
	UK_ASSERT(idtptr.limit != 0);

	uk_arch_lidt(&idtptr);
}

void uk_plat_native_traps_table_init(void)
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
	FILL_TRAP_GATE(debug,		3); /* on IST3 (lcpu_except_stack) */
	FILL_TRAP_GATE(nmi,		3); /* on IST3 (lcpu_except_stack) */
	FILL_TRAP_GATE(int3,		3); /* on IST3 (lcpu_except_stack) */
	FILL_TRAP_GATE(overflow,	2);
	FILL_TRAP_GATE(bounds,		2);
	FILL_TRAP_GATE(invalid_op,	2);
	FILL_TRAP_GATE(no_device,	2);
	FILL_TRAP_GATE(double_fault,	3); /* on IST3 (lcpu_except_stack) */

	FILL_TRAP_GATE(invalid_tss,	2);
	FILL_TRAP_GATE(no_segment,	2);
	FILL_TRAP_GATE(stack_error,	2);
	FILL_TRAP_GATE(gp_fault,	2);
	FILL_TRAP_GATE(page_fault,	2);

	FILL_TRAP_GATE(coproc_error,	2);
	FILL_TRAP_GATE(alignment_check,	2);
	FILL_TRAP_GATE(machine_check,	3); /* on IST3 (lcpu_except_stack) */
	FILL_TRAP_GATE(simd_error,	2);
	FILL_TRAP_GATE(virt_error,	2);

	/*
	 * Load IRQ vectors. All IRQs run on IST1 (lcpu_except_stack).
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
	FILL_IRQ_GATE(64, 1);
	FILL_IRQ_GATE(65, 1);
	FILL_IRQ_GATE(66, 1);
	FILL_IRQ_GATE(67, 1);
	FILL_IRQ_GATE(68, 1);
	FILL_IRQ_GATE(69, 1);
	FILL_IRQ_GATE(70, 1);
	FILL_IRQ_GATE(71, 1);
	FILL_IRQ_GATE(72, 1);
	FILL_IRQ_GATE(73, 1);
	FILL_IRQ_GATE(74, 1);
	FILL_IRQ_GATE(75, 1);
	FILL_IRQ_GATE(76, 1);
	FILL_IRQ_GATE(77, 1);
	FILL_IRQ_GATE(78, 1);
	FILL_IRQ_GATE(79, 1);
	FILL_IRQ_GATE(80, 1);
	FILL_IRQ_GATE(81, 1);
	FILL_IRQ_GATE(82, 1);
	FILL_IRQ_GATE(83, 1);
	FILL_IRQ_GATE(84, 1);
	FILL_IRQ_GATE(85, 1);
	FILL_IRQ_GATE(86, 1);
	FILL_IRQ_GATE(87, 1);
	FILL_IRQ_GATE(88, 1);
	FILL_IRQ_GATE(89, 1);
	FILL_IRQ_GATE(90, 1);
	FILL_IRQ_GATE(91, 1);
	FILL_IRQ_GATE(92, 1);
	FILL_IRQ_GATE(93, 1);
	FILL_IRQ_GATE(94, 1);
	FILL_IRQ_GATE(95, 1);
	FILL_IRQ_GATE(96, 1);
	FILL_IRQ_GATE(97, 1);
	FILL_IRQ_GATE(98, 1);
	FILL_IRQ_GATE(99, 1);
	FILL_IRQ_GATE(100, 1);
	FILL_IRQ_GATE(101, 1);
	FILL_IRQ_GATE(102, 1);
	FILL_IRQ_GATE(103, 1);
	FILL_IRQ_GATE(104, 1);
	FILL_IRQ_GATE(105, 1);
	FILL_IRQ_GATE(106, 1);
	FILL_IRQ_GATE(107, 1);
	FILL_IRQ_GATE(108, 1);
	FILL_IRQ_GATE(109, 1);
	FILL_IRQ_GATE(110, 1);
	FILL_IRQ_GATE(111, 1);
	FILL_IRQ_GATE(112, 1);
	FILL_IRQ_GATE(113, 1);
	FILL_IRQ_GATE(114, 1);
	FILL_IRQ_GATE(115, 1);
	FILL_IRQ_GATE(116, 1);
	FILL_IRQ_GATE(117, 1);
	FILL_IRQ_GATE(118, 1);
	FILL_IRQ_GATE(119, 1);
	FILL_IRQ_GATE(120, 1);
	FILL_IRQ_GATE(121, 1);
	FILL_IRQ_GATE(122, 1);
	FILL_IRQ_GATE(123, 1);
	FILL_IRQ_GATE(124, 1);
	FILL_IRQ_GATE(125, 1);
	FILL_IRQ_GATE(126, 1);
	FILL_IRQ_GATE(127, 1);
	FILL_IRQ_GATE(128, 1);
	FILL_IRQ_GATE(129, 1);
	FILL_IRQ_GATE(130, 1);
	FILL_IRQ_GATE(131, 1);
	FILL_IRQ_GATE(132, 1);
	FILL_IRQ_GATE(133, 1);
	FILL_IRQ_GATE(134, 1);
	FILL_IRQ_GATE(135, 1);
	FILL_IRQ_GATE(136, 1);
	FILL_IRQ_GATE(137, 1);
	FILL_IRQ_GATE(138, 1);
	FILL_IRQ_GATE(139, 1);
	FILL_IRQ_GATE(140, 1);
	FILL_IRQ_GATE(141, 1);
	FILL_IRQ_GATE(142, 1);
	FILL_IRQ_GATE(143, 1);
	FILL_IRQ_GATE(144, 1);
	FILL_IRQ_GATE(145, 1);
	FILL_IRQ_GATE(146, 1);
	FILL_IRQ_GATE(147, 1);
	FILL_IRQ_GATE(148, 1);
	FILL_IRQ_GATE(149, 1);
	FILL_IRQ_GATE(150, 1);
	FILL_IRQ_GATE(151, 1);
	FILL_IRQ_GATE(152, 1);
	FILL_IRQ_GATE(153, 1);
	FILL_IRQ_GATE(154, 1);
	FILL_IRQ_GATE(155, 1);
	FILL_IRQ_GATE(156, 1);
	FILL_IRQ_GATE(157, 1);
	FILL_IRQ_GATE(158, 1);
	FILL_IRQ_GATE(159, 1);
	FILL_IRQ_GATE(160, 1);
	FILL_IRQ_GATE(161, 1);
	FILL_IRQ_GATE(162, 1);
	FILL_IRQ_GATE(163, 1);
	FILL_IRQ_GATE(164, 1);
	FILL_IRQ_GATE(165, 1);
	FILL_IRQ_GATE(166, 1);
	FILL_IRQ_GATE(167, 1);
	FILL_IRQ_GATE(168, 1);
	FILL_IRQ_GATE(169, 1);
	FILL_IRQ_GATE(170, 1);
	FILL_IRQ_GATE(171, 1);
	FILL_IRQ_GATE(172, 1);
	FILL_IRQ_GATE(173, 1);
	FILL_IRQ_GATE(174, 1);
	FILL_IRQ_GATE(175, 1);
	FILL_IRQ_GATE(176, 1);
	FILL_IRQ_GATE(177, 1);
	FILL_IRQ_GATE(178, 1);
	FILL_IRQ_GATE(179, 1);
	FILL_IRQ_GATE(180, 1);
	FILL_IRQ_GATE(181, 1);
	FILL_IRQ_GATE(182, 1);
	FILL_IRQ_GATE(183, 1);
	FILL_IRQ_GATE(184, 1);
	FILL_IRQ_GATE(185, 1);
	FILL_IRQ_GATE(186, 1);
	FILL_IRQ_GATE(187, 1);
	FILL_IRQ_GATE(188, 1);
	FILL_IRQ_GATE(189, 1);
	FILL_IRQ_GATE(190, 1);
	FILL_IRQ_GATE(191, 1);
	FILL_IRQ_GATE(192, 1);
	FILL_IRQ_GATE(193, 1);
	FILL_IRQ_GATE(194, 1);
	FILL_IRQ_GATE(195, 1);
	FILL_IRQ_GATE(196, 1);
	FILL_IRQ_GATE(197, 1);
	FILL_IRQ_GATE(198, 1);
	FILL_IRQ_GATE(199, 1);
	FILL_IRQ_GATE(200, 1);
	FILL_IRQ_GATE(201, 1);
	FILL_IRQ_GATE(202, 1);
	FILL_IRQ_GATE(203, 1);
	FILL_IRQ_GATE(204, 1);
	FILL_IRQ_GATE(205, 1);
	FILL_IRQ_GATE(206, 1);
	FILL_IRQ_GATE(207, 1);
	FILL_IRQ_GATE(208, 1);
	FILL_IRQ_GATE(209, 1);
	FILL_IRQ_GATE(210, 1);
	FILL_IRQ_GATE(211, 1);
	FILL_IRQ_GATE(212, 1);
	FILL_IRQ_GATE(213, 1);
	FILL_IRQ_GATE(214, 1);
	FILL_IRQ_GATE(215, 1);
	FILL_IRQ_GATE(216, 1);
	FILL_IRQ_GATE(217, 1);
	FILL_IRQ_GATE(218, 1);
	FILL_IRQ_GATE(219, 1);
	FILL_IRQ_GATE(220, 1);
	FILL_IRQ_GATE(221, 1);
	FILL_IRQ_GATE(222, 1);
	FILL_IRQ_GATE(223, 1);

	idtptr.limit = sizeof(cpu_idt) - 1;
	idtptr.base = (__u64)&cpu_idt;
}

void uk_plat_native_traps_init(struct uk_lcpu *this_lcpu)
{
	gdt_init(this_lcpu->idx);
	tss_init(this_lcpu->idx);
	idt_init();
}

__isr __uptr uk_plat_native_except_get_except_stack_base(void)
{
	return (__uptr)&lcpu_except_stack;
}

void uk_plat_native_set_auxsp(__uptr auxsp)
{
	struct uk_lcpu *lcpu = uk_lcpu_get_current();
	struct uk_plat_native_sysctx *sc;
	struct ukarch_auxspcb *auxspcb;

	lcpu->auxsp = auxsp;
	auxspcb = ukarch_auxsp_get_cb(auxsp);
	sc = (struct uk_plat_native_sysctx *)auxspcb->uksysctx;
	sc->gsbase = (__u64)lcpu;
}

__uptr uk_plat_native_get_auxsp(void)
{
	return uk_lcpu_get_current()->auxsp;
}

__isr __uptr uk_plat_native_get_auxsp_in_except(void)
{
	return uk_lcpu_get_current_in_except()->auxsp;
}
