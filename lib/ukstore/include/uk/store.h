/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Craciunoiu Cezar <cezar.craciunoiu@gmail.com>
 *
 * Copyright (c) 2021, University Politehnica of Bucharest. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __UK_STORE_H__
#define __UK_STORE_H__

#if CONFIG_LIBUKSTORE

#include <string.h>
#include <uk/config.h>
#include <uk/assert.h>
#include <uk/list.h>
#include <uk/arch/atomic.h>
#include <uk/alloc.h>

#endif /* CONFIG_LIBUKSTORE */

#include <uk/arch/types.h>
#include <uk/essentials.h>

#ifdef __cplusplus
extern "C" {
#endif

/* All basic types that exist - use these when interacting with the API.
 * The list of types is: s8, u8, s16, u16, s32, u32, s64, u64, uptr, charp
 */
enum uk_store_entry_type {
	_LIB_UK_STORE_ENTRY_TYPE___undef = 0,
	_LIB_UK_STORE_ENTRY_TYPE___s8,
	_LIB_UK_STORE_ENTRY_TYPE___u8,
	_LIB_UK_STORE_ENTRY_TYPE___s16,
	_LIB_UK_STORE_ENTRY_TYPE___u16,
	_LIB_UK_STORE_ENTRY_TYPE___s32,
	_LIB_UK_STORE_ENTRY_TYPE___u32,
	_LIB_UK_STORE_ENTRY_TYPE___s64,
	_LIB_UK_STORE_ENTRY_TYPE___u64,
	_LIB_UK_STORE_ENTRY_TYPE___uptr,
	_LIB_UK_STORE_ENTRY_TYPE___charp
};

/* Transforms a simple data type to a ukstore specific one */
#define UK_STORE_ENTRY_TYPE(simple_type)	\
	(_LIB_UK_STORE_ENTRY_TYPE___ ## simple_type)

/* Getter definitions */
typedef int (*uk_store_get_s8_func_t)(void *, __s8 *);
typedef int (*uk_store_get_u8_func_t)(void *, __u8 *);
typedef int (*uk_store_get_s16_func_t)(void *, __s16 *);
typedef int (*uk_store_get_u16_func_t)(void *, __u16 *);
typedef int (*uk_store_get_s32_func_t)(void *, __s32 *);
typedef int (*uk_store_get_u32_func_t)(void *, __u32 *);
typedef int (*uk_store_get_s64_func_t)(void *, __s64 *);
typedef int (*uk_store_get_u64_func_t)(void *, __u64 *);
typedef int (*uk_store_get_uptr_func_t)(void *, __uptr *);
typedef int (*uk_store_get_charp_func_t)(void *, char **);

/* Setter definitions */
typedef int (*uk_store_set_s8_func_t)(void *, __s8);
typedef int (*uk_store_set_u8_func_t)(void *, __u8);
typedef int (*uk_store_set_s16_func_t)(void *, __s16);
typedef int (*uk_store_set_u16_func_t)(void *, __u16);
typedef int (*uk_store_set_s32_func_t)(void *, __s32);
typedef int (*uk_store_set_u32_func_t)(void *, __u32);
typedef int (*uk_store_set_s64_func_t)(void *, __s64);
typedef int (*uk_store_set_u64_func_t)(void *, __u64);
typedef int (*uk_store_set_uptr_func_t)(void *, __uptr);
typedef int (*uk_store_set_charp_func_t)(void *, const char *);

struct uk_store_entry {
	/* Function getter pointer */
	union {
		uk_store_get_s8_func_t     s8;
		uk_store_get_u8_func_t     u8;
		uk_store_get_s16_func_t    s16;
		uk_store_get_u16_func_t    u16;
		uk_store_get_s32_func_t    s32;
		uk_store_get_u32_func_t    u32;
		uk_store_get_s64_func_t    s64;
		uk_store_get_u64_func_t    u64;
		uk_store_get_uptr_func_t   uptr;
		uk_store_get_charp_func_t  charp;
	} get;

	/* Function setter pointer */
	union {
		uk_store_set_s8_func_t    s8;
		uk_store_set_u8_func_t    u8;
		uk_store_set_s16_func_t   s16;
		uk_store_set_u16_func_t   u16;
		uk_store_set_s32_func_t   s32;
		uk_store_set_u32_func_t   u32;
		uk_store_set_s64_func_t   s64;
		uk_store_set_u64_func_t   u64;
		uk_store_set_uptr_func_t  uptr;
		uk_store_set_charp_func_t charp;
	} set;

	/* The entry name */
	char *name;

	/* Entry flags */
	int flags;

	/* Entry getter/setter type */
	enum uk_store_entry_type type;

	/* Extra cookie that is handed over to getter and setter */
	void *cookie;
} __align8;

/* Flags if an entry is static or dynamic */
#define UK_STORE_ENTRY_FLAG_STATIC	(1 << 0)

#if !CONFIG_LIBUKSTORE

#define uk_store_libid(libname) (-1)

/* Do not call directly */
#define __UK_STORE_STATIC_ENTRY(entry, lib_str, e_type, e_get, e_set,	\
				arg_cookie)				\
	static const struct uk_store_entry				\
	__unused __uk_store_entries_list ## _ ## entry = {		\
		.name = STRINGIFY(entry),				\
		.type       = (UK_STORE_ENTRY_TYPE(e_type)),		\
		.get.e_type = (e_get),					\
		.set.e_type = (e_set),					\
		.flags      = UK_STORE_ENTRY_FLAG_STATIC,		\
		.cookie     = (arg_cookie)				\
	}

