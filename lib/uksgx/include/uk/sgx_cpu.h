/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Xiangyi Meng <xymeng16@gmail.com>
 *
 * Copyright (c) 2022. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _UK_SGX_CPU_H_
#define _UK_SGX_CPU_H_

#include <uk/arch/types.h>
#include <errno.h>

#define	EX_TYPE_NONE			 0
#define	EX_TYPE_DEFAULT			 1
#define	EX_TYPE_FAULT			 2
#define	EX_TYPE_UACCESS			 3
#define	EX_TYPE_COPY			 4
#define	EX_TYPE_CLEAR_FS		 5
#define	EX_TYPE_FPU_RESTORE		 6
#define	EX_TYPE_WRMSR			 7
#define	EX_TYPE_RDMSR			 8
#define	EX_TYPE_BPF			 9

#define	EX_TYPE_WRMSR_IN_MCE		10
#define	EX_TYPE_RDMSR_IN_MCE		11

#define	EX_TYPE_DEFAULT_MCE_SAFE	12
#define	EX_TYPE_FAULT_MCE_SAFE		13

#define	BIT(nr)			        (1UL << (nr))

#define __stringify_1(x...)	#x
#define __stringify(x...)	__stringify_1(x)

# define _ASM_EXTABLE_TYPE(from, to, type)			\
	" .pushsection \"__ex_table\",\"a\"\n"			\
	" .balign 4\n"						\
	" .long (" #from ") - .\n"				\
	" .long (" #to ") - .\n"				\
	" .long " __stringify(type) " \n"			\
	" .popsection\n"

#define _ASM_EXTABLE(from, to)					\
	_ASM_EXTABLE_TYPE(from, to, EX_TYPE_DEFAULT)

/* Referred to as IA32_FEATURE_CONTROL in Intel's SDM. */
#define X86_MSR_IA32_FEAT_CTL		            0x0000003a
#define X86_FEAT_CTL_LOCKED				        BIT(0)
#define X86_FEAT_CTL_VMX_ENABLED_INSIDE_SMX		BIT(1)
#define X86_FEAT_CTL_VMX_ENABLED_OUTSIDE_SMX	BIT(2)
#define X86_FEAT_CTL_SGX_LC_ENABLED			    BIT(17)
#define X86_FEAT_CTL_SGX_ENABLED			    BIT(18)
#define X86_FEAT_CTL_LMCE_ENABLED			    BIT(20)

#define X86_MSR_IA32_EFER			            0xC0000080 /* extended feature enables */
#define X86_EFER_SYSCALL			    		0x1UL
#define X86_MSR_IA32_STAR			            0xC0000081 /* legacy mode SYSCALL target */
#define X86_MSR_IA32_LSTAR			            0xC0000082 /* long mode SYSCALL target */
#define X86_MSR_IA32_FMASK			            0xC0000084 /* legacy mode SYSCALL mask */

/* Intel SGX Launch Enclave Public Key Hash MSRs */
#define MSR_IA32_SGXLEPUBKEYHASH0	0x0000008C
#define MSR_IA32_SGXLEPUBKEYHASH1	0x0000008D
#define MSR_IA32_SGXLEPUBKEYHASH2	0x0000008E
#define MSR_IA32_SGXLEPUBKEYHASH3	0x0000008F

/* __cpuid_count(unsinged int info[4], unsigned int leaf, unsigned int subleaf); */
#define __cpuid_count(x, y, z)                                                       \
	asm volatile("cpuid"                                                   \
		     : "=a"(x[0]), "=b"(x[1]), "=c"(x[2]), "=d"(x[3])          \
		     : "a"(y), "c"(z))

/* __cpuid(unsinged int info[4], unsigned int leaf); */
#define __cpuid(x, y)                                                    \
	asm volatile("cpuid"                                                   \
		     : "=a"(x[0]), "=b"(x[1]), "=c"(x[2]), "=d"(x[3])          \
		     : "a"(y))

