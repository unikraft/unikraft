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

/* Intel SGX Launch Enclave Public Key Hash MSRs */
#define MSR_IA32_SGXLEPUBKEYHASH0	0x0000008C
#define MSR_IA32_SGXLEPUBKEYHASH1	0x0000008D
#define MSR_IA32_SGXLEPUBKEYHASH2	0x0000008E
#define MSR_IA32_SGXLEPUBKEYHASH3	0x0000008F

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

/* Can be uninlined because referenced by paravirt */
static inline int native_write_msr_safe(unsigned int msr, __u32 low, __u32 high)
{
	int err = 0;
	/*
	asm volatile("2: wrmsr ; xor %[err],%[err]\n"
		     "1:\n\t"
		     ".section .fixup,\"ax\"\n\t"
		     "3:  mov %[fault],%[err] ; jmp 1b\n\t"
		     ".previous\n\t"
		     _ASM_EXTABLE(2b, 3b)
		     : [err] "=a" (err)
		     : "c" (msr), "0" (low), "d" (high),
		       [fault] "i" (-EIO)
		     : "memory");
	*/
	asm volatile("2: wrmsr ; xor %[err],%[err]\n"
		     "1:\n\t"
		     ".section .fixup,\"ax\"\n\t"
		     "3:  mov %[fault],%[err] ; jmp 1b\n\t"
		     ".previous\n\t"
		     _ASM_EXTABLE(2b, 3b)
		     : [err] "=a" (err)
		     : "c" (msr), "0" (low), "d" (high),
		       [fault] "i" (-EIO)
		     : "memory");

	return err;
}

/* wrmsr with exception handling */
static inline int wrmsr_safe(unsigned int msr, __u32 low, __u32 high)
{
	return native_write_msr_safe(msr, low, high);
}

/*
 * 64-bit version of wrmsr_safe():
 */
static inline int wrmsrl_safe(__u32 msr, __u64 val)
{
	return wrmsr_safe(msr, (__u32)val,  (__u32)(val >> 32));
}

#endif