/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_STREAMBUF_H__
#define __UK_STREAMBUF_H__

#include <uk/config.h>
#include <stdarg.h>
#include <uk/arch/types.h>
#if CONFIG_LIBUKALLOC
#include <uk/alloc.h>
#endif /* CONFIG_LIBUKALLOC */
#include <uk/essentials.h>

#ifdef __cplusplus
extern "C" {
#endif

struct uk_streambuf {
	/** Pointer to corresponding buffer */
	char *bufp;
	/** Size of the corresponding buffer (bufp) */
	__sz buflen;
	/** Current write position within the buffer (bufp) */
	__sz seek;

	/** Configuration flags */
#define UK_STREAMBUF_C_TERMSHIFT 0x001 /** Successive writes will overwrite the
					 *  termination byte of a previous
					 *  write. This is recommended to
					 *  automatically handle C-string
					 *  concatenations and the
					 *  '\0'-termination symbol. This
					 *  configuration flag will also write
					 *  a single '\0' byte to the beginning
					 *  of the corresponding buffer.
					 */
#define UK_STREAMBUF_C_WIPEZERO  0x002 /** Zero out the buffer during
					 *  initialization/reset.
					 */
#define UK_STREAMBUF_C_MASK      0x00F /** Bitmask to filter config flags */

	/** Status flags */
#define UK_STREAMBUF_S_TRUNCATED 0x010 /** Buffer contains truncated data */
#define UK_STREAMBUF_S_MASK      0x0F0 /** Bitmask to filter status flags */
	/** Configuration and status flags */
	int flags;

#if CONFIG_LIBUKALLOC
	/** Internally used to remember allocator for free operation */
	struct uk_alloc *_a;
#endif /* CONFIG_LIBUKALLOC */
};

/**
 * Initializes a streambuf structure
 *
 * @param sb Reference to streambuf structure to initialize
 * @param buf Pointer to memory area that should be used as buffer (required)
 * @param buflen Size of the buffer memory area (minimum is `1`)
 * @param cfg_flags Configuration flags (see `UK_STREAMBUF_C_*`)
 */
void uk_streambuf_init(struct uk_streambuf *sb, void *buf, __sz buflen,
		       int cfg_flags);

/**
 * Resets a streambuf object and its corresponding buffer according to its
 * configuration (see flags `UK_STREAMBUF_C_*`)
 */
void uk_streambuf_reset(struct uk_streambuf *sb);

#if CONFIG_LIBUKALLOC
/**
 * Allocates and initializes a streambuf object together with
 * a buffer.
 *
 * @param a Reference to an allocator that is used for the allocation
 * @param buflen Target size of the buffer memory area (minimum is `1`)
 * @param bufalign Alignment for buffer memory area
 * @param cfg_flags Configuration flags (see `UK_STREAMBUF_C_*`)
 * @return An allocated streambuf object with buffer or __NULL in case of errors
 */
struct uk_streambuf *uk_streambuf_alloc2(struct uk_alloc *a, __sz buflen,
					 __sz bufalign, int cfg_flags);

/** Shortcut for `uk_streambuf_alloc2()` without specifying an alignment */
#define uk_streambuf_alloc(a, buflen, flags)				\
	uk_streambuf_alloc2((a), (buflen), 1, (flags))

/**
 * Releases a streambuf object that was allocated with `uk_streambuf_alloc()`
 * or `uk_streambuf_alloc2()`.
 *
 * @param sb Streambuf structure to release
 */
void uk_streambuf_free(struct uk_streambuf *sb);
#endif /* CONFIG_LIBUKALLOC */

/**
 * Get a pointer to the buffer of a streambuf object.
 *
 * @param sb Streambuf object
 * @return Pointer to corresponding buffer
 */
#define uk_streambuf_buf(sb)						\
	((void *) (sb)->bufp)

/**
 * Current seek position
 * NOTE: If `UK_STREAMBUF_C_TERMSHIFT` is configured, the seek position always
 *       points to the last byte of the previous write, assuming it is a
 *       termination byte (e.g., '\0'-terminating character of a string) so that
 *       it will be overwritten with appending data.
 * NOTE: For a C-string streambuf (assuming `UK_STREAMBUF_C_TERMSHIFT` is
 *       configured), `uk_streambuf_seek()` returns the number of characters,
 *       like `strlen()`.
 *
 * @param sb Streambuf object
 * @return Current seek offset
 */
#define uk_streambuf_seek(sb)						\
	 ((((sb)->flags & UK_STREAMBUF_C_TERMSHIFT) && ((sb)->seek > 0)) \
	 ? (sb)->seek - 1 : (sb)->seek)

/**
 * Current write pointer (derived from seek position)
 *
 * @param sb Streambuf object
 * @return Current write pointer
 */
#define uk_streambuf_wptr(sb)						\
	((void *) ((sb)->bufp +	uk_streambuf_seek(sb)))

/**
 * Current number of bytes used on the corresponding buffer of a streambuf.
 * HINT: If `UK_STREAMBUF_C_TERMSHIFT` is _not_ configured, the length is equal
 *       to the seek offset (`uk_streambuf_seek()`).
 * NOTE: If `UK_STREAMBUF_C_TERMSHIFT` is configured, `uk_streambuf_seek()`
 *       returns the number of bytes _without_ the termination symbol: For
 *       a C-string streambuf, `uk_streambuf_seek()` returns the number of
 *       characters, like `strlen()`.
 *       Independent of the configuration, `uk_streambuf_len()` will always
 *       return the number of used bytes on the buffer which _includes_ the
 *       termination symbol.
 *
 * @param sb Streambuf object
 * @return Current number of bytes used on the corresponding buffer
 */
#define uk_streambuf_len(sb)						\
	((sb)->seek)

/**
 * Length of the corresponding buffer of a streambuf
 *
 * @param sb Streambuf object
 * @return Length of the corresponding buffer
 */
#define uk_streambuf_buflen(sb)						\
	((sb)->buflen)

/**
 * Left bytes on the buffer of a streambuf object
 * NOTE: If `UK_STREAMBUF_C_TERMSHIFT` is configured, the reported number of
 *       bytes includes the terminating byte of a previous append operation
 *       (e.g., `uk_streambuf_printf()`). As a result, a buffer configured
 *       with `UK_STREAMBUF_C_TERMSHIFT` will still report 1 byte left for
 *       overwriting the termination byte.
 *
 * @param sb Streambuf object
 * @return Current number of bytes left on the corresponding buffer
 */
#define uk_streambuf_left(sb)						\
	((sb)->buflen - (__sz) uk_streambuf_seek((sb)))

/**
 * Report if content on buffer got truncated
 * This happens when an appending operation (e.g., `uk_streambuf_printf()`)
 * cannot write all bytes to the corresponding buffer because of limited buffer
 * space.
 *
 * @param sb Streambuf object
 * @return Non-zero if the content got truncated, zero otherwise.
 */
#define uk_streambuf_istruncated(sb)					\
	((sb)->flags & UK_STREAMBUF_S_TRUNCATED)

#ifdef __cplusplus
}
#endif

#endif /* __UK_STREAMBUF_H__ */
