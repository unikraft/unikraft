/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2012 Andrew Turner
 * All rights reserved.
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

#include <uk/arch/types.h>

__u64 __qdivrem(__u64 u, __u64 v, __u64 *rem);

#ifndef CONFIG_HAVE_LIBC
__s64 __divdi3(__s64 a, __s64 b)
{
	__u64 ua, ub, uq;
	int neg;

	if (a < 0)
		ua = -(__u64)a, neg = 1;
	else
		ua = a, neg = 0;
	if (b < 0)
		ub = -(__u64)b, neg ^= 1;
	else
		ub = b;
	uq = __qdivrem(ua, ub, (__u64 *)0);
	return neg ? -uq : uq;
}
#endif

/*
 * Helper for __aeabi_ldivmod.
 * TODO: __divdi3 calls __qdivrem. We should do the same and use the
 * remainder value rather than re-calculating it.
 */
long long __kern_ldivmod(long long, long long, long long *);

long long __kern_ldivmod(long long n, long long m, long long *rem)
{
	long long q;

	q = __divdi3(n, m); /* q = n / m */
	*rem = n - m * q;

	return q;
}
