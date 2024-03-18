/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#define _GNU_SOURCE /* asprintf */
#include <stdio.h>

#include <uk/alloc.h>
#include <uk/essentials.h>
#include <uk/errptr.h>
#include <uk/event.h>
#include <uk/falloc.h>
#include <uk/falloc_store.h>
#include <uk/init.h>
#include <uk/libid.h>
#include <uk/falloc.h>
#include <uk/spinlock.h>
#include <uk/store.h>

/* As the frame allocator does not provide an ID
 * we assign each instance with a monotonic value.
 */
static unsigned int falloc_id;

static struct uk_store_event_data event_data;

static int get_free_memory_global(void *cookie __unused, __u64 *out)
{
	*out = uk_falloc_stats_global.free_memory;

	return 0;
}
UK_STORE_STATIC_ENTRY(UK_FALLOC_STATS_FREE_MEMORY, free_memory_global, u64,
		      get_free_memory_global, NULL);

static int get_total_memory_global(void *cookie __unused, __u64 *out)
{
	*out = uk_falloc_stats_global.total_memory;

	return 0;
}
UK_STORE_STATIC_ENTRY(UK_FALLOC_STATS_TOTAL_MEMORY, total_memory_global, u64,
		      get_total_memory_global, NULL);

static int get_free_memory(void *cookie, __u64 *out)
{
	struct uk_falloc *fa = (struct uk_falloc *)cookie;

	UK_ASSERT(fa);

	*out = fa->free_memory;

	return 0;
}

static int get_total_memory(void *cookie, __u64 *out)
{
	struct uk_falloc *fa = (struct uk_falloc *)cookie;

	UK_ASSERT(fa);

	*out = fa->total_memory;

	return 0;
}

static const struct uk_store_entry *dyn_entries[] = {
	UK_STORE_ENTRY(UK_FALLOC_STATS_FREE_MEMORY, "free_memory", u64,
		       get_free_memory, NULL, NULL),
	UK_STORE_ENTRY(UK_FALLOC_STATS_TOTAL_MEMORY, "total_memory", u64,
		       get_total_memory, NULL, NULL),
	NULL
};

int uk_falloc_init_stats(struct uk_falloc *fa)
{
	int res;
	struct uk_store_object *obj = NULL;
	__u64 obj_id = falloc_id++;
	char *obj_name;

	/* If the heap is not ready this will be handled
	 * by init. Yet we don't return an error value here,
	 * as it does not provide useful information to the
	 * caller, but instead document this behavior.
	 */
	if (!uk_alloc_get_default())
		return 0;

	/* Create stats object */
	res = asprintf(&obj_name, "falloc%ld", obj_id);
	if (res == -1)
		return -ENOMEM;

	obj = uk_store_allocate_object(uk_alloc_get_default(), obj_id, obj_name,
				       dyn_entries, (void *)fa);

	if (PTRISERR(obj))
		return PTR2ERR(obj);

	res = uk_store_add_object(obj);
	if (unlikely(res))
		return res;

	/* Notify consumers */
	event_data = (struct uk_store_event_data) {
		.library_id = uk_libid_self(),
		.object_id = obj->id
	};
	uk_raise_event(UKSTORE_EVENT_CREATE_OBJECT, &event_data);

	return 0;
}

static int uk_falloc_init_stats_from_list(void)
{
	struct uk_falloc *fa;

	uk_list_for_each_entry(fa, &uk_falloc_head, list_head) {
		if (!fa->stats_obj)
			uk_falloc_init_stats(fa);
	}

	return 0;
}

/* The frame allocator starts before the heap is initialized,
 * so we cannot call init_stats from uk_falloc_init at early
 * boot. We work around this by deferring initialization to
 * init. We use lib_initcall as the heap should be initialized
 * by then.
 */
uk_lib_initcall(uk_falloc_init_stats_from_list);
