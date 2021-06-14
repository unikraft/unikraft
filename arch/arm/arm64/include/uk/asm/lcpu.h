/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2009, Citrix Systems, Inc.
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation.
 * Copyright (c) 2018, Arm Ltd.
 *
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

#ifndef __UKARCH_LCPU_H__
#error Do not include this header directly
#endif

#define CACHE_LINE_SIZE	64

#ifndef __ASSEMBLY__
/*
 * Stack size to save general-purpose registers and essential system
 * registers: 8 * (30 + lr + pc + pstate + sp) = 272. For exceptions that come
 * from EL0, we save sp_el0 in sp. For exceptions from EL1, we save sp_el1 in
 * sp. For exceptions, we save elr_el1 in pc.
 *
 * Changes to this structure must be reflected in the following OFFSETOF/SIZEOF
 * macros. This data structure must be 16-byte aligned.
 */
struct __regs {
	/* General-purpose registers, from x0 ~ x29 */
	unsigned long x[30];

	/* Link Register (x30) */
	unsigned long lr;

	/* Program Counter */
	unsigned long pc;

	/* Processor State Register */
	unsigned long pstate;

	/* Stack Pointer */
	unsigned long sp;
};
#endif /* !__ASSEMBLY__ */

#define __REGS_OFFSETOF_X0        0
#define __REGS_OFFSETOF_LR        240
#define __REGS_OFFSETOF_PC        248
#define __REGS_OFFSETOF_PSTATE    256
#define __REGS_OFFSETOF_SP        264

#define __REGS_PAD_SIZE           __REGS_OFFSETOF_X0
#define __REGS_SIZEOF             272

/* sanity check */
#if __REGS_SIZEOF & 0xf
#error "__regs structure size should be a multiple of 16."
#endif

#ifndef __ASSEMBLY__
/*
 * In a thread context switch, we save the callee-saved registers (x19 ~ x28),
 * the frame pointer register and the link register to the previous thread's
 * stack: https://github.com/ARM-software/abi-aa/releases
 *
 * Changes to this structure must be reflected in following OFFSETOF/SIZEOF
 * macros.
 */
struct __callee_saved_regs {
	/* Callee-saved registers, from x19 ~ x28 */
	unsigned long callee[10];

	/* Frame Pointer Register (x29) */
	unsigned long fp;

	/* Link Register (x30) */
	unsigned long lr;
};
#endif /* !__ASSEMBLY__ */

#define __CALLEE_SAVED_REGS_OFFSETOF_X19  0
#define __CALLEE_SAVED_REGS_OFFSETOF_FP   80
#define __CALLEE_SAVED_REGS_OFFSETOF_LR   88

#define __CALLEE_SAVED_REGS_SIZEOF        96

#ifndef __ASSEMBLY__
/*
 * Instruction Synchronization Barrier flushes the pipeline in the
 * processor, so that all instructions following the ISB are fetched
 * from cache or memory, after the instruction has been completed.
 */
#define isb()   __asm__ __volatile("isb" ::: "memory")

/*
 * Options for DMB and DSB:
 *	oshld	Outer Shareable, load
 *	oshst	Outer Shareable, store
 *	osh	Outer Shareable, all
 *	nshld	Non-shareable, load
 *	nshst	Non-shareable, store
 *	nsh	Non-shareable, all
 *	ishld	Inner Shareable, load
 *	ishst	Inner Shareable, store
 *	ish	Inner Shareable, all
 *	ld	Full system, load
 *	st	Full system, store
 *	sy	Full system, all
 */
#define dmb(opt)    __asm__ __volatile("dmb " #opt ::: "memory")
#define dsb(opt)    __asm__ __volatile("dsb " #opt ::: "memory")

/* We probably only need "dmb" here, but we'll start by being paranoid. */
#ifndef mb
#define mb()    dsb(sy) /* Full system memory barrier all */
#endif

#ifndef rmb
#define rmb()   dsb(ld) /* Full system memory barrier load */
#endif

#ifndef wmb
#define wmb()   dsb(st) /* Full system memory barrier store */
#endif

static inline unsigned long ukarch_read_sp(void)
{
	unsigned long sp;

	__asm__ __volatile("mov %0, sp": "=&r"(sp));

	return sp;
}

static inline void ukarch_spinwait(void)
{
	/* Intelligent busy wait not supported on arm64. */
}

#endif /* !__ASSEMBLY__ */
