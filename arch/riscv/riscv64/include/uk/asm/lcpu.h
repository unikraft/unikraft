/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Eduard Vintila <eduard.vintila47@gmail.com>
 *
 * Copyright (c) 2022, University of Bucharest. All rights reserved.
 *
 * CSR ops are taken from OpenSBI:
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
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

#ifndef __UKARCH_LCPU_H__
#error Do not include this header directly
#endif

#include <uk/arch/types.h>

#define REGBYTES 8

#define CACHE_LINE_SIZE	64

#define __CALLEE_SAVED_SIZE    (REGBYTES * 16)

#ifndef __ASSEMBLY__
struct __regs {
	/* Temporary registers t0-t6 */
	unsigned long t[7];

	/* Argument/return registers a0-a7 */
	unsigned long a[8];

	/* Saved registers s0-s11 */
	unsigned long s[12];

	/* Return address */
	unsigned long ra;

	/* Thread pointer */
	unsigned long tp;

	/* Stack pointer */
	unsigned long sp;

	/* Program counter */
	unsigned long pc;

	/* Padding for achieving 16-byte structure alignment */
	unsigned long pad;
};

#ifndef mb
/* Full memory barrier */
#define mb() ({ __asm__ __volatile__("fence" : : : "memory"); })
#endif

#ifndef rmb
/* Read memory barrier */
#define rmb() ({ __asm__ __volatile__("fence ir, ir" : : : "memory"); })
#endif

#ifndef wmb
/* Write memory barrier */
#define wmb() ({ __asm__ __volatile__("fence ow, ow" : : : "memory"); })
#endif

#ifndef nop
#define nop() ({ __asm__ __volatile__("nop" : : : "memory"); })
#endif

#define _csr_swap(csr, val)                                                    \
	({                                                                     \
		unsigned long __v = (unsigned long)(val);                      \
		__asm__ __volatile__("csrrw %0, " __ASM_STR(csr) ", %1"        \
				     : "=r"(__v)                               \
				     : "rK"(__v)                               \
				     : "memory");                              \
		__v;                                                           \
	})

#define _csr_read(csr)                                                         \
	({                                                                     \
		register unsigned long __v;                                    \
		__asm__ __volatile__("csrr %0, " __ASM_STR(csr)                \
				     : "=r"(__v)                               \
				     :                                         \
				     : "memory");                              \
		__v;                                                           \
	})

#define _csr_write(csr, val)                                                   \
	({                                                                     \
		unsigned long __v = (unsigned long)(val);                      \
		__asm__ __volatile__("csrw " __ASM_STR(csr) ", %0"             \
				     :                                         \
				     : "rK"(__v)                               \
				     : "memory");                              \
	})

#define _csr_read_set(csr, val)                                                \
	({                                                                     \
		unsigned long __v = (unsigned long)(val);                      \
		__asm__ __volatile__("csrrs %0, " __ASM_STR(csr) ", %1"        \
				     : "=r"(__v)                               \
				     : "rK"(__v)                               \
				     : "memory");                              \
		__v;                                                           \
	})

#define _csr_set(csr, val)                                                     \
	({                                                                     \
		unsigned long __v = (unsigned long)(val);                      \
		__asm__ __volatile__("csrs " __ASM_STR(csr) ", %0"             \
				     :                                         \
				     : "rK"(__v)                               \
				     : "memory");                              \
	})

#define _csr_read_clear(csr, val)                                              \
	({                                                                     \
		unsigned long __v = (unsigned long)(val);                      \
		__asm__ __volatile__("csrrc %0, " __ASM_STR(csr) ", %1"        \
				     : "=r"(__v)                               \
				     : "rK"(__v)                               \
				     : "memory");                              \
		__v;                                                           \
	})

#define _csr_clear(csr, val)                                                   \
	({                                                                     \
		unsigned long __v = (unsigned long)(val);                      \
		__asm__ __volatile__("csrc " __ASM_STR(csr) ", %0"             \
				     :                                         \
				     : "rK"(__v)                               \
				     : "memory");                              \
	})

static inline __u8 ioreg_read8(const volatile __u8 *address)
{
	__u8 value;

	asm volatile("lb %0, 0(%1)" : "=r"(value) : "r"(address));
	return value;
}

static inline __u16 ioreg_read16(const volatile __u16 *address)
{
	__u16 value;

	asm volatile("lh %0, 0(%1)" : "=r"(value) : "r"(address));
	return value;
}

static inline __u32 ioreg_read32(const volatile __u32 *address)
{
	__u32 value;

	asm volatile("lw %0, 0(%1)" : "=r"(value) : "r"(address));
	return value;
}

static inline __u64 ioreg_read64(const volatile __u64 *address)
{
	__u64 value;

	asm volatile("ld %0, 0(%1)" : "=r"(value) : "r"(address));
	return value;
}

static inline void ioreg_write8(const volatile __u8 *address, __u8 value)
{
	asm volatile("sb %0, 0(%1)" : : "rZ"(value), "r"(address));
}

static inline void ioreg_write16(const volatile __u16 *address, __u16 value)
{
	asm volatile("sh %0, 0(%1)" : : "rZ"(value), "r"(address));
}

static inline void ioreg_write32(const volatile __u32 *address, __u32 value)
{
	asm volatile("sw %0, 0(%1)" : : "rZ"(value), "r"(address));
}

static inline void ioreg_write64(const volatile __u64 *address, __u64 value)
{
	asm volatile("sd %0, 0(%1)" : : "rZ"(value), "r"(address));
}

static inline unsigned long ukarch_read_sp(void)
{
	unsigned long sp;

	__asm__ __volatile("mv %0, sp" : "=&r"(sp));

	return sp;
}

#endif
