/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_SYSCALL_H__
#error Do not include this header directly
#endif

/* NOTE:
 * syscall.h is going to be included by many C-source files that may not
 * include headers from uk/plat/common and this is going to result in
 * lots of build errors.
 * TODO: Plat re-architecting will help with this, but for now, simply
 * re-define this macro to not waste time on this trivial matter.
 */
#ifndef LCPU_AUXSP_OFFSET
#define LCPU_AUXSP_OFFSET		0x20
#endif /* !LCPU_AUXSP_OFFSET */

#if !__ASSEMBLY__

#include <uk/essentials.h>

#define UK_SYSCALL_USC_PROLOGUE_DEFINE(pname, fname, x, ...)		\
	long __used __naked __noreturn					\
	pname(UK_ARG_MAPx(x, UK_S_ARG_LONG_MAYBE_UNUSED, __VA_ARGS__))	\
	{								\
		__asm__ __volatile__(					\
		"cli\n\t"						\
		"/* Switch to the per-CPU auxiliary stack */\n\t"	\
		"/* AMD64 SysV ABI: r11 is scratch register */\n\t"	\
		"movq   %%rsp, %%r11\n\t"				\
		"movq	%%gs:("STRINGIFY(LCPU_AUXSP_OFFSET)"), %%rsp\n\t"\
		"/* Auxiliary stack is already ECTX aligned */\n\t"	\
		"/* Make room for `struct uk_syscall_ctx` */\n\t"	\
		"subq	$("STRINGIFY(UK_SYSCALL_CTX_SIZE -		\
				     __REGS_SIZEOF)"), %%rsp\n\t"	\
		"/* Now build stack frame beginning with 5 pointers\n\t"\
		" * in the classical iretq/`struct __regs` format\n\t"	\
		" */\n\t"						\
		"/* Push stack segment, GDT data segment selector:\n\t"	\
		" * [15: 3]: Selector Index - second GDT entry\n\t"	\
		" * [ 2: 2]: Table Indicator - GDT, table 0\n\t"	\
		" * [ 1: 0]: Requestor Privilege Level - ring 0\n\t"	\
		" */\n\t"						\
		"pushq	$(0x10)\n\t"					\
		"/* Push saving original rsp stored in r11 */\n\t"	\
		"pushq	%%r11\n\t"					\
		"/* Push EFLAGS register */\n\t"			\
		"pushfq\n\t"						\
		"/* Push code segment, GDT code segment selector:\n\t"	\
		" * [15: 3]: Selector Index - first GDT entry\n\t"	\
		" * [ 2: 2]: Table Indicator - GDT, table 0\n\t"	\
		" * [ 1: 0]: Requestor Privilege Level - ring 0\n\t"	\
		" */\n\t"						\
		"pushq	$(0x8)\n\t"					\
		"/* Save caller next rip, this part here\n\t"		\
		" * is the reason why we depend on `__naked`, as we\n\t"\
		" * rely on the aforementioned rip being placed at\n\t"	\
		" * rsp + 8 initially w.r.t. `call` instruction.\n\t"	\
		" */\n\t"						\
		"movq	(%%r11), %%r11\n\t"				\
		"pushq	%%r11\n\t"					\
		"/* Now just push the rest of `struct __regs` */\n\t"	\
		"pushq	%%rax\n\t"					\
		"pushq	%%rdi\n\t"					\
		"pushq	%%rsi\n\t"					\
		"pushq	%%rdx\n\t"					\
		"pushq	%%rcx\n\t"					\
		"pushq	%%rax\n\t"					\
		"pushq	%%r8\n\t"					\
		"pushq	%%r9\n\t"					\
		"pushq	%%r10\n\t"					\
		"pushq	%%r11\n\t"					\
		"pushq	%%rbx\n\t"					\
		"pushq	%%rbp\n\t"					\
		"pushq	%%r12\n\t"					\
		"pushq	%%r13\n\t"					\
		"pushq	%%r14\n\t"					\
		"pushq	%%r15\n\t"					\
		"subq	$("STRINGIFY(__REGS_PAD_SIZE)"), %%rsp\n\t"	\
		"/* ECTX at slot w.r.t. `struct uk_syscall_ctx` */\n\t"\
		"movq	%%rsp, %%rdi\n\t"				\
		"addq	$("STRINGIFY(__REGS_SIZEOF +			\
				     UKARCH_SYSREGS_SIZE)"), %%rdi\n\t"	\
		"call	ukarch_ectx_store\n\t"				\
		"/* SYSREGS at slot w.r.t. `struct uk_syscall_ctx` */\n\t"\
		"movq	%%rsp, %%rdi\n\t"				\
		"addq	$("STRINGIFY(__REGS_SIZEOF)"), %%rdi\n\t"	\
		"call	ukarch_sysregs_switch_uk\n\t"			\
		"movq	%%rsp, %%rdi\n\t"				\
		"sti\n\t"						\
		"call	"STRINGIFY(fname)"\n\t"				\
		"addq	$("STRINGIFY(__REGS_PAD_SIZE)"), %%rsp\n\t"	\
		"/* Only restore callee preserved regs (ABI) */\n\t"	\
		"popq	%%r15\n\t"					\
		"popq	%%r14\n\t"					\
		"popq	%%r13\n\t"					\
		"popq	%%r12\n\t"					\
		"popq	%%rbp\n\t"					\
		"popq	%%rbx\n\t"					\
		"/* Restore rsp from where it was stored */\n\t"	\
		"movq   104(%%rsp), %%rsp\n\t"				\
		"ret\n\t"						\
		::							\
		);							\
	}

#endif /* !__ASSEMBLY__ */
