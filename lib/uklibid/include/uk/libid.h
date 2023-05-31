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
/**
 * Compile-time library identifier of current library compilation
 *
 * @return Library identifier (__u16)
 */
#define uk_libid_self() \
	uk_libid_static(__LIBNAME__)
#endif /* __LIBNAME__ */

/**
 * Return the number of libraries that are part of this build
 *
 * @return Number of libraries
 */
#define uk_libid_count() \
	__UKLIBID_COUNT__

#ifdef __cplusplus
}
#endif

#endif /* __LIBUKLIBID_H__ */
