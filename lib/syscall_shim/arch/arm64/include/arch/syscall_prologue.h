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

/**
 * Define a default value for SPSR_EL1, since we are not actually taking an
 * exception. Most fields do not interest us yet and we ignore NZCF.
 * Additionaly, we leave IRQ's unmasked as, normally, when a SVC call would
 * happen in userspace, IRQ's would be enabled.
 */
#define SPSR_EL1_SVC64_DEFAULT_VALUE					\
	((0b1 << 0) /* M[0] must be 1 for non-EL0 state */ |		\
	 (0b0 << 1) /* M[1] must be 0 for AArch64 state */ |		\
	 (0b01 << 2) /* M[3:2] (EL) must be 0b01 for EL1 */ |		\
	 (0b0 << 4) /* M[4] = 0 for AArch64 state */ |			\
	 (0b0 << 5) /* T32, does not matter */ |			\
	 (0b1101 << 6) /* D, A, I, F, only IRQ's unmasked */)

#define UK_SYSCALL_EXECENV_PROLOGUE_DEFINE(pname, fname, x, ...)	\
	long __used __noreturn __attribute__((optimize("O3")))		\
	pname(UK_ARG_MAPx(x, UK_S_ARG_LONG_MAYBE_UNUSED, __VA_ARGS__))	\
	{								\
		__asm__ __volatile__(					\
		"/* No IRQ's during register saving please */\n\t"	\
		"msr	daifset, #2\n\t"				\
		"/* Use TPIDRRO_EL0 as a scratch register. This\n\t"	\
		" * should be fine since the applications assume \n\t"	\
		" * they can't change its value and, therefore, \n\t"	\
		" * its original value will always be known by us \n\t"	\
		" * and predictable. Thus, if we ever do decide to \n\t"\
		" * use it for anything else, we can always just \n\t"	\
		" * restore to that value afterwards (unless there\n\t"	\
		" * is a better scratch registers for this job).\n\t"	\
		" * We are however an EL1 only kernel and, \n\t"	\
		" * therefore, we do not have an automatically \n\t"	\
		" * indexed stack switch.\n\t"				\
		" */\n\t"						\
		"msr	tpidrro_el0, x0\n\t"				\
		"/* Use `struct lcpu` pointer from TPIDR_EL1 */\n\t"	\
		"mrs	x0, tpidr_el1\n\t"				\
		" /* Switch to per-CPU auxiliary stack */\n\t"		\
		"ldr	x0, [x0, #"STRINGIFY(LCPU_AUXSP_OFFSET)"]\n\t"	\
		"sub	x0, x0, #"STRINGIFY(UKARCH_AUXSPCB_SIZE)"\n\t"	\
		"ldr	x0, [x0, #"					\
			STRINGIFY(UKARCH_AUXSPCB_OFFSETOF_CURR_FP)"]\n\t"\
		"/* Auxiliary stack is already ECTX aligned */\n\t"	\
		"/* Make room for `struct ukarch_execenv` */\n\t"	\
		"sub	x0, x0, #"STRINGIFY(UKARCH_EXECENV_SIZE)"\n\t"	\
		"/* Swap x0 and (old) sp */\n\t"			\
		"add	sp, sp, x0\n\t"					\
		"sub	x0, sp, x0\n\t"					\
		"sub	sp, sp, x0\n\t"					\
		"/* Now store old sp w.r.t. `struct __regs` */\n\t"	\
		"str	x0, [sp, #"STRINGIFY(__SP_OFFSET)"]\n\t"	\
		"/* Restore x0 from scratch register TPIDRRO_EL0 */\n\t"\
		"mrs	x0, tpidrro_el0\n\t"				\
		"/* Now just store the rest of `struct __regs` */\n\t"	\
		"stp	x0, x1, [sp, #16 * 0]\n\t"			\
		"stp	x2, x3, [sp, #16 * 1]\n\t"			\
		"stp	x4, x5, [sp, #16 * 2]\n\t"			\
		"stp	x6, x7, [sp, #16 * 3]\n\t"			\
		"stp	x8, x9, [sp, #16 * 4]\n\t"			\
		"stp	x10, x11, [sp, #16 * 5]\n\t"			\
		"stp	x12, x13, [sp, #16 * 6]\n\t"			\
		"stp	x14, x15, [sp, #16 * 7]\n\t"			\
		"stp	x16, x17, [sp, #16 * 8]\n\t"			\
		"stp	x18, x19, [sp, #16 * 9]\n\t"			\
		"stp	x20, x21, [sp, #16 * 10]\n\t"			\
		"stp	x22, x23, [sp, #16 * 11]\n\t"			\
		"stp	x24, x25, [sp, #16 * 12]\n\t"			\
		"stp	x26, x27, [sp, #16 * 13]\n\t"			\
		"stp	x28, x29, [sp, #16 * 14]\n\t"			\
		"/* Here we should push lr and elr_el1, however\n\t"	\
		" * we are not in an actual exception, but instead\n\t"	\
		" * we are trying to emulate a SVC with a function\n\t"	\
		" * call which makes elr_el1 invalid and we instead\n\t"\
		" * double push lr (x30).\n\t"				\
		" */\n\t"						\
		"stp	x30, x30, [sp, #16 * 15]\n\t"			\
		"/* Just like above for elr_el1, spsr_el1 is also\n\t"	\
		" * invalid. Therefore we use a sane default value\n\t"	\
		" * which one would normally see in spsr_el1\n\t"	\
		" * following a SVC.\n\t"				\
		" */\n\t"						\
		"mov	x22, #"STRINGIFY(SPSR_EL1_SVC64_DEFAULT_VALUE)"\n\t"\
		"/* Same for esr_el1, make it look like a SVC\n\t"	\
		" * happened.\n\t"					\
		" */\n\t"						\
		"mov	x23, xzr\n\t"					\
		"add	x23, x23, #"STRINGIFY(ESR_EL1_EC_SVC64)"\n\t"	\
		"orr	x23, xzr, x23, lsl #"STRINGIFY(ESR_EC_SHIFT)"\n\t"\
		"orr	x23, x23, #"STRINGIFY(ESR_IL)"\n\t"		\
		"stp	x22, x23, [sp, #16 * 16]\n\t"			\
		"/* ECTX at slot w.r.t. `struct ukarch_execenv` */\n\t"	\
		"mov	x0, sp\n\t"					\
		"add	x0, x0, #("STRINGIFY(__REGS_SIZEOF +		\
				     UKARCH_SYSCTX_SIZE)")\n\t"		\
		"bl	ukarch_ectx_store\n\t"				\
		"/* SYSCTX at slot w.r.t. `struct ukarch_execenv` */\n\t"\
		"mov	x0, sp\n\t"					\
		"add	x0, x0, #"STRINGIFY(__REGS_SIZEOF)"\n\t"	\
		"bl	ukarch_sysctx_store\n\t"			\
		"mov	x0, sp\n\t"					\
		"msr	daifclr, #2\n\t"				\
		"bl	"STRINGIFY(fname)"\n\t"				\
		"/* Only restore callee preserved regs (ABI) */\n\t"	\
		"ldr	x30, [sp, #16 * 15]\n\t"			\
		"ldp	x28, x29, [sp, #16 * 14]\t\n"			\
		"ldp	x26, x27, [sp, #16 * 13]\t\n"			\
		"ldp	x24, x25, [sp, #16 * 12]\t\n"			\
		"ldp	x22, x23, [sp, #16 * 11]\t\n"			\
		"ldp	x20, x21, [sp, #16 * 10]\t\n"			\
		"ldp	x18, x19, [sp, #16 * 9]\t\n"			\
		"/* Restore rsp from where it was stored */\n\t"	\
		"ldr	x9, [sp, #"STRINGIFY(__SP_OFFSET)"]\n\t"	\
		"mov	sp, x9\n\t"					\
		"ret\n\t"						\
		::							\
		);							\
	}

#endif /* !__ASSEMBLY__ */
