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

#ifdef __ASSEMBLY__
/*
 * Stack size to save general purpose registers and essential system
 * registers. 8 * (30 + lr + elr_el1 + spsr_el1 + esr_el1) = 272.
 * From exceptions come from EL0, we have to save sp_el0. So the
 * TRAP_STACK_SIZE should be 272 + 8 = 280. But we enable the stack
 * alignment check, we will force align the stack for EL1 exceptions,
 * so we add a sp to save original stack pointer: 280 + 8 = 288
 *
 * TODO: We'd better to calculate this size automatically later.
 */
#define __TRAP_STACK_SIZE	288
#define __SP_OFFSET		272
#define __SP_EL0_OFFSET		280
#else
/*
 * Change this structure must update TRAP_STACK_SIZE at the same time.
 * This data structure must be 16-byte alignment.
 */
struct __regs {
	/* Generic Purpose registers, from x0 ~ x29 */
	unsigned long x[30];

	/* Link Register (x30) */
	unsigned long lr;

	/* Exception Link Register */
	unsigned long elr_el1;

	/* Processor State Register */
	unsigned long spsr_el1;

	/* Exception Status Register */
	unsigned long esr_el1;

	/* Stack Pointer */
	unsigned long sp;

	/* Stack Pointer from el0 */
	unsigned long sp_el0;
};

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

#endif /* __ASSEMBLY__ */
