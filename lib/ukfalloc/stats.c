/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#define _GNU_SOURCE /* asprintf */
#include <stdio.h>

#include <uk/alloc.h>
#include <uk/essentials.h>
#include <uk/errptr.h>
#include <uk/falloc.h>
#include <uk/falloc/store.h>
#include <uk/init.h>
#include <uk/libid.h>
#include <uk/spinlock.h>

static UK_LIST_HEAD(uk_falloc_store_obj_list);

struct uk_falloc_stats_global uk_falloc_stats_global;
struct uk_spinlock uk_falloc_stats_global_lock = UK_SPINLOCK_INITIALIZER();

/* As frame allocator instances do not provide an ID
 * we assign each instance with a monotonic value.
 */
static unsigned int falloc_id;

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
	struct uk_falloc *fa;

	UK_ASSERT(cookie);
	UK_ASSERT(out);

	fa = (struct uk_falloc *)cookie;

	*out = fa->free_memory;

	return 0;
}

static int get_total_memory(void *cookie, __u64 *out)
{
	struct uk_falloc *fa;

	UK_ASSERT(cookie);
	UK_ASSERT(out);

	fa = (struct uk_falloc *)cookie;

	*out = fa->total_memory;

	return 0;
}

static const struct uk_store_entry *dyn_entries[] = {
	UK_STORE_ENTRY(UK_FALLOC_STATS_FREE_MEMORY, "free_memory", u64,
		       get_free_memory, NULL),
	UK_STORE_ENTRY(UK_FALLOC_STATS_TOTAL_MEMORY, "total_memory", u64,
		       get_total_memory, NULL),
	NULL
};

int uk_falloc_init_stats(struct uk_falloc *fa)
{
	struct uk_store_object *obj = NULL;
	__u64 obj_id = falloc_id++;
	struct uk_alloc *a;
	char *obj_name;
	int rc;

	if (unlikely(!fa))
		return -EINVAL;

	/* If the heap is not ready this will be handled
	 * by init. Yet we don't return an error value here,
	 * as it does not provide useful information to the
	 * caller. Instead, we document this behavior.
	 */
	a = uk_alloc_get_default();
	if (unlikely(!a)) {
		uk_list_add(&fa->store_obj_list, &uk_falloc_store_obj_list);
		return 0;
	}

	/* Create stats object */
	rc = asprintf(&obj_name, "falloc%ld", obj_id);
	if (unlikely(rc == -1)) {
		uk_pr_err("Could not allocate object name\n");
		return -ENOMEM;
	}

	obj = uk_store_obj_alloc(a, obj_id, obj_name, dyn_entries, (void *)fa);
	if (unlikely(PTRISERR(obj))) {
		rc = PTR2ERR(obj);
		uk_pr_err("Could not allocate object (%d)\n", rc);
		goto err_obj_alloc;
	}

	rc = uk_store_obj_add(obj);
	if (unlikely(rc)) {
		uk_pr_err("Could not add object (%d)\n", rc);
		goto err_obj_add;
	}

	fa->stats = obj;

	return 0;

err_obj_add:
	uk_free(a, obj);

err_obj_alloc:
	uk_free(a, obj_name);

	return rc;
}

static int uk_falloc_init_stats_from_list(struct uk_init_ctx *ctx __unused)
{
	struct uk_falloc *fa;

	uk_list_for_each_entry(fa, &uk_falloc_store_obj_list, store_obj_list) {
		if (!fa->stats)
			uk_falloc_init_stats(fa);
	}
	return 0;
}

/* The frame allocator starts before the heap is initialized,
 * so we cannot call init_stats from uk_falloc_init at early
 * boot. We work around this by deferring initialization to
 * init. The selected priority is not significant.
 */
uk_lib_initcall(uk_falloc_init_stats_from_list, UK_PRIO_EARLIEST);
