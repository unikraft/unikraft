/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 1992, 1993
 * The Regents of the University of California.  All rights reserved.
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/* Derived from FreeBSD commit 4736ccf (Nov 20, 2017) */

#ifndef __SYS_SELECT_H__
#define __SYS_SELECT_H__

#include <sys/param.h>
#ifdef __cplusplus
extern "C" {
#endif

#define __NEED_time_t
#define __NEED_suseconds_t
#define __NEED_struct_timespec
#include <nolibc-internal/shareddefs.h>

typedef unsigned long __fd_mask;

/*
 * Select uses bit masks of file descriptors in longs. These macros
 * manipulate such bit fields (the filesystem macros use chars).
 * FD_SETSIZE may be defined by the user, but the default here should
 * be enough for most uses.
 */
#ifndef FD_SETSIZE
#define FD_SETSIZE 64
#endif

#define _NFDBITS (sizeof(__fd_mask) * 8) /* bits per mask */

typedef struct fd_set {
	__fd_mask __fds_bits[howmany(FD_SETSIZE, _NFDBITS)];
} fd_set;

#define	__fdset_mask(n)	((__fd_mask)1 << ((n) % _NFDBITS))
#define	FD_CLR(n, p)	((p)->__fds_bits[(n)/_NFDBITS] &= ~__fdset_mask(n))
#define	FD_ISSET(n, p)	(((p)->__fds_bits[(n)/_NFDBITS] & __fdset_mask(n)) != 0)
#define	FD_SET(n, p)	((p)->__fds_bits[(n)/_NFDBITS] |= __fdset_mask(n))
#define	FD_ZERO(p) do {					\
	fd_set *_p;					\
	__ssz _n;					\
							\
	_p = (p);					\
	_n = howmany(FD_SETSIZE, _NFDBITS);		\
	while (_n > 0)					\
		_p->__fds_bits[--_n] = 0;		\
} while (0)

#ifdef __cplusplus
}
#endif

#endif /* __SYS_SELECT_H__ */
