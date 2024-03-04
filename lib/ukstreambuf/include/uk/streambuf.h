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

/**
 * Append a formatted string to the corresponding buffer at the current seek
 * position. '\0'-termination is always ensured. If the left space on the buffer
 * is too small for the generated string, the output is truncated to fit the
 * buffer and `UK_STREAMBUF_S_TRUNCATED` is set (see:
 * `uk_streambuf_istruncated()`).
 *
 * @param sb Streambuf object
 * @param fmt Format string (see `printf()`)
 * @param ... Additional arguments depending on the format string
 * @return Number of bytes written to the buffer
 */
__sz uk_streambuf_printf(struct uk_streambuf *sb, const char *fmt, ...)
	__printf(2, 3);

/**
 * Append a formatted string to the corresponding buffer at the current seek
 * position. '\0'-termination is always ensured. If the left space on the buffer
 * is too small for the generated string, the output is truncated to fit the
 * buffer and `UK_STREAMBUF_S_TRUNCATED` is set (see:
 * `uk_streambuf_istruncated()`).
 *
 * @param sb Streambuf object
 * @param fmt Format string (see `printf()`)
 * @param ap Additional arguments depending on the format string
 * @return Number of bytes written to the buffer
 */
__sz uk_streambuf_vprintf(struct uk_streambuf *sb, const char *fmt, va_list ap);

/**
 * Copy a given '\0'-terminated string to the buffer of a streambuf object at
 * the current seek position. '\0'-termination is always ensured. If the left
 * space on the buffer is too small to hold the given string, the output is
 * truncated to fit the buffer and `UK_STREAMBUF_S_TRUNCATED` is set (see:
 * `uk_streambuf_istruncated()`).
 *
 * @param sb Streambuf object
 * @param src Reference to C-string to copy
 * @return Number of bytes written to the buffer
 */
__sz uk_streambuf_strcpy(struct uk_streambuf *sb, const char *src);

/**
 * Append a copy of a binary memory blob to the buffer of a streambuf object at
 * the current seek position. If the left space on the buffer is too small for
 * holding the given memory blob, the output is truncated to fit the buffer size
 * and `UK_STREAMBUF_S_TRUNCATED` is set (see: `uk_streambuf_istruncated()`).
 * NOTE: Please note that no extra termination symbol is added by this function.
 *       A successive append call to a streambuf object that is configured
 *       with `UK_STREAMBUF_C_TERMSHIFT` will cause overwriting the last byte of
 *       the copied binary blob.
 * HINT: With `uk_streambuf_reserve(sb, 1)`, the seek position can be moved
 *       further by one byte, afterwards.
 *
 * @param sb Streambuf object
 * @param src Pointer to binary data to copy
 * @param len Number of bytes of the binary blob
 * @return Number of bytes written to the buffer (if it is equal to `len`,
 *         no truncation happened)
 */
__sz uk_streambuf_memcpy(struct uk_streambuf *sb, const void *src, __sz len);

/**
 * Returns a pointer at the current seek position of the corresponding buffer of
 * a streambuf object. A given length is reserved on the buffer for in-place
 * writing (zero-copy). The seek is increased by this reservation length for
 * successive calls.
 * NOTE: Please note that no extra termination symbol is added by this function.
 *       A successive append call to a streambuf object that is configured
 *       with `UK_STREAMBUF_C_TERMSHIFT` will cause overwriting the last byte
 *       of the returned buffer area. It also means that writing this last byte
 *       after a successive call, will cause overwriting the first byte of the
 *       successive call.
 * NOTE: Instead of truncating, this function returns `__NULL` if there is not
 *       enough space left on the corresponding buffer.
 * HINT: With `uk_streambuf_reserve(sb, 1)`, the seek position can be moved
 *       further by one byte, afterwards (use 2 for
 *       `UK_STREAMBUF_C_TERMSHIFT`).
 *
 * @param sb Streambuf object
 * @param len Number of bytes to reserve
 * @return Pointer to location in buffer that was reserved. `__NULL` when there
 *         is not enough space left to reserve `len` bytes.
 */
static inline void *uk_streambuf_reserve(struct uk_streambuf *sb, __sz len)
{
	void *ret;

	if (uk_streambuf_left(sb) < len)
		return __NULL;

	ret = uk_streambuf_wptr(sb);
	sb->seek = uk_streambuf_seek(sb) + len;
	return ret;
}

#ifdef __cplusplus
}
#endif

#endif /* __UK_STREAMBUF_H__ */
