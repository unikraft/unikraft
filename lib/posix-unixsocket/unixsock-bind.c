/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <string.h>

#include <uk/rwlock.h>

#include "unixsock-bind.h"


#define UNIX_BUCKETS 128

static struct unix_addr_entry *hash_tbl[UNIX_BUCKETS];
static struct uk_rwlock hash_lock = UK_RWLOCK_INITIALIZER(hash_lock, 0);


static inline
size_t unix_addr_hash(const char *name, size_t namelen)
{
	/* TODO: use actual string hash func */
	size_t c = 0;

	for (size_t i = 0; i < namelen; i++)
		c ^= name[i];
	return c % UNIX_BUCKETS;
}

static inline
struct unix_addr_entry **unix_addr_get(const char *name, size_t namelen)
{
	struct unix_addr_entry **p;

	for (p = &hash_tbl[unix_addr_hash(name, namelen)];
	     *p;
	     p = &(*p)->next) {
		struct unix_addr_entry *ent = *p;

		if (namelen == ent->namelen &&
		    !memcmp(name, ent->name, namelen))
			break;
	}
	return p;
}

int unix_addr_bind(struct unix_addr_entry *ent)
{
	int ret = 0;
	struct unix_addr_entry **p;

	UK_ASSERT(!ent->next);
	UK_ASSERT(ent->sock);

	uk_rwlock_wlock(&hash_lock);

	p = unix_addr_get(ent->name, ent->namelen);
	if (unlikely(*p))
		ret = -EADDRINUSE;
	else
		*p = ent;

	uk_rwlock_wunlock(&hash_lock);
	if (!ret)
		uk_file_acquire_weak(ent->sock);
	return ret;
}

posix_sock *unix_addr_lookup(const char *name, size_t namelen)
{
	posix_sock *ret = NULL;
	struct unix_addr_entry **p;

	uk_rwlock_rlock(&hash_lock);

	p = unix_addr_get(name, namelen);
	if (likely(*p)) {
		ret = (*p)->sock;
		uk_file_acquire_weak(ret);
	}

	uk_rwlock_runlock(&hash_lock);
	return ret;
}

int unix_addr_release(const char *name, size_t namelen)
{
	int ret = 0;
	struct unix_addr_entry **p;
	struct unix_addr_entry *ent;

	uk_rwlock_wlock(&hash_lock);

	p = unix_addr_get(name, namelen);
	ent = *p;
	if (unlikely(!ent)) {
		ret = -ENOENT;
	} else {
		UK_ASSERT(ent->sock);
		*p = ent->next;
		ent->next = NULL;
	}

	uk_rwlock_wunlock(&hash_lock);

	if (!ret)
		uk_file_release_weak(ent->sock);
	return ret;
}
