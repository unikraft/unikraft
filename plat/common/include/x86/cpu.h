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
#include <sw_ctx.h>
#include <stdint.h>

void halt(void);
void system_off(void);

enum save_cmd {
	X86_SAVE_NONE,
	X86_SAVE_FSAVE,
	X86_SAVE_FXSAVE,
	X86_SAVE_XSAVE,
	X86_SAVE_XSAVEOPT
};

struct _x86_features {
	unsigned long extregs_size;	/* Size of the extregs area */
	unsigned long extregs_align;	/* Alignment of the extregs area */
	enum save_cmd save;		/* which CPU instruction to use for
					 * saving/restoring extregs.
					 */
};

extern struct _x86_features x86_cpu_features;

static inline void save_extregs(struct sw_ctx *ctx)
{
	switch (x86_cpu_features.save) {
	case X86_SAVE_NONE:
		/* nothing to do */
		break;
	case X86_SAVE_FSAVE:
		asm volatile("fsave (%0)" :: "r"(ctx->extregs) : "memory");
		break;
	case X86_SAVE_FXSAVE:
		asm volatile("fxsave (%0)" :: "r"(ctx->extregs) : "memory");
		break;
	case X86_SAVE_XSAVE:
		asm volatile("xsave (%0)" :: "r"(ctx->extregs),
				"a"(0xffffffff), "d"(0xffffffff) : "memory");
		break;
	case X86_SAVE_XSAVEOPT:
		asm volatile("xsaveopt (%0)" :: "r"(ctx->extregs),
				"a"(0xffffffff), "d"(0xffffffff) : "memory");
		break;
	}
}
static inline void restore_extregs(struct sw_ctx *ctx)
{
	switch (x86_cpu_features.save) {
	case X86_SAVE_NONE:
		/* nothing to do */
		break;
	case X86_SAVE_FSAVE:
		asm volatile("frstor (%0)" :: "r"(ctx->extregs));
		break;
	case X86_SAVE_FXSAVE:
		asm volatile("fxrstor (%0)" :: "r"(ctx->extregs));
		break;
	case X86_SAVE_XSAVE:
	case X86_SAVE_XSAVEOPT:
		asm volatile("xrstor (%0)" :: "r"(ctx->extregs),
				"a"(0xffffffff), "d"(0xffffffff));
		break;
	}
}

static inline void _init_cpufeatures(void)
{
	__u32 eax, ebx, ecx, edx;

	/* Why are we saving the eax register content to the eax variable with
	 * "=a(eax)", but then never use it?
	 * Because gcc otherwise will assume that the eax register still
	 * contains "1" after this asm expression. See the "Warning" note at
	 * https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html#InputOperands
	 */
	asm volatile("cpuid" : "=a"(eax), "=c"(ecx), "=d"(edx) : "a"(1)
			: "ebx");
	if (ecx & X86_CPUID1_ECX_OSXSAVE) {
		asm volatile("cpuid" : "=a"(eax), "=c"(ecx) : "a"(0xd), "c"(1)
				: "ebx", "edx");
		if (eax & X86_CPUIDD1_EAX_XSAVEOPT)
			x86_cpu_features.save = X86_SAVE_XSAVEOPT;
		else
			x86_cpu_features.save = X86_SAVE_XSAVE;
		asm volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx)
				: "a"(0xd), "c"(0) : "edx");
		x86_cpu_features.extregs_size = ebx;
		x86_cpu_features.extregs_align = 64;
	} else if (edx & X86_CPUID1_EDX_FXSR) {
		x86_cpu_features.save = X86_SAVE_FXSAVE;
		x86_cpu_features.extregs_size = 512;
		x86_cpu_features.extregs_align = 16;
	} else {
		x86_cpu_features.save = X86_SAVE_FSAVE;
		x86_cpu_features.extregs_size = 108;
		x86_cpu_features.extregs_align = 1;
	}
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

#endif /* __PLAT_COMMON_X86_CPU_H__ */
