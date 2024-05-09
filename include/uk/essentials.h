/**
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
 * Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_ESSENTIALS_H__
#define __UK_ESSENTIALS_H__

#include <uk/config.h>
#include <uk/compiler.h>

#if CONFIG_LIBNEWLIBC
/* Needed for __STRINGIFY */
#include <sys/param.h>
/* Needed for MIN, MAX */
#include <inttypes.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __GNUC__

#ifdef CONFIG_HAVE_SCHED
#define __uk_tls __thread
#else
#define __uk_tls
#endif

#endif /* __GNUC__ */

#define __align4       __align(0x04)
#define __align8       __align(0x08)
#define __align16      __align(0x010)
#define __align32      __align(0x020)
#define __align64      __align(0x040)
#define __align4k      __align(0x1000)
#define __align32b     __align4
#define __align64b     __align8
#define __align128b    __align16
#define __align256b    __align32
#define __align512b    __align64

#ifndef STRINGIFY
#ifndef __STRINGIFY
#define __STRINGIFY(x) #x
#endif
#define STRINGIFY(x) __STRINGIFY(x)
#endif

#ifndef UK_CONCAT
#define __UK_CONCAT_X(a, b) a##b
#define UK_CONCAT(a, b) __UK_CONCAT_X(a, b)
#endif

#ifndef MIN
#define MIN(a, b)                                                              \
	({                                                                     \
		__typeof__(a) __a = (a);                                       \
		__typeof__(b) __b = (b);                                       \
		__a < __b ? __a : __b;                                         \
	})
#endif
#ifndef MIN3
#define MIN3(a, b, c) MIN(MIN((a), (b)), (c))
#endif
#ifndef MIN4
#define MIN4(a, b, c, d) MIN(MIN((a), (b)), MIN((c), (d)))
#endif

#ifndef MAX
#define MAX(a, b)                                                              \
	({                                                                     \
		__typeof__(a) __a = (a);                                       \
		__typeof__(b) __b = (b);                                       \
		__a > __b ? __a : __b;                                         \
	})
#endif
#ifndef MAX3
#define MAX3(a, b, c) MAX(MAX((a), (b)), (c))
#endif
#ifndef MAX4
#define MAX4(a, b, c, d) MAX(MAX((a), (b)), MAX((c), (d)))
#endif

#ifndef POWER_OF_2
#define POWER_OF_2(v) ((0 != v) && (0 == (v & (v - 1))))
#endif

#ifndef DIV_ROUND_UP
#define DIV_ROUND_UP(v, d) (((v) + (d)-1) / (d))
#endif

/* Note: a has to be a power of 2 */
#ifndef ALIGN_UP
#define ALIGN_UP(v, a) (((v) + (a)-1) & ~((a)-1))
#endif

/* Note: a has to be a power of 2 */
#ifndef ALIGN_DOWN
#define ALIGN_DOWN(v, a) ((v) & ~((a)-1))
#endif

/* Note: a has to be a power of 2 */
#ifndef IS_ALIGNED
#define IS_ALIGNED(v, a) (((v) & ~((a)-1)) == (v))
#endif

/**
 * Tests if `val` is part of the range defined by `base` and `len`
 */
#define IN_RANGE(val, base, len)			\
	(((val) >= (base)) && ((val) < (base) + (len)))

/**
 * Tests if range 0 is equal to range 1
 */
#define RANGE_ISEQUAL(base0, len0, base1, len1)		\
	(((base0) == (base1))				\
	 && (len0) == (len1))

/**
 * Tests if range 0 contains range 1
 * This is the case when both of the following conditions are true:
 *  - The start of range 1 is within range 0
 *  - The end of range 1 is within range 0
 *  NOTE: The expressions take into account that `base + len` points to
 *        the first value that is outside of a range.
 */
#define RANGE_CONTAIN(base0, len0, base1, len1)				\
	(IN_RANGE((base1), (base0), (len0))				\
	 && (((base1) + (len1)) >= (base0))				\
	 && (((base1) + (len1)) <= ((base0) + (len0))))

/**
 * Tests if two ranges overlap
 *  NOTE: The expressions take into account that `base + len` points to
 *        the first value that is outside of a range.
 */
#define RANGE_OVERLAP(base0, len0, base1, len1)				\
	(((((base1) + (len1)) > (base0))) &&				\
	((base1) < ((base0) + (len0))))

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#ifndef DECONST
#include <uk/arch/types.h>
#define DECONST(t, a) ((t)((__uptr)((const void *)(a))))
#endif

#ifndef DEQUALIFY
#include <uk/arch/types.h>
#define DEQUALIFY(t, a) ((t)(__uptr)(const volatile void *)(a))
#endif

#ifndef UK_CTASSERT
#define UK_CTASSERT(x)             _UK_CTASSERT(x, __LINE__)
#define _UK_CTASSERT(x, y)         __UK_CTASSERT(x, y)
#define __UK_CTASSERT(x, y)        typedef __maybe_unused \
	char __assert_ ## y [(x) ? 1 : -1]
#endif /* UK_CTASSERT */

#ifndef UK_NARGS
#define __UK_NARGS_X(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, \
		     t, u, v, w, x, y, z, count, ...) count
#define UK_NARGS(...) \
	__UK_NARGS_X(, ##__VA_ARGS__, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, \
		     15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#endif /* UK_NARGS */

#ifdef __cplusplus
}
#endif

#endif /* __UK_ESSENTIALS_H__ */