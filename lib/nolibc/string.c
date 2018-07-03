/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 *
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
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
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#include <stdint.h>
#include <string.h>
#include <limits.h>

void *memcpy(void *dst, const void *src, size_t len)
{
	size_t p;

	for (p = 0; p < len; ++p)
		*((__u8 *)(((__uptr)dst) + p)) = *((__u8 *)(((__uptr)src) + p));

	return dst;
}

void *memset(void *ptr, int val, size_t len)
{
	__u8 *p = (__u8 *) ptr;

	for (; len > 0; --len)
		*(p++) = (__u8)val;

	return ptr;
}

void *memchr(const void *ptr, int val, size_t len)
{
	uintptr_t o = 0;

	for (o = 0; o < (uintptr_t)len; ++o)
		if (*((const uint8_t *)(((uintptr_t)ptr) + o)) == (uint8_t)val)
			return (void *)((uintptr_t)ptr + o);

	return NULL; /* did not find val */
}

void *memmove(void *dst, const void *src, size_t len)
{
	uint8_t *d = dst;
	const uint8_t *s = src;

	if (src > dst) {
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

int memcmp(const void *ptr1, const void *ptr2, size_t len)
{
	const unsigned char *c1 = (const unsigned char *)ptr1;
	const unsigned char *c2 = (const unsigned char *)ptr2;

	for (; len > 0; --len, ++c1, ++c2) {
		if ((*c1) != (*c2))
			return ((*c1) - (*c2));
	}

	return 0;
}

size_t strlen(const char *str)
{
	return strnlen(str, SIZE_MAX);
}

size_t strnlen(const char *str, size_t len)
{
	return (size_t)((uintptr_t)memchr(str, '\0', len) - (uintptr_t)str);
}

char *strncpy(char *dst, const char *src, size_t len)
{
	size_t clen;

	clen = strnlen(src, len);
	memcpy(dst, src, clen);

	/* instead of filling up the rest of left space with zeros,
	 * append a termination character if we did not copy one
	 */
	if (clen < len && dst[clen - 1] != '\0')
		dst[clen] = '\0';
	return dst;
}

char *strcpy(char *dst, const char *src)
{
	return strncpy(dst, src, SIZE_MAX);
}

const char *strchr(const char *str, int c)
{
	const char *pos = str;

	for (; *pos != '\0'; ++pos)
		if (*pos == (char) c)
			return pos;
	if (c == 0)
		return pos;

	return NULL;
}

int strncmp(const char *str1, const char *str2, size_t len)
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

int strcmp(const char *str1, const char *str2)
{
	register signed char __res;

	while ((__res = *str1 - *str2++) == 0 && *str1++)
		;

	return __res;
}
