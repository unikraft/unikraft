/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#ifndef __LIBUKLIBID_H__
#define __LIBUKLIBID_H__

#include <uk/arch/limits.h>
#include <uk/arch/types.h>
#include <uk/bits/libid.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UKLIBID_NONE __U16_MAX

#define _uk_libid_static(l) \
	__UKLIBID_ ## l

/**
 * Compile-time mapping of a library name to its unique library identifier
 * WARNING: Any unknown library name (e.g., library not part of build) will
 *          make this pre-processor macro fail.
 *
 * @param libname Non-quoted name of the library (no C-string!)
 * @return Library identifier (__u16)
 */
#define uk_libid_static(libname) \
	_uk_libid_static(libname)

#ifndef __LIBNAME__
#define uk_libid_self() \
	UKLIBID_NONE
#else /* __LIBNAME__ */
#define __uk_libid_self_varname(l)		\
	_uk_libid_self_ ## l
#define _uk_libid_self_varname(self)		\
	__uk_libid_self_varname(self)

/**
 * Library identifier of current library compilation
 *
 * @return Library identifier (__u16)
 */
extern const __u16 _uk_libid_self_varname(__LIBNAME__);

#define uk_libid_self()				\
	({ _uk_libid_self_varname(__LIBNAME__); })
#endif /* __LIBNAME__ */

/**
 * Return the number of libraries that are part of this build
 * (resolved at compile time)
 *
 * @return Number of libraries
 */
#define uk_libid_static_count() \
	__UKLIBID_COUNT__

/**
 * Return the number of libraries that are part of this build
 * (resolved at runtime)
 *
 * @return Number of libraries
 */
__u16 uk_libid_count(void);

/**
 * Return the library name for a given identifier
 *
 * @param libid Library identifier
 * @return Reference to a '\0'-terminated C-string that contains the library
 *         name, a __NULL reference if the library identifier could not be
 *         resolved
 */
const char *uk_libname(__u16 libid);

/**
 * Return the library name of the current library compilation
 *
 * @return Reference to a '\0'-terminated C-string that contains the library
 *         name
 */
#define uk_libname_self() \
	uk_libname(uk_libid_self())

#if CONFIG_HAVE_LIBC || CONFIG_LIBNOLIBC
/**
 * Mapping of a library name to its unique library identifier
 *
 * @param libname Reference to `\0`-terminated C-string containing the name of
 *                the library
 * @return Identifier of the library, UKLIBID_NONE if the library was not found
 */
__u16 uk_libid(const char *libname);
#endif /* CONFIG_HAVE_LIBC || CONFIG_LIBNOLIBC */

#ifdef __cplusplus
}
#endif

#endif /* __LIBUKLIBID_H__ */
