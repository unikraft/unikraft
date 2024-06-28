/* SPDX-License-Identifier: BSD-3-Clause */
/*
 ****************************************************************************
 *
 *        File: printf.c
 *      Author: Juergen Gross <jgross@suse.com>
 *
 *        Date: Jun 2016
 *
 * Environment: Xen Minimal OS
 * Description: Library functions for printing
 *              (FreeBSD port)
 *
 ****************************************************************************
 */

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Copyright (c) 2011 The FreeBSD Foundation
 * All rights reserved.
 * Portions of this software were developed by David Chisnall
 * under sponsorship from the FreeBSD Foundation.
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

#include <stddef.h>
#include <stdlib.h>
#include <inttypes.h>
#include <limits.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <uk/essentials.h>
#include <uk/arch/lcpu.h>

#if CONFIG_LIBUKCONSOLE
#include <uk/console.h>

/* TODO: Some consoles require both a newline and a carriage return to
 * go to the start of the next line. This kind of behavior should be in
 * a single place in posix-tty. We keep this workaround until we have feature
 * in posix-tty that handles newline characters correctly.
 */
static inline __ssz _console_out(const char *buf, __sz len)
{
	const char *next_nl = NULL;
	__sz l = len;
	__sz off = 0;
	__ssz rc = 0;

	if (unlikely(!len))
		return 0;
	if (unlikely(!buf))
		return -EINVAL;

	while (l > 0) {
		next_nl = memchr(buf, '\n', l);
		if (next_nl) {
			off = next_nl - buf;
			if ((rc = uk_console_out(buf, off)) < 0)
				return rc;
			if ((rc = uk_console_out("\r\n", 2)) < 0)
				return rc;
			buf = next_nl + 1;
			l -= off + 1;
		} else {
			if ((rc = uk_console_out(buf, l)) < 0)
				return rc;
			break;
		}
	}

	return len;
}
#endif /* CONFIG_LIBUKCONSOLE */

/* 64 bits + 0-Byte at end */
#define MAXNBUF 65

/* basic support, gives the notion of having standard files but they do not have
 * any meaning
 */
FILE *stdin = (FILE *)&stdin;
FILE *stdout = (FILE *)&stdout;
FILE *stderr = (FILE *)&stderr;

static char const hex2ascii_data[] = "0123456789abcdefghijklmnopqrstuvwxyz";
/*
 * Put a NUL-terminated ASCII number (base <= 36) in a buffer in reverse
 * order; return an optional length and a pointer to the last character
 * written in the buffer (i.e., the first character of the string).
 * The buffer pointed to by `nbuf' must have length >= MAXNBUF.
 */
static inline char *ksprintn(char *nbuf, uintmax_t num, int base, int *lenp,
			     int upper)
{
	char *p, c;

	p = nbuf;
	*p = '\0';
	do {
		c = hex2ascii_data[num % base];
		*++p = upper ? toupper(c) : c;
	} while (num /= base);
	if (lenp)
		*lenp = p - nbuf;
	return p;
}

/*
 * Scaled down version of printf(3).
 */
