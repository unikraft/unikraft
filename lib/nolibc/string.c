/* SPDX-License-Identifier: BSD-3-Clause AND MIT */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
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

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>

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

void *memrchr(const void *m, int c, size_t n)
{
	const unsigned char *s = m;

	c = (unsigned char) c;
	while (n--)
		if (s[n] == c)
			return (void *) (s + n);
	return 0;
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
	const char *p = memchr(str, 0, len);
	return p ? (size_t) (p - str) : len;
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

/* The following code is taken from musl libc */
#define ALIGN (sizeof(size_t))
#define ONES ((size_t) -1 / UCHAR_MAX)
#define HIGHS (ONES * (UCHAR_MAX / 2 + 1))
#define HASZERO(x) (((x) - ONES) & ~(x) & HIGHS)
#define BITOP(a, b, op) \
		((a)[(size_t)(b) / (8*sizeof *(a))] op \
		(size_t)1 << ((size_t)(b) % (8 * sizeof *(a))))

char *strchrnul(const char *s, int c)
{
	size_t *w, k;

	c = (unsigned char)c;
	if (!c)
		return (char *)s + strlen(s);

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

char *strchr(const char *str, int c)
{
	char *r = strchrnul(str, c);
	return *(unsigned char *)r == (unsigned char)c ? r : 0;
}

char *strrchr(const char *s, int c)
{
	return memrchr(s, c, strlen(s) + 1);
}

size_t strcspn(const char *s, const char *c)
{
	const char *a = s;
	size_t byteset[32 / sizeof(size_t)];

	if (!c[0] || !c[1])
		return strchrnul(s, *c)-a;

	memset(byteset, 0, sizeof(byteset));
	for (; *c && BITOP(byteset, *(unsigned char *)c, |=); c++)
		;
	for (; *s && !BITOP(byteset, *(unsigned char *)s, &); s++)
		;
	return s-a;
}

size_t strspn(const char *s, const char *c)
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

char *strtok(char *restrict s, const char *restrict sep)
{
	static char *p;

	if (!s && !(s = p))
		return NULL;
	s += strspn(s, sep);
	if (!*s)
		return p = 0;
	p = s + strcspn(s, sep);
	if (*p)
		*p++ = 0;
	else
		p = 0;
	return s;
}

char *strndup(const char *str, size_t len)
{
	char *__res;
	int __len;

	__len = strnlen(str, len);

	__res = malloc(__len + 1);
	if (__res) {
		memcpy(__res, str, __len);
		__res[__len] = '\0';
	}

	return __res;
}

char *strdup(const char *str)
{
	return strndup(str, SIZE_MAX);
}

/* strlcpy has different ALIGN */
#undef ALIGN
#define ALIGN (sizeof(size_t)-1)
size_t strlcpy(char *d, const char *s, size_t n)
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
	return d-d0 + strlen(s);
}

size_t strlcat(char *d, const char *s, size_t n)
{
	size_t l = strnlen(d, n);
	if (l == n)
		return l + strlen(s);
	return l + strlcpy(d+l, s, n-l);
}

/* GNU-specific version of strerror_r */
char *strerror_r(int errnum, char *buf, size_t buflen)
{
	const char *strerr;

	switch (errnum) {
	case EPERM:
		strerr = "Operation not permitted";
		break;
	case ENOENT:
		strerr = "No such file or directory";
		break;
	case ESRCH:
		strerr = "No such process";
		break;
	case EINTR:
		strerr = "Interrupted system call";
		break;
	case EIO:
		strerr = "Input/output error";
		break;
	case ENXIO:
		strerr = "Device not configured";
		break;
	case E2BIG:
		strerr = "Argument list too long";
		break;
	case ENOEXEC:
		strerr = "Exec format error";
		break;
	case EBADF:
		strerr = "Bad file descriptor";
		break;
	case ECHILD:
		strerr = "No child processes";
		break;
	case EDEADLK:
		strerr = "Resource deadlock avoided";
		break;
	case ENOMEM:
		strerr = "Cannot allocate memory";
		break;
	case EACCES:
		strerr = "Permission denied";
		break;
	case EFAULT:
		strerr = "Bad address";
		break;
	case ENOTBLK:
		strerr = "Block device required";
		break;
	case EBUSY:
		strerr = "Device busy";
		break;
	case EEXIST:
		strerr = "File exists";
		break;
	case EXDEV:
		strerr = "Cross-device link";
		break;
	case ENODEV:
		strerr = "Operation not supported by device";
		break;
	case ENOTDIR:
		strerr = "Not a directory";
		break;
	case EISDIR:
		strerr = "Is a directory";
		break;
	case EINVAL:
		strerr = "Invalid argument";
		break;
	case ENFILE:
		strerr = "Too many open files in system";
		break;
	case EMFILE:
		strerr = "Too many open files";
		break;
	case ENOTTY:
		strerr = "Inappropriate ioctl for device";
		break;
	case ETXTBSY:
		strerr = "Text file busy";
		break;
	case EFBIG:
		strerr = "File too large";
		break;
	case ENOSPC:
		strerr = "No space left on device";
		break;
	case ESPIPE:
		strerr = "Illegal seek";
		break;
	case EROFS:
		strerr = "Read-only file system";
		break;
	case EMLINK:
		strerr = "Too many links";
		break;
	case EPIPE:
		strerr = "Broken pipe";
		break;
	case EDOM:
		strerr = "Numerical argument out of domain";
		break;
	case ERANGE:
		strerr = "Result too large";
		break;
	case EAGAIN:
		strerr = "Resource temporarily unavailable";
		break;
	case EINPROGRESS:
		strerr = "Operation now in progress";
		break;
	case EALREADY:
		strerr = "Operation already in progress";
		break;
	case ENOTSOCK:
		strerr = "Socket operation on non-socket";
		break;
	case EDESTADDRREQ:
		strerr = "Destination address required";
		break;
	case EMSGSIZE:
		strerr = "Message too long";
		break;
	case EPROTOTYPE:
		strerr = "Protocol wrong type for socket";
		break;
	case ENOPROTOOPT:
		strerr = "Protocol not available";
		break;
	case EPROTONOSUPPORT:
		strerr = "Protocol not supported";
		break;
	case ESOCKTNOSUPPORT:
		strerr = "Socket type not supported";
		break;
	case EOPNOTSUPP:
		strerr = "Operation not supported on socket";
		break;
	case EPFNOSUPPORT:
		strerr = "Protocol family not supported";
		break;
	case EAFNOSUPPORT:
		strerr = "Address family not supported by protocol family";
		break;
	case EADDRINUSE:
		strerr = "Address already in use";
		break;
	case EADDRNOTAVAIL:
		strerr = "Can't assign requested address";
		break;
	case ENETDOWN:
		strerr = "Network is down";
		break;
	case ENETUNREACH:
		strerr = "Network is unreachable";
		break;
	case ENETRESET:
		strerr = "Network dropped connection on reset";
		break;
	case ECONNABORTED:
		strerr = "Software caused connection abort";
		break;
	case ECONNRESET:
		strerr = "Connection reset by peer";
		break;
	case ENOBUFS:
		strerr = "No buffer space available";
		break;
	case EISCONN:
		strerr = "Socket is already connected";
		break;
	case ENOTCONN:
		strerr = "Socket is not connected";
		break;
	case ESHUTDOWN:
		strerr = "Can't send after socket shutdown";
		break;
	case ETIMEDOUT:
		strerr = "Operation timed out";
		break;
	case ECONNREFUSED:
		strerr = "Connection refused";
		break;
	case ELOOP:
		strerr = "Too many levels of symbolic links";
		break;
	case ENAMETOOLONG:
		strerr = "File name too long";
		break;
	case EHOSTDOWN:
		strerr = "Host is down";
		break;
	case EHOSTUNREACH:
		strerr = "No route to host";
		break;
	case ENOTEMPTY:
		strerr = "Directory not empty";
		break;
	case EPROCLIM:
		strerr = "Too many processes";
		break;
	case EUSERS:
		strerr = "Too many users";
		break;
	case EDQUOT:
		strerr = "Disc quota exceeded";
		break;
	case ESTALE:
		strerr = "Stale NFS file handle";
		break;
	case EBADRPC:
		strerr = "RPC struct is bad";
		break;
	case ERPCMISMATCH:
		strerr = "RPC version wrong";
		break;
	case EPROGUNAVAIL:
		strerr = "RPC prog";
		break;
	case EPROGMISMATCH:
		strerr = "Program version wrong";
		break;
	case EPROCUNAVAIL:
		strerr = "Bad procedure for program";
		break;
	case ENOLCK:
		strerr = "No locks available";
		break;
	case ENOSYS:
		strerr = "Function not implemented";
		break;
	case EFTYPE:
		strerr = "Inappropriate file type or format";
		break;
	case EAUTH:
		strerr = "Authentication error";
		break;
	case ENEEDAUTH:
		strerr = "Need authenticator";
		break;
	case EIDRM:
		strerr = "Identifier removed";
		break;
	case ENOMSG:
		strerr = "No message of desired type";
		break;
	case EOVERFLOW:
		strerr = "Value too large to be stored in data type";
		break;
	case ECANCELED:
		strerr = "Operation canceled";
		break;
	case EILSEQ:
		strerr = "Illegal byte sequence";
		break;
	case ENOATTR:
		strerr = "Attribute not found";
		break;
	case EDOOFUS:
		strerr = "Programming error";
		break;
	case EBADMSG:
		strerr = "Bad message";
		break;
	case EMULTIHOP:
		strerr = "Multihop attempted";
		break;
	case ENOLINK:
		strerr = "Link has been severed";
		break;
	case EPROTO:
		strerr = "Protocol error";
		break;
	case ENOTCAPABLE:
		strerr = "Capabilities insufficient";
		break;
	case ECAPMODE:
		strerr = "Not permitted in capability mode";
		break;
	case ENOTRECOVERABLE:
		strerr = "State not recoverable";
		break;
	case EOWNERDEAD:
		strerr = "Previous owner died";
		break;
	case ENOTSUP:
		strerr = "Not supported";
		break;
	default:
		strerr = NULL;
		errno = EINVAL; /* Unknown errnum requires errno to be set */
		break;
	}

	if (!buflen)
		return buf;

	/*
	 * NOTE: If target buffer is too small, we are supposed to set
	 *       errno to ERANGE. We ignore this case for simplification.
	 */
	if (strerr)
		strncpy(buf, strerr, buflen);
	else
		snprintf(buf, buflen, "Unknown error %d", errnum);

	/* ensure null termination */
	buf[buflen - 1] = '\0';
	return buf;
}

/* NOTE: strerror() is not thread-safe, nor reentrant-safe */
char *strerror(int errnum)
{
	/* NOTE: Our longest message is currently 48 characters. With
	 *       64 characters we should have room for minor changes
	 *       in the future.
	 */
	static char buf[64];

	return strerror_r(errnum, buf, sizeof(buf));
}
