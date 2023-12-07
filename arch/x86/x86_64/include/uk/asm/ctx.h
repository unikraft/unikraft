/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2021, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
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

#ifndef __UKARCH_CTX_H__
#error Do not include this header directly
#endif

/**
 * Legacy FP State - 160 bytes
 * Legacy SSE State - 352 bytes
 * XSAVE Header Data - 64 bytes (needs sanitization)
 * YMM_H State (AVX) - 256 bytes
 * MPX_BNDREGS - 64 bytes X !We do not enable/use!
 * MPX_BNDCSR - 64 bytes X !We do not enable/use!
 * AVX-512 KMASK - 64 bytes X !We do not enable/use!
 * AVX-512 ZMM_H - 512 bytes X !We do not enable/use!
 * AVX-512 ZMM - 1024 bytes X !We do not enable/use!
 *
 * Total for now: 160 + 352 + 64 + 256 = 832 bytes!
 *
 * NOTE: Increase as we support more of the above!
 */
#define UKARCH_ECTX_SIZE			832 /* Max possible size */
#define UKARCH_ECTX_ALIGN			64 /* Max needed alignment */

#define ukarch_rstack_push(sp, value)			\
	({						\
		unsigned long __sp__ = (sp);		\
		__sp__ -= sizeof(value);		\
		*((typeof(value) *) __sp__) = (value);	\
		__sp__;					\
	})

#define ukarch_rstack_push_packed(sp, value)		\
	ukarch_rstack_push(sp, value)

#define UKARCH_SP_ALIGN		(1 << 4)
#define UKARCH_SP_ALIGN_MASK	(UKARCH_SP_ALIGN - 1)

#define ukarch_gen_sp(base, len)					\
	({								\
		unsigned long __sp__ = (unsigned long) (base)		\
			+ (unsigned long) (len);			\
		__sp__ &= ~((unsigned long) UKARCH_SP_ALIGN_MASK);	\
		__sp__;							\
	})
