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
#include <uk/alloc.h>
#include <uk/config.h>
#include <uk/assert.h>
#include <uk/arch/atomic.h>
#include <uk/libid.h>

#endif /* CONFIG_LIBUKSTORE */

#include <uk/arch/limits.h>
#include <uk/arch/types.h>
#include <uk/essentials.h>
#include <uk/list.h>

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

	/* Entry unique id */
	__u64 id;

	/* The entry name */
	char *name;

	/* Entry flags */
	int flags;

	/* Entry getter/setter type */
	enum uk_store_entry_type type;
} __align8;

/* Flags if an entry is static or dynamic */
#define UK_STORE_ENTRY_FLAG_STATIC	(1 << 0)

#if !CONFIG_LIBUKSTORE

/* Do not call directly */
#define __UK_STORE_STATIC_ENTRY(eid, entry, lib_str, e_type, e_get, e_set)\
	static const struct uk_store_entry				\
	__unused __uk_store_entries_list ## _ ## entry = {		\
		.id             = eid,					\
		.name           = STRINGIFY(entry),			\
		.type           = (UK_STORE_ENTRY_TYPE(e_type)),	\
		.get.e_type     = (e_get),				\
		.set.e_type     = (e_set),				\
		.flags          = UK_STORE_ENTRY_FLAG_STATIC		\
	}

/* Do not call directly */
#define _UK_STORE_STATIC_ENTRY(id, entry, lib_str, e_type, e_get, e_set)\
	__UK_STORE_STATIC_ENTRY(id, entry, lib_str, e_type, e_get, e_set)

/**
 * Adds an entry to the entry section of a library.
 *
 * @param id entry id
 * @param entry the entry in the section
 * @param e_type type for the entry, e.g., s8, u16, charp
 * @param e_get getter pointer (optional, can be NULL)
 * @param e_set setter pointer (optional, can be NULL)
 */
#define UK_STORE_STATIC_ENTRY(id, entry, e_type, e_get, e_set)		\
	_UK_STORE_STATIC_ENTRY(id, entry, STRINGIFY(__LIBNAME__),	\
			       e_type, e_get, e_set)

#else /* CONFIG_LIBUKSTORE */

/* Library-specific information generated at compile-time */
#include <uk/bits/store_libs.h>

struct uk_store_object {
	/* List for the dynamic objects only */
	struct uk_list_head object_head;

	/* List for the entries in the object */
	struct uk_list_head entry_head;

	/* Allocator to be used in allocations */
	struct uk_alloc *a;

	/* Parent library id */
	__u16 libid;

	/* Object unique id */
	__u64 id;

	/* The object name */
	char *name;

	/* The object's refcount */
	__atomic refcount;

	/* User data */
	void *cookie;
} __align8;

/**
 * Payload to pass to uk_store events.
 */
struct uk_store_event_data {
	__u16 library_id;
	__u64 object_id;
};
/* Do not call directly */
#define __UK_STORE_STATIC_ENTRY(eid, entry, lib_str, e_type, e_get, e_set)\
	static const struct uk_store_entry				\
	__used __section(".uk_store_lib_" lib_str) __align8		\
	__uk_store_entries_list ## _ ## entry = {			\
		.id		= eid,					\
		.name           = STRINGIFY(entry),			\
		.type           = (UK_STORE_ENTRY_TYPE(e_type)),	\
		.get.e_type     = (e_get),				\
		.set.e_type     = (e_set),				\
		.flags          = UK_STORE_ENTRY_FLAG_STATIC		\
	}

/* Do not call directly */
#define _UK_STORE_STATIC_ENTRY(id, entry, lib_str, e_type, e_get, e_set)\
	__UK_STORE_STATIC_ENTRY(id, entry, lib_str, e_type, e_get, e_set)


/**
 * Helper to create entry array elements for uk_store_create_object()
 *
 * @_id   entry id
 * @_name entry name (without quotes)
 * @_type entry type, e.g s8, u16, charp
 * @get   getter function
 * @set   setter function
 */
#define UK_STORE_ENTRY(_id, _name, _type, _get, _set)		\
(								\
	&(struct uk_store_entry) {				\
		.id = _id,					\
		.name = STRINGIFY(_name),			\
		.type = UK_STORE_ENTRY_TYPE(_type),		\
		.get._type = _get,				\
		.set._type = _set,				\
	}							\
)

/**
 * Adds an entry to the entry section of a library.
 *
 * @param id entry id
 * @param entry the entry in the section
 * @param e_type type for the entry, e.g., s8, u16, charp
 * @param e_get getter pointer (optional, can be NULL)
 * @param e_set setter pointer (optional, can be NULL)
 */