/* Do not call directly */
#define _UK_STORE_STATIC_ENTRY(entry, lib_str, e_type, e_get, e_set,	\
				cookie)					\
	__UK_STORE_STATIC_ENTRY(entry, lib_str, e_type, e_get, e_set, cookie)

/**
 * Adds an entry to the entry section of a library.
 *
 * @param entry the entry in the section
 * @param e_type type for the entry, e.g., s8, u16, charp
 * @param e_get getter pointer (optional, can be NULL)
 * @param e_set setter pointer (optional, can be NULL)
 * @param cookie a cookie for extra storage
 */
#define UK_STORE_STATIC_ENTRY(entry, e_type, e_get, e_set, cookie)	\
	_UK_STORE_STATIC_ENTRY(entry, STRINGIFY(__LIBNAME__),		\
		e_type, e_get, e_set, cookie)

#else /* !CONFIG_LIBUKSTORE */

/* Library-specific information generated at compile-time */
#include <uk/bits/store_libs.h>

/* Do not call directly */
#define _uk_store_libid(libname)	\
	__UK_STORE_ ## libname

/**
 * Returns the id of a given library.
 *
 * @param libname the library to search for (not a string, e.g. LIBVFSCORE)
 * @return the id
 */
#define uk_store_libid(libname)	\
	_uk_store_libid(libname)

/**
 * Returns the id of the place it was called from.
 *
 * @return this libraries' id
 */
#define uk_store_libid_self()	\
	uk_store_libid(__LIBNAME__)

/**
 * Checks if an entry is static
 *
 * @param entry the entry to check
 * @return the condition result
 */
#define UK_STORE_ENTRY_ISSTATIC(entry)	\
	((entry)->flags & UK_STORE_ENTRY_FLAG_STATIC)

/* Do not call directly */
#define __UK_STORE_STATIC_ENTRY(entry, lib_str, e_type, e_get, e_set,	\
				arg_cookie)				\
	static const struct uk_store_entry				\
	__used __section(".uk_store_lib_" lib_str) __align8		\
	__uk_store_entries_list ## _ ## entry = {			\
		.name = STRINGIFY(entry),				\
		.type       = (UK_STORE_ENTRY_TYPE(e_type)),		\
		.get.e_type = (e_get),					\
		.set.e_type = (e_set),					\
		.flags      = UK_STORE_ENTRY_FLAG_STATIC,		\
		.cookie     = (arg_cookie)				\
	}

/* Do not call directly */
#define _UK_STORE_STATIC_ENTRY(entry, lib_str, e_type, e_get, e_set,	\
				cookie)	\
	__UK_STORE_STATIC_ENTRY(entry, lib_str, e_type, e_get, e_set, cookie)

/**
 * Adds an entry to the entry section of a library.
 *
 * @param entry the entry in the section
 * @param e_type type for the entry, e.g., s8, u16, charp
 * @param e_get getter pointer (optional, can be NULL)
 * @param e_set setter pointer (optional, can be NULL)
 * @param cookie a cookie for extra storage
 */
