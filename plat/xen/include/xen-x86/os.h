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

#include <x86/cpu_defs.h>


#define LOCK_PREFIX ""
#define ADDR (*(volatile long *)addr)

/* Everything below this point is not included by assembler (.S) files. */
#ifndef __ASSEMBLY__

extern shared_info_t *HYPERVISOR_shared_info;

#include <xen-x86/irq.h>


/*
 * Make sure gcc doesn't try to be clever and move things around
 * on us. We need to use _exactly_ the address the user gave us,
 * not some alias that contains the same information.
 */
typedef struct {
	volatile int counter;
} atomic_t;


void block_domain(__snsec until);

#endif /* not assembly */
#endif /* _OS_H_ */
