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

struct uk_store_folder {
	/* List for the dynamic folders only */
	struct uk_list_head folder_head;

	/* List for the entries in the folder */
	struct uk_list_head entry_head;

	/* Allocator to be used in allocations */
	struct uk_alloc *a;

	/* The folder name */
	char *name;
} __align8;

struct uk_store_folder_entry {
	/* Saved entry */
	struct uk_store_entry entry;

	/* Folder reference */
	struct uk_store_folder *folder;

	/* Allocator to be used in allocations */
	struct uk_alloc *a;

	/* Cleanup function */
	uk_store_entry_freed_func_t clean;

	/* The linked list head */
	struct uk_list_head list_head;

	/* Refcount */
	__atomic refcount;
} __align8;

#define UK_STORE_NR_MAX_LEN 32
#define UK_STORE_NR_SIZE8_LEN 5
#define UK_STORE_NR_SIZE16_LEN 7
#define UK_STORE_NR_SIZE32_LEN 12
#define UK_STORE_NR_SIZE64_LEN 21

/* The starting point of all dynamic folders for each library */
static struct uk_list_head dynamic_heads[__UK_STORE_COUNT] = { NULL, };

/* static struct uk_store_folder *static_folders[2 * __UK_STORE_COUNT] */
#include <uk/_store_array.h>

/**
 * Adds a folder entry to a folder list
 *
 * @param folder pointer to the place where to add the new entry
 * @param folder_entry pointer to the entry to add
 */
#define uk_store_add_folder_entry(folder, folder_entry)	\
	uk_list_add(&(folder_entry)->list_head, &(folder)->entry_head)


/**
 * Returns the folder_entry of an entry
 *
 * @param entry the entry
 * @return the folder_entry
 */
static inline struct uk_store_folder_entry *
uk_store_get_folder_entry(const struct uk_store_entry *p_entry)
{
	UK_ASSERT(!UK_STORE_ENTRY_ISSTATIC(p_entry));
	return __containerof(p_entry, struct uk_store_folder_entry, entry);
}

/**
 * Releases an entry (decreases the refcount and sets the reference to NULL)
 * If the refcount is 0 (it was also released by the creator), it is removed
 * from the list and memory is freed
 *
 * @param p_entry pointer to the entry to release
 */
void
_uk_store_release_entry(const struct uk_store_entry **p_entry)
{
	struct uk_store_folder_entry *res;

	if (UK_STORE_ENTRY_ISSTATIC(*p_entry))
		return;

	res = uk_store_get_folder_entry(*p_entry);

	ukarch_dec(&res->refcount.counter);

	if (ukarch_load_n(&res->refcount.counter) == 0) {
		uk_list_del(&res->list_head);
		if (res->clean)
			res->clean(res->entry.cookie);
		uk_store_free_entry(*p_entry);
	}

	*p_entry = NULL;
}

void
_uk_store_free_entry(const struct uk_store_entry *entry)
{
	struct uk_alloc *a = uk_store_get_folder_entry(entry)->a;

	if (UK_STORE_ENTRY_ISSTATIC(entry))
		return;

	a->free(a, entry->name);
	a->free(a, uk_store_get_folder_entry(entry));
}

/**
 * Creates a folder
 *
 * @param name the folder to allocate
 * @return the allocated folder
 */
struct uk_store_folder *
_uk_store_dynamic_folder_alloc(struct uk_alloc *a, const char *name)
{
	struct uk_store_folder *new_folder;

	UK_ASSERT(name);

	new_folder = a->malloc(a, sizeof(*new_folder));
	if (!new_folder)
		return NULL;

	new_folder->name = a->malloc(a, sizeof(*name) * strlen(name));
	if (!new_folder->name) {
		a->free(a, new_folder);
		return NULL;
	}

	strcpy(new_folder->name, name);

	new_folder->a = a;

	UK_INIT_LIST_HEAD(&new_folder->folder_head);
	UK_INIT_LIST_HEAD(&new_folder->entry_head);

	return new_folder;
}

/**
 * Adds a folder to the dynamic list of folders
 *
 * @param library_id the library list to add to
 * @param folder the dynamic folder to be added
 * @return 0 on success and < 0 on failure
 */
