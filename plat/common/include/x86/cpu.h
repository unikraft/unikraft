/* SPDX-License-Identifier: BSD-2-Clause */
/*
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
/******************************************************************************
 * cpu.h
 *
 * CPU related macros and definitions copied from mini-os/os.h
 */

#ifndef __PLAT_COMMON_X86_CPU_H__
#define __PLAT_COMMON_X86_CPU_H__

#include <uk/arch/types.h>
#include <x86/cpu_defs.h>
#include <stdint.h>
#include <uk/assert.h>
#include <uk/alloc.h>
#include <string.h>

void halt(void);
void system_off(enum ukplat_gstate request);

static inline void cpuid(__u32 fn, __u32 subfn,
			 __u32 *eax, __u32 *ebx,
			 __u32 *ecx, __u32 *edx)
{
	asm volatile("cpuid"
		     : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
		     : "a"(fn), "c" (subfn));
}

unsigned long read_cr2(void);

static inline void write_cr3(unsigned long cr3)
{
	asm volatile("mov %0, %%cr3" : : "r"(cr3) : "memory");
}

static inline void invlpg(unsigned long va)
{
	asm volatile("invlpg %0" : : "m"(*(const char *)(va)) : "memory");
}


static inline void rdmsr(unsigned int msr, __u32 *lo, __u32 *hi)
{
	asm volatile("rdmsr" : "=a"(*lo), "=d"(*hi)
			     : "c"(msr));
}

static inline __u64 rdmsrl(unsigned int msr)
{
	__u32 lo, hi;

	rdmsr(msr, &lo, &hi);
	return ((__u64) lo | (__u64) hi << 32);
}

static inline void wrmsr(unsigned int msr, __u32 lo, __u32 hi)
{
	asm volatile("wrmsr"
			     : /* no outputs */
			     : "c"(msr), "a"(lo), "d"(hi));
}

static inline void wrmsrl(unsigned int msr, __u64 val)
{
	wrmsr(msr, (__u32) (val & 0xffffffffULL), (__u32) (val >> 32));
}


static inline __u64 rdtsc(void)
{
	__u64 l, h;

	__asm__ __volatile__("rdtsc" : "=a"(l), "=d"(h));
	return (h << 32) | l;
}

/* accessing devices via memory */
static inline __u8 readb(__u8 *addr)
{
	__u8 v;

	__asm__ __volatile__("movb %1, %0" : "=q"(v) : "m"(*addr));
	return v;
}

static inline __u16 readw(__u16 *addr)
{
	__u16 v;

	__asm__ __volatile__("movw %1, %0" : "=r"(v) : "m"(*addr));
	return v;
}

static inline __u32 readl(__u32 *addr)
{
	__u32 v;

	__asm__ __volatile__("movl %1, %0" : "=r"(v) : "m"(*addr));
	return v;
}

static inline __u64 readq(__u64 *addr)
{
	__u64 v;

	__asm__ __volatile__("movq %1, %0" : "=r"(v) : "m"(*addr));
	return v;
}

static inline void writeb(__u8 *addr, __u8 v)
{
	__asm__ __volatile__("movb %0, %1" : : "q"(v), "m"(*addr));
}

static inline void writew(__u16 *addr, __u16 v)
{
	__asm__ __volatile__("movw %0, %1" : : "r"(v), "m"(*addr));
}

static inline void writel(__u32 *addr, __u32 v)
{
	__asm__ __volatile__("movl %0, %1" : : "r"(v), "m"(*addr));
}

static inline void writeq(__u64 *addr, __u64 v)
{
	__asm__ __volatile__("movq %0, %1" : : "r"(v), "m"(*addr));
}

/* accessing devices via port space */
static inline __u8 inb(__u16 port)
{
	__u8 v;

	__asm__ __volatile__("inb %1,%0" : "=a"(v) : "dN"(port));
	return v;
}