#define UK_STORE_STATIC_ENTRY(id, entry, e_type, e_get, e_set)		\
	_UK_STORE_STATIC_ENTRY(id, entry, STRINGIFY(__LIBNAME__),	\
			       e_type, e_get, e_set)

/**
 * Creates an object
 *
 * Allocates memory and initializes a new uk_store object.
 *
 * @param a       allocator instance to allocate this object from
 * @param id      the id of the new object
 * @param name    the name of the new object
 * @param entries NULL terminated array of struct uk_store_entry describing
 *                the entries of this object. These can be defined with the
 *                UK_STORE_ENTRY helper. You should define the superset of
 *                entries, including ones disabled by default. At runtime,
 *                disabled entries are indicated by the return code of
 *                get_value()
 * @param cookie  caller defined data. This is passed to the getter function
 * @return        pointer to the allocated object, errptr otherwise
 */
struct uk_store_object *
uk_store_obj_alloc(struct uk_alloc *a, __u64 id, const char *name,
		   const struct uk_store_entry *entries[], void *cookie);

/**
 * Associates an object to the caller library
 *
 * Guard this with `#if CONFIG_LIBUKSTORE`
 *
 * @param object     the object to add to the caller library
 * @return           0 on success and < 0 on failure
 */
#define uk_store_obj_add(obj)					\
	_uk_store_obj_add(uk_libid_self(), (obj))

int
_uk_store_obj_add(__u16 library_id, struct uk_store_object *object);

/**
 * Acquires an object
 *
 * Increments the object's refcount and returns the object.
 * Evern call must be paired with a call to uk_store_release_object()
 * to decrement the refcount.
 *
 * @param library_id owner library id as returned by uklibid
 * @param object_id  object id
 * @return           pointer to object
 */
struct uk_store_object *
uk_store_obj_acquire(__u16 library_id, __u64 object_id);

/**
 * Releases an object
 *
 * Decrements the object's refcount.
 *
 * @param object  object to release
 */
void uk_store_obj_release(struct uk_store_object *object);

/**
 * Looks up a static entry of a library.
 *
 * @param libid    owner library
 * @param entry_id the id of the entry to look up
 * @return the matched entry or NULL
 */
const struct uk_store_entry *
uk_store_static_entry_get(__u16 library_id, __u64 entry_id);

/**
 * Looks up an entry in an object of a library.
 *
 * @param object   target object
 * @param entry_id the id of the entry to look up
 * @return the matched entry or NULL
 */
const struct uk_store_entry *
uk_store_obj_entry_get(struct uk_store_object *object_id, __u64 entry_id);

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

int _uk_store_get_u8(const struct uk_store_entry *e, __u8 *out);
int _uk_store_get_s8(const struct uk_store_entry *e, __s8 *out);
int _uk_store_get_u16(const struct uk_store_entry *e, __u16 *out);
int _uk_store_get_s16(const struct uk_store_entry *e, __s16 *out);
int _uk_store_get_u32(const struct uk_store_entry *e, __u32 *out);
int _uk_store_get_s32(const struct uk_store_entry *e, __s32 *out);
int _uk_store_get_u64(const struct uk_store_entry *e, __u64 *out);
int _uk_store_get_s64(const struct uk_store_entry *e, __s64 *out);
int _uk_store_get_uptr(const struct uk_store_entry *e, __uptr *out);
int _uk_store_get_charp(const struct uk_store_entry *e, char **out);
int _uk_store_get_ncharp(const struct uk_store_entry *e, char *out, __sz maxlen);

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

int _uk_store_set_u8(const struct uk_store_entry *e, __u8 val);
int _uk_store_set_s8(const struct uk_store_entry *e, __s8 val);
int _uk_store_set_u16(const struct uk_store_entry *e, __u16 val);
int _uk_store_set_s16(const struct uk_store_entry *e, __s16 val);
int _uk_store_set_u32(const struct uk_store_entry *e, __u32 val);
int _uk_store_set_s32(const struct uk_store_entry *e, __s32 val);
int _uk_store_set_u64(const struct uk_store_entry *e, __u64 val);
int _uk_store_set_s64(const struct uk_store_entry *e, __s64 val);
int _uk_store_set_uptr(const struct uk_store_entry *e, __uptr val);
int _uk_store_set_charp(const struct uk_store_entry *e, const char *val);

#endif /* CONFIG_LIBUKSTORE */

#ifdef __cplusplus
}
#endif

#endif /* __UK_STORE_H__ */
