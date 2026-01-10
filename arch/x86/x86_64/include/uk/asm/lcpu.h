/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Authors: Grzegorz Milos <gm281@cam.ac.uk>
 *          Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2005, Grzegorz Milos, Intel Research Cambridge
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation.
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

#include <uk/arch.h>

#define __REGS_OFFSETOF_PAD       0
#define __REGS_OFFSETOF_R15       8
#define __REGS_OFFSETOF_R14       16
#define __REGS_OFFSETOF_R13       24
#define __REGS_OFFSETOF_R12       32
#define __REGS_OFFSETOF_RBP       40
#define __REGS_OFFSETOF_RBX       48
#define __REGS_OFFSETOF_R11       56
#define __REGS_OFFSETOF_R10       64
#define __REGS_OFFSETOF_R9        72
#define __REGS_OFFSETOF_R8        80
#define __REGS_OFFSETOF_RAX       88
#define __REGS_OFFSETOF_RCX       96
#define __REGS_OFFSETOF_RDX       104
#define __REGS_OFFSETOF_RSI       112
#define __REGS_OFFSETOF_RDI       120
#define __REGS_OFFSETOF_ORIG_RAX  128
#define __REGS_OFFSETOF_RIP       136
#define __REGS_OFFSETOF_CS        144
#define __REGS_OFFSETOF_EFLAGS    152
#define __REGS_OFFSETOF_RSP       160
#define __REGS_OFFSETOF_SS        168

#define __REGS_PAD_SIZE           __REGS_OFFSETOF_R15
#define __REGS_SIZEOF             176

#ifndef __ASSEMBLY__
#include <uk/arch/types.h>
#include <uk/essentials.h>

/*
 * Mappings of `struct __regs` register fields
 * according to AMD64 SysV ABI definition for function calls
 *  rargX    - Arguments 0..5
 *  rretX    - Function call return value
 */

#define __fn_rarg0		rdi
#define __fn_rarg1		rsi
#define __fn_rarg2		rdx
#define __fn_rarg3		rcx
#define __fn_rarg4		r8
#define __fn_rarg5		r9

#define __fn_rret0		rax
#define __fn_rret1		rdx

struct __regs {
	unsigned long pad; /* 8 bytes to make struct size multiple of 16 */
	unsigned long r15;
	unsigned long r14;
	unsigned long r13;
	unsigned long r12;
	unsigned long rbp;
	unsigned long rbx;
/* arguments: non interrupts/non tracing syscalls only save upto here*/
	unsigned long r11;
	unsigned long r10;
	unsigned long r9;
	unsigned long r8;
	unsigned long rax;
	unsigned long rcx;
	unsigned long rdx;
	unsigned long rsi;
	unsigned long rdi;
	unsigned long orig_rax;
/* end of arguments */
/* cpu exception frame or undefined */
	unsigned long rip;
	unsigned long cs;
	unsigned long eflags;
	unsigned long rsp;
	unsigned long ss;
/* top of stack page */
};

UK_CTASSERT(sizeof(struct __regs) == __REGS_SIZEOF);

static inline __uptr ukarch_regs_get_sp(struct __regs *r)
{
	return r->rsp;
}

static inline void ukarch_regs_set_sp(__uptr sp, struct __regs *r)
{
	r->rsp = sp;
}

static inline __uptr ukarch_regs_get_pc(struct __regs *r)
{
	return r->rip;
}

static inline void ukarch_regs_set_pc(__uptr pc, struct __regs *r)
{
	r->rip = pc;
}

#endif /* !__ASSEMBLY__ */

/* sanity check */
#if __REGS_SIZEOF & 0xf
#error "__regs structure size should be multiple of 16."
#endif
