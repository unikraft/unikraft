/* SPDX-License-Identifier: BSD-3-Clause AND MIT */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 * Authors: Cristian Vijelie <cristianvijelie@gmail.com>
 *
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
 * Copyright (c) 2020, University Politehnica of Bucharest. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
/* For the parts taken from musl (marked as such below), the MIT licence
 * applies instead:
 * ----------------------------------------------------------------------
 * Copyright (c) 2005-2014 Rich Felker, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * ----------------------------------------------------------------------
 */

#include <uk/isr/string.h>
#include <uk/arch/types.h>
#include <stdint.h>
#include <limits.h>

void *memcpy_isr(void *dst, const void *src, size_t len)
{
	size_t p;

	for (p = 0; p < len; ++p)
		*((__u8 *)(((__uptr)dst) + p)) = *((__u8 *)(((__uptr)src) + p));

	return dst;
}

void *memset_isr(void *ptr, int val, size_t len)
{
	__u8 *p = (__u8 *) ptr;

	for (; len > 0; --len)
		*(p++) = (__u8)val;

	return ptr;
}

void *memchr_isr(const void *ptr, int val, size_t len)
{
	uintptr_t o = 0;

	for (o = 0; o < (uintptr_t)len; ++o)
		if (*((const uint8_t *)(((uintptr_t)ptr) + o)) == (uint8_t)val)
			return (void *)((uintptr_t)ptr + o);

	return NULL; /* did not find val */
}

void *memrchr_isr(const void *m, int c, size_t n)
{
	const unsigned char *s = m;

	c = (unsigned char) c;
	while (n--)
		if (s[n] == c)
			return (void *) (s + n);
	return 0;
}

void *memmove_isr(void *dst, const void *src, size_t len)
{
	uint8_t *d = dst;
	const uint8_t *s = src;

	if ((intptr_t)src == (intptr_t)dst) {
		return dst;
	} else if ((intptr_t)src > (intptr_t)dst) {
		for (; len > 0; --len)
			*(d++) = *(s++);
	} else {
		s += len;
		d += len;

		for (; len > 0; --len)
			*(d--) = *(s--);
	}

	return dst;
}

int memcmp_isr(const void *ptr1, const void *ptr2, size_t len)
{
	const unsigned char *c1 = (const unsigned char *)ptr1;
	const unsigned char *c2 = (const unsigned char *)ptr2;

	for (; len > 0; --len, ++c1, ++c2) {
		if ((*c1) != (*c2))
			return ((*c1) - (*c2));
	}

	return 0;
}

size_t strlen_isr(const char *str)
{
	return strnlen_isr(str, SIZE_MAX);
}

size_t strnlen_isr(const char *str, size_t len)
{
	const char *p = memchr_isr(str, 0, len);
	return p ? (size_t) (p - str) : len;
}

char *strncpy_isr(char *dst, const char *src, size_t len)
{
	if (len != 0) {
		char *d = dst;
		const char *s = src;

		do {
			if ((*d++ = *s++) == 0) {
				/* NUL pad the remaining n-1 bytes */
				while (--len != 0)
					*d++ = 0;
				break;
			}
		} while (--len != 0);
	}

	return dst;
}

char *strcpy_isr(char *dst, const char *src)
{
	return strncpy_isr(dst, src, SIZE_MAX);
}

int strncmp_isr(const char *str1, const char *str2, size_t len)
{
	const char *c1 = (const char *)str1;
	const char *c2 = (const char *)str2;

	for (; len > 0; --len, ++c1, ++c2) {
		if ((*c1) != (*c2))
			return (int)((*c1) - (*c2));
		if ((*c1) == '\0')
			break;
	}
	return 0;
}

int strcmp_isr(const char *str1, const char *str2)
{
	register signed char __res;

	while ((__res = *str1 - *str2++) == 0 && *str1++)
		;

	return __res;
}

