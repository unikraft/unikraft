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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <uk/arch/limits.h>
#include <uk/arch/types.h>
#include <uk/store.h>

#define _U8_STRLEN (3 + 1) /* 3 digits plus terminating '\0' */
#define _S8_STRLEN (1 + _U8_STRLEN) /* space for potential sign in front */
#define _U16_STRLEN (5 + 1)
#define _S16_STRLEN (1 + _U16_STRLEN)
#define _U32_STRLEN (10 + 1)
#define _S32_STRLEN (1 + _U32_STRLEN)
#define _U64_STRLEN (19 + 1)
#define _S64_STRLEN (1 + _U64_STRLEN)
#define _UPTR_STRLEN (2 + 16 + 1) /* 64bit in hex with leading `0x` prefix */

#include <uk/bits/store_array.h>

/**
 * Releases an entry (decreases the refcount and sets the reference to NULL)
 * If the refcount is 0 (it was also released by the creator), it is removed
 * from the list and memory is freed
 *
 * @param p_entry pointer to the entry to release
 */
void
_uk_store_release_entry(const struct uk_store_entry *p_entry)
{
	if (UK_STORE_ENTRY_ISSTATIC(p_entry))
		return;
}

/**
 * Find a static entry and returns it.
 *
 * @param libid the id of the library to search in
 * @param e_name the name of the entry to search for
 * @return the found entry or NULL
 */
const struct uk_store_entry *
_uk_store_get_static_entry(unsigned int libid, const char *e_name)
{
	struct uk_store_entry *entry = static_entries[2 * libid];
	struct uk_store_entry *stop = static_entries[2 * libid + 1];

	for (; entry != stop; ++entry)
		if (!strcmp(entry->name, e_name))
			return entry;

	return NULL;
}

/* Capital types used internally */
#define S8  __do_not_expand__
#define U8  __do_not_expand__
#define S16 __do_not_expand__
#define U16 __do_not_expand__
#define S32 __do_not_expand__
#define U32 __do_not_expand__
#define S64 __do_not_expand__
#define U64 __do_not_expand__
#define PTR __do_not_expand__

/**
 * Case defines used internally for shortening code
 *
 * @param entry the entry to do the case for
 * @param etype the type of the setter
 * @param ETYPE capital type of the setter
 * @param var the value to set
 * @param eparam the setter type
 */

