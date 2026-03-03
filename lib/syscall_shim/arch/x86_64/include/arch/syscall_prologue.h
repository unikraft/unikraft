/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_SYSCALL_H__
#error Do not include this header directly
#endif

#if !__ASSEMBLY__

#include <uk/essentials.h>
#include <uk/lcpu/auxsp.h>

#define UK_SYSCALL_EXECENV_PROLOGUE_DEFINE(pname, fname, x, ...)	\
	long __used							\
	pname(UK_ARG_MAPx(x, UK_S_ARG_LONG_MAYBE_UNUSED, __VA_ARGS__));	\
	__asm__ (							\
		".global " STRINGIFY(pname) "\n\t"			\
		"" STRINGIFY(pname) ":\n\t"				\
		"cli\n\t"						\
		"/* Switch to the per-CPU auxiliary stack */\n\t"	\
		"/* AMD64 SysV ABI: r11 is scratch register */\n\t"	\
		"/* Our stack top now contains a return address\n\t"    \
		" * pushed by call; this must be ignored when\n\t"      \
		" * saving the stack pointer to the interrupt\n\t"      \
		" * return structure, but taken into account when\n\t"  \
		" * we actually return execution\n\t"                   \
		" */\n\t"                                               \
		"movq   %rsp, %r11\n\t"					\
		"movq	" UK_PCPUVAR_X86_64_GSREL_GLOBAL(UK_LCPU_AUXSP_SYM) \
			", %rsp\n\t"					\
		"subq	$(" STRINGIFY(UKARCH_AUXSPCB_SIZE) "), %rsp\n\t"\
		"movq	" STRINGIFY(UKARCH_AUXSPCB_OFFSETOF_CURR_FP)	\
						"(%rsp), %rsp\n\t"	\
		"/* Auxiliary stack is already ECTX aligned */\n\t"	\
		"/* Make room for `struct UKARCH_EXECENV` */\n\t"	\
		"subq	$(" STRINGIFY(UKARCH_EXECENV_SIZE -		\
				     UK_LCPU_REGS_SIZE)" ), %rsp\n\t"\
		"/* Now build stack frame beginning with 5 pointers\n\t"\
		" * in the classical iretq/`struct uk_lcpu_regs` format\n\t"\
		" */\n\t"						\
		"/* Push stack segment, GDT data segment selector:\n\t"	\
		" * [15: 3]: Selector Index - second GDT entry\n\t"	\
		" * [ 2: 2]: Table Indicator - GDT, table 0\n\t"	\
		" * [ 1: 0]: Requestor Privilege Level - ring 0\n\t"	\
		" */\n\t"						\
		"pushq	$(0x10)\n\t"					\
		"/* Push saving original rsp - 8 stored in r11 */\n\t"	\
		"pushq	%r11\n\t"					\
		"/* Above pushed rsp is actually the caller's\n\t"	\
		" * rsp minus 8, because the call instruction\n\t"	\
		" * pushes the address the ret instruction is\n\t"	\
		" * supposed to return to. This means that to truly\n\t"\
		" * mimic a trap/syscall we must store/restore\n\t"	\
		" * the rsp we were given, plus 8.\n\t"			\
		" */\n\t"						\
		"addq   $8, (%rsp)\n\t"                                 \
		"/* Push RFLAGS register. Additionally, since we\n\t"	\
		" * pushed it with IRQs disabled, it won't have\n\t"	\
		" * the corresponding bit flag set, making it look\n\t"	\
		" * like the caller of the syscall had IRQs off,\n\t"	\
		" * which no sane application would do, therefore\n\t"	\
		" * manually set the flag.\n\t"				\
		" */\n\t"						\
		"pushfq\n\t"						\
		"orq	$(" STRINGIFY(UK_ARCH_RFLAGS_IF) "), 0(%rsp)\n\t"\
		"/* Push code segment, GDT code segment selector:\n\t"	\
		" * [15: 3]: Selector Index - first GDT entry\n\t"	\
		" * [ 2: 2]: Table Indicator - GDT, table 0\n\t"	\
		" * [ 1: 0]: Requestor Privilege Level - ring 0\n\t"	\
		" */\n\t"						\
		"pushq	$(0x8)\n\t"					\
		"/* Save caller next rip, this part here.\n\t"		\
		" * Rely on the aforementioned rip being placed at\n\t"	\
		" * rsp + 8 initially w.r.t. `call` instruction.\n\t"	\
		" */\n\t"						\
		"movq	(%r11), %r11\n\t"				\
		"pushq	%r11\n\t"					\
		"/* Now just push the rest of `struct uk_lcpu_regs` */\n\t"\
		"pushq	%rax\n\t"					\
		"pushq	%rdi\n\t"					\
		"pushq	%rsi\n\t"					\
		"pushq	%rdx\n\t"					\
		"pushq	%rcx\n\t"					\
		"pushq	%rax\n\t"					\
		"pushq	%r8\n\t"					\
		"pushq	%r9\n\t"					\
		"pushq	%r10\n\t"					\
		"pushq	%r11\n\t"					\
		"pushq	%rbx\n\t"					\
		"pushq	%rbp\n\t"					\
		"pushq	%r12\n\t"					\
		"pushq	%r13\n\t"					\
		"pushq	%r14\n\t"					\
		"pushq	%r15\n\t"					\
		"subq	$(" STRINGIFY(UK_LCPU_X86_64_REGS_OFFSETOF_R15)	\
			"), %rsp\n\t"					\
		"/* ECTX at slot w.r.t. `struct UKARCH_EXECENV` */\n\t" \
		"movq	%rsp, %rdi\n\t"					\
		"addq	$(" STRINGIFY(UK_LCPU_REGS_SIZE +		\
				     UK_LCPU_SYSCTX_SIZE) "), %rdi\n\t"\
		"call	" STRINGIFY(UK_LCPU_ECTX_STORE_FNSYM) "\n\t"	\
		"/* SYSCTX at slot w.r.t. `struct UKARCH_EXECENV` */\n\t"\
		"movq	%rsp, %rdi\n\t"					\
		"addq	$(" STRINGIFY(UK_LCPU_REGS_SIZE) "), %rdi\n\t"	\
		"call	" STRINGIFY(UK_LCPU_SYSCTX_STORE_FNSYM) "\n\t"	\
		"movq	%rsp, %rdi\n\t"					\
		"sti\n\t"						\
		"call	" STRINGIFY(fname) "\n\t"			\
		"addq	$(" STRINGIFY(UK_LCPU_X86_64_REGS_OFFSETOF_R15)	\
			"), %rsp\n\t"					\
		"/* Only restore callee preserved regs (ABI) */\n\t"	\
		"popq	%r15\n\t"					\
		"popq	%r14\n\t"					\
		"popq	%r13\n\t"					\
		"popq	%r12\n\t"					\
		"popq	%rbp\n\t"					\
		"/* Now when we pop we get the old rbx but we hold\n\t"	\
		" * onto it for a little while so that we can do\n\t"	\
		" * what we do below.\n\t"				\
		" */\n\t"						\
		"/* Restore rsp from where it was stored, but put\n\t"	\
		" * it for now in the r11 scratch register so that\n\t"	\
		" * we can still have access to the auxstack\n\t"	\
		" */\n\t"						\
		"movq   112(%rsp), %r11\n\t"				\
		"/* Put in rbx the rip we are supposed to return\n\t"	\
		" * to.\n\t"						\
		" */\n\t"						\
		"movq	88(%rsp), %rbx\n\t"				\
		"/* Exchange stacks: after this we will have in\n\t"	\
		" * r11 the auxstack and in rsp the stack our\n\t"	\
		" * caller had.\n\t"					\
		" */\n\t"						\
		"xchgq	%r11, %rsp\n\t"					\
		"/* Adjust saved stack to original value; ret\n\t"      \
		" * expects return address on top of stack\n\t"         \
		" */\n\t"						\
		"pushq	%rbx\n\t"					\
		"/* Lastly, restore the callee-saved rbx we did not\n\t"\
		" * previously pop together with the others.\n\t"	\
		" */\n\t"						\
		"movq	(%r11), %rbx\n\t"				\
		"ret\n\t"						\
	);

#endif /* !__ASSEMBLY__ */
