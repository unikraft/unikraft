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
#include <uk/errptr.h>
#include <uk/event.h>
#include <uk/refcount.h>
#include <uk/spinlock.h>
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

#define UK_STORE_ENTRY_ISSTATIC(entry)				\
	((entry)->flags & UK_STORE_ENTRY_FLAG_STATIC)

#define OBJECT_ENTRY(e)						\
	(__containerof(e, struct uk_store_object_entry, entry))

#define OBJECT(e)						\
	(OBJECT_ENTRY(e)->object)

struct uk_store_object_entry {
	struct uk_store_entry entry;
	struct uk_store_object *object;
	struct uk_list_head list_head;
} __align8;

/* The starting point of all dynamic objects for each library */
static struct uk_list_head dynamic_heads[__UKLIBID_COUNT__] = { NULL, };
static __spinlock dynamic_heads_lock = UK_SPINLOCK_INITIALIZER();

#include <uk/bits/store_array.h>

#define _UK_STORE_DYNAMIC_CREATE_TYPED(gen_type)			\
	const struct uk_store_entry *					\
	_uk_store_create_dynamic_entry_ ## gen_type(			\
		struct uk_store_object *object,				\
		__u64 entry_id,						\
		const char *name,					\
		uk_store_get_ ## gen_type ## _func_t get,		\
		uk_store_set_ ## gen_type ## _func_t set)		\
	{								\
		struct uk_store_object_entry *new_object_entry;		\
									\
		UK_ASSERT(object || name || object->a);			\
									\
		new_object_entry = object->a->malloc(object->a,		\
					sizeof(*new_object_entry));	\
		if (!new_object_entry)					\
			return ERR2PTR(ENOMEM);				\
									\
		new_object_entry->entry.name = object->a->malloc(	\
			object->a, sizeof(*name) * (strlen(name) + 1));	\
		if (!new_object_entry->entry.name) {			\
			object->a->free(object->a, new_object_entry);	\
			return ERR2PTR(ENOMEM);				\
		}							\
		strcpy(new_object_entry->entry.name, name);		\
		new_object_entry->entry.id = entry_id;			\
		new_object_entry->entry.type =				\
				UK_STORE_ENTRY_TYPE(gen_type);		\
		new_object_entry->entry.get.gen_type = get;		\
		new_object_entry->entry.set.gen_type = set;		\
		new_object_entry->entry.flags = 0;			\
		new_object_entry->object = object;			\
									\
		uk_list_add(&(new_object_entry)->list_head,		\
			    &(object)->entry_head);			\
									\
		return &new_object_entry->entry;			\
	}

/* Generate a function creator for each type */
_UK_STORE_DYNAMIC_CREATE_TYPED(u8);
_UK_STORE_DYNAMIC_CREATE_TYPED(s8);
_UK_STORE_DYNAMIC_CREATE_TYPED(u16);
_UK_STORE_DYNAMIC_CREATE_TYPED(s16);
_UK_STORE_DYNAMIC_CREATE_TYPED(u32);
_UK_STORE_DYNAMIC_CREATE_TYPED(s32);
_UK_STORE_DYNAMIC_CREATE_TYPED(u64);
_UK_STORE_DYNAMIC_CREATE_TYPED(s64);
_UK_STORE_DYNAMIC_CREATE_TYPED(uptr);
_UK_STORE_DYNAMIC_CREATE_TYPED(charp);

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

static void free_entry(const struct uk_store_entry *entry)
{
	struct uk_alloc *a = OBJECT(entry)->a;

	if (UK_STORE_ENTRY_ISSTATIC(entry))
		return;

	a->free(a, entry->name);
	a->free(a, OBJECT_ENTRY(entry));
}

static void release_entry(const struct uk_store_entry *entry)
{
	struct uk_store_object_entry *obj_entry;

	if (UK_STORE_ENTRY_ISSTATIC(entry))
		return;

	obj_entry = OBJECT_ENTRY(entry);

	uk_list_del(&obj_entry->list_head);

	free_entry(entry);
}

static void free_object(struct uk_store_object *object)
{
	struct uk_store_object_entry *iter, *sec;
	const struct uk_store_entry *entry;

	object->a->free(object->a, object->name);

	uk_list_for_each_entry_safe(iter, sec,
				&object->entry_head, list_head) {
		uk_list_del(&iter->list_head);
		iter->object = NULL;
		entry = &iter->entry;
		release_entry(entry);
	}
}

static struct uk_store_object *get_obj_by_id(unsigned int lib_id, __u64 obj_id)
{
	struct uk_store_object *obj = NULL;

	uk_list_for_each_entry(obj, &dynamic_heads[lib_id], object_head)
		if (obj->id == obj_id)
			break;

	return obj;
}

