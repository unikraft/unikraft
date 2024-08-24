/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/essentials.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>

#if CONFIG_LIBUKCONSOLE
#include <uk/console.h>
#endif /* CONFIG_LIBUKCONSOLE */

static int
uk_scanf(void *buffer __maybe_unused, size_t *cnt)
{
#if CONFIG_LIBUKCONSOLE
	int bytes_read;
	size_t bytes_total = 0, count;
	char *buf = buffer;

	/* Need at least two bytes: one for user input and one for
	 * the NULL termination. The byte for user input is required,
	 * otherwise we run into an endless loop on `ukplat_cink`.
	 */
	if (*cnt <= 1)
		return -EINVAL;

	count = *cnt - 1;

	do {
		do {
			bytes_read = uk_console_in(buf,
						   count - bytes_total);
		} while (bytes_read <= 0);

		buf = buf + bytes_read;
		*(buf - 1) = *(buf - 1) == '\r' ?
					'\n' : *(buf - 1);

		/* Echo the input */
		if (*(buf - 1) == '\177') {
			/* DELETE control character */
			if (buf - 1 != buffer) {
				/* If this is not the first byte */
				uk_console_out("\b \b", 3);
				buf -= 1;
				if (bytes_total > 0)
					bytes_total--;
			}
			buf -= 1;
		} else {
			uk_console_out(buf - bytes_read, bytes_read);
			bytes_total += bytes_read;
		}

	} while (bytes_total < count &&
	((bytes_total == 0) || (*(buf - 1) != '\n' && *(buf - 1) != '\0')));

	((char *)buffer)[bytes_total] = '\0';
	*cnt = bytes_total;

	return 0;
#else /* !CONFIG_LIBUKCONSOLE */
	*cnt = 0;
	return EOF;
#endif /* !CONFIG_LIBUKCONSOLE */
}

int
vfscanf(FILE *fp, const char *fmt, va_list ap)
{
	int ret = 0;
	char buf[1024];
	size_t size = sizeof(buf);

	if (fp == stdin)
		ret = uk_scanf(buf, &size);
	else
		return 0;

	if (ret < 0)
		return ret;

	ret = vsscanf(buf, fmt, ap);
	return ret;
}

int
vscanf(const char *fmt, va_list ap)
{
	return vfscanf(stdin, fmt, ap);
}

int
fscanf(FILE *fp, const char *fmt, ...)
{
	int ret;
	va_list ap;

	va_start(ap, fmt);
	ret = vfscanf(fp, fmt, ap);
	va_end(ap);

	return ret;
}

int
scanf(const char *fmt, ...)
{
	int ret;
	va_list ap;

	va_start(ap, fmt);
	ret = vscanf(fmt, ap);
	va_end(ap);
	return ret;
}