int
_uk_store_add_folder(unsigned int library_id, struct uk_store_folder *folder)
{
	if (unlikely(library_id >= __UK_STORE_COUNT))
		return -EINVAL;

	if (!dynamic_heads[library_id].next)
		UK_INIT_LIST_HEAD(&dynamic_heads[library_id]);

	uk_list_add(&folder->folder_head, &dynamic_heads[library_id]);

	return 0;
}

/**
 * Frees a folder and all its contents
 *
 * @param folder reference to the folder to free
 */
void
_uk_store_free_folder(struct uk_store_folder **folder)
{
	struct uk_store_folder_entry *iter, *sec;
	const struct uk_store_entry *entry;

	(*folder)->a->free((*folder)->a, (*folder)->name);

	uk_list_for_each_entry_safe(iter, sec,
				&(*folder)->entry_head, list_head) {
		uk_list_del(&iter->list_head);
		iter->folder = NULL;
		entry = &iter->entry;
		uk_store_release_entry(&entry);
	}

	*folder = NULL;
}

/**
 * Removes a folder from a library
 *
 * @param folder the folder to remove
 */
void
_uk_store_remove_folder(struct uk_store_folder *folder)
{
	uk_list_del(&(folder->folder_head));
}

/**
 * Searches for an folder in a library and returns it.
 *
 * @param library_id the library id to search in
 * @param name the name of the folder to search for
 * @return the found folder or NULL
 */