int vsnprintf(char *str, size_t size, const char *fmt, va_list ap)
{
#define PCHAR(c)                                                               \
	{                                                                      \
		int cc = (c);                                                  \
		if (size >= 2) {                                               \
			*str++ = cc;                                           \
			size--;                                                \
		}                                                              \
		retval++;                                                      \
	}
	char nbuf[MAXNBUF];
	const char *p, *percent;
	int ch, n;
	uintmax_t num;
	int base, lflag, llflag, tmp, width, ladjust, sharpflag, neg, sign, dot;
	int cflag, hflag, jflag, tflag, zflag;
	int dwidth, upper;
	char padc;
	int stop = 0, retval = 0;

	num = 0;

	if (fmt == NULL)
		fmt = "(fmt null)\n";

	for (;;) {
		padc = ' ';
		width = 0;
		while ((ch = (unsigned char)*fmt++) != '%' || stop) {
			if (ch == '\0') {
				if (size >= 1)
					*str++ = '\0';
				return retval;
			}
			PCHAR(ch);
		}
		percent = fmt - 1;
		llflag = 0;
		lflag = 0;
		ladjust = 0;
		sharpflag = 0;
		neg = 0;
		sign = 0;
		dot = 0;
		dwidth = 0;
		upper = 0;
		cflag = 0;
		hflag = 0;
		jflag = 0;
		tflag = 0;
		zflag = 0;
reswitch:
		switch (ch = (unsigned char)*fmt++) {
		case '.':
			dot = 1;
			goto reswitch;
		case '#':
			sharpflag = 1;
			goto reswitch;
		case '+':
			sign = 1;
			goto reswitch;
		case '-':
			ladjust = 1;
			goto reswitch;
		case '%':
			PCHAR(ch);
			break;
		case '*':
			if (!dot) {
				width = va_arg(ap, int);
				if (width < 0) {
					ladjust = !ladjust;
					width = -width;
				}
			} else {
				dwidth = va_arg(ap, int);
			}
			goto reswitch;
		case '0':
			if (!dot) {
				padc = '0';
				goto reswitch;
			}
			__fallthrough;
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			for (n = 0;; ++fmt) {
				n = n * 10 + ch - '0';
				ch = *fmt;
				if (ch < '0' || ch > '9')
					break;
			}
			if (dot)
				dwidth = n;
			else
				width = n;
			goto reswitch;
		case 'c':
			PCHAR(va_arg(ap, int));
			break;
		case 'd':
		case 'i':
			base = 10;
			sign = 1;
			goto handle_sign;
		case 'h':
			if (hflag) {
				hflag = 0;
				cflag = 1;
			} else
				hflag = 1;
			goto reswitch;
		case 'j':
			jflag = 1;
			goto reswitch;
		case 'l':
			if (lflag) {
				lflag = 0;
				llflag = 1;
			} else
				lflag = 1;
			goto reswitch;
		case 'n':
			if (jflag)
				*(va_arg(ap, intmax_t *)) = retval;
			else if (llflag)
				*(va_arg(ap, long long *)) = retval;
			else if (lflag)
				*(va_arg(ap, long *)) = retval;
			else if (zflag)
				*(va_arg(ap, size_t *)) = retval;
			else if (hflag)
				*(va_arg(ap, short *)) = retval;
			else if (cflag)
				*(va_arg(ap, char *)) = retval;
			else
				*(va_arg(ap, int *)) = retval;
			break;
		case 'o':
			base = 8;
			goto handle_nosign;
		case 'p':
			base = 16;
			sharpflag = (width == 0);
			sign = 0;
			num = (uintptr_t)va_arg(ap, void *);
			goto number;
		case 'q':
			llflag = 1;
			goto reswitch;
		case 'r':
			base = 10;
			if (sign)
				goto handle_sign;
			goto handle_nosign;
		case 's':
			p = va_arg(ap, char *);
			if (p == NULL)
				p = "(null)";
			if (!dot)
				n = strlen(p);
			else
				for (n = 0; n < dwidth && p[n]; n++)
					continue;

			width -= n;

			if (!ladjust && width > 0)
				while (width--)
					PCHAR(padc);
			while (n--)
				PCHAR(*p++);
			if (ladjust && width > 0)
				while (width--)
					PCHAR(padc);
			break;
		case 't':
			tflag = 1;
			goto reswitch;
		case 'u':
			base = 10;
			goto handle_nosign;
		case 'X':
			upper = 1;
			/* Fall through */
		case 'x':
			base = 16;
			goto handle_nosign;
		case 'y':
			base = 16;
			sign = 1;
			goto handle_sign;
		case 'z':
			zflag = 1;
			goto reswitch;
handle_nosign:
			sign = 0;
			if (jflag)
				num = va_arg(ap, uintmax_t);
			else if (llflag)
				num = va_arg(ap, unsigned long long);
			else if (tflag)
				num = va_arg(ap, ptrdiff_t);
			else if (lflag)
				num = va_arg(ap, unsigned long);
			else if (zflag)
				num = va_arg(ap, size_t);
			else if (hflag)
				num = (unsigned short)va_arg(ap, int);
			else if (cflag)
				num = (unsigned char)va_arg(ap, int);
			else
				num = va_arg(ap, unsigned int);
			goto number;
handle_sign:
			if (jflag)
				num = va_arg(ap, intmax_t);
			else if (llflag)
				num = va_arg(ap, long long);
			else if (tflag)
				num = va_arg(ap, ptrdiff_t);
			else if (lflag)
				num = va_arg(ap, long);
			else if (zflag)
				num = va_arg(ap, ssize_t);
			else if (hflag)
				num = (short)va_arg(ap, int);
			else if (cflag)
				num = (char)va_arg(ap, int);
			else
				num = va_arg(ap, int);
number:
			if (sign && (intmax_t)num < 0) {
				neg = 1;
				num = -(intmax_t)num;
			}
			p = ksprintn(nbuf, num, base, &n, upper);
			tmp = 0;
			if (sharpflag && num != 0) {
				if (base == 8)
					tmp++;
				else if (base == 16)
					tmp += 2;
			}
			if (neg)
				tmp++;

			if (!ladjust && padc == '0')
				dwidth = width - tmp;
			width -= tmp + (dwidth > n ? dwidth : n);
			dwidth -= n;
			if (!ladjust)
				while (width-- > 0)
					PCHAR(' ');
			if (neg)
				PCHAR('-');
			if (sharpflag && num != 0) {
				if (base == 8) {
					PCHAR('0');
				} else if (base == 16) {
					PCHAR('0');
					PCHAR('x');
				}
			}
			while (dwidth-- > 0)
				PCHAR('0');

			while (*p)
				PCHAR(*p--);

			if (ladjust)
				while (width-- > 0)
					PCHAR(' ');

			break;
		default:
			while (percent < fmt)
				PCHAR(*percent++);
			/*
			 * Since we ignore a formatting argument it is no
			 * longer safe to obey the remaining formatting
			 * arguments as the arguments will no longer match
			 * the format specs.
			 */
			stop = 1;
			break;
		}
	}
#undef PCHAR
}

