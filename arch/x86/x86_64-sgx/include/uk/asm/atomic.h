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
#ifndef __UKARCH_ATOMIC_H__
#error Do not include this header directly
#endif

/**
 * Find first (lowest) set bit in word.
 * @param x The word to operate on
 *
 * @return The index of the least significant 1-bit of x, or if x is zero,
 *   the result is undefined. ffs(1)=0, ffs(3)=0, ffs(0x80000000)=31
 */
static inline unsigned int ukarch_ffs(unsigned int x)
{
	__asm__("bsfl %1,%0"
		: "=r" (x)
		: "rm" (x));
	return x;
}

/**
 * Find last (highest) set bit in word.
 * @param x The word to operate on
 *
 * @return The index of the most significant 1-bit of x, or if x is zero,
 *   the result is undefined. fls(1)=0, fls(3)=1, fls(0x80000001)=31
 */
static inline unsigned int ukarch_fls(unsigned int x)
{
	__asm__("bsrl %1,%0"
		: "=r" (x)
		: "rm" (x));
	return x;
}

/**
 * Find first (lowest) set bit in long word.
 * @param x The long word to operate on
 *
 * @return The index of the least significant 1-bit of x, or if x is zero,
 *    the result is undefined. ffs(1)=0, ffs(3)=0, ffs(0x80000000)=31
 */
static inline unsigned int ukarch_ffsl(unsigned long x)
{
	__asm__("bsfq %1,%0"
		: "=r" (x)
		: "rm" (x));
	return x;
}

/**
 * Find last (highest) set bit in long word.
 * @param x The long word to operate on
 *
 * @return The index of the most significant 1-bit of x, or if x is zero,
 *   the result is undefined. fls(1)=0, fls(3)=1, fls(0x80000001)=31
 */
static inline unsigned int ukarch_flsl(unsigned long x)
{
	__asm__("bsrq %1,%0"
		: "=r" (x)
		: "rm" (x));
	return x;
}
