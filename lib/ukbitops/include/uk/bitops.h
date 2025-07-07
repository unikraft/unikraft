/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Port from Mini-OS: include/x86/os.h
 */
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

#ifndef __UK_BITOPS_H__
#define __UK_BITOPS_H__

#define __UKARCH_BITOPS_H__

#include <uk/essentials.h>
#include <uk/bitops/bitscan.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define	UK_BIT(nr)			(1UL << (nr))
#define	UK_BIT_ULL(nr)		(1ULL << (nr))

#ifdef __LP64__
#define	UK_BITS_PER_LONG		64
#else
#define	UK_BITS_PER_LONG		32
#endif

#define	UK_BITS_PER_LONG_LONG	64

#define BITS_PER_BYTE  8

static inline __u32
uk_ror32(__u32 word, unsigned int shift)
{
	return ((word >> shift) | (word << (32 - shift)));
}

static inline int uk_get_count_order(unsigned int count)
{
	int order;

	order = uk_mssb(count);
	if (count & (count - 1))
		order++;
	return order;
}


static inline __u64
uk_sign_extend64(__u64 value, int index)
{
	__u8 shift = 63 - index;

	return ((__s64)(value << shift) >> shift);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __UK_BITOPS_H__ */
