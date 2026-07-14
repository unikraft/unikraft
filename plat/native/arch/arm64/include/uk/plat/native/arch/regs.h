/* SPDX-License-Identifier: BSD-2-Clause */
/* Copyright (c) 2018, Arm Ltd.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-2-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_NATIVE_ARCH_REGS_H__
#define __UK_PLAT_NATIVE_ARCH_REGS_H__

#include <uk/arch/types.h>
#include <uk/essentials.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X0		0
#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X1		8
#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X2		16
#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X3		24
#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X4		32
#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X5		40
#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X6		48
#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X7		56
#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X8		64
#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X9		72
#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X10		80
#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X11		88
#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X12		96
#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X13		104
#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X14		112
#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X15		120
#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X16		128
#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X17		136
#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X18		144
#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X19		152
#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X20		160
#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X21		168
#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X22		176
#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X23		184
#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X24		192
#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X25		200
#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X26		208
#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X27		216
#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X28		224
#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X29		232
#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_LR		240
#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_ELR_EL1	248
#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_SPSR_EL1	256
#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_ESR_EL1	264
#define UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_SP		272

#define UK_PLAT_NATIVE_REGS_PAD_SIZE			8
#define UK_PLAT_NATIVE_REGS_SIZE			288

#define UK_PLAT_NATIVE_REGS_OFFSETOF_SP					\
	UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_SP
#define UK_PLAT_NATIVE_REGS_OFFSETOF_PC					\
	UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_ELR_EL1

#if !__ASSEMBLY__
struct uk_plat_native_regs {
	/* General Purpose registers (x0 - x29) */
	__u64 x0;
	__u64 x1;
	__u64 x2;
	__u64 x3;
	__u64 x4;
	__u64 x5;
	__u64 x6;
	__u64 x7;
	__u64 x8;
	__u64 x9;
	__u64 x10;
	__u64 x11;
	__u64 x12;
	__u64 x13;
	__u64 x14;
	__u64 x15;
	__u64 x16;
	__u64 x17;
	__u64 x18;
	__u64 x19;
	__u64 x20;
	__u64 x21;
	__u64 x22;
	__u64 x23;
	__u64 x24;
	__u64 x25;
	__u64 x26;
	__u64 x27;
	__u64 x28;
	__u64 x29;

	/* Link Register (x30) */
	__u64 lr;

	/* Exception Link Register */
	__u64 elr_el1;

	/* Processor State Register */
	__u64 spsr_el1;

	/* Exception Status Register */
	__u64 esr_el1;

	/* Stack Pointer */
	__u64 sp;

	/* Padding to comply with AArch64 16-byte stack alignment */
	__u8 pad[UK_PLAT_NATIVE_REGS_PAD_SIZE];
};

UK_CTASSERT(sizeof(struct uk_plat_native_regs) == UK_PLAT_NATIVE_REGS_SIZE);

UK_CTASSERT(__offsetof(struct uk_plat_native_regs, x0) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X0);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, x1) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X1);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, x2) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X2);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, x3) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X3);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, x4) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X4);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, x5) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X5);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, x6) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X6);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, x7) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X7);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, x8) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X8);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, x9) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X9);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, x10) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X10);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, x11) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X11);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, x12) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X12);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, x13) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X13);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, x14) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X14);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, x15) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X15);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, x16) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X16);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, x17) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X17);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, x18) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X18);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, x19) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X19);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, x20) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X20);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, x21) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X21);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, x22) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X22);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, x23) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X23);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, x24) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X24);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, x25) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X25);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, x26) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X26);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, x27) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X27);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, x28) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X28);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, x29) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_X29);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, lr) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_LR);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, elr_el1) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_ELR_EL1);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, spsr_el1) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_SPSR_EL1);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, esr_el1) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_ESR_EL1);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, sp) ==
	    UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_SP);

/**
 * Read a register value by byte offset.
 *
 * @param regs    Register context to read from
 * @param offset  Byte offset of register (use
 *                UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_*)
 * @return Register value as 64-bit unsigned integer
 */
__isr static inline __u64
uk_plat_native_regs_get(const struct uk_plat_native_regs *regs,
			__sz offset)
{
	return *(__u64 *)((const char *)regs + offset);
}

/**
 * Write a register value by byte offset.
 *
 * @param regs    Register context to modify
 * @param offset  Byte offset of register (use
 *                UK_PLAT_NATIVE_ARM64_REGS_OFFSETOF_*)
 * @param val     New 64-bit value to write
 */
__isr static inline void
uk_plat_native_regs_set(struct uk_plat_native_regs *regs, __sz offset,
			__u64 val)
{
	*(__u64 *)((char *)regs + offset) = val;
}

#endif /* !__ASSEMBLY__ */

#ifdef __cplusplus
}
#endif
#endif /* __UK_PLAT_NATIVE_ARCH_REGS_H__ */