struct uk_store_object *
uk_store_obj_alloc(struct uk_alloc *a, __u64 id, const char *name,
		   const struct uk_store_entry *entries[], void *cookie)
{
	struct uk_store_object *new_object;
	const struct uk_store_entry *e;

	UK_ASSERT(name);
	UK_ASSERT(entries);

	new_object = a->malloc(a, sizeof(*new_object));
	if (!new_object)
		return ERR2PTR(ENOMEM);

	new_object->name = a->malloc(a, sizeof(*name) * (strlen(name) + 1));
	if (!new_object->name) {
		a->free(a, new_object);
		return ERR2PTR(ENOMEM);
	}

	strcpy(new_object->name, name);

	new_object->a = a;
	new_object->id = id;
	new_object->cookie = cookie;

	uk_refcount_init(&new_object->refcount, 1);

	UK_INIT_LIST_HEAD(&new_object->object_head);
	UK_INIT_LIST_HEAD(&new_object->entry_head);

	for (__sz i = 0; entries[i]; i++) {
		e = entries[i];
		switch (e->type) {
		case UK_STORE_ENTRY_TYPE(s8):
			_uk_store_create_dynamic_entry_s8(new_object,
							  e->id,
							  e->name,
							  e->get.s8,
							  e->set.s8);
			break;
		case UK_STORE_ENTRY_TYPE(u8):
			_uk_store_create_dynamic_entry_u8(new_object,
							  e->id,
							  e->name,
							  e->get.u8,
							  e->set.u8);
			break;
		case UK_STORE_ENTRY_TYPE(s16):
			_uk_store_create_dynamic_entry_s16(new_object,
							   e->id,
							   e->name,
							   e->get.s16,
							   e->set.s16);
			break;
		case UK_STORE_ENTRY_TYPE(u16):
			_uk_store_create_dynamic_entry_u16(new_object,
							   e->id,
							   e->name,
							   e->get.u16,
							   e->set.u16);
			break;
		case UK_STORE_ENTRY_TYPE(s32):
			_uk_store_create_dynamic_entry_s32(new_object,
							   e->id,
							   e->name,
							   e->get.s32,
							   e->set.s32);
			break;
		case UK_STORE_ENTRY_TYPE(u32):
			_uk_store_create_dynamic_entry_u32(new_object,
							   e->id,
							   e->name,
							   e->get.u32,
							   e->set.u32);
			break;
		case UK_STORE_ENTRY_TYPE(s64):
			_uk_store_create_dynamic_entry_s64(new_object,
							   e->id,
							   e->name,
							   e->get.s64,
							   e->set.s64);
			break;
		case UK_STORE_ENTRY_TYPE(u64):
			_uk_store_create_dynamic_entry_u64(new_object,
							   e->id,
							   e->name,
							   e->get.u64,
							   e->set.u64);
			break;

		case UK_STORE_ENTRY_TYPE(uptr):
			_uk_store_create_dynamic_entry_uptr(new_object,
							    e->id,
							    e->name,
							    e->get.uptr,
							    e->set.uptr);
			break;
		case UK_STORE_ENTRY_TYPE(charp):
			_uk_store_create_dynamic_entry_charp(new_object,
							     e->id,
							     e->name,
							     e->get.charp,
							     e->set.charp);
			break;
		default:
			return ERR2PTR(EINVAL);
		};
	}

	return new_object;
}

int _uk_store_obj_add(__u16 library_id, struct uk_store_object *object)
{
	struct uk_store_event_data event_data;

	UK_ASSERT(object);

	if (!dynamic_heads[library_id].next)
		UK_INIT_LIST_HEAD(&dynamic_heads[library_id]);

	object->libid = library_id;
	uk_list_add(&object->object_head, &dynamic_heads[library_id]);

	/* Notify consumers */
	event_data = (struct uk_store_event_data) {
		.library_id = object->libid,
		.object_id = object->id
	};

	uk_raise_event(UKSTORE_EVENT_CREATE_OBJECT, &event_data);

	return 0;
}

struct uk_store_object *uk_store_obj_acquire(__u16 library_id, __u64 object_id)
{
	struct uk_store_object *obj = NULL;

	if (!dynamic_heads[library_id].next)
		return NULL;

	uk_spin_lock(&dynamic_heads_lock);

	obj = get_obj_by_id(library_id, object_id);
	if (unlikely(!obj))
		goto out;

	uk_refcount_acquire(&obj->refcount);
out:
	uk_spin_unlock(&dynamic_heads_lock);

	return obj;
}

void uk_store_obj_release(struct uk_store_object *object)
{
	int res;

	UK_ASSERT(object);

	if (!dynamic_heads[object->libid].next)
		return;

	uk_spin_lock(&dynamic_heads_lock);

	res = uk_refcount_release_if_not_last(&object->refcount);
	if (!res) {
		uk_list_del(&(object->object_head));
		free_object(object);
	}
	uk_spin_unlock(&dynamic_heads_lock);
}

const struct uk_store_entry *uk_store_static_entry_get(__u16 libid,
						       __u64 entry_id)
{
	struct uk_store_entry *entry = static_entries[2 * libid];
	struct uk_store_entry *stop = static_entries[2 * libid + 1];

	for (; entry != stop; ++entry)
		if (entry->id == entry_id)
			return entry;

	return NULL;
}