/* Signed input, unsigned etype */
#define SETCASE_DOWNCASTSU(entry, etype, ETYPE, var, eparam)		\
	do {								\
		case UK_STORE_ENTRY_TYPE(etype):			\
		if (unlikely(var < 0 ||					\
			var > (__ ## eparam) __ ## ETYPE ## _MAX))	\
			return -ERANGE;					\
		return (entry)->set.etype(entry->cookie,		\
					(__ ## etype) var);		\
	} while (0)

/* Unsigned input, signed etype */
#define SETCASE_DOWNCASTUS(entry, etype, ETYPE, var, eparam)	\
	do {							\
		case UK_STORE_ENTRY_TYPE(etype):		\
		if (unlikely(var > __ ## ETYPE ## _MAX))	\
			return -ERANGE;				\
		return (entry)->set.etype(entry->cookie,	\
					(__ ## etype) var);	\
	} while (0)

/* Both signed */
#define SETCASE_DOWNCASTSS(entry, etype, ETYPE, var, eparam)	\
	do {							\
		case UK_STORE_ENTRY_TYPE(etype):		\
		if (unlikely(var < __ ## ETYPE ## _MIN ||	\
				var > __ ## ETYPE ## _MAX))	\
			return -ERANGE;				\
		return (entry)->set.etype(entry->cookie,	\
					(__ ## etype) var);	\
	} while (0)

/* Both unsigned */
#define SETCASE_DOWNCASTUU(entry, etype, ETYPE, var, eparam)	\
	do {							\
		case UK_STORE_ENTRY_TYPE(etype):		\
		if (unlikely(var > __ ## ETYPE ## _MAX))	\
			return -ERANGE;				\
		return (entry)->set.etype(entry->cookie,	\
					(__ ## etype) var);	\
	} while (0)

/* Signed input, unsigned etype */
#define SETCASE_UPCASTSU(entry, etype, ETYPE, var, eparam)	\
	do {							\
		case UK_STORE_ENTRY_TYPE(etype):		\
		if (unlikely(var < 0))				\
			return -ERANGE;				\
		return (entry)->set.etype(entry->cookie,	\
					(__ ## etype) var);	\
	} while (0)

/* All other cases */
#define SETCASE_UPCAST(entry, etype, ETYPE, var, eparam)	\
	do {							\
		case UK_STORE_ENTRY_TYPE(etype):		\
		;						\
		return (entry)->set.etype(entry->cookie,	\
					(__ ## etype) var);	\
	} while (0)

/**
 * All setters below use this description.
 * Checks the ranges of the values and sets a new value using the save setter.
 *
 * @param e the entry to call the setter from
 * @param val the value to set
 * @return the return value of the setter
 */

int
_uk_store_set_u8(const struct uk_store_entry *e, __u8 val)
{
	UK_ASSERT(e);
	if (unlikely(e->set.u8 == NULL))
		return -EIO;

	switch (e->type) {
	SETCASE_DOWNCASTUS(e, s8, S8, val, u8);
	SETCASE_UPCAST(e, u8, U8, val, u8);
	SETCASE_UPCAST(e, u16, U16, val, u8);
	SETCASE_UPCAST(e, s16, S16, val, u8);
	SETCASE_UPCAST(e, u32, U32, val, u8);
	SETCASE_UPCAST(e, s32, S32, val, u8);
	SETCASE_UPCAST(e, u64, U64, val, u8);
	SETCASE_UPCAST(e, s64, S64, val, u8);
	SETCASE_UPCAST(e, uptr, PTR, val, u8);

	case UK_STORE_ENTRY_TYPE(charp): {
		int ret;
		char to_set[_U8_STRLEN];

		snprintf(to_set, _U8_STRLEN, "%" __PRIu8, val);
		ret = e->set.charp(e->cookie, to_set);
		return ret;
	}

	default:
		return -EINVAL;
	}
	return 0;
}

int
_uk_store_set_s8(const struct uk_store_entry *e, __s8 val)
{
	UK_ASSERT(e);
	if (unlikely(e->set.u8 == NULL))
		return -EIO;

	switch (e->type) {
	SETCASE_UPCAST(e, s8, S8, val, s8);
	SETCASE_UPCASTSU(e, u8, U8, val, s8);
	SETCASE_UPCAST(e, s16, S16, val, s8);
	SETCASE_UPCASTSU(e, u16, U16, val, s8);
	SETCASE_UPCAST(e, s32, S32, val, s8);
	SETCASE_UPCASTSU(e, u32, U32, val, s8);
	SETCASE_UPCAST(e, u64, U64, val, s8);
	SETCASE_UPCASTSU(e, s64, S64, val, s8);
	SETCASE_UPCASTSU(e, uptr, PTR, val, s8);

	case UK_STORE_ENTRY_TYPE(charp): {
		int ret;
		char to_set[_S8_STRLEN];

		snprintf(to_set, _S8_STRLEN, "%" __PRIs8, val);
		ret = e->set.charp(e->cookie, to_set);
		return ret;
	}

	default:
		return -EINVAL;
	}
	return 0;
}

int
_uk_store_set_u16(const struct uk_store_entry *e, __u16 val)
{
	UK_ASSERT(e);
	if (unlikely(e->set.u8 == NULL))
		return -EIO;

	switch (e->type) {
	SETCASE_DOWNCASTUS(e, s8, S8, val, u16);
	SETCASE_DOWNCASTUU(e, u8, U8, val, u16);
	SETCASE_DOWNCASTUS(e, s16, S16, val, u16);
	SETCASE_UPCAST(e, u16, U16, val, u16);
	SETCASE_UPCAST(e, s32, S32, val, u16);
	SETCASE_UPCAST(e, u32, U32, val, u16);
	SETCASE_UPCAST(e, u64, S64, val, u16);
	SETCASE_UPCAST(e, uptr, PTR, val, u16);

	case UK_STORE_ENTRY_TYPE(charp): {
		int ret;
		char to_set[_U16_STRLEN];

		snprintf(to_set, _U16_STRLEN, "%" __PRIu16, val);
		ret = e->set.charp(e->cookie, to_set);
		return ret;
	}

	default:
		return -EINVAL;
	}
	return 0;
}

int
_uk_store_set_s16(const struct uk_store_entry *e, __s16 val)
{
	UK_ASSERT(e);
	if (unlikely(e->set.u8 == NULL))
		return -EIO;

	switch (e->type) {
	SETCASE_DOWNCASTSS(e, s8, S8, val, s16);
	SETCASE_DOWNCASTSU(e, u8, U8, val, s16);
	SETCASE_UPCAST(e, s16, S16, val, s16);
	SETCASE_UPCASTSU(e, u16, U16, val, s16);
	SETCASE_UPCAST(e, s32, S32, val, s16);
	SETCASE_UPCASTSU(e, u32, U32, val, s16);
	SETCASE_UPCAST(e, s64, S64, val, s16);
	SETCASE_UPCASTSU(e, u64, U64, val, s16);
	SETCASE_UPCASTSU(e, uptr, PTR, val, s16);

	case UK_STORE_ENTRY_TYPE(charp): {
		int ret;
		char to_set[_S16_STRLEN];

		snprintf(to_set, _S16_STRLEN, "%" __PRIs16, val);
		ret = e->set.charp(e->cookie, to_set);
		return ret;
	}

	default:
		return -EINVAL;
	}
	return 0;
}

int
_uk_store_set_u32(const struct uk_store_entry *e, __u32 val)
{
	UK_ASSERT(e);
	if (unlikely(e->set.u8 == NULL))
		return -EIO;

	switch (e->type) {
	SETCASE_DOWNCASTUS(e, s8, S8, val, u32);
	SETCASE_DOWNCASTUU(e, u8, U8, val, u32);
	SETCASE_DOWNCASTUS(e, s16, S16, val, u32);
	SETCASE_DOWNCASTUU(e, u16, U16, val, u32);
	SETCASE_DOWNCASTUS(e, s32, S32, val, u32);
	SETCASE_UPCAST(e, u32, U32, val, u32);
	SETCASE_UPCAST(e, s64, S64, val, u32);
	SETCASE_UPCAST(e, u64, U64, val, u32);
	SETCASE_UPCAST(e, uptr, UPTR, val, u32);

	case UK_STORE_ENTRY_TYPE(charp): {
		int ret;
		char to_set[_U32_STRLEN];

		snprintf(to_set, _U32_STRLEN, "%" __PRIu32, val);
		ret = e->set.charp(e->cookie, to_set);
		return ret;
	}

	default:
		return -EINVAL;
	}
	return 0;
}

int
_uk_store_set_s32(const struct uk_store_entry *e, __s32 val)
{
	UK_ASSERT(e);
	if (unlikely(e->set.u8 == NULL))
		return -EIO;

	switch (e->type) {
	SETCASE_DOWNCASTSS(e, s8, S8, val, s32);
	SETCASE_DOWNCASTSU(e, u8, U8, val, s32);
	SETCASE_DOWNCASTSS(e, s16, S16, val, s32);
	SETCASE_DOWNCASTSU(e, u16, U16, val, s32);
	SETCASE_UPCAST(e, s32, S32, val, s32);
	SETCASE_UPCASTSU(e, u32, S32, val, s32);
	SETCASE_UPCAST(e, s64, S64, val, s32);
	SETCASE_UPCASTSU(e, u64, U64, val, s32);
	SETCASE_UPCASTSU(e, uptr, PTR, val, s32);

	case UK_STORE_ENTRY_TYPE(charp): {
		int ret;
		char to_set[_S32_STRLEN];

		snprintf(to_set, _S32_STRLEN, "%" __PRIs32, val);
		ret = e->set.charp(e->cookie, to_set);
		return ret;
	}

	default:
		return -EINVAL;
	}
	return 0;
}

int
_uk_store_set_u64(const struct uk_store_entry *e, __u64 val)
{
	UK_ASSERT(e);
	if (unlikely(e->set.u8 == NULL))
		return -EIO;

	switch (e->type) {
	SETCASE_DOWNCASTUS(e, s8, S8, val, u64);
	SETCASE_DOWNCASTUU(e, u8, U8, val, u64);
	SETCASE_DOWNCASTUS(e, s16, S16, val, u64);
	SETCASE_DOWNCASTUU(e, u16, U16, val, u64);
	SETCASE_DOWNCASTUS(e, s32, S32, val, u64);
	SETCASE_DOWNCASTUU(e, u32, U32, val, u64);
	SETCASE_DOWNCASTUS(e, s64, S64, val, u64);
	SETCASE_UPCAST(e, u64, U64, val, u64);
	SETCASE_UPCAST(e, uptr, PTR, val, u64);

	case UK_STORE_ENTRY_TYPE(charp): {
		int ret;
		char to_set[_U64_STRLEN];

		snprintf(to_set, _U64_STRLEN, "%" __PRIu64, val);
		ret = e->set.charp(e->cookie, to_set);
		return ret;
	}

	default:
		return -EINVAL;
	}
	return 0;
}

int
_uk_store_set_s64(const struct uk_store_entry *e, __s64 val)
{
	UK_ASSERT(e);
	if (unlikely(e->set.u8 == NULL))
		return -EIO;

	switch (e->type) {
	SETCASE_DOWNCASTSS(e, s8, S8, val, s64);
	SETCASE_DOWNCASTSU(e, u8, U8, val, s64);
	SETCASE_DOWNCASTSS(e, s16, S16, val, s64);
	SETCASE_DOWNCASTSU(e, u16, U16, val, s64);
	SETCASE_DOWNCASTSS(e, s32, S32, val, s64);
	SETCASE_DOWNCASTSU(e, u32, S32, val, s64);
	SETCASE_UPCAST(e, s64, S64, val, s64);
	SETCASE_UPCASTSU(e, u64, U64, val, s64);
	SETCASE_UPCASTSU(e, uptr, PTR, val, s64);

	case UK_STORE_ENTRY_TYPE(charp): {
		int ret;
		char to_set[_S64_STRLEN];

		snprintf(to_set, _S64_STRLEN, "%" __PRIs64, val);
		ret = e->set.charp(e->cookie, to_set);
		return ret;
	}

	default:
		return -EINVAL;
	}
	return 0;
}

int
_uk_store_set_uptr(const struct uk_store_entry *e, __uptr val)
{
	UK_ASSERT(e);
	if (unlikely(e->set.u8 == NULL))
		return -EIO;

	switch (e->type) {
	SETCASE_DOWNCASTUS(e, s8, S8, val, uptr);
	SETCASE_DOWNCASTUU(e, u8, U8, val, uptr);
	SETCASE_DOWNCASTUS(e, s16, S16, val, uptr);
	SETCASE_DOWNCASTUU(e, u16, U16, val, uptr);
	SETCASE_DOWNCASTUS(e, s32, S32, val, uptr);
	SETCASE_DOWNCASTUU(e, u32, U32, val, uptr);
	SETCASE_DOWNCASTUS(e, s64, S64, val, uptr);
	SETCASE_UPCAST(e, u64, U64, val, uptr);
	SETCASE_UPCAST(e, uptr, PTR, val, uptr);

	case UK_STORE_ENTRY_TYPE(charp): {
		int ret;
		char to_set[_UPTR_STRLEN];

		snprintf(to_set, _UPTR_STRLEN, "0x%" __PRIx64, val);
		ret = e->set.charp(e->cookie, to_set);
		return ret;
	}

	default:
		return -EINVAL;
	}
	return 0;
}

int
_uk_store_set_charp(const struct uk_store_entry *e, const char *val)
{
	int ret;

	UK_ASSERT(e);
	if (unlikely(e->set.u8 == NULL))
		return -EIO;


	switch (e->type) {
	case UK_STORE_ENTRY_TYPE(u8): {
		__u8 to_set;

		ret = sscanf(val, "%" __SCNu8, &to_set);
		if (ret < 0)
			return ret;

		return e->set.u8(e->cookie, to_set);
	}

	case UK_STORE_ENTRY_TYPE(s8): {
		__s8 to_set;

		ret = sscanf(val, "%" __SCNs8, &to_set);
		if (ret < 0)
			return ret;

		return e->set.s8(e->cookie, to_set);
	}

	case UK_STORE_ENTRY_TYPE(u16): {
		__u16 to_set;

		ret = sscanf(val, "%" __SCNu16, &to_set);
		if (ret < 0)
			return ret;

		return e->set.u16(e->cookie, to_set);
	}

	case UK_STORE_ENTRY_TYPE(s16): {
		__s16 to_set;

		ret = sscanf(val, "%" __SCNs16, &to_set);
		if (ret < 0)
			return ret;

		return e->set.s16(e->cookie, to_set);
	}

	case UK_STORE_ENTRY_TYPE(u32): {
		__u32 to_set;

		ret = sscanf(val, "%" __SCNu32, &to_set);
		if (ret < 0)
			return ret;

		return e->set.u32(e->cookie, to_set);
	}

	case UK_STORE_ENTRY_TYPE(s32): {
		__s32 to_set;

		ret = sscanf(val, "%" __SCNs32, &to_set);
		if (ret < 0)
			return ret;

		return e->set.s32(e->cookie, to_set);
	}

	case UK_STORE_ENTRY_TYPE(u64): {
		__u64 to_set;

		ret = sscanf(val, "%" __SCNu64, &to_set);
		if (ret < 0)
			return ret;

		return e->set.u64(e->cookie, to_set);
	}

	case UK_STORE_ENTRY_TYPE(s64): {
		__s64 to_set;

		ret = sscanf(val, "%" __SCNs64, &to_set);
		if (ret < 0)
			return ret;

		return e->set.s64(e->cookie, to_set);
	}

	case UK_STORE_ENTRY_TYPE(uptr): {
		__uptr to_set;

		if (strncmp(val, "0x", 2) == 0)
			ret = sscanf(val, "0x%" __SCNx64, &to_set);
		else
			ret = sscanf(val, "%" __SCNx64, &to_set);

		if (ret < 0)
			return ret;

		return e->set.uptr(e->cookie, to_set);
	}

	case UK_STORE_ENTRY_TYPE(charp): {
		ret = e->set.charp(e->cookie, val);

		return ret;
	}

	default:
		return -EINVAL;
	}
}

/**
 * Case defines used internally for shortening code
 *
 * @param entry the entry to do the case for
 * @param etype the type of the getter
 * @param PTYPE capital type of the value
 * @param var the value to get in
 * @param eparam the type of the value
 */

/* Signed input, unsigned etype */
#define GETCASE_DOWNCASTSU(entry, etype, PTYPE, var, eparam)		\
	do {								\
		case UK_STORE_ENTRY_TYPE(etype): {			\
			__ ## etype val;				\
									\
			ret = (entry)->get.etype((entry)->cookie,	\
						&val);			\
			if (ret < 0)					\
				return ret;				\
			if (unlikely(val < 0 ||				\
			val > (__ ## etype) __ ## PTYPE ## _MAX))	\
				return -ERANGE;				\
			*(var)  = (__ ## eparam) val;			\
			return ret;					\
		}							\
	} while (0)

/* Unsigned input, signed etype */
#define GETCASE_DOWNCASTUS(entry, etype, PTYPE, var, eparam)		\
	do {								\
		case UK_STORE_ENTRY_TYPE(etype): {			\
			__ ## etype val;				\
									\
			ret = (entry)->get.etype((entry)->cookie,	\
						&val);			\
			if (ret < 0)					\
				return ret;				\
			if (unlikely(val > __ ## PTYPE ## _MAX))	\
				return -ERANGE;				\
			*(var)  = (__ ## eparam) val;			\
			return ret;					\
		}							\
	} while (0)

/* Both signed */
#define GETCASE_DOWNCASTSS(entry, etype, PTYPE, var, eparam)		\
	do {								\
		case UK_STORE_ENTRY_TYPE(etype): {			\
			__ ## etype val;				\
									\
			ret = (entry)->get.etype((entry)->cookie,	\
						&val);			\
			if (ret < 0)					\
				return ret;				\
			if (unlikely(val < __ ## PTYPE ## _MIN		\
				|| val > __ ## PTYPE ##_MAX))		\
				return -ERANGE;				\
			*(var)  = (__ ## eparam) val;			\
			return ret;					\
		}							\
	} while (0)

/* Both unsigned */
#define GETCASE_DOWNCASTUU(entry, etype, PTYPE, var, eparam)		\
	do {								\
		case UK_STORE_ENTRY_TYPE(etype): {			\
			__ ## etype val;				\
									\
			ret = (entry)->get.etype((entry)->cookie,	\
							&val);		\
			if (ret < 0)					\
				return ret;				\
			if (unlikely(val > __ ## PTYPE ## _MAX))	\
				return -ERANGE;				\
			*(var)  = (__ ## eparam) val;			\
			return ret;					\
		}							\
	} while (0)

/* All other cases */
#define GETCASE_UPCAST(entry, etype, PTYPE, var, eparam)		\
	do {								\
		case UK_STORE_ENTRY_TYPE(etype): {			\
			__ ## etype val;				\
									\
			ret = (entry)->get.etype((entry)->cookie,	\
							&val);		\
			if (ret < 0)					\
				return ret;				\
			*(var)  = (__ ## eparam) val;			\
			return ret;					\
		}							\
	} while (0)

/* Unsigned input, signed etype */
#define GETCASE_UPCASTSU(entry, etype, PTYPE, var, eparam)		\
	do {								\
		case UK_STORE_ENTRY_TYPE(etype): {			\
			__ ## etype val;				\
									\
			ret = (entry)->get.etype((entry)->cookie,	\
						&val);			\
			if (ret < 0)					\
				return ret;				\
			if (unlikely(val < 0))				\
				return -ERANGE;				\
			*(var)  = (__ ## eparam) val;			\
			return ret;					\
		}							\
	} while (0)

/**
 * All getters below use this description.
 * Gets a new value using the save getter and checks the ranges of the values.
 *
 * @param e the entry to call the getter from
 * @param out the value to get in
 * @return the return value of the getter
 */

int
_uk_store_get_u8(const struct uk_store_entry *e, __u8 *out)
{
	int ret;

	UK_ASSERT(e);
	if (unlikely(e->get.u8 == NULL))
		return -EIO;

	switch (e->type) {
	GETCASE_UPCAST(e, u8, U8, out, u8);
	GETCASE_UPCASTSU(e, s8, U8, out, u8);
	GETCASE_DOWNCASTUU(e, u16, U8, out, u8);
	GETCASE_DOWNCASTSU(e, s16, U8, out, u8);
	GETCASE_DOWNCASTUU(e, u32, U8, out, u8);
	GETCASE_DOWNCASTSU(e, s32, U8, out, u8);
	GETCASE_DOWNCASTUU(e, u64, U8, out, u8);
	GETCASE_DOWNCASTSU(e, s64, U8, out, u8);
	GETCASE_DOWNCASTUU(e, uptr, U8, out, u8);

	case UK_STORE_ENTRY_TYPE(charp): {
		char *val = NULL;
		__s16 sscanf_ret;

		ret = e->get.charp(e->cookie, &val);
		if (ret < 0)
			return ret;

		sscanf_ret = sscanf(val, "%" __SCNu8, out);
		if (sscanf_ret < 0)
			return sscanf_ret;

		return ret;
	}

	default:
		return -EINVAL;
	}
}

int
_uk_store_get_s8(const struct uk_store_entry *e, __s8 *out)
{
	int ret;

	UK_ASSERT(e);
	if (unlikely(e->get.u8 == NULL))
		return -EIO;

	switch (e->type) {
	GETCASE_DOWNCASTUS(e, u8, S8, out, s8);
	GETCASE_UPCAST(e, s8, S8, out, s8);
	GETCASE_DOWNCASTUS(e, u16, S8, out, s8);
	GETCASE_DOWNCASTSS(e, s16, S8, out, s8);
	GETCASE_DOWNCASTUS(e, u32, S8, out, s8);
	GETCASE_DOWNCASTSS(e, s32, S8, out, s8);
	GETCASE_DOWNCASTUS(e, u64, S8, out, s8);
	GETCASE_DOWNCASTSS(e, s64, S8, out, s8);
	GETCASE_DOWNCASTUS(e, uptr, S8, out, s8);

	case UK_STORE_ENTRY_TYPE(charp): {
		char *val = NULL;
		__s16 sscanf_ret;

		ret = e->get.charp(e->cookie, &val);
		if (ret < 0)
			return ret;

		sscanf_ret = sscanf(val, "%" __SCNs8, out);
		if (sscanf_ret < 0)
			return sscanf_ret;

		return ret;
	}

	default:
		return -EINVAL;
	}
}

int
_uk_store_get_u16(const struct uk_store_entry *e, __u16 *out)
{
	int ret;

	UK_ASSERT(e);
	if (unlikely(e->get.u8 == NULL))
		return -EIO;

	switch (e->type) {
	GETCASE_UPCAST(e, u8, U16, out, u16);
	GETCASE_UPCASTSU(e, s8, U16, out, u16);
	GETCASE_UPCAST(e, u16, U16, out, u16);
	GETCASE_UPCASTSU(e, s16, U16, out, u16);
	GETCASE_DOWNCASTUU(e, u32, U16, out, u16);
	GETCASE_DOWNCASTSU(e, s32, U16, out, u16);
	GETCASE_DOWNCASTUU(e, u64, U16, out, u16);
	GETCASE_DOWNCASTSU(e, s64, U16, out, u16);
	GETCASE_DOWNCASTUU(e, uptr, U16, out, u16);

	case UK_STORE_ENTRY_TYPE(charp): {
		char *val = NULL;
		__s16 sscanf_ret;

		ret = e->get.charp(e->cookie, &val);
		if (ret < 0)
			return ret;

		sscanf_ret = sscanf(val, "%" __SCNu16, out);
		if (sscanf_ret < 0)
			return sscanf_ret;

		return ret;
	}

	default:
		return -EINVAL;
	}
}

int
_uk_store_get_s16(const struct uk_store_entry *e, __s16 *out)
{
	int ret;

	UK_ASSERT(e);
	if (unlikely(e->get.u8 == NULL))
		return -EIO;

	switch (e->type) {
	GETCASE_UPCAST(e, u8, S16, out, s16);
	GETCASE_UPCAST(e, s8, S16, out, s16);
	GETCASE_DOWNCASTUS(e, u16, S16, out, s16);
	GETCASE_UPCAST(e, s16, S16, out, s16);
	GETCASE_DOWNCASTUS(e, u32, S16, out, s16);
	GETCASE_DOWNCASTSS(e, s32, S16, out, s16);
	GETCASE_DOWNCASTUS(e, u64, S16, out, s16);
	GETCASE_DOWNCASTSS(e, s64, S16, out, s16);
	GETCASE_DOWNCASTUS(e, uptr, S16, out, s16);

	case UK_STORE_ENTRY_TYPE(charp): {
		char *val = NULL;
		__s16 sscanf_ret;

		ret = e->get.charp(e->cookie, &val);
		if (ret < 0)
			return ret;

		sscanf_ret = sscanf(val, "%" __SCNs16, out);
		if (sscanf_ret < 0)
			return sscanf_ret;

		return ret;
	}

	default:
		return -EINVAL;
	}
}

int
_uk_store_get_u32(const struct uk_store_entry *e, __u32 *out)
{
	int ret;

	UK_ASSERT(e);
	if (unlikely(e->get.u8 == NULL))
		return -EIO;

	switch (e->type) {
	GETCASE_UPCAST(e, u8, U32, out, u32);
	GETCASE_UPCASTSU(e, s8, U32, out, u32);
	GETCASE_UPCAST(e, u16, U32, out, u32);
	GETCASE_UPCASTSU(e, s16, U32, out, u32);
	GETCASE_UPCAST(e, u32, U32, out, u32);
	GETCASE_UPCASTSU(e, s32, U32, out, u32);
	GETCASE_DOWNCASTUU(e, u64, U32, out, u32);
	GETCASE_DOWNCASTSU(e, s64, U32, out, u32);
	GETCASE_DOWNCASTUU(e, uptr, U32, out, u32);

	case UK_STORE_ENTRY_TYPE(charp): {
		char *val = NULL;
		__s16 sscanf_ret;

		ret = e->get.charp(e->cookie, &val);
		if (ret < 0)
			return ret;

		sscanf_ret = sscanf(val, "%" __SCNu32, out);
		if (sscanf_ret < 0)
			return sscanf_ret;

		return ret;
	}

	default:
		return -EINVAL;
	}
}

int
_uk_store_get_s32(const struct uk_store_entry *e, __s32 *out)
{
	int ret;

	UK_ASSERT(e);
	if (unlikely(e->get.u8 == NULL))
		return -EIO;

	switch (e->type) {
	GETCASE_UPCAST(e, u8, S32, out, s32);
	GETCASE_UPCAST(e, s8, S32, out, s32);
	GETCASE_UPCAST(e, u16, S32, out, s32);
	GETCASE_UPCAST(e, s16, S32, out, s32);
	GETCASE_DOWNCASTUS(e, u32, S32, out, s32);
	GETCASE_UPCAST(e, s32, S32, out, s32);
	GETCASE_DOWNCASTUS(e, u64, S32, out, s32);
	GETCASE_DOWNCASTSS(e, s64, S32, out, s32);
	GETCASE_DOWNCASTUS(e, uptr, S32, out, s32);

	case UK_STORE_ENTRY_TYPE(charp): {
		char *val = NULL;
		__s16 sscanf_ret;

		ret = e->get.charp(e->cookie, &val);
		if (ret < 0)
			return ret;

		sscanf_ret = sscanf(val, "%" __SCNs32, out);
		if (sscanf_ret < 0)
			return sscanf_ret;

		return ret;
	}

	default:
		return -EINVAL;
	}
}

int
_uk_store_get_u64(const struct uk_store_entry *e, __u64 *out)
{
	int ret;

	UK_ASSERT(e);
	if (unlikely(e->get.u8 == NULL))
		return -EIO;

	switch (e->type) {
	GETCASE_UPCAST(e, u8, U64, out, u64);
	GETCASE_UPCASTSU(e, s8, U64, out, u64);
	GETCASE_UPCAST(e, u16, U64, out, u64);
	GETCASE_UPCASTSU(e, s16, U64, out, u64);
	GETCASE_UPCAST(e, u32, U64, out, u64);
	GETCASE_UPCASTSU(e, s32, U64, out, u64);
	GETCASE_UPCAST(e, u64, U64, out, u64);
	GETCASE_UPCASTSU(e, s64, U64, out, u64);
	GETCASE_UPCAST(e, uptr, U64, out, u64);

	case UK_STORE_ENTRY_TYPE(charp): {
		char *val = NULL;
		__s16 sscanf_ret;

		ret = e->get.charp(e->cookie, &val);
		if (ret < 0)
			return ret;

		sscanf_ret = sscanf(val, "%" __SCNu64, out);
		if (sscanf_ret < 0)
			return sscanf_ret;

		return ret;
	}

	default:
		return -EINVAL;
	}
}

int
_uk_store_get_s64(const struct uk_store_entry *e, __s64 *out)
{
	int ret;

	UK_ASSERT(e);
	if (unlikely(e->get.u8 == NULL))
		return -EIO;

	switch (e->type) {
	GETCASE_UPCAST(e, u8, S64, out, s64);
	GETCASE_UPCAST(e, s8, S64, out, s64);
	GETCASE_UPCAST(e, u16, S64, out, s64);
	GETCASE_UPCAST(e, s16, S64, out, s64);
	GETCASE_UPCAST(e, u32, S64, out, s64);
	GETCASE_UPCAST(e, s32, S64, out, s64);
	GETCASE_DOWNCASTUS(e, u64, S64, out, s64);
	GETCASE_UPCAST(e, s64, S64, out, s64);
	GETCASE_DOWNCASTUS(e, uptr, S64, out, s64);

	case UK_STORE_ENTRY_TYPE(charp): {
		char *val = NULL;
		__s16 sscanf_ret;

		ret = e->get.charp(e->cookie, &val);
		if (ret < 0)
			return ret;

		sscanf_ret = sscanf(val, "%" __SCNs64, out);
		if (sscanf_ret < 0)
			return sscanf_ret;

		return ret;
	}

	default:
		return -EINVAL;
	}
}

int
_uk_store_get_uptr(const struct uk_store_entry *e, __uptr *out)
{
	int ret;

	UK_ASSERT(e);
	if (unlikely(e->get.u8 == NULL))
		return -EIO;

	switch (e->type) {
	GETCASE_UPCAST(e, u8, PTR, out, uptr);
	GETCASE_UPCASTSU(e, s8, PTR, out, uptr);
	GETCASE_UPCAST(e, u16, PTR, out, uptr);
	GETCASE_UPCASTSU(e, s16, PTR, out, uptr);
	GETCASE_UPCAST(e, u32, PTR, out, uptr);
	GETCASE_UPCASTSU(e, s32, PTR, out, uptr);
	GETCASE_UPCAST(e, u64, PTR, out, uptr);
	GETCASE_UPCASTSU(e, s64, PTR, out, uptr);
	GETCASE_UPCAST(e, uptr, PTR, out, uptr);

	case UK_STORE_ENTRY_TYPE(charp): {
		char *val = NULL;
		__s16 sscanf_ret;

		ret = e->get.charp(e->cookie, &val);
		if (ret < 0)
			return ret;

		if (strncmp(val, "0x", 2) == 0)
			sscanf_ret = sscanf(val, "0x%" __SCNx64, out);
		else
			sscanf_ret = sscanf(val, "%" __SCNx64, out);
		if (sscanf_ret < 0)
			return sscanf_ret;

		return ret;
	}

	default:
		return -EINVAL;
	}
}

int
_uk_store_get_charp(const struct uk_store_entry *e, char **out)
{
	int ret;
	char *str;

	UK_ASSERT(e);
	if (unlikely(e->get.u8 == NULL))
		return -EIO;

	switch (e->type) {
	case UK_STORE_ENTRY_TYPE(u8): {
		__u8 val;

		str = calloc(_U8_STRLEN, sizeof(char));
		if (unlikely(!str))
			return -ENOMEM;

		ret = e->get.u8(e->cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(str, _U8_STRLEN, "%" __PRIu8, val);
		*out  = str;
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(s8): {
		__s8 val;

		str = calloc(_S8_STRLEN, sizeof(char));
		if (unlikely(!str))
			return -ENOMEM;

		ret = e->get.s8(e->cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(str, _S8_STRLEN, "%" __PRIs8, val);
		*out  = str;
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(u16): {
		__u16 val;

		str = calloc(_U16_STRLEN, sizeof(char));
		if (unlikely(!str))
			return -ENOMEM;

		ret = e->get.u16(e->cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(str, _U16_STRLEN, "%" __PRIu16, val);
		*out  = str;
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(s16): {
		__s16 val;

		str = calloc(_S16_STRLEN, sizeof(char));
		if (unlikely(!str))
			return -ENOMEM;

		ret = e->get.s16(e->cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(str, _S16_STRLEN, "%" __PRIs16, val);
		*out  = str;
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(u32): {
		__u32 val;

		str = calloc(_U32_STRLEN, sizeof(char));
		if (unlikely(!str))
			return -ENOMEM;

		ret = e->get.u32(e->cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(str, _U32_STRLEN, "%" __PRIu32, val);
		*out  = str;
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(s32): {
		__s32 val;

		str = calloc(_S32_STRLEN, sizeof(char));
		if (unlikely(!str))
			return -ENOMEM;

		ret = e->get.s32(e->cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(str, _S32_STRLEN, "%" __PRIs32, val);
		*out  = str;
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(u64): {
		__u64 val;

		str = calloc(_U64_STRLEN, sizeof(char));
		if (unlikely(!str))
			return -ENOMEM;

		ret = e->get.u64(e->cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(str, _U64_STRLEN, "%" __PRIu64, val);
		*out  = str;
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(s64): {
		__s64 val;

		str = calloc(_S64_STRLEN, sizeof(char));
		if (unlikely(!str))
			return -ENOMEM;

		ret = e->get.s64(e->cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(str, _S64_STRLEN, "%" __PRIs64, val);
		*out  = str;
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(uptr): {
		__uptr val;

		str = calloc(_UPTR_STRLEN, sizeof(char));
		if (unlikely(!str))
			return -ENOMEM;

		ret = e->get.uptr(e->cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(str, _UPTR_STRLEN, "0x%" __PRIx64, val);
		*out  = str;
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(charp): {
		char *val = NULL;

		ret = e->get.charp(e->cookie, &val);
		if (ret < 0)
			return ret;
		*out  = val;
		return ret;
	}

	default:
		return -EINVAL;
	}
}


int
uk_store_get_ncharp(const struct uk_store_entry *e, char *out, __sz maxlen)
{
	int ret;

	UK_ASSERT(e);
	UK_ASSERT(out);
	if (unlikely(e->get.u8 == NULL))
		return -EIO;

	switch (e->type) {
	case UK_STORE_ENTRY_TYPE(u8): {
		__u8 val;

		ret = e->get.u8(e->cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(out, maxlen, "%" __PRIu8, val);
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(s8): {
		__s8 val;

		ret = e->get.s8(e->cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(out, maxlen, "%" __PRIs8, val);
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(u16): {
		__u16 val;

		ret = e->get.u16(e->cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(out, maxlen, "%" __PRIu16, val);
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(s16): {
		__s16 val;

		ret = e->get.s16(e->cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(out, maxlen, "%" __PRIs16, val);
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(u32): {
		__u32 val;

		ret = e->get.u32(e->cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(out, maxlen, "%" __PRIu32, val);
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(s32): {
		__s32 val;

		ret = e->get.s32(e->cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(out, maxlen, "%" __PRIs32, val);
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(u64): {
		__u64 val;

		ret = e->get.u64(e->cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(out, maxlen, "%" __PRIu64, val);
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(s64): {
		__s64 val;

		ret = e->get.s64(e->cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(out, maxlen, "%" __PRIs64, val);
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(uptr): {
		__uptr val;

		ret = e->get.uptr(e->cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(out, maxlen, "0x%" __PRIx64, val);
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(charp): {
		char *val;

		ret = e->get.charp(e->cookie, &val);
		if (ret < 0)
			return ret;
		strncpy(out, val, maxlen);
		free(val);
		return ret;
	}

	default:
		return -EINVAL;
	}
}