int vsprintf(char *str, const char *fmt, va_list ap)
{
	return vsnprintf(str, SIZE_MAX, fmt, ap);
}

int snprintf(char *str, size_t size, const char *fmt, ...)
{
	int ret;
	va_list ap;

	va_start(ap, fmt);
	ret = vsnprintf(str, size, fmt, ap);
	va_end(ap);

	return ret;
}

int sprintf(char *str, const char *fmt, ...)
{
	int ret;
	va_list ap;

	va_start(ap, fmt);
	ret = vsprintf(str, fmt, ap);
	va_end(ap);

	return ret;
}

int vfprintf(FILE *fp __maybe_unused, const char *fmt __maybe_unused,
	     va_list ap __maybe_unused)
{
	int ret;
	char buf[1024];

	ret = vsnprintf(buf, sizeof(buf), fmt, ap);
	if (ret < 0)
		return ret;

	/* we do not support device handling for now, we
	 * just send the buffer content to the kernel console
	 */
	if (fp == stdout || fp == stderr)
#if CONFIG_LIBUKCONSOLE
		ret = _console_out(buf, ret);
#else /* !CONFIG_LIBUKCONSOLE */
		ret = strlen(buf);
#endif /* !CONFIG_LIBUKCONSOLE */
	else
		return 0;

	return ret;
}

int fprintf(FILE *fp, const char *fmt, ...)
{
	int ret;
	va_list ap;

	va_start(ap, fmt);
	ret = vfprintf(fp, fmt, ap);
	va_end(ap);

	return ret;
}

int vprintf(const char *fmt, va_list ap)
{
	return vfprintf(stdout, fmt, ap);
}

int printf(const char *fmt, ...)
{
	int ret;
	va_list ap;

	va_start(ap, fmt);
	ret = vprintf(fmt, ap);
	va_end(ap);

	return ret;
}

int fflush(FILE *fp __unused)
{
	/* nolibc is not working with buffers */
	return 0;
}

int fputc(int _c __maybe_unused, FILE *fp __maybe_unused)
{
#if CONFIG_LIBUKCONSOLE
	int ret = 0;
	unsigned char c = _c;

	if (fp == stdout || fp == stderr)
		ret = _console_out((char *)&c, 1);

	if (ret == 1)
		return _c;

	return EOF;
#endif /* CONFIG_LIBUKCONSOLE */
	return _c;
}

int putchar(int c)
{
	return fputc(c, stdout);
}

static int
fputs_internal(const char *restrict s __maybe_unused,
	       FILE *restrict stream __maybe_unused,
	       int newline __maybe_unused)
{
#if CONFIG_LIBUKCONSOLE
	int ret;
	size_t len;

	len = strlen(s);

	if (stream == stdout || stream == stderr)
		ret = _console_out(s, len);
	else
		return EOF;

	/* If _console_out wasn't able to write all characters, assume
	 * that an error happened and there is no point in retrying.
	 */
	if ((size_t)ret != len)
		return EOF;

	if (newline)
		return fputc('\n', stream);
#endif /* !CONFIG_LIBUKCONSOLE */
	return 1;
}

int fputs(const char *restrict s, FILE *restrict stream)
{
	return fputs_internal(s, stream, 0);
}

int puts(const char *s)
{
	return fputs_internal(s, stdout, 1);
}