/* The following code is taken from musl libc */
#define ALIGN (sizeof(size_t))
#define ONES ((size_t) -1 / UCHAR_MAX)
#define HIGHS (ONES * (UCHAR_MAX / 2 + 1))
#define HASZERO(x) (((x) - ONES) & ~(x) & HIGHS)
#define BITOP(a, b, op) \
		((a)[(size_t)(b) / (8*sizeof *(a))] op \
		(size_t)1 << ((size_t)(b) % (8 * sizeof *(a))))

char *strchrnul_isr(const char *s, int c)
{
	size_t *w, k;

	c = (unsigned char)c;
	if (!c)
		return (char *)s + strlen_isr(s);

	for (; (uintptr_t)s % ALIGN; s++)
		if (!*s || *(unsigned char *)s == c)
			return (char *)s;
	k = ONES * c;
	for (w = (void *)s; !HASZERO(*w) && !HASZERO(*w ^ k); w++)
		;
	for (s = (void *)w; *s && *(unsigned char *)s != c; s++)
		;
	return (char *)s;
}

char *strchr_isr(const char *str, int c)
{
	char *r = strchrnul_isr(str, c);
	return *(unsigned char *)r == (unsigned char)c ? r : 0;
}

char *strrchr_isr(const char *s, int c)
{
	return memrchr_isr(s, c, strlen_isr(s) + 1);
}

size_t strcspn_isr(const char *s, const char *c)
{
	const char *a = s;
	size_t byteset[32 / sizeof(size_t)];

	if (!c[0] || !c[1])
		return strchrnul_isr(s, *c)-a;

	memset_isr(byteset, 0, sizeof(byteset));
	for (; *c && BITOP(byteset, *(unsigned char *)c, |=); c++)
		;
	for (; *s && !BITOP(byteset, *(unsigned char *)s, &); s++)
		;
	return s-a;
}

size_t strspn_isr(const char *s, const char *c)
{
	const char *a = s;
	size_t byteset[32 / sizeof(size_t)] = { 0 };

	if (!c[0])
		return 0;
	if (!c[1]) {
		for (; *s == *c; s++)
			;
		return s-a;
	}

	for (; *c && BITOP(byteset, *(unsigned char *)c, |=); c++)
		;
	for (; *s && BITOP(byteset, *(unsigned char *)s, &); s++)
		;
	return s-a;
}

char *strtok_isr(char *restrict s, const char *restrict sep, char **restrict p)
{
	if (!s && !(s = *p))
		return NULL;
	s += strspn_isr(s, sep);
	if (!*s)
		return *p = 0;
	*p = s + strcspn_isr(s, sep);
	if (**p)
		*(*p)++ = 0;
	else
		*p = 0;
	return s;
}

/* strlcpy has different ALIGN */
#undef ALIGN
#define ALIGN (sizeof(size_t)-1)
size_t strlcpy_isr(char *d, const char *s, size_t n)
{
	char *d0 = d;
	size_t *wd;
	const size_t *ws;

	if (!n--)
		goto finish;

	if (((uintptr_t)s & ALIGN) == ((uintptr_t)d & ALIGN)) {
		for (; ((uintptr_t) s & ALIGN) && n && (*d = *s);
		     n--, s++, d++)
			;

		if (n && *s) {
			wd = (void *)d; ws = (const void *)s;
			for (; n >= sizeof(size_t) && !HASZERO(*ws);
			     n -= sizeof(size_t), ws++, wd++)
				*wd = *ws;

			d = (void *)wd; s = (const void *)ws;
		}
	}

	for (; n && (*d = *s); n--, s++, d++)
		;
	*d = 0;
finish:
	return d-d0 + strlen_isr(s);
}

size_t strlcat_isr(char *d, const char *s, size_t n)
{
	size_t l = strnlen_isr(d, n);
	if (l == n)
		return l + strlen_isr(s);
	return l + strlcpy_isr(d+l, s, n-l);
}