const struct uk_store_entry *
uk_store_obj_entry_get(struct uk_store_object *object, __u64 entry_id)
{
	struct uk_store_object_entry *res = NULL;

	UK_ASSERT(object);

	uk_list_for_each_entry(res, &object->entry_head, list_head)
		if (res->entry.id == entry_id)
			break;

	if (res)
		return &res->entry;

	return NULL;
}

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
#define SETCASE_DOWNCASTSU(entry, etype, ETYPE, var, eparam, cookie)	\
	do {								\
		case UK_STORE_ENTRY_TYPE(etype):			\
		if (unlikely(var < 0 ||					\
			var > (__ ## eparam) __ ## ETYPE ## _MAX))	\
			return -ERANGE;					\
		return (entry)->set.etype(cookie, (__ ## etype) var);	\
	} while (0)

/* Unsigned input, signed etype */
#define SETCASE_DOWNCASTUS(entry, etype, ETYPE, var, eparam, cookie)	\
	do {								\
		case UK_STORE_ENTRY_TYPE(etype):			\
		if (unlikely(var > __ ## ETYPE ## _MAX))		\
			return -ERANGE;					\
		return (entry)->set.etype(cookie, (__ ## etype) var);	\
	} while (0)

/* Both signed */
#define SETCASE_DOWNCASTSS(entry, etype, ETYPE, var, eparam, cookie)	\
	do {								\
		case UK_STORE_ENTRY_TYPE(etype):			\
		if (unlikely(var < __ ## ETYPE ## _MIN ||		\
				var > __ ## ETYPE ## _MAX))		\
			return -ERANGE;					\
		return (entry)->set.etype(cookie, (__ ## etype) var);	\
	} while (0)

/* Both unsigned */
#define SETCASE_DOWNCASTUU(entry, etype, ETYPE, var, eparam, cookie)	\
	do {								\
		case UK_STORE_ENTRY_TYPE(etype):			\
		if (unlikely(var > __ ## ETYPE ## _MAX))		\
			return -ERANGE;					\
		return (entry)->set.etype(cookie, (__ ## etype) var);	\
	} while (0)

/* Signed input, unsigned etype */
#define SETCASE_UPCASTSU(entry, etype, ETYPE, var, eparam, cookie)	\
	do {								\
		case UK_STORE_ENTRY_TYPE(etype):			\
		if (unlikely(var < 0))					\
			return -ERANGE;					\
		return (entry)->set.etype(cookie, (__ ## etype) var);	\
	} while (0)

/* All other cases */
#define SETCASE_UPCAST(entry, etype, ETYPE, var, eparam, cookie)	\
	do {								\
		case UK_STORE_ENTRY_TYPE(etype):			\
		;							\
		return (entry)->set.etype(cookie, (__ ## etype) var);	\
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
	void *cookie = NULL;

	UK_ASSERT(e);
	if (unlikely(e->set.u8 == NULL))
		return -EIO;

	if (!UK_STORE_ENTRY_ISSTATIC(e))
		cookie = OBJECT(e)->cookie;

	switch (e->type) {
	SETCASE_DOWNCASTUS(e, s8, S8, val, u8, cookie);
	SETCASE_UPCAST(e, u8, U8, val, u8, cookie);
	SETCASE_UPCAST(e, u16, U16, val, u8, cookie);
	SETCASE_UPCAST(e, s16, S16, val, u8, cookie);
	SETCASE_UPCAST(e, u32, U32, val, u8, cookie);
	SETCASE_UPCAST(e, s32, S32, val, u8, cookie);
	SETCASE_UPCAST(e, u64, U64, val, u8, cookie);
	SETCASE_UPCAST(e, s64, S64, val, u8, cookie);
	SETCASE_UPCAST(e, uptr, PTR, val, u8, cookie);

	case UK_STORE_ENTRY_TYPE(charp): {
		int ret;
		char to_set[_U8_STRLEN];

		snprintf(to_set, _U8_STRLEN, "%" __PRIu8, val);
		ret = e->set.charp(cookie, to_set);
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
	void *cookie = NULL;

	UK_ASSERT(e);
	if (unlikely(e->set.u8 == NULL))
		return -EIO;

	if (!UK_STORE_ENTRY_ISSTATIC(e))
		cookie = OBJECT(e)->cookie;

	switch (e->type) {
	SETCASE_UPCAST(e, s8, S8, val, s8, cookie);
	SETCASE_UPCASTSU(e, u8, U8, val, s8, cookie);
	SETCASE_UPCAST(e, s16, S16, val, s8, cookie);
	SETCASE_UPCASTSU(e, u16, U16, val, s8, cookie);
	SETCASE_UPCAST(e, s32, S32, val, s8, cookie);
	SETCASE_UPCASTSU(e, u32, U32, val, s8, cookie);
	SETCASE_UPCAST(e, u64, U64, val, s8, cookie);
	SETCASE_UPCASTSU(e, s64, S64, val, s8, cookie);
	SETCASE_UPCASTSU(e, uptr, PTR, val, s8, cookie);

	case UK_STORE_ENTRY_TYPE(charp): {
		int ret;
		char to_set[_S8_STRLEN];

		snprintf(to_set, _S8_STRLEN, "%" __PRIs8, val);
		ret = e->set.charp(cookie, to_set);
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
	void *cookie = NULL;

	UK_ASSERT(e);
	if (unlikely(e->set.u8 == NULL))
		return -EIO;

	if (!UK_STORE_ENTRY_ISSTATIC(e))
		cookie = OBJECT(e)->cookie;

	switch (e->type) {
	SETCASE_DOWNCASTUS(e, s8, S8, val, u16, cookie);
	SETCASE_DOWNCASTUU(e, u8, U8, val, u16, cookie);
	SETCASE_DOWNCASTUS(e, s16, S16, val, u16, cookie);
	SETCASE_UPCAST(e, u16, U16, val, u16, cookie);
	SETCASE_UPCAST(e, s32, S32, val, u16, cookie);
	SETCASE_UPCAST(e, u32, U32, val, u16, cookie);
	SETCASE_UPCAST(e, u64, S64, val, u16, cookie);
	SETCASE_UPCAST(e, uptr, PTR, val, u16, cookie);

	case UK_STORE_ENTRY_TYPE(charp): {
		int ret;
		char to_set[_U16_STRLEN];

		snprintf(to_set, _U16_STRLEN, "%" __PRIu16, val);
		ret = e->set.charp(cookie, to_set);
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
	void *cookie = NULL;

	UK_ASSERT(e);
	if (unlikely(e->set.u8 == NULL))
		return -EIO;

	if (!UK_STORE_ENTRY_ISSTATIC(e))
		cookie = OBJECT(e)->cookie;

	switch (e->type) {
	SETCASE_DOWNCASTSS(e, s8, S8, val, s16, cookie);
	SETCASE_DOWNCASTSU(e, u8, U8, val, s16, cookie);
	SETCASE_UPCAST(e, s16, S16, val, s16, cookie);
	SETCASE_UPCASTSU(e, u16, U16, val, s16, cookie);
	SETCASE_UPCAST(e, s32, S32, val, s16, cookie);
	SETCASE_UPCASTSU(e, u32, U32, val, s16, cookie);
	SETCASE_UPCAST(e, s64, S64, val, s16, cookie);
	SETCASE_UPCASTSU(e, u64, U64, val, s16, cookie);
	SETCASE_UPCASTSU(e, uptr, PTR, val, s16, cookie);

	case UK_STORE_ENTRY_TYPE(charp): {
		int ret;
		char to_set[_S16_STRLEN];

		snprintf(to_set, _S16_STRLEN, "%" __PRIs16, val);
		ret = e->set.charp(cookie, to_set);
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
	void *cookie = NULL;

	UK_ASSERT(e);
	if (unlikely(e->set.u8 == NULL))
		return -EIO;

	if (!UK_STORE_ENTRY_ISSTATIC(e))
		cookie = OBJECT(e)->cookie;

	switch (e->type) {
	SETCASE_DOWNCASTUS(e, s8, S8, val, u32, cookie);
	SETCASE_DOWNCASTUU(e, u8, U8, val, u32, cookie);
	SETCASE_DOWNCASTUS(e, s16, S16, val, u32, cookie);
	SETCASE_DOWNCASTUU(e, u16, U16, val, u32, cookie);
	SETCASE_DOWNCASTUS(e, s32, S32, val, u32, cookie);
	SETCASE_UPCAST(e, u32, U32, val, u32, cookie);
	SETCASE_UPCAST(e, s64, S64, val, u32, cookie);
	SETCASE_UPCAST(e, u64, U64, val, u32, cookie);
	SETCASE_UPCAST(e, uptr, UPTR, val, u32, cookie);

	case UK_STORE_ENTRY_TYPE(charp): {
		int ret;
		char to_set[_U32_STRLEN];

		snprintf(to_set, _U32_STRLEN, "%" __PRIu32, val);
		ret = e->set.charp(cookie, to_set);
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
	void *cookie = NULL;

	UK_ASSERT(e);
	if (unlikely(e->set.u8 == NULL))
		return -EIO;

	if (!UK_STORE_ENTRY_ISSTATIC(e))
		cookie = OBJECT(e)->cookie;

	switch (e->type) {
	SETCASE_DOWNCASTSS(e, s8, S8, val, s32, cookie);
	SETCASE_DOWNCASTSU(e, u8, U8, val, s32, cookie);
	SETCASE_DOWNCASTSS(e, s16, S16, val, s32, cookie);
	SETCASE_DOWNCASTSU(e, u16, U16, val, s32, cookie);
	SETCASE_UPCAST(e, s32, S32, val, s32, cookie);
	SETCASE_UPCASTSU(e, u32, S32, val, s32, cookie);
	SETCASE_UPCAST(e, s64, S64, val, s32, cookie);
	SETCASE_UPCASTSU(e, u64, U64, val, s32, cookie);
	SETCASE_UPCASTSU(e, uptr, PTR, val, s32, cookie);

	case UK_STORE_ENTRY_TYPE(charp): {
		int ret;
		char to_set[_S32_STRLEN];

		snprintf(to_set, _S32_STRLEN, "%" __PRIs32, val);
		ret = e->set.charp(cookie, to_set);
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
	void *cookie = NULL;

	UK_ASSERT(e);
	if (unlikely(e->set.u8 == NULL))
		return -EIO;

	if (!UK_STORE_ENTRY_ISSTATIC(e))
		cookie = OBJECT(e)->cookie;

	switch (e->type) {
	SETCASE_DOWNCASTUS(e, s8, S8, val, u64, cookie);
	SETCASE_DOWNCASTUU(e, u8, U8, val, u64, cookie);
	SETCASE_DOWNCASTUS(e, s16, S16, val, u64, cookie);
	SETCASE_DOWNCASTUU(e, u16, U16, val, u64, cookie);
	SETCASE_DOWNCASTUS(e, s32, S32, val, u64, cookie);
	SETCASE_DOWNCASTUU(e, u32, U32, val, u64, cookie);
	SETCASE_DOWNCASTUS(e, s64, S64, val, u64, cookie);
	SETCASE_UPCAST(e, u64, U64, val, u64, cookie);
	SETCASE_UPCAST(e, uptr, PTR, val, u64, cookie);

	case UK_STORE_ENTRY_TYPE(charp): {
		int ret;
		char to_set[_U64_STRLEN];

		snprintf(to_set, _U64_STRLEN, "%" __PRIu64, val);
		ret = e->set.charp(cookie, to_set);
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
	void *cookie = NULL;

	UK_ASSERT(e);
	if (unlikely(e->set.u8 == NULL))
		return -EIO;

	if (!UK_STORE_ENTRY_ISSTATIC(e))
		cookie = OBJECT(e)->cookie;

	switch (e->type) {
	SETCASE_DOWNCASTSS(e, s8, S8, val, s64, cookie);
	SETCASE_DOWNCASTSU(e, u8, U8, val, s64, cookie);
	SETCASE_DOWNCASTSS(e, s16, S16, val, s64, cookie);
	SETCASE_DOWNCASTSU(e, u16, U16, val, s64, cookie);
	SETCASE_DOWNCASTSS(e, s32, S32, val, s64, cookie);
	SETCASE_DOWNCASTSU(e, u32, S32, val, s64, cookie);
	SETCASE_UPCAST(e, s64, S64, val, s64, cookie);
	SETCASE_UPCASTSU(e, u64, U64, val, s64, cookie);
	SETCASE_UPCASTSU(e, uptr, PTR, val, s64, cookie);

	case UK_STORE_ENTRY_TYPE(charp): {
		int ret;
		char to_set[_S64_STRLEN];

		snprintf(to_set, _S64_STRLEN, "%" __PRIs64, val);
		ret = e->set.charp(cookie, to_set);
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
	void *cookie = NULL;

	UK_ASSERT(e);
	if (unlikely(e->set.u8 == NULL))
		return -EIO;

	if (!UK_STORE_ENTRY_ISSTATIC(e))
		cookie = OBJECT(e)->cookie;

	switch (e->type) {
	SETCASE_DOWNCASTUS(e, s8, S8, val, uptr, cookie);
	SETCASE_DOWNCASTUU(e, u8, U8, val, uptr, cookie);
	SETCASE_DOWNCASTUS(e, s16, S16, val, uptr, cookie);
	SETCASE_DOWNCASTUU(e, u16, U16, val, uptr, cookie);
	SETCASE_DOWNCASTUS(e, s32, S32, val, uptr, cookie);
	SETCASE_DOWNCASTUU(e, u32, U32, val, uptr, cookie);
	SETCASE_DOWNCASTUS(e, s64, S64, val, uptr, cookie);
	SETCASE_UPCAST(e, u64, U64, val, uptr, cookie);
	SETCASE_UPCAST(e, uptr, PTR, val, uptr, cookie);

	case UK_STORE_ENTRY_TYPE(charp): {
		int ret;
		char to_set[_UPTR_STRLEN];

		snprintf(to_set, _UPTR_STRLEN, "0x%" __PRIx64, val);
		ret = e->set.charp(cookie, to_set);
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
	void *cookie = NULL;

	UK_ASSERT(e);
	if (unlikely(e->set.u8 == NULL))
		return -EIO;

	if (!UK_STORE_ENTRY_ISSTATIC(e))
		cookie = OBJECT(e)->cookie;

	switch (e->type) {
	case UK_STORE_ENTRY_TYPE(u8): {
		__u8 to_set;

		ret = sscanf(val, "%" __SCNu8, &to_set);
		if (ret < 0)
			return ret;

		return e->set.u8(cookie, to_set);
	}

	case UK_STORE_ENTRY_TYPE(s8): {
		__s8 to_set;

		ret = sscanf(val, "%" __SCNs8, &to_set);
		if (ret < 0)
			return ret;

		return e->set.s8(cookie, to_set);
	}

	case UK_STORE_ENTRY_TYPE(u16): {
		__u16 to_set;

		ret = sscanf(val, "%" __SCNu16, &to_set);
		if (ret < 0)
			return ret;

		return e->set.u16(cookie, to_set);
	}

	case UK_STORE_ENTRY_TYPE(s16): {
		__s16 to_set;

		ret = sscanf(val, "%" __SCNs16, &to_set);
		if (ret < 0)
			return ret;

		return e->set.s16(cookie, to_set);
	}

	case UK_STORE_ENTRY_TYPE(u32): {
		__u32 to_set;

		ret = sscanf(val, "%" __SCNu32, &to_set);
		if (ret < 0)
			return ret;

		return e->set.u32(cookie, to_set);
	}

	case UK_STORE_ENTRY_TYPE(s32): {
		__s32 to_set;

		ret = sscanf(val, "%" __SCNs32, &to_set);
		if (ret < 0)
			return ret;

		return e->set.s32(cookie, to_set);
	}

	case UK_STORE_ENTRY_TYPE(u64): {
		__u64 to_set;

		ret = sscanf(val, "%" __SCNu64, &to_set);
		if (ret < 0)
			return ret;

		return e->set.u64(cookie, to_set);
	}

	case UK_STORE_ENTRY_TYPE(s64): {
		__s64 to_set;

		ret = sscanf(val, "%" __SCNs64, &to_set);
		if (ret < 0)
			return ret;

		return e->set.s64(cookie, to_set);
	}

	case UK_STORE_ENTRY_TYPE(uptr): {
		__uptr to_set;

		if (strncmp(val, "0x", 2) == 0)
			ret = sscanf(val, "0x%" __SCNx64, &to_set);
		else
			ret = sscanf(val, "%" __SCNx64, &to_set);

		if (ret < 0)
			return ret;

		return e->set.uptr(cookie, to_set);
	}

	case UK_STORE_ENTRY_TYPE(charp): {
		ret = e->set.charp(cookie, val);

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
#define GETCASE_DOWNCASTSU(entry, etype, PTYPE, var, eparam, cookie)	\
	do {								\
		case UK_STORE_ENTRY_TYPE(etype): {			\
			__ ## etype val;				\
									\
			ret = (entry)->get.etype(cookie, &val);		\
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
#define GETCASE_DOWNCASTUS(entry, etype, PTYPE, var, eparam, cookie)	\
	do {								\
		case UK_STORE_ENTRY_TYPE(etype): {			\
			__ ## etype val;				\
									\
			ret = (entry)->get.etype(cookie, &val);		\
			if (ret < 0)					\
				return ret;				\
			if (unlikely(val > __ ## PTYPE ## _MAX))	\
				return -ERANGE;				\
			*(var)  = (__ ## eparam) val;			\
			return ret;					\
		}							\
	} while (0)

/* Both signed */
#define GETCASE_DOWNCASTSS(entry, etype, PTYPE, var, eparam, cookie)	\
	do {								\
		case UK_STORE_ENTRY_TYPE(etype): {			\
			__ ## etype val;				\
									\
			ret = (entry)->get.etype(cookie, &val);		\
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
#define GETCASE_DOWNCASTUU(entry, etype, PTYPE, var, eparam, cookie)	\
	do {								\
		case UK_STORE_ENTRY_TYPE(etype): {			\
			__ ## etype val;				\
									\
			ret = (entry)->get.etype(cookie, &val);		\
			if (ret < 0)					\
				return ret;				\
			if (unlikely(val > __ ## PTYPE ## _MAX))	\
				return -ERANGE;				\
			*(var)  = (__ ## eparam) val;			\
			return ret;					\
		}							\
	} while (0)

/* All other cases */
#define GETCASE_UPCAST(entry, etype, PTYPE, var, eparam, cookie)	\
	do {								\
		case UK_STORE_ENTRY_TYPE(etype): {			\
			__ ## etype val;				\
									\
			ret = (entry)->get.etype(cookie, &val);		\
			if (ret < 0)					\
				return ret;				\
			*(var)  = (__ ## eparam) val;			\
			return ret;					\
		}							\
	} while (0)

/* Unsigned input, signed etype */
#define GETCASE_UPCASTSU(entry, etype, PTYPE, var, eparam, cookie)	\
	do {								\
		case UK_STORE_ENTRY_TYPE(etype): {			\
			__ ## etype val;				\
									\
			ret = (entry)->get.etype(cookie, &val);		\
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
	void *cookie = NULL;

	UK_ASSERT(e);
	if (unlikely(e->get.u8 == NULL))
		return -EIO;

	if (!UK_STORE_ENTRY_ISSTATIC(e))
		cookie = OBJECT(e)->cookie;

	switch (e->type) {
	GETCASE_UPCAST(e, u8, U8, out, u8, cookie);
	GETCASE_UPCASTSU(e, s8, U8, out, u8, cookie);
	GETCASE_DOWNCASTUU(e, u16, U8, out, u8, cookie);
	GETCASE_DOWNCASTSU(e, s16, U8, out, u8, cookie);
	GETCASE_DOWNCASTUU(e, u32, U8, out, u8, cookie);
	GETCASE_DOWNCASTSU(e, s32, U8, out, u8, cookie);
	GETCASE_DOWNCASTUU(e, u64, U8, out, u8, cookie);
	GETCASE_DOWNCASTSU(e, s64, U8, out, u8, cookie);
	GETCASE_DOWNCASTUU(e, uptr, U8, out, u8, cookie);

	case UK_STORE_ENTRY_TYPE(charp): {
		char *val = NULL;
		__s16 sscanf_ret;

		ret = e->get.charp(cookie, &val);
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
	void *cookie = NULL;

	UK_ASSERT(e);
	if (unlikely(e->get.u8 == NULL))
		return -EIO;

	if (!UK_STORE_ENTRY_ISSTATIC(e))
		cookie = OBJECT(e)->cookie;

	switch (e->type) {
	GETCASE_DOWNCASTUS(e, u8, S8, out, s8, cookie);
	GETCASE_UPCAST(e, s8, S8, out, s8, cookie);
	GETCASE_DOWNCASTUS(e, u16, S8, out, s8, cookie);
	GETCASE_DOWNCASTSS(e, s16, S8, out, s8, cookie);
	GETCASE_DOWNCASTUS(e, u32, S8, out, s8, cookie);
	GETCASE_DOWNCASTSS(e, s32, S8, out, s8, cookie);
	GETCASE_DOWNCASTUS(e, u64, S8, out, s8, cookie);
	GETCASE_DOWNCASTSS(e, s64, S8, out, s8, cookie);
	GETCASE_DOWNCASTUS(e, uptr, S8, out, s8, cookie);

	case UK_STORE_ENTRY_TYPE(charp): {
		char *val = NULL;
		__s16 sscanf_ret;

		ret = e->get.charp(cookie, &val);
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
	void *cookie = NULL;

	UK_ASSERT(e);
	if (unlikely(e->get.u8 == NULL))
		return -EIO;

	if (!UK_STORE_ENTRY_ISSTATIC(e))
		cookie = OBJECT(e)->cookie;

	switch (e->type) {
	GETCASE_UPCAST(e, u8, U16, out, u16, cookie);
	GETCASE_UPCASTSU(e, s8, U16, out, u16, cookie);
	GETCASE_UPCAST(e, u16, U16, out, u16, cookie);
	GETCASE_UPCASTSU(e, s16, U16, out, u16, cookie);
	GETCASE_DOWNCASTUU(e, u32, U16, out, u16, cookie);
	GETCASE_DOWNCASTSU(e, s32, U16, out, u16, cookie);
	GETCASE_DOWNCASTUU(e, u64, U16, out, u16, cookie);
	GETCASE_DOWNCASTSU(e, s64, U16, out, u16, cookie);
	GETCASE_DOWNCASTUU(e, uptr, U16, out, u16, cookie);

	case UK_STORE_ENTRY_TYPE(charp): {
		char *val = NULL;
		__s16 sscanf_ret;

		ret = e->get.charp(cookie, &val);
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
	void *cookie = NULL;

	UK_ASSERT(e);
	if (unlikely(e->get.u8 == NULL))
		return -EIO;

	if (!UK_STORE_ENTRY_ISSTATIC(e))
		cookie = OBJECT(e)->cookie;

	switch (e->type) {
	GETCASE_UPCAST(e, u8, S16, out, s16, cookie);
	GETCASE_UPCAST(e, s8, S16, out, s16, cookie);
	GETCASE_DOWNCASTUS(e, u16, S16, out, s16, cookie);
	GETCASE_UPCAST(e, s16, S16, out, s16, cookie);
	GETCASE_DOWNCASTUS(e, u32, S16, out, s16, cookie);
	GETCASE_DOWNCASTSS(e, s32, S16, out, s16, cookie);
	GETCASE_DOWNCASTUS(e, u64, S16, out, s16, cookie);
	GETCASE_DOWNCASTSS(e, s64, S16, out, s16, cookie);
	GETCASE_DOWNCASTUS(e, uptr, S16, out, s16, cookie);

	case UK_STORE_ENTRY_TYPE(charp): {
		char *val = NULL;
		__s16 sscanf_ret;

		ret = e->get.charp(cookie, &val);
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
	void *cookie = NULL;

	UK_ASSERT(e);
	if (unlikely(e->get.u8 == NULL))
		return -EIO;

	if (!UK_STORE_ENTRY_ISSTATIC(e))
		cookie = OBJECT(e)->cookie;

	switch (e->type) {
	GETCASE_UPCAST(e, u8, U32, out, u32, cookie);
	GETCASE_UPCASTSU(e, s8, U32, out, u32, cookie);
	GETCASE_UPCAST(e, u16, U32, out, u32, cookie);
	GETCASE_UPCASTSU(e, s16, U32, out, u32, cookie);
	GETCASE_UPCAST(e, u32, U32, out, u32, cookie);
	GETCASE_UPCASTSU(e, s32, U32, out, u32, cookie);
	GETCASE_DOWNCASTUU(e, u64, U32, out, u32, cookie);
	GETCASE_DOWNCASTSU(e, s64, U32, out, u32, cookie);
	GETCASE_DOWNCASTUU(e, uptr, U32, out, u32, cookie);

	case UK_STORE_ENTRY_TYPE(charp): {
		char *val = NULL;
		__s16 sscanf_ret;

		ret = e->get.charp(cookie, &val);
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
	void *cookie = NULL;

	UK_ASSERT(e);
	if (unlikely(e->get.u8 == NULL))
		return -EIO;

	if (!UK_STORE_ENTRY_ISSTATIC(e))
		cookie = OBJECT(e)->cookie;

	switch (e->type) {
	GETCASE_UPCAST(e, u8, S32, out, s32, cookie);
	GETCASE_UPCAST(e, s8, S32, out, s32, cookie);
	GETCASE_UPCAST(e, u16, S32, out, s32, cookie);
	GETCASE_UPCAST(e, s16, S32, out, s32, cookie);
	GETCASE_DOWNCASTUS(e, u32, S32, out, s32, cookie);
	GETCASE_UPCAST(e, s32, S32, out, s32, cookie);
	GETCASE_DOWNCASTUS(e, u64, S32, out, s32, cookie);
	GETCASE_DOWNCASTSS(e, s64, S32, out, s32, cookie);
	GETCASE_DOWNCASTUS(e, uptr, S32, out, s32, cookie);

	case UK_STORE_ENTRY_TYPE(charp): {
		char *val = NULL;
		__s16 sscanf_ret;

		ret = e->get.charp(cookie, &val);
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
	void *cookie = NULL;

	UK_ASSERT(e);
	if (unlikely(e->get.u8 == NULL))
		return -EIO;

	if (!UK_STORE_ENTRY_ISSTATIC(e))
		cookie = OBJECT(e)->cookie;

	switch (e->type) {
	GETCASE_UPCAST(e, u8, U64, out, u64, cookie);
	GETCASE_UPCASTSU(e, s8, U64, out, u64, cookie);
	GETCASE_UPCAST(e, u16, U64, out, u64, cookie);
	GETCASE_UPCASTSU(e, s16, U64, out, u64, cookie);
	GETCASE_UPCAST(e, u32, U64, out, u64, cookie);
	GETCASE_UPCASTSU(e, s32, U64, out, u64, cookie);
	GETCASE_UPCAST(e, u64, U64, out, u64, cookie);
	GETCASE_UPCASTSU(e, s64, U64, out, u64, cookie);
	GETCASE_UPCAST(e, uptr, U64, out, u64, cookie);

	case UK_STORE_ENTRY_TYPE(charp): {
		char *val = NULL;
		__s16 sscanf_ret;

		ret = e->get.charp(cookie, &val);
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
	void *cookie = NULL;

	UK_ASSERT(e);
	if (unlikely(e->get.u8 == NULL))
		return -EIO;

	if (!UK_STORE_ENTRY_ISSTATIC(e))
		cookie = OBJECT(e)->cookie;

	switch (e->type) {
	GETCASE_UPCAST(e, u8, S64, out, s64, cookie);
	GETCASE_UPCAST(e, s8, S64, out, s64, cookie);
	GETCASE_UPCAST(e, u16, S64, out, s64, cookie);
	GETCASE_UPCAST(e, s16, S64, out, s64, cookie);
	GETCASE_UPCAST(e, u32, S64, out, s64, cookie);
	GETCASE_UPCAST(e, s32, S64, out, s64, cookie);
	GETCASE_DOWNCASTUS(e, u64, S64, out, s64, cookie);
	GETCASE_UPCAST(e, s64, S64, out, s64, cookie);
	GETCASE_DOWNCASTUS(e, uptr, S64, out, s64, cookie);

	case UK_STORE_ENTRY_TYPE(charp): {
		char *val = NULL;
		__s16 sscanf_ret;

		ret = e->get.charp(cookie, &val);
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
	void *cookie = NULL;

	UK_ASSERT(e);
	if (unlikely(e->get.u8 == NULL))
		return -EIO;

	if (!UK_STORE_ENTRY_ISSTATIC(e))
		cookie = OBJECT(e)->cookie;

	switch (e->type) {
	GETCASE_UPCAST(e, u8, PTR, out, uptr, cookie);
	GETCASE_UPCASTSU(e, s8, PTR, out, uptr, cookie);
	GETCASE_UPCAST(e, u16, PTR, out, uptr, cookie);
	GETCASE_UPCASTSU(e, s16, PTR, out, uptr, cookie);
	GETCASE_UPCAST(e, u32, PTR, out, uptr, cookie);
	GETCASE_UPCASTSU(e, s32, PTR, out, uptr, cookie);
	GETCASE_UPCAST(e, u64, PTR, out, uptr, cookie);
	GETCASE_UPCASTSU(e, s64, PTR, out, uptr, cookie);
	GETCASE_UPCAST(e, uptr, PTR, out, uptr, cookie);

	case UK_STORE_ENTRY_TYPE(charp): {
		char *val = NULL;
		__s16 sscanf_ret;

		ret = e->get.charp(cookie, &val);
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
	void *cookie = NULL;
	struct uk_alloc *a = OBJECT(e)->a;

	UK_ASSERT(e);
	if (unlikely(e->get.u8 == NULL))
		return -EIO;

	if (!UK_STORE_ENTRY_ISSTATIC(e))
		cookie = OBJECT(e)->cookie;

	switch (e->type) {
	case UK_STORE_ENTRY_TYPE(u8): {
		__u8 val;

		if (UK_STORE_ENTRY_ISSTATIC(e))
			str = calloc(_U8_STRLEN, sizeof(char));
		else
			str = a->calloc(a, _U8_STRLEN, sizeof(char));

		if (unlikely(!str))
			return -ENOMEM;

		ret = e->get.u8(cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(str, _U8_STRLEN, "%" __PRIu8, val);
		*out  = str;
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(s8): {
		__s8 val;

		if (UK_STORE_ENTRY_ISSTATIC(e))
			str = calloc(_S8_STRLEN, sizeof(char));
		else
			str = a->calloc(a, _S8_STRLEN, sizeof(char));

		if (unlikely(!str))
			return -ENOMEM;

		ret = e->get.s8(cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(str, _S8_STRLEN, "%" __PRIs8, val);
		*out  = str;
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(u16): {
		__u16 val;

		if (UK_STORE_ENTRY_ISSTATIC(e))
			str = calloc(_U16_STRLEN, sizeof(char));
		else
			str = a->calloc(a, _U16_STRLEN, sizeof(char));

		if (unlikely(!str))
			return -ENOMEM;

		ret = e->get.u16(cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(str, _U16_STRLEN, "%" __PRIu16, val);
		*out  = str;
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(s16): {
		__s16 val;

		if (UK_STORE_ENTRY_ISSTATIC(e))
			str = calloc(_S16_STRLEN, sizeof(char));
		else
			str = a->calloc(a, _S16_STRLEN, sizeof(char));

		if (unlikely(!str))
			return -ENOMEM;

		ret = e->get.s16(cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(str, _S16_STRLEN, "%" __PRIs16, val);
		*out  = str;
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(u32): {
		__u32 val;

		if (UK_STORE_ENTRY_ISSTATIC(e))
			str = calloc(_U32_STRLEN, sizeof(char));
		else
			str = a->calloc(a, _U32_STRLEN, sizeof(char));

		if (unlikely(!str))
			return -ENOMEM;

		ret = e->get.u32(cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(str, _U32_STRLEN, "%" __PRIu32, val);
		*out  = str;
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(s32): {
		__s32 val;

		if (UK_STORE_ENTRY_ISSTATIC(e))
			str = calloc(_S32_STRLEN, sizeof(char));
		else
			str = a->calloc(a, _S32_STRLEN, sizeof(char));

		if (unlikely(!str))
			return -ENOMEM;

		ret = e->get.s32(cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(str, _S32_STRLEN, "%" __PRIs32, val);
		*out  = str;
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(u64): {
		__u64 val;

		if (UK_STORE_ENTRY_ISSTATIC(e))
			str = calloc(_U64_STRLEN, sizeof(char));
		else
			str = a->calloc(a, _U64_STRLEN, sizeof(char));

		if (unlikely(!str))
			return -ENOMEM;

		ret = e->get.u64(cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(str, _U64_STRLEN, "%" __PRIu64, val);
		*out  = str;
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(s64): {
		__s64 val;

		if (UK_STORE_ENTRY_ISSTATIC(e))
			str = calloc(_S64_STRLEN, sizeof(char));
		else
			str = a->calloc(a, _S64_STRLEN, sizeof(char));

		if (unlikely(!str))
			return -ENOMEM;

		ret = e->get.s64(cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(str, _S64_STRLEN, "%" __PRIs64, val);
		*out  = str;
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(uptr): {
		__uptr val;

		if (UK_STORE_ENTRY_ISSTATIC(e))
			str = calloc(_UPTR_STRLEN, sizeof(char));
		else
			str = a->calloc(a, _UPTR_STRLEN, sizeof(char));

		if (unlikely(!str))
			return -ENOMEM;

		ret = e->get.uptr(cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(str, _UPTR_STRLEN, "0x%" __PRIx64, val);
		*out  = str;
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(charp): {
		char *val = NULL;

		ret = e->get.charp(cookie, &val);
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
_uk_store_get_ncharp(const struct uk_store_entry *e, char *out, __sz maxlen)
{
	int ret;
	void *cookie = NULL;

	UK_ASSERT(e);
	UK_ASSERT(out);
	if (unlikely(e->get.u8 == NULL))
		return -EIO;

	if (!UK_STORE_ENTRY_ISSTATIC(e))
		cookie = OBJECT(e)->cookie;

	switch (e->type) {
	case UK_STORE_ENTRY_TYPE(u8): {
		__u8 val;

		ret = e->get.u8(cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(out, maxlen, "%" __PRIu8, val);
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(s8): {
		__s8 val;

		ret = e->get.s8(cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(out, maxlen, "%" __PRIs8, val);
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(u16): {
		__u16 val;

		ret = e->get.u16(cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(out, maxlen, "%" __PRIu16, val);
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(s16): {
		__s16 val;

		ret = e->get.s16(cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(out, maxlen, "%" __PRIs16, val);
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(u32): {
		__u32 val;

		ret = e->get.u32(cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(out, maxlen, "%" __PRIu32, val);
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(s32): {
		__s32 val;

		ret = e->get.s32(cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(out, maxlen, "%" __PRIs32, val);
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(u64): {
		__u64 val;

		ret = e->get.u64(cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(out, maxlen, "%" __PRIu64, val);
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(s64): {
		__s64 val;

		ret = e->get.s64(cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(out, maxlen, "%" __PRIs64, val);
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(uptr): {
		__uptr val;

		ret = e->get.uptr(cookie, &val);
		if (ret < 0)
			return ret;
		snprintf(out, maxlen, "0x%" __PRIx64, val);
		return ret;
	}

	case UK_STORE_ENTRY_TYPE(charp): {
		char *val;

		ret = e->get.charp(cookie, &val);
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

UK_EVENT(UKSTORE_EVENT_CREATE_OBJECT);
UK_EVENT(UKSTORE_EVENT_RELEASE_OBJECT);
