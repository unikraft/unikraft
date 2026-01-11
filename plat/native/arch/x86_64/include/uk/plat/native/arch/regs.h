/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_NATIVE_ARCH_REGS_H__
#define __UK_PLAT_NATIVE_ARCH_REGS_H__

#include <uk/arch/types.h>
#include <uk/arch/util.h>
#include <uk/config.h>

/**
 * x86_64 register context layout.
 * Structure mirrors the stack frame saved by interrupt/exception entry code.
 * Offsets used by assembly code for efficient register access without
 * including C headers.
 */

/* Padding to ensure 16-byte alignment */
#define UK_PLAT_NATIVE_X86_64_REGS_PAD_SIZE		8

/* General-purpose registers */
#define UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_R15		8
#define UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_R14		16
#define UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_R13		24
#define UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_R12		32
#define UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_RBP		40
#define UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_RBX		48
#define UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_R11		56
#define UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_R10		64
#define UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_R9		72
#define UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_R8		80
#define UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_RAX		88
#define UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_RCX		96
#define UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_RDX		104
#define UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_RSI		112
#define UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_RDI		120

/**
 * Original RAX value (syscall number, 0 for exceptions that don't push an
 * error code or the actual error code pushed by relevant exceptions).
 */
#define UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_ORIG_RAX	128

/* CPU exception frame X86_64_(pushed by hardware on interrupt/exception) */
#define UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_RIP		136
#define UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_CS		144
#define UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_RFLAGS	152
#define UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_RSP		160
#define UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_SS		168

/* Architecture-neutral aliases */
#define UK_PLAT_NATIVE_REGS_OFFSETOF_SP					\
	UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_RSP
#define UK_PLAT_NATIVE_REGS_OFFSETOF_PC					\
	UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_RIP

/* Total size: 176 bytes (multiple of 16 for stack alignment) */
#define UK_PLAT_NATIVE_REGS_SIZE			176

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * AMD64 System V ABI function calling convention mappings.
 * Maps struct uk_plat_native_regs fields to their ABI roles:
 *   rargX - Function arguments 0-5 (RDI, RSI, RDX, RCX, R8, R9)
 *   rretX - Return values (RAX primary, RDX secondary for 128-bit)
 *
 * Used to extract/set argument and return values when manipulating
 * execution contexts (e.g., syscall handling, context switching).
 */
#define uk_plat_native_fn_rarg0				rdi
#define uk_plat_native_fn_rarg1				rsi
#define uk_plat_native_fn_rarg2				rdx
#define uk_plat_native_fn_rarg3				rcx
#define uk_plat_native_fn_rarg4				r8
#define uk_plat_native_fn_rarg5				r9

#define uk_plat_native_fn_rret0				rax
#define uk_plat_native_fn_rret1				rdx

/**
 * x86_64 CPU register context snapshot.
 * Captures the full general-purpose register state and CPU exception frame
 * at the time of an interrupt, exception, or context switch.
 *
 * Layout matches the stack frame created by interrupt entry code:
 *   1. Software-saved registers (R15-RDI, ORIG_RAX)
 *   2. Hardware-saved exception frame (RIP, CS, RFLAGS, RSP, SS)
 *
 * The 8-byte padding ensures the structure size is a multiple of 16,
 * required for proper stack alignment.
 *
 * Register grouping:
 *   - Callee-saved: RBP, RBX, R12-R15
 *   - Caller-saved: RAX, RCX, RDX, RSI, RDI, R8-R11
 *   - Special: ORIG_RAX (syscall number, 0 or error), RIP, CS, RFLAGS, RSP, SS
 */
struct uk_plat_native_regs {
	__u64 pad;
	__u64 r15;
	__u64 r14;
	__u64 r13;
	__u64 r12;
	__u64 rbp;
	__u64 rbx;
	/* Args: non interrupts/non tracing syscalls only save up to here */
	__u64 r11;
	__u64 r10;
	__u64 r9;
	__u64 r8;
	__u64 rax;
	__u64 rcx;
	__u64 rdx;
	__u64 rsi;
	__u64 rdi;
	__u64 orig_rax;
	/* Hardware-pushed exception frame begins here */
	__u64 rip;
	__u64 cs;
	__u64 rflags;
	__u64 rsp;
	__u64 ss;
};

/* Compile-time verification of register offsets */
UK_CTASSERT(sizeof(struct uk_plat_native_regs) == UK_PLAT_NATIVE_REGS_SIZE);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, r15) ==
	    UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_R15);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, r14) ==
	    UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_R14);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, r13) ==
	    UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_R13);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, r12) ==
	    UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_R12);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, rbp) ==
	    UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_RBP);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, rbx) ==
	    UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_RBX);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, r11) ==
	    UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_R11);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, r10) ==
	    UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_R10);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, r9) ==
	    UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_R9);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, r8) ==
	    UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_R8);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, rax) ==
	    UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_RAX);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, rcx) ==
	    UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_RCX);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, rdx) ==
	    UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_RDX);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, rsi) ==
	    UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_RSI);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, rdi) ==
	    UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_RDI);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, orig_rax) ==
	    UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_ORIG_RAX);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, rip) ==
	    UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_RIP);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, cs) ==
	    UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_CS);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, rflags) ==
	    UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_RFLAGS);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, rsp) ==
	    UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_RSP);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, ss) ==
	    UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_SS);

/**
 * Read a register value by byte offset.
 * Allows generic access to registers when the specific register is not
 * known at compile time (e.g., iterating over all registers).
 *
 * @param regs    Register context to read from
 * @param offset  Byte offset of register (use UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_*)
 * @return Register value as 64-bit unsigned integer
 */
__isr static inline __u64
uk_plat_native_regs_get(const struct uk_plat_native_regs *regs, __sz offset)
{
	return *(__u64 *)((const char *)regs + offset);
}

/**
 * Write a register value by byte offset.
 * Allows generic modification of registers when the specific register is not
 * known at compile time (e.g., setting PC for context manipulation).
 *
 * @param regs    Register context to modify
 * @param offset  Byte offset of register (use UK_PLAT_NATIVE_X86_64_REGS_OFFSETOF_*)
 * @param val     New 64-bit value to write
 */
__isr static inline void
uk_plat_native_regs_set(struct uk_plat_native_regs *regs, __sz offset,
			__u64 val)
{
	*(__u64 *)((char *)regs + offset) = val;
}

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PLAT_NATIVE_ARCH_REGS_H__ */
