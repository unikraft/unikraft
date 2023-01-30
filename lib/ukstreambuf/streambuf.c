/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <stdio.h>
#include <string.h>
#include <uk/streambuf.h>
#include <uk/assert.h>

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