static inline __u16 inw(__u16 port)
{
	__u16 v;

	__asm__ __volatile__("inw %1,%0" : "=a" (v) : "dN" (port));
	return v;
}

static inline __u32 inl(__u16 port)
{
	__u32 v;

	__asm__ __volatile__("inl %1,%0" : "=a" (v) : "dN" (port));
	return v;
}

static inline __u64 inq(__u16 port_lo)
{
	__u16 port_hi = port_lo + 4;
	__u32 lo, hi;

	__asm__ __volatile__("inl %1,%0" : "=a" (lo) : "dN" (port_lo));
	__asm__ __volatile__("inl %1,%0" : "=a" (hi) : "dN" (port_hi));

	return ((__u64) lo) | ((__u64) hi << 32);
}

static inline void outb(__u16 port, __u8 v)
{
	__asm__ __volatile__("outb %0,%1" : : "a"(v), "dN"(port));
}

static inline void outw(__u16 port, __u16 v)
{
	__asm__ __volatile__("outw %0,%1" : : "a"(v), "dN"(port));
}

static inline void outl(__u16 port, __u32 v)
{
	__asm__ __volatile__("outl %0,%1" : : "a" (v), "dN" (port));
}

static inline __u64 mul64_32(__u64 a, __u32 b)
{
	__u64 prod;

	__asm__ (
		"mul %%rdx ; "
		"shrd $32, %%rdx, %%rax"
		: "=a" (prod)
		: "0" (a), "d" ((__u64) b)
	);

	return prod;
}

#ifdef CONFIG_HAVE_SYSCALL
/* syscall entrance provided by platform library */
void _ukplat_syscall(void);

/* _init_syscall is derived from hermitux: `processor.c`:
 *
 * Copyright (c) 2010, Stefan Lankes, RWTH Aachen University
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the University nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

static inline void _init_syscall(void)
{
	__u32 eax, ebx, ecx, edx;
	int have_syscall = 0;

	/* Check for availability of extended features */
	ukarch_x86_cpuid(0x80000000, 0, &eax, &ebx, &ecx, &edx);
	if (eax >= 0x80000001) {
		ukarch_x86_cpuid(0x80000001, 0, &eax, &ebx, &ecx, &edx);
		have_syscall = (edx & X86_CPUID3_SYSCALL);
	}

	if (!have_syscall)
		UK_CRASH("CPU does not support SYSCALL/SYSRET!\n");

	/* Enable and program syscall/sysret */
	wrmsrl(X86_MSR_EFER,
	       rdmsrl(X86_MSR_EFER)
	       | X86_EFER_LMA | X86_EFER_LME | X86_EFER_SCE);
	wrmsrl(X86_MSR_STAR,
	       (0x08ULL << 48) | (0x08ULL << 32));
	wrmsrl(X86_MSR_LSTAR,
	       (__uptr) _ukplat_syscall);

	/* Clear IF flag during an interrupt */
	wrmsrl(X86_MSR_SYSCALL_MASK,
	       X86_EFLAGS_TF | X86_EFLAGS_DF | X86_EFLAGS_IF
	       | X86_EFLAGS_AC | X86_EFLAGS_NT);

	uk_pr_info("SYSCALL entrance @ %p\n", _ukplat_syscall);
}
#endif /* CONFIG_HAVE_SYSCALL */

#if CONFIG_HAVE_X86PKU
static inline void _check_ospke(void)
{
	__u32 eax, ebx, ecx, edx;
	cpuid(0x7, 0, &eax, &ebx, &ecx, &edx);
	if (!(ecx & X86_CPUID7_ECX_OSPKE)) {
		/* if PKU is not enabled, abort the boot process. Images
		 * compiled with HAVE_X86PKU are *specialized* to be executed on
		 * PKU-enabled hardware. This allows us to avoid checks later at
		 * runtime. */
		UK_CRASH("CPU does not support PKU!\n");
	}
}
#endif /* CONFIG_HAVE_X86PKU */

#endif /* __PLAT_COMMON_X86_CPU_H__ */
