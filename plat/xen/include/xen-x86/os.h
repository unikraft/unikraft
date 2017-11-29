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
/* Taken from Mini-OS */
/******************************************************************************
 * os.h
 *
 * random collection of macros and definition
 */

#ifndef _OS_H_
#define _OS_H_

#include <xen-x86/smp.h>

#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <uk/arch/types.h>
#include <uk/arch/limits.h>
#include <uk/arch/time.h>
#include <xen/xen.h>

#ifndef __ASSEMBLY__

typedef __s64 quad_t;
typedef __u64 u_quad_t;

typedef __sptr intptr_t;
typedef __uptr uintptr_t;
typedef __sptr ptrdiff_t;

typedef unsigned char u_char;
typedef unsigned int u_int;
typedef unsigned long u_long;

#include <xen-x86/hypercall.h>

#include <xen/event_channel.h>
#include <xen/xsm/flask_op.h>
#endif

#define MSR_EFER        0xc0000080
#define _EFER_LME       8           /* Long mode enable */

#define X86_CR0_PG      0x80000000  /* Paging */
#define X86_CR4_PAE     0x00000020  /* enable physical address extensions */
#define X86_CR4_OSFXSR  0x00000200  /* enable fast FPU save and restore */

#define X86_EFLAGS_IF   0x00000200

#define __KERNEL_CS     FLAT_KERNEL_CS
#define __KERNEL_DS     FLAT_KERNEL_DS
#define __KERNEL_SS     FLAT_KERNEL_SS

#define TRAP_divide_error      0
#define TRAP_debug             1
#define TRAP_nmi               2
#define TRAP_int3              3
#define TRAP_overflow          4
#define TRAP_bounds            5
#define TRAP_invalid_op        6
#define TRAP_no_device         7
#define TRAP_double_fault      8
#define TRAP_copro_seg         9
#define TRAP_invalid_tss      10
#define TRAP_no_segment       11
#define TRAP_stack_error      12
#define TRAP_gp_fault         13
#define TRAP_page_fault       14
#define TRAP_spurious_int     15
#define TRAP_copro_error      16
#define TRAP_alignment_check  17
#define TRAP_machine_check    18
#define TRAP_simd_error       19
#define TRAP_deferred_nmi     31
#define TRAP_xen_callback     32

#define LOCK_PREFIX ""
#define ADDR (*(volatile long *)addr)

/* Everything below this point is not included by assembler (.S) files. */
#ifndef __ASSEMBLY__

extern shared_info_t *HYPERVISOR_shared_info;

void arch_fini(void);

#include <xen-x86/irq.h>


/*
 * Make sure gcc doesn't try to be clever and move things around
 * on us. We need to use _exactly_ the address the user gave us,
 * not some alias that contains the same information.
 */
typedef struct {
	volatile int counter;
} atomic_t;

#include <xen-x86/cpu.h>

/********************* common i386 and x86_64  ****************************/
#define xen_mb() mb()
#define xen_rmb() rmb()
#define xen_wmb() wmb()
#define xen_barrier() asm volatile("" : : : "memory")

void block_domain(__snsec until);

#endif /* not assembly */
#endif /* _OS_H_ */
