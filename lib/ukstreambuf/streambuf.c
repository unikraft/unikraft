/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <stdio.h>
#include <string.h>
#include <uk/streambuf.h>
#include <uk/assert.h>

__ssz fastncat(char *buf, __sz buflen, const char *src)
{
	char *wptr = buf;
	const char *rptr = src;
	__sz left = buflen;

	UK_ASSERT(wptr);

	if (unlikely(!rptr || (left == 0)))
		return 0;

	/* Copy string to wptr position */
	while ((*rptr != '\0') && (left > 1)) {
		*(wptr++) = *(rptr++);
		left--;
	}
	/* Ensure NULL-termination */
	*(wptr++) = '\0';

	if (*rptr != '\0') {
		/* If we get here the target buffer is full and there
		 * are still characters available on the source string
		 */
		return -1;
	}

	/* Return the number of appended bytes (including '\0' termination) */
	return (__ssz)((__uptr) wptr - (__uptr) buf);
}

void uk_streambuf_reset(struct uk_streambuf *sb)
{
	sb->seek = 0;
	sb->flags &= UK_STREAMBUF_C_MASK; /* Clear status flags */
	if (sb->flags & UK_STREAMBUF_C_WIPEZERO)
		memset(sb->bufp, 0x0, sb->buflen);
	else if (sb->flags & UK_STREAMBUF_C_TERMSHIFT)
		sb->bufp[0] = '\0'; /* Quick clear buffer */
}

void uk_streambuf_init(struct uk_streambuf *sb,
		       void *buf, __sz buflen,
		       int cfg_flags)
{
	UK_ASSERT(buf);
	UK_ASSERT(buflen);
	/* Allow only config flags as input */
	UK_ASSERT((cfg_flags & UK_STREAMBUF_C_MASK) == cfg_flags);

	sb->bufp   = (char *) buf;
	sb->buflen = buflen;
	sb->flags  = cfg_flags;
#if CONFIG_LIBUKALLOC
	sb->_a    = __NULL;
#endif /* CONFIG_LIBUKALLOC */

	uk_streambuf_reset(sb);
}

#if CONFIG_LIBUKALLOC
struct uk_streambuf *uk_streambuf_alloc2(struct uk_alloc *a,
					 __sz buflen, __sz bufalign, int flags)
{
	struct uk_streambuf *sb;
	__sz alloc_len;

	UK_ASSERT(POWER_OF_2(bufalign));

	alloc_len = ALIGN_UP(sizeof(*sb), bufalign) + buflen;

	sb = (struct uk_streambuf *) uk_memalign(a, bufalign, alloc_len);
	if (unlikely(!sb))
		return NULL;

	uk_streambuf_init(sb, (void *)((__uptr) sb
			      + ALIGN_UP(sizeof(*sb), bufalign)),
			  buflen, flags);
	sb->_a = a;
	return sb;
}

void uk_streambuf_free(struct uk_streambuf *sb)
{
	UK_ASSERT(sb->_a);
	uk_free(sb->_a, sb);
}
#endif /* CONFIG_LIBUKALLOC */

__sz uk_streambuf_vprintf(struct uk_streambuf *sb, const char *fmt, va_list ap)
{
	int rc;

	if (uk_streambuf_left((sb)) == 0) {
		(sb)->flags |= UK_STREAMBUF_S_TRUNCATED;
		return 0;
	}
	if (uk_streambuf_left((sb)) == 1) {
		/* No need to call vsnprintf(), there is only space for '\0' */
		*((char *) uk_streambuf_wptr(sb)) = '\0';
		sb->seek = uk_streambuf_seek(sb) + 1;
		sb->flags |= UK_STREAMBUF_S_TRUNCATED;
		return 1;
	}

	rc = vsnprintf(uk_streambuf_wptr(sb), uk_streambuf_left(sb), fmt, ap);
	if (rc < 0) {
		/* Error happened, we undo operation: Wipe with putting
		 * terminating '\0' at current position
		 */
		*((char *) uk_streambuf_wptr(sb)) = '\0';
		return 0;
	} else if ((__sz) rc >= uk_streambuf_left(sb)) {
		/* We did not have enough space, snprintf should have filled
		 * up everything of our buffer
		 */
		rc = uk_streambuf_left(sb);
		sb->flags |= UK_STREAMBUF_S_TRUNCATED;
	} else {
		/* vsnprintf() returns number of bytes without terminating '\0',
		 * so we need to add 1
		 */
		rc += 1;
	}

	sb->seek = uk_streambuf_seek(sb) + rc;

	UK_ASSERT(sb->seek <= sb->buflen);
	return (__sz) rc;
}

__sz uk_streambuf_printf(struct uk_streambuf *sb, const char *fmt, ...)
{
	__sz ret;
	va_list ap;

	va_start(ap, fmt);
	ret = uk_streambuf_vprintf(sb, fmt, ap);
	va_end(ap);

	return ret;
}

__sz uk_streambuf_strcpy(struct uk_streambuf *sb, const char *src)
{
	__ssz wlen;

	wlen = fastncat(uk_streambuf_wptr(sb), uk_streambuf_left(sb), src);
	if (wlen < 0) {
		/* We could not copy everything from the source string */
		wlen = (__ssz) uk_streambuf_left(sb);
		(sb)->flags |= UK_STREAMBUF_S_TRUNCATED;
	}

	sb->seek = uk_streambuf_seek(sb) + wlen;
	return wlen;
}

__sz uk_streambuf_memcpy(struct uk_streambuf *sb, const void *src, __sz srclen)
{
	__sz cpylen;

	UK_ASSERT(src);

	cpylen = MIN(uk_streambuf_left(sb), srclen);
	if (cpylen < srclen)
		sb->flags |= UK_STREAMBUF_S_TRUNCATED;
	memcpy(uk_streambuf_wptr(sb), src, cpylen);
	sb->seek = uk_streambuf_seek(sb) + cpylen;
	return cpylen;
}
