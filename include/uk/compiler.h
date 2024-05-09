/* SPDX-License-Identifier: BSD-3-Clause */
/**
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
 * Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_COMPILER_H__
#define __UK_COMPILER_H__

#include <uk/config.h>

#if CONFIG_LIBNEWLIBC
/*
 * Needed for __used, __unused, __packed, __section,
 *   __nonnull, __offsetof, __containerof
 */
#include <sys/cdefs.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <uk/asm/compiler.h>

#ifdef __GNUC__
#ifndef __packed
#define __packed               __attribute__((packed))
#endif
#ifndef __noreturn
#define __noreturn             __attribute__((noreturn))
#endif
#ifndef __weak
#define __weak                 __attribute__((weak))
#endif
#ifndef __used
#define __used                 __attribute__((used))
#endif
#ifndef __maybe_unused
#define __maybe_unused         __attribute__((unused))
#endif
#ifndef __unused
#define __unused               __attribute__((unused))
#endif
#ifndef __section
#define __section(s)           __attribute__((section(s)))
#endif
#ifndef __nonnull
#define __nonnull              __attribute__((nonnull))
#endif
#ifndef __printf
#define __printf(fmt, args)    __attribute__((format(printf, (fmt), (args))))
#endif
#ifndef __scanf
#define __scanf(fmt, args)     __attribute__((format(scanf, (fmt), (args))))
#endif
#ifndef __align
#define __align(bytes)         __attribute__((aligned(bytes)))
#endif
#ifndef __unalign
#define __unalign              __align(1)
#endif
#ifndef __noinline
#define __noinline             __attribute__((noinline))
#endif
#ifndef __check_result
#define __check_result         __attribute__((warn_unused_result))
#endif
#ifndef __may_alias
#define __may_alias            __attribute__((may_alias))
#endif
#ifndef __fallthrough
#define __fallthrough          __attribute__((fallthrough))
#endif
/* NOTE: naked attribute is not yet defined for AArch64 and would generate a
 * warning.
 */
#ifndef __naked
#define __naked                __attribute__((naked))
#endif

#ifndef __alias
#define __alias(old, new) \
	extern __typeof(old) new __attribute__((alias(#old)))
#endif
/* NOTE: weak aliasing does not work well with link-time optimization
 * currently. Hopefully this will be fixed in gcc 9. The problem is,
 * if a weak symbol is referenced in the library, gcc resolves calls
 * to that weak symbol during the first round of linking. Even if a
 * strong symbol is provided in the other library.
 */
#ifndef __weak_alias
#define __weak_alias(old, new) \
	extern __typeof(old) new __attribute__((weak, alias(#old)))
#endif

/**
  * Mark a function as constructor
  * The compiler/linker will populate a function pointer
  * to the init_array section
  */
#ifndef __constructor
#define __constructor __attribute__ ((constructor))
#endif

/**
  * Mark a function as constructor with priority
  * The compiler/linker will populate a function pointer
  * (sorted by priority) to the init_array section
  * Prioritized constructors are called before
  * non-prioritized ones
  *
  * @param lvl
  *   Priority level (101 (earliest)...onwards (latest))
  */
#ifndef __constructor_prio
#define __constructor_prio(lvl) __attribute__ ((constructor (lvl)))
#endif

#else
/* TO BE DEFINED */
#endif /* __GNUC__ */

/**
 *  The following code was taken from FreeBSD: sys/sys/cdefs.h
 */
/**
 * Macro to test if we're using a specific version of gcc or later.
 */
#ifndef __GNUC_PREREQ__
#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
#define	__GNUC_PREREQ__(ma, mi)	\
	(__GNUC__ > (ma) || __GNUC__ == (ma) && __GNUC_MINOR__ >= (mi))
#else
#define __GNUC_PREREQ__(ma, mi) 0
#endif
#endif /* !__GNUC_PREREQ__ */

#ifndef __offsetof
#if __GNUC_PREREQ__(4, 1)
#define __offsetof(t, field)       __builtin_offsetof(t, field)
#else
#define __offsetof(t, field) \
	((__sz)(__uptr)((const volatile void *)&((t *)0)->field))
#endif
#endif /* !__offsetof */

/**
 * Given the pointer x to the member m of the struct s, return
 * a pointer to the containing structure.  When using GCC, we first
 * assign pointer x to a local variable, to check that its type is
 * compatible with member m.
 */
#ifndef __containerof
#if __GNUC_PREREQ__(3, 1)
#define __containerof(x, s, m) ({ \
	const volatile __typeof(((s *)0)->m) *__x = (x); \
	DEQUALIFY(s *, (const volatile char *)__x - __offsetof(s, m)); \
})
#else
#define __containerof(x, s, m) \
	DEQUALIFY(s *, (const volatile char *)(x) - __offsetof(s, m))
#endif
#endif /* !__containerof */

/**
 *  Computes the offsset and size of a field within a struct and checks
 * if it is within bounds
 */
#ifndef __contains
#define __contains(t, field, size)					\
	((__offsetof(t, field) + sizeof(((t *)0)->field)) <= (size))
#endif

#ifdef __GNUC__
#ifndef __return_addr
#define __return_addr(lvl) \
	((__uptr) __builtin_extract_return_addr(__builtin_return_address(lvl)))
#endif
#ifndef __frame_addr
#define __frame_addr(lvl) \
	((__uptr) __builtin_frame_address(lvl))
#endif
#else
	/* to be defined */
#endif /* !__GNUC__ */

#ifdef __cplusplus
}
#endif

#endif /* __UK_COMPILER_H__ */
