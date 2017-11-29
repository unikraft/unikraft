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
 * CPU related macros and definitions copied from os.h
 */

#ifndef PLAT_XEN_INCLUDE_XEN_X86_CPU_H_
#define PLAT_XEN_INCLUDE_XEN_X86_CPU_H_

#ifdef CONFIG_PARAVIRT
#include <common/hypervisor.h>
#endif

static inline void write_cr3(unsigned long cr3)
{
	asm volatile("mov %0, %%cr3" : : "r"(cr3) : "memory");
}

static inline void invlpg(unsigned long va)
{
	asm volatile("invlpg %0" : : "m"(*(const char *)(va)) : "memory");
}

/************************** i386 *******************************/
#ifdef __X64_32__

#define rdtscll(val) (asm volatile("rdtsc" : "=A"(val)))

/************************** x86_84 *******************************/
#elif defined __X86_64__

#define rdtscll(val)                                                   \
	do {                                                               \
		unsigned int __a, __d;                                         \
		asm volatile("rdtsc" : "=a"(__a), "=d"(__d));                  \
		(val) = ((unsigned long)__a) | (((unsigned long)__d) << 32);   \
	} while (0)

#else /* ifdef __x86_64__ */
#error "Unsupported architecture"
#endif

/********************* common i386 and x86_64  ****************************/

#define wrmsr(msr, val1, val2) \
({ \
	asm volatile("wrmsr" \
			     : /* no outputs */ \
			     : "c"(msr), "a"(val1), "d"(val2)); \
})

static inline void wrmsrl(unsigned int msr, uint64_t val)
{
	wrmsr(msr, (uint32_t)(val & 0xffffffffULL), (uint32_t)(val >> 32));
}

static inline void cpuid(uint32_t leaf, uint32_t *eax, uint32_t *ebx,
			 uint32_t *ecx, uint32_t *edx)
{
	asm volatile("cpuid"
		     : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
		     : "0"(leaf));
}

#ifdef CONFIG_PARAVIRT
static inline unsigned long read_cr2(void)
{
	return HYPERVISOR_shared_info->vcpu_info[smp_processor_id()].arch.cr2;
}
#else
static inline unsigned long read_cr2(void)
{
	unsigned long cr2;

	asm volatile("mov %%cr2,%0\n\t" : "=r"(cr2));
	return cr2;
}
#endif

#endif /* PLAT_XEN_INCLUDE_XEN_X86_CPU_H_ */
