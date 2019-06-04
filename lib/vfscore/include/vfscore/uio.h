/*-
 * Copyright (c) 1982, 1986, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
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
 *
 *	@(#)uio.h	8.5 (Berkeley) 2/22/94
 * $FreeBSD$
 */

#ifndef _UIO_H_
#define	_UIO_H_

#include <sys/types.h>
#include <sys/uio.h>
#include <limits.h>

enum	uio_rw { UIO_READ, UIO_WRITE };

/*
 * Safe default to prevent possible overflows in user code, otherwise could
 * be SSIZE_T_MAX.
 */
#define IOSIZE_MAX      INT_MAX

#define UIO_MAXIOV 1024

#define UIO_SYSSPACE 0

struct uio {
	struct iovec *uio_iov;		/* scatter/gather list */
	int	uio_iovcnt;		/* length of scatter/gather list */
	off_t	uio_offset;		/* offset in target object */
	ssize_t	uio_resid;		/* remaining bytes to process */
	enum	uio_rw uio_rw;		/* operation */
};

/* This is a wrapper for functions f with a "memcpy-like" signature
 * "dst, src, cnt" to be executed over a scatter-gather list provided
 * by a struct uio. f() might be called multiple times to read from or
 * write to cp up to n bytes of data (or up to the capacity of the uio
 * scatter-gather buffers).
 */
static inline
int vfscore_uioforeach(int (*f)(void *, void *, size_t *), void *cp,
		      size_t n, struct uio *uio)
{
	int ret = 0;

	UK_ASSERT(uio->uio_rw == UIO_READ || uio->uio_rw == UIO_WRITE);

	while (n > 0 && uio->uio_resid) {
		struct iovec *iov = uio->uio_iov;
		size_t cnt = iov->iov_len;

		if (cnt == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			continue;
		}
		if (cnt > n)
			cnt = n;

		if (uio->uio_rw == UIO_READ)
			ret = f(iov->iov_base, cp, &cnt);
		else
			ret = f(cp, iov->iov_base, &cnt);

		iov->iov_base = (char *)iov->iov_base + cnt;
		iov->iov_len -= cnt;
		uio->uio_resid -= cnt;
		uio->uio_offset += cnt;
		cp = (char *)cp + cnt;
		n -= cnt;

		if (ret)
			break;
	}

	return ret;
}

int	vfscore_uiomove(void *cp, int n, struct uio *uio);

#endif /* !_UIO_H_ */
