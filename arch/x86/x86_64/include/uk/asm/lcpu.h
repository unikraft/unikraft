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

#define CACHE_LINE_SIZE	64

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
#endif /* !__ASSEMBLY__ */

/* sanity check */
#if __REGS_SIZEOF & 0xf
#error "__regs structure size should be multiple of 16."
#endif

#ifndef __ASSEMBLY__
#ifndef mb
#define mb()    __asm__ __volatile__ ("mfence" : : : "memory")
#endif

#ifndef rmb
#define rmb()   __asm__ __volatile__ ("lfence" : : : "memory")
#endif

#ifndef wmb
#define wmb()   __asm__ __volatile__ ("sfence" : : : "memory")
#endif

#ifndef nop
#define nop()   __asm__ __volatile__ ("nop" : : : "memory")
#endif
#endif /* !__ASSEMBLY__ */

#ifndef __ASSEMBLY__
static inline unsigned long ukarch_read_sp(void)
{
	unsigned long sp;

	__asm__ __volatile__("mov %%rsp, %0" : "=r"(sp));
	return sp;
}

static inline void ukarch_spinwait(void)
{
	__asm__ __volatile__("pause" : : : "memory");
}
#endif /* !__ASSEMBLY__ */

/* CPUID feature bits in ECX and EDX when EAX=1 */
#define X86_CPUID1_ECX_x2APIC   (1 << 21)
#define X86_CPUID1_ECX_XSAVE    (1 << 26)
#define X86_CPUID1_ECX_OSXSAVE  (1 << 27)
#define X86_CPUID1_ECX_AVX      (1 << 28)
#define X86_CPUID1_ECX_RDRAND	(1 << 30)
#define X86_CPUID1_EDX_FPU      (1 << 0)
#define X86_CPUID1_EDX_PAT      (1 << 16)
#define X86_CPUID1_EDX_FXSR     (1 << 24)
#define X86_CPUID1_EDX_SSE      (1 << 25)
/* CPUID feature bits in EBX and ECX when EAX=7, ECX=0 */
#define X86_CPUID7_EBX_FSGSBASE (1 << 0)
#define X86_CPUID7_ECX_PKU	(1 << 3)
#define X86_CPUID7_ECX_OSPKE	(1 << 4)
#define X86_CPUID7_ECX_LA57		(1 << 16)
#define X86_CPUID7_EBX_RDSEED		(1 << 18)
/* CPUID feature bits when EAX=0xd, ECX=1 */
#define X86_CPUIDD1_EAX_XSAVEOPT (1<<0)
/* CPUID 80000001H:EDX feature list */
#define X86_CPUID81_NX			(1 << 20)
#define X86_CPUID81_PAGE1GB		(1 << 26)
#define X86_CPUID81_LM			(1 << 29)
#define X86_CPUID3_SYSCALL      (1 << 11)

#ifndef __ASSEMBLY__
static inline void ukarch_x86_cpuid(__u32 fn, __u32 subfn,
				    __u32 *eax, __u32 *ebx,
				    __u32 *ecx, __u32 *edx)
{
	__asm__ __volatile__("cpuid"
			     : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
			     : "a"(fn), "c" (subfn));
}
#endif /* !__ASSEMBLY__ */