struct uk_store_folder *
_uk_store_get_folder(unsigned int library_id, const char *name)
{
	struct uk_store_folder *res = NULL;

	if (!dynamic_heads[library_id].next)
		return NULL;

	uk_list_for_each_entry(res, &dynamic_heads[library_id], folder_head)
		if (!strcmp(res->name, name))
			return res;

	return NULL;
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

/**
 * Searches for a folder in a library. Then search for an entry in the folder.
 * Increases the refcount.
 *
 * @param libid the library to search in
 * @param f_name the name of the folder to search for
 * @param e_name the name of the entry to search for
 * @return the found entry or NULL
 */
const struct uk_store_entry *
_uk_store_get_dynamic_entry(unsigned int libid, const char *f_name,
				const char *e_name)
{
	struct uk_store_folder *folder = _uk_store_get_folder(libid, f_name);
	struct uk_store_folder_entry *res = NULL;

	if (!folder)
		return NULL;

	uk_list_for_each_entry(res, &folder->entry_head, list_head)
		if (!strcmp(res->entry.name, e_name))
			break;

	if (res) {
		ukarch_inc(&res->refcount.counter);
		return &res->entry;
	}

	return NULL;
}

/**
 * Generates a funtion that creates the given type
 *
 * @param gen_type the type to use to generate
 */
#define _UK_STORE_DYNAMIC_CREATE_TYPED(gen_type)			\
	const struct uk_store_entry *					\
	_uk_store_dynamic_entry_create_ ## gen_type(			\
		struct uk_store_folder *folder,				\
		const char *name,					\
		uk_store_get_ ## gen_type ## _func_t get,		\
		uk_store_set_ ## gen_type ## _func_t set,		\
		void *cookie, uk_store_entry_freed_func_t clean)	\
	{								\
		struct uk_store_folder_entry *new_folder_entry;		\
									\
		UK_ASSERT(folder || name || folder->a);			\
									\
		new_folder_entry = folder->a->malloc(			\
				folder->a, sizeof(*new_folder_entry));	\
		if (!new_folder_entry)					\
			return NULL;					\
									\
		new_folder_entry->entry.name = folder->a->malloc(	\
			folder->a, sizeof(*name) * strlen(name));	\
		if (!new_folder_entry->entry.name) {			\
			folder->a->free(folder->a, new_folder_entry);	\
			return NULL;					\
		}							\
		strcpy(new_folder_entry->entry.name, name);		\
		new_folder_entry->entry.type =				\
				uk_store_get_entry_type(gen_type);	\
		new_folder_entry->entry.get.gen_type = get;		\
		new_folder_entry->entry.set.gen_type = set;		\
		new_folder_entry->entry.flags = 0;			\
		new_folder_entry->entry.cookie = cookie;		\
		new_folder_entry->folder = folder;			\
		new_folder_entry->a = folder->a;			\
		new_folder_entry->clean = clean;			\
									\
		ukarch_store_n(&new_folder_entry->refcount.counter, 1);	\
									\
		uk_store_add_folder_entry(folder, new_folder_entry);	\
									\
		return &new_folder_entry->entry;			\
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
		case uk_store_get_entry_type(etype):			\
		if (unlikely(var < 0 ||					\
			var > (__ ## eparam) __ ## ETYPE ## _MAX))	\
			return -ERANGE;					\
		return (entry)->set.etype(entry->cookie,		\
					(__ ## etype) var);		\
	} while (0)

/* Unsigned input, signed etype */
#define SETCASE_DOWNCASTUS(entry, etype, ETYPE, var, eparam)	\
	do {							\
		case uk_store_get_entry_type(etype):		\
		if (unlikely(var > __ ## ETYPE ## _MAX))	\
			return -ERANGE;				\
		return (entry)->set.etype(entry->cookie,	\
					(__ ## etype) var);	\
	} while (0)

/* Both signed */
#define SETCASE_DOWNCASTSS(entry, etype, ETYPE, var, eparam)	\
	do {							\
		case uk_store_get_entry_type(etype):		\
		if (unlikely(var < __ ## ETYPE ## _MIN ||	\
				var > __ ## ETYPE ## _MAX))	\
			return -ERANGE;				\
		return (entry)->set.etype(entry->cookie,	\
					(__ ## etype) var);	\
	} while (0)

/* Both unsigned */
#define SETCASE_DOWNCASTUU(entry, etype, ETYPE, var, eparam)	\
	do {							\
		case uk_store_get_entry_type(etype):		\
		if (unlikely(var > __ ## ETYPE ## _MAX))	\
			return -ERANGE;				\
		return (entry)->set.etype(entry->cookie,	\
					(__ ## etype) var);	\
	} while (0)

/* Signed input, unsigned etype */
#define SETCASE_UPCASTSU(entry, etype, ETYPE, var, eparam)	\
	do {							\
		case uk_store_get_entry_type(etype):		\
		if (unlikely(var < 0))				\
			return -ERANGE;				\
		return (entry)->set.etype(entry->cookie,	\
					(__ ## etype) var);	\
	} while (0)

/* All other cases */
#define SETCASE_UPCAST(entry, etype, ETYPE, var, eparam)	\
	do {							\
		case uk_store_get_entry_type(etype):		\
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

	case uk_store_get_entry_type(charp): {
		int ret;
		char to_set[UK_STORE_NR_MAX_LEN];

		sprintf(to_set, "%" __PRIu8, val);
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

	case uk_store_get_entry_type(charp): {
		int ret;
		char to_set[UK_STORE_NR_MAX_LEN];

		sprintf(to_set, "%" __PRIs8, val);
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
	switch (e->type) {
	SETCASE_DOWNCASTUS(e, s8, S8, val, u16);
	SETCASE_DOWNCASTUU(e, u8, U8, val, u16);
	SETCASE_DOWNCASTUS(e, s16, S16, val, u16);
	SETCASE_UPCAST(e, u16, U16, val, u16);
	SETCASE_UPCAST(e, s32, S32, val, u16);
	SETCASE_UPCAST(e, u32, U32, val, u16);
	SETCASE_UPCAST(e, u64, S64, val, u16);
	SETCASE_UPCAST(e, uptr, PTR, val, u16);

	case uk_store_get_entry_type(charp): {
		int ret;
		char to_set[UK_STORE_NR_MAX_LEN];

		sprintf(to_set, "%" __PRIu16, val);
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

	case uk_store_get_entry_type(charp): {
		int ret;
		char to_set[UK_STORE_NR_MAX_LEN];

		sprintf(to_set, "%" __PRIs16, val);
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

	case uk_store_get_entry_type(charp): {
		int ret;
		char to_set[UK_STORE_NR_MAX_LEN];

		sprintf(to_set, "%" __PRIu32, val);
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

	case uk_store_get_entry_type(charp): {
		int ret;
		char to_set[UK_STORE_NR_MAX_LEN];

		sprintf(to_set, "%" __PRIs32, val);
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

	case uk_store_get_entry_type(charp): {
		int ret;
		char to_set[UK_STORE_NR_MAX_LEN];

		sprintf(to_set, "%" __PRIu64, val);
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

	case uk_store_get_entry_type(charp): {
		int ret;
		char to_set[UK_STORE_NR_MAX_LEN];

		sprintf(to_set, "%" __PRIs64, val);
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

	case uk_store_get_entry_type(charp): {
		int ret;
		char to_set[UK_STORE_NR_MAX_LEN];

		sprintf(to_set, "%p", (void *) val);
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

	switch (e->type) {
	case uk_store_get_entry_type(u8): {
		__u8 to_set;

		ret = sscanf(val, "%" __SCNu8, &to_set);
		if (ret < 0)
			return ret;

		return e->set.u8(e->cookie, to_set);
	}

	case uk_store_get_entry_type(s8): {
		__s8 to_set;

		ret = sscanf(val, "%" __SCNs8, &to_set);
		if (ret < 0)
			return ret;

		return e->set.s8(e->cookie, to_set);
	}

	case uk_store_get_entry_type(u16): {
		__u16 to_set;

		ret = sscanf(val, "%" __SCNu16, &to_set);
		if (ret < 0)
			return ret;

		return e->set.u16(e->cookie, to_set);
	}

	case uk_store_get_entry_type(s16): {
		__s16 to_set;

		ret = sscanf(val, "%" __SCNs16, &to_set);
		if (ret < 0)
			return ret;

		return e->set.s16(e->cookie, to_set);
	}

	case uk_store_get_entry_type(u32): {
		__u32 to_set;

		ret = sscanf(val, "%" __SCNu32, &to_set);
		if (ret < 0)
			return ret;

		return e->set.u32(e->cookie, to_set);
	}

	case uk_store_get_entry_type(s32): {
		__s32 to_set;

		ret = sscanf(val, "%" __SCNs32, &to_set);
		if (ret < 0)
			return ret;

		return e->set.s32(e->cookie, to_set);
	}

	case uk_store_get_entry_type(u64): {
		__u64 to_set;

		ret = sscanf(val, "%" __SCNu64, &to_set);
		if (ret < 0)
			return ret;

		return e->set.u64(e->cookie, to_set);
	}

	case uk_store_get_entry_type(s64): {
		__s64 to_set;

		ret = sscanf(val, "%" __SCNs64, &to_set);
		if (ret < 0)
			return ret;

		return e->set.s64(e->cookie, to_set);
	}

	case uk_store_get_entry_type(uptr): {
		__uptr to_set;

		ret = sscanf(val, "%p", (void **) &to_set);
		if (ret < 0)
			return ret;

		return e->set.uptr(e->cookie, to_set);
	}

	case uk_store_get_entry_type(charp): {
		struct uk_alloc *a = uk_store_get_folder_entry(e)->folder->a;
		char *to_set = a->malloc(a, sizeof(*val) * strlen(val));

		if (unlikely(!to_set))
			return -ENOMEM;

		strcpy(to_set, val);

		ret = e->set.charp(e->cookie, to_set);

		a->free(a, to_set);
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
		case uk_store_get_entry_type(etype): {			\
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
		case uk_store_get_entry_type(etype): {			\
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
		case uk_store_get_entry_type(etype): {			\
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
		case uk_store_get_entry_type(etype): {			\
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
		case uk_store_get_entry_type(etype): {			\
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
		case uk_store_get_entry_type(etype): {			\
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

	case uk_store_get_entry_type(charp): {
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

	case uk_store_get_entry_type(charp): {
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

	case uk_store_get_entry_type(charp): {
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

	case uk_store_get_entry_type(charp): {
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

	case uk_store_get_entry_type(charp): {
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

	case uk_store_get_entry_type(charp): {
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

	case uk_store_get_entry_type(charp): {
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

	case uk_store_get_entry_type(charp): {
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

	case uk_store_get_entry_type(charp): {
		char *val = NULL;
		__s16 sscanf_ret;

		ret = e->get.charp(e->cookie, &val);
		if (ret < 0)
			return ret;

		sscanf_ret = sscanf(val, "%p", (void **) out);
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
	struct uk_alloc *a = uk_store_get_folder_entry(e)->folder->a;

	UK_ASSERT(e);

	switch (e->type) {
	case uk_store_get_entry_type(u8): {
		__u8 val;

		str = a->calloc(a, UK_STORE_NR_SIZE8_LEN, sizeof(char));
		if (unlikely(!str))
			return -ENOMEM;

		ret = e->get.u8(e->cookie, &val);
		if (ret < 0)
			return ret;
		sprintf(str, "%" __PRIu8, val);
		*out  = str;
		return ret;
	}

	case uk_store_get_entry_type(s8): {
		__s8 val;

		str = a->calloc(a, UK_STORE_NR_SIZE8_LEN, sizeof(char));
		if (unlikely(!str))
			return -ENOMEM;

		ret = e->get.s8(e->cookie, &val);
		if (ret < 0)
			return ret;
		sprintf(str, "%" __PRIs8, val);
		*out  = str;
		return ret;
	}

	case uk_store_get_entry_type(u16): {
		__u16 val;

		str = a->calloc(a, UK_STORE_NR_SIZE16_LEN, sizeof(char));
		if (unlikely(!str))
			return -ENOMEM;

		ret = e->get.u16(e->cookie, &val);
		if (ret < 0)
			return ret;
		sprintf(str, "%" __PRIu16, val);
		*out  = str;
		return ret;
	}

	case uk_store_get_entry_type(s16): {
		__s16 val;

		str = a->calloc(a, UK_STORE_NR_SIZE16_LEN, sizeof(char));
		if (unlikely(!str))
			return -ENOMEM;

		ret = e->get.s16(e->cookie, &val);
		if (ret < 0)
			return ret;
		sprintf(str, "%" __PRIs16, val);
		*out  = str;
		return ret;
	}

	case uk_store_get_entry_type(u32): {
		__u32 val;

		str = a->calloc(a, UK_STORE_NR_SIZE32_LEN, sizeof(char));
		if (unlikely(!str))
			return -ENOMEM;

		ret = e->get.u32(e->cookie, &val);
		if (ret < 0)
			return ret;
		sprintf(str, "%" __PRIu32, val);
		*out  = str;
		return ret;
	}

	case uk_store_get_entry_type(s32): {
		__s32 val;

		str = a->calloc(a, UK_STORE_NR_SIZE32_LEN, sizeof(char));
		if (unlikely(!str))
			return -ENOMEM;

		ret = e->get.s32(e->cookie, &val);
		if (ret < 0)
			return ret;
		sprintf(str, "%" __PRIs32, val);
		*out  = str;
		return ret;
	}

	case uk_store_get_entry_type(u64): {
		__u64 val;

		str = a->calloc(a, UK_STORE_NR_SIZE64_LEN, sizeof(char));
		if (unlikely(!str))
			return -ENOMEM;

		ret = e->get.u64(e->cookie, &val);
		if (ret < 0)
			return ret;
		sprintf(str, "%" __PRIu64, val);
		*out  = str;
		return ret;
	}

	case uk_store_get_entry_type(s64): {
		__s64 val;

		str = a->calloc(a, UK_STORE_NR_SIZE64_LEN, sizeof(char));
		if (unlikely(!str))
			return -ENOMEM;

		ret = e->get.s64(e->cookie, &val);
		if (ret < 0)
			return ret;
		sprintf(str, "%" __PRIs64, val);
		*out  = str;
		return ret;
	}

	case uk_store_get_entry_type(uptr): {
		__uptr val;

		str = a->calloc(a, UK_STORE_NR_SIZE64_LEN, sizeof(char));
		if (unlikely(!str))
			return -ENOMEM;

		ret = e->get.uptr(e->cookie, &val);
		if (ret < 0)
			return ret;
		sprintf(str, "%p", (void *) val);
		*out  = str;
		return ret;
	}

	case uk_store_get_entry_type(charp): {
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
