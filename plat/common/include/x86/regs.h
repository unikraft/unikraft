/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2005, Grzegorz Milos, Intel Research Cambridge
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation.
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
#ifndef __UKARCH_REGS_H__
#define __UKARCH_REGS_H__

#ifndef __ASSEMBLY__
struct __regs {
	unsigned long pad; /* for 16 bytes alignment */
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
#endif

#define OFFSETOF_REGS_PAD       0
#define OFFSETOF_REGS_R15       8
#define OFFSETOF_REGS_R14       16
#define OFFSETOF_REGS_R13       24
#define OFFSETOF_REGS_R12       32
#define OFFSETOF_REGS_RBP       40
#define OFFSETOF_REGS_RBX       48
#define OFFSETOF_REGS_R11       56
#define OFFSETOF_REGS_R10       64
#define OFFSETOF_REGS_R9        72
#define OFFSETOF_REGS_R8        80
#define OFFSETOF_REGS_RAX       88
#define OFFSETOF_REGS_RCX       96
#define OFFSETOF_REGS_RDX       104
#define OFFSETOF_REGS_RSI       112
#define OFFSETOF_REGS_RDI       120
#define OFFSETOF_REGS_ORIG_RAX  128
#define OFFSETOF_REGS_RIP       136
#define OFFSETOF_REGS_CS        144
#define OFFSETOF_REGS_EFLAGS    152
#define OFFSETOF_REGS_RSP       160
#define OFFSETOF_REGS_SS        168

#define REGS_PAD_SIZE           OFFSETOF_REGS_R15
#define SIZEOF_REGS             176

#if SIZEOF_REGS & 0xf
#error "__regs structure size should be multiple of 16."
#endif

/* This should be better defined in the thread header */
#define OFFSETOF_UKTHREAD_REGS  16

#endif /* __UKARCH_REGS_H__ */
