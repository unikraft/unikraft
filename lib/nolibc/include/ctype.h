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

#ifndef __CTYPE_H__
#define __CTYPE_H__

/*
 * NOTE! This ctype does not handle EOF like the standard C
 * library is required to.
 */

#ifdef __cplusplus
extern "C" {
#endif

#define _U	0x01	/* upper */
#define _L	0x02	/* lower */
#define _D	0x04	/* digit */
#define _C	0x08	/* cntrl */
#define _P	0x10	/* punct */
#define _S	0x20	/* white space (space/lf/tab) */
#define _X	0x40	/* hex digit */
#define _SP	0x80	/* hard space (0x20) */


extern const unsigned char _nolibc_ctype[];

#define _nolibc_ismask(x) (_nolibc_ctype[(int)((unsigned char)(x))])

#define isalnum(c)	((_nolibc_ismask(c)&(_U|_L|_D)) != 0)
#define isalpha(c)	((_nolibc_ismask(c)&(_U|_L)) != 0)
#define iscntrl(c)	((_nolibc_ismask(c)&(_C)) != 0)
#define isdigit(c)	((_nolibc_ismask(c)&(_D)) != 0)
#define isgraph(c)	((_nolibc_ismask(c)&(_P|_U|_L|_D)) != 0)
#define islower(c)	((_nolibc_ismask(c)&(_L)) != 0)
#define isprint(c)	((_nolibc_ismask(c)&(_P|_U|_L|_D|_SP)) != 0)
#define ispunct(c)	((_nolibc_ismask(c)&(_P)) != 0)
#define isspace(c)	((_nolibc_ismask(c)&(_S)) != 0)
#define isupper(c)	((_nolibc_ismask(c)&(_U)) != 0)
#define isxdigit(c)	((_nolibc_ismask(c)&(_D|_X)) != 0)

#define isascii(c)	(((unsigned char)(c)) <= 0x7f)
#define toascii(c)	((int)(((unsigned char)(c)) & 0x7f))

static inline int tolower(int c)
{
	if (isupper(c))
		c -= 'A'-'a';
	return c;
}

static inline int toupper(int c)
{
	if (islower(c))
		c -= 'a'-'A';
	return c;
}

#ifdef __cplusplus
}
#endif

#endif /* __CTYPE_H__ */