#if CONFIG_LIBVFSCORE
struct _nolibc_file {
	int fd;
	int errno;
	bool eof;
	off_t offset;
};

void clearerr(FILE *stream)
{
	stream->eof = 0;
	stream->errno = 0;
}

/* The following code is derived from musl libc */
static int __fmodeflags(const char *mode, int *flags)
{
	if (!strchr("rwa", *mode)) {
		errno = EINVAL;
		return -1;
	}
	if (strchr(mode, '+'))
		*flags = O_RDWR;
	else if (*mode == 'r')
		*flags = O_RDONLY;
	else
		*flags = O_WRONLY;
	if (strchr(mode, 'x'))
		*flags |= O_EXCL;
	if (strchr(mode, 'e'))
		*flags |= O_CLOEXEC;
	if (*mode != 'r')
		*flags |= O_CREAT;
	if (*mode == 'w')
		*flags |= O_TRUNC;
	if (*mode == 'a')
		*flags |= O_APPEND;

	return 0;
}

FILE *fopen(const char *pathname, const char *mode)
{
	int flags;
	int fd;
	int ret;

	ret = __fmodeflags(mode, &flags);

	if (ret < 0)
		return NULL;

	fd = open(pathname, flags, 0666);

	if (fd < 0)
		return NULL;

	return fdopen(fd, mode);
}

int feof(FILE *stream)
{
	return stream->eof;
}

int ferror(FILE *stream)
{
	return stream->errno;
}

int fclose(FILE *stream)
{
	int ret = close(stream->fd);

	free(stream);
	return ret;
}

FILE *fdopen(int fd, const char *mode __unused)
{
	FILE *f = (FILE *)malloc(sizeof(FILE));

	if (!f)
		return NULL;
	f->fd = fd;
	f->errno = 0;
	f->eof = 0;
	f->offset = 0;
	return f;
}

int fseek(FILE *stream, long offset, int whence)
{
	off_t new_offset;

	switch (whence) {
	case SEEK_SET:
	{
		// Absolute offset from the beginning
		if (unlikely(offset < 0)) {
			stream->errno = 1;
			return -1;
		}
		new_offset = offset;
		break;
	}

	case SEEK_CUR:
	{
		// Offset from the current position
		new_offset = stream->offset + offset;
		if (unlikely(new_offset < 0)) {
			stream->errno = 1;
			return -1;
		}
		break;
	}

	case SEEK_END:
	{
		struct stat st;

		if (unlikely(fstat(stream->fd, &st) != 0)) {
			stream->errno = 1;
			return -1;
		}

		new_offset = st.st_size + offset;
		if (unlikely(new_offset < 0)) {
			stream->errno = 1;
			return -1;
		}
	}
	break;

	default:
	{
		// Invalid whence
		stream->errno = 1;
		return -1;
	}
	}

	// Update the stream's offset
	stream->offset = new_offset;
	return 0;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	if (unlikely(!stream))
		return 0;

	if (unlikely(!ptr || !size || !nmemb)) {
		stream->errno = EINVAL;
		return 0;
	}

	if (unlikely(stream->fd < 0)) {
		stream->errno = EBADF;
		return 0;
	}

	if (unlikely(SIZE_MAX / size < nmemb)) {
		stream->errno = EOVERFLOW;
		return 0;
	}

	size_t total = 0;

	while (total < size * nmemb) {
		ssize_t ret = pread(stream->fd,
				((char *) ptr) + total,
				size * nmemb - total, stream->offset);
		if (ret > 0)
			stream->offset += ret;  // Update the offset

		if (ret < 0)
			return total / size;
		if (ret == 0) {
			stream->eof = 1;
			return total / size;
		}
		total += ret;
	}

	return total / size;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	if (unlikely(!stream))
		return 0;

	if (unlikely(!ptr || !size || !nmemb)) {
		stream->errno = EINVAL;
		return 0;
	}

	if (unlikely(stream->fd < 0)) {
		stream->errno = EBADF;
		return 0;
	}

	if (unlikely(SIZE_MAX / size < nmemb)) {
		stream->errno = EOVERFLOW;
		return 0;
	}

	size_t total = 0;

	while (total < size * nmemb) {
		ssize_t ret = pwrite(stream->fd,
				((char *) ptr) + total,
				size * nmemb - total, stream->offset);
		if (ret > 0)
			stream->offset += ret;  // Update the offset

		if (ret < 0)
			return total / size;
		total += ret;
	}

	return total / size;
}
#endif /* CONFIG_LIBVFSCORE */
