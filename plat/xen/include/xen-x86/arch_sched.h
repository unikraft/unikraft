/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2009 Citrix Systems, Inc. All rights reserved.
 *
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
/*
 * Port from Mini-OS: include/x86/arch_sched.h
 */

#ifndef __ARCH_SCHED_H__
#define __ARCH_SCHED_H__

#include "uk/arch/limits.h"

static inline struct ukplat_thread_ctx *get_current_ctx(void)
{
	struct ukplat_thread_ctx **current;
#ifdef __i386__
	register unsigned long sp asm("esp");
#else
	register unsigned long sp asm("rsp");
#endif
	current = (struct ukplat_thread_ctx **)
		  (unsigned long)(sp & ~(__STACK_SIZE-1));

	return *current;
}

extern void __arch_switch_threads(unsigned long *prevctx,
				  unsigned long *nextctx);

#define arch_switch_threads(prev, next) \
	__arch_switch_threads(&(prev)->sp, &(next)->sp)

#endif /* __ARCH_SCHED_H__ */
