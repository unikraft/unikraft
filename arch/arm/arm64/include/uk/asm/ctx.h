/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *          Robert Kuban <robert.kuban@opensynergy.com
 *
 * Copyright (c) 2021, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
 * Copyright (c) 2022, OpenSynergy GmbH All rights reserved.
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

#include <uk/essentials.h>

#ifndef __UKARCH_CTX_H__
#error Do not include this header directly
#endif

/* 32 * 128-bit NEON registers + 4 bytes (FPSR) + 4 bytes (FPCR) */
#define UKARCH_ECTX_SIZE	520
#define UKARCH_ECTX_ALIGN	16

/* Stack needs to be aligned to 16 bytes */
#define UKARCH_SP_ALIGN		16
#define UKARCH_SP_ALIGN_MASK	(UKARCH_SP_ALIGN - 1)

/*
 * NOTE: Since we use the SP for single registers, we need to add some padding.
 * https://community.arm.com/arm-community-blogs/b/
 *  architectures-and-processors-blog/posts/
 *  using-the-stack-in-aarch64-implementing-push-and-pop
 *
 * WARNING: Changes here need also be reflected in arch/arm/arm64/ctx.S
 */
#define ukarch_rstack_push(sp, value)			\
	({						\
		unsigned long __sp__ = (sp);		\
		__sp__ -= ALIGN_UP(sizeof(value),	\
				  UKARCH_SP_ALIGN);	\
		*((typeof(value) *) __sp__) = (value);	\
		__sp__;					\
	})

#define ukarch_rstack_push_packed(sp, value)		\
	({						\
		unsigned long __sp__ = (sp);		\
		__sp__ -= sizeof(value);		\
		*((typeof(value) *) __sp__) = (value);	\
		__sp__;					\
	})

#define ukarch_gen_sp(base, len)					\
	({								\
		unsigned long __sp__ = (unsigned long) (base)		\
			+ (unsigned long) (len);			\
		__sp__ &= ~((unsigned long) UKARCH_SP_ALIGN_MASK);	\
		__sp__;							\
	})
