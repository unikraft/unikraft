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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/* Taken from Mini-OS */

#ifndef _OS_H_
#define _OS_H_

#include <inttypes.h>
#include <stddef.h>
#include <limits.h>
#include <xen/xen.h>
#include <xen-arm/smp.h>
#include <uk/arch/time.h>

#ifndef __ASSEMBLY__

typedef struct __pte pte_t;
#define __pte(x) npte(x)

typedef __s64 quad_t;
typedef __u64 u_quad_t;
typedef __u64 uintmax_t;
typedef __s64 intmax_t;

typedef __sptr intptr_t;
typedef __uptr uintptr_t;
typedef __sptr ptrdiff_t;

typedef unsigned char u_char;
typedef unsigned int u_int;
typedef unsigned long u_long;

#include <xen-arm/hypercall.h>
#include <xen/event_channel.h>
#include <xen-arm/traps.h>

#define PAGE_SHIFT __PAGE_SHIFT
#define PAGE_MASK  __PAGE_MASK
#define PAGE_SIZE  __PAGE_SIZE

#define STACK_SIZE_PAGE_ORDER __STACK_SIZE_PAGE_ORDER
#define STACK_SIZE            __STACK_SIZE

void arch_fini(void);
void timer_handler(evtchn_port_t port, struct pt_regs *regs, void *ign);

#define smp_processor_id() 0

extern void *HYPERVISOR_dtb;
extern shared_info_t *HYPERVISOR_shared_info;

// disable interrupts
static inline void local_irq_disable(void)
{
	__asm__ __volatile__("cpsid i" ::: "memory");
}

// enable interrupts
static inline void local_irq_enable(void)
{
	__asm__ __volatile__("cpsie i" ::: "memory");
}

#define local_irq_save(x)                                                      \
	{                                                                      \
		__asm__ __volatile__("mrs %0, cpsr;cpsid i"                    \
				     : "=r"(x)::"memory");                     \
	}

#define local_irq_restore(x)                                                   \
	{                                                                      \
		__asm__ __volatile__("msr cpsr_c, %0" ::"r"(x) : "memory");    \
	}

#define local_save_flags(x)                                                    \
	{                                                                      \
		__asm__ __volatile__("mrs %0, cpsr" : "=r"(x)::"memory");      \
	}

static inline int irqs_disabled(void)
{
	int x;

	local_save_flags(x);
	return x & 0x80;
}

void block_domain(__snsec until);

#endif
#endif