#define Genu 0x756e6547
#define ineI 0x49656e69
#define ntel 0x6c65746e

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

/* wrmsr with exception handling */
static inline void wrmsr(unsigned int msr, __u32 low, __u32 high)
{
	asm volatile("wrmsr"
			     : /* no outputs */
			     : "c"(msr), "a"(low), "d"(high));
}

/*
 * 64-bit version of wrmsr():
 */
static inline void wrmsrl(__u32 msr, __u64 val)
{
	wrmsr(msr, (__u32)val,  (__u32)(val >> 32));
}


/*
 * Volatile isn't enough to prevent the compiler from reordering the
 * read/write functions for the control registers and messing everything up.
 * A memory clobber would solve the problem, but would prevent reordering of
 * all loads stores around it, which can hurt performance. Solution is to
 * use a variable and mimic reads and writes to it to enforce serialization
 */
extern unsigned long __force_order;

static inline unsigned long native_read_cr0(void)
{
	unsigned long val;
	asm volatile("mov %%cr0,%0\n\t" : "=r" (val), "=m" (__force_order));
	return val;
}

static inline void native_write_cr0(unsigned long val)
{
	asm volatile("mov %0,%%cr0": : "r" (val), "m" (__force_order));
}

static inline unsigned long native_read_cr2(void)
{
	unsigned long val;
	asm volatile("mov %%cr2,%0\n\t" : "=r" (val), "=m" (__force_order));
	return val;
}

static inline void native_write_cr2(unsigned long val)
{
	asm volatile("mov %0,%%cr2": : "r" (val), "m" (__force_order));
}

static inline unsigned long native_read_cr3(void)
{
	unsigned long val;
	asm volatile("mov %%cr3,%0\n\t" : "=r" (val), "=m" (__force_order));
	return val;
}

static inline void native_write_cr3(unsigned long val)
{
	asm volatile("mov %0,%%cr3": : "r" (val), "m" (__force_order));
}

static inline unsigned long native_read_cr4(void)
{
	unsigned long val;
	asm volatile("mov %%cr4,%0\n\t" : "=r" (val), "=m" (__force_order));
	return val;
}

static inline unsigned long native_read_cr4_safe(void)
{
	unsigned long val;
	val = native_read_cr4();
	return val;
}

static inline void native_write_cr4(unsigned long val)
{
	asm volatile("mov %0,%%cr4": : "r" (val), "m" (__force_order));
}


static inline unsigned long native_read_cr8(void)
{
	unsigned long cr8;
	asm volatile("movq %%cr8,%0" : "=r" (cr8));
	return cr8;
}

static inline void native_write_cr8(unsigned long val)
{
	asm volatile("movq %0,%%cr8" :: "r" (val) : "memory");
}


static inline void native_wbinvd(void)
{
	asm volatile("wbinvd": : :"memory");
}

static inline unsigned long read_cr0(void)
{
	return native_read_cr0();
}

static inline void write_cr0(unsigned long x)
{
	native_write_cr0(x);
}

static inline unsigned long read_cr2(void)
{
	return native_read_cr2();
}

static inline void write_cr2(unsigned long x)
{
	native_write_cr2(x);
}

static inline unsigned long read_cr3(void)
{
	return native_read_cr3();
}

static inline void write_cr3(unsigned long x)
{
	native_write_cr3(x);
}

static inline unsigned long __read_cr4(void)
{
	return native_read_cr4();
}

static inline unsigned long __read_cr4_safe(void)
{
	return native_read_cr4_safe();
}

static inline void __write_cr4(unsigned long x)
{
	native_write_cr4(x);
}

static inline void wbinvd(void)
{
	native_wbinvd();
}

static inline unsigned long read_cr8(void)
{
	return native_read_cr8();
}

static inline void write_cr8(unsigned long x)
{
	native_write_cr8(x);
}

#endif