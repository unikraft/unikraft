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

#include <uk/arch/types.h>


void halt(void);
void system_off(void);

static inline void cpuid(__u32 leaf, __u32 *eax, __u32 *ebx,
		__u32 *ecx, __u32 *edx)
{
	asm volatile("cpuid"
		     : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
		     : "0"(leaf));
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