#define UK_STORE_STATIC_ENTRY(entry, e_type, e_get, e_set, cookie)	\
	_UK_STORE_STATIC_ENTRY(entry, STRINGIFY(__LIBNAME__),		\
		e_type, e_get, e_set, cookie)

const struct uk_store_entry *
_uk_store_get_static_entry(unsigned int libid, const char *e_name);

static inline const struct uk_store_entry *
_uk_store_get_entry(unsigned int libid, const char *f_name __unused,
			const char *e_name)
{
	return _uk_store_get_static_entry(libid, e_name);
}

/**
 * Searches for an entry in a folder in a library. Increases the refcount.
 *
 * @param libname the name of the library to search in
 * @param foldername the name of the folder to search (NULL for static entries)
 * @param entryname the name of the entry to search for
 * @return the found entry or NULL
 */
#define uk_store_get_entry(libname, foldername, entryname)	\
	_uk_store_get_entry(uk_store_libid(libname), foldername, entryname)

/**
 * Decreases the refcount. When it reaches 0, the memory is freed
 *
 * Guard this with `#if CONFIG_LIBUKSTORE`
 *
 * @param entry the entry to release
 */
#define uk_store_release_entry(entry)	\
	_uk_store_release_entry((entry))

void
_uk_store_release_entry(const struct uk_store_entry *entry);

int
_uk_store_get_u8(const struct uk_store_entry *e, __u8 *out);
int
_uk_store_get_s8(const struct uk_store_entry *e, __s8 *out);
int
_uk_store_get_u16(const struct uk_store_entry *e, __u16 *out);
int
_uk_store_get_s16(const struct uk_store_entry *e, __s16 *out);
int
_uk_store_get_u32(const struct uk_store_entry *e, __u32 *out);
int
_uk_store_get_s32(const struct uk_store_entry *e, __s32 *out);
int
_uk_store_get_u64(const struct uk_store_entry *e, __u64 *out);
int
_uk_store_get_s64(const struct uk_store_entry *e, __s64 *out);
int
_uk_store_get_uptr(const struct uk_store_entry *e, __uptr *out);
int
_uk_store_get_charp(const struct uk_store_entry *e, char **out);

/**
 * Calls the getter to get it's value.
 * The caller is responsible for freeing the returned value if it is a charp.
 *
 * @param entry the entry to call the getter from
 * @param outtype the type of the variable used (e.g., u8, s8, u16, uptr, ...)
 * @param out space to store the result of the call
 * @return 0 or error on fail
 */
#define uk_store_get_value(entry, outtype, out)	\
	_uk_store_get_ ## outtype((entry), (out))

/**
 * Calls the ncharp getter to get it's value.
 * The caller must provide a buffer with enough space to store the result.
 * The max length of the result is given by maxlen.
 *
 * @param e the entry to call the getter from
 * @param out space to store the result of the call
 * @param maxlen the maximum length of the result
 * @return 0 or error on fail
 */
int
uk_store_get_ncharp(const struct uk_store_entry *e, char *out, __sz maxlen);

int
_uk_store_set_u8(const struct uk_store_entry *e, __u8 val);
int
_uk_store_set_s8(const struct uk_store_entry *e, __s8 val);
int
_uk_store_set_u16(const struct uk_store_entry *e, __u16 val);
int
_uk_store_set_s16(const struct uk_store_entry *e, __s16 val);
int
_uk_store_set_u32(const struct uk_store_entry *e, __u32 val);
int
_uk_store_set_s32(const struct uk_store_entry *e, __s32 val);
int
_uk_store_set_u64(const struct uk_store_entry *e, __u64 val);
int
_uk_store_set_s64(const struct uk_store_entry *e, __s64 val);
int
_uk_store_set_uptr(const struct uk_store_entry *e, __uptr val);
int
_uk_store_set_charp(const struct uk_store_entry *e, const char *val);

/**
 * Calls the setter to set a new value.
 * The setter is responsible for duplicating the value if it is a charp.
 *
 * @param entry the entry to call the setter from
 * @param intype the type of the variable used (e.g., u8, s8, u16, uptr, ...)
 * @param in value to set
 * @return 0 or error on fail
 */
#define uk_store_set_value(entry, intype, in)	\
	_uk_store_set_ ## intype((entry), (in))

#endif /* CONFIG_LIBUKSTORE */

#ifdef __cplusplus
}
#endif

#endif /* __UK_STORE_H__ */
