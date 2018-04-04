/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Authors: Grzegorz Milos <gm281@cam.ac.uk>
 *          Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2005, Grzegorz Milos, Intel Research Cambridge
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation.
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

#ifndef __UKARCH_LCPU_H__
#error Do not include this header directly
#endif

#ifndef mb
#define mb()    __asm__ __volatile__ ("mfence" : : : "memory")
#endif

#ifndef rmb
#define rmb()   __asm__ __volatile__ ("lfence" : : : "memory")
#endif

#ifndef wmb
#define wmb()   __asm__ __volatile__ ("sfence" : : : "memory")
#endif

#ifndef nop
#define nop()   __asm__ __volatile__ ("nop" : : : "memory")
#endif

static inline unsigned long ukarch_read_sp(void)
{
	unsigned long sp;

	__asm__ __volatile__("mov %%rsp, %0" : "=r"(sp));
	return sp;
}
